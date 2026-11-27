// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The delay model of the search, read from STA.

#include <algorithm>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "RegisterTiming.h"
#include "RegisterUtils.h"
#include "Retime.h"
#include "RetimingGraph.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace ret {

namespace {

constexpr float kNoArrival = -1e29f;

// The latest arrival at `to` from `reached`, another output of the gate that
// drives it (a half adder's carry into its sum).
std::optional<float> arrivalThroughOutput(sta::Graph* graph,
                                          sta::Vertex* reached,
                                          sta::Vertex* to,
                                          const sta::RiseFall* rf,
                                          const float at_reached,
                                          const int ap)
{
  std::optional<float> latest;
  sta::VertexOutEdgeIterator edge_iter(reached, graph);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (edge->isWire() || edge->to(graph) != to) {
      continue;
    }
    for (const sta::TimingArc* arc : edge->timingArcSet()->arcs()) {
      if (arc->fromEdge()->asRiseFall() == rf) {
        const float at
            = at_reached + sta::delayAsFloat(graph->arcDelay(edge, arc, ap));
        latest = std::max(latest.value_or(at), at);
      }
    }
  }
  return latest;
}

std::optional<float> arrivalThroughGate(sta::Graph* graph,
                                        sta::Vertex* from,
                                        sta::Vertex* to,
                                        const float in_rise,
                                        const float in_fall,
                                        const int ap)
{
  std::optional<float> latest;
  sta::VertexOutEdgeIterator edge_iter(from, graph);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (edge->isWire() || edge->timingArcSet()->role()->isTimingCheck()) {
      continue;
    }
    sta::Vertex* reached = edge->to(graph);
    for (const sta::TimingArc* arc : edge->timingArcSet()->arcs()) {
      const sta::RiseFall* rf = arc->fromEdge()->asRiseFall();
      const float in = rf == sta::RiseFall::rise() ? in_rise : in_fall;
      if (rf == nullptr || in < kNoArrival) {
        continue;
      }
      const float at_reached
          = in + sta::delayAsFloat(graph->arcDelay(edge, arc, ap));
      const std::optional<float> at
          = reached == to ? at_reached
                          : arrivalThroughOutput(graph,
                                                 reached,
                                                 to,
                                                 arc->toEdge()->asRiseFall(),
                                                 at_reached,
                                                 ap);
      if (at) {
        latest = std::max(latest.value_or(*at), *at);
      }
    }
  }
  return latest;
}

// In the library of the scene.
float sceneCap(const sta::Scene* scene, const sta::LibertyPort* port)
{
  const sta::LibertyPort* scene_port
      = port->scenePort(scene, sta::MinMax::max());
  return (scene_port != nullptr ? scene_port : port)->capacitance();
}

}  // namespace

void Retime::buildModel()
{
  sta_->updateTiming(/*full=*/false);
  scenes_ = {sta_->cmdScene()};
  dcalc_ap_ = scenes_.front()->dcalcAnalysisPtIndex(sta::MinMax::max());

  resolveFlopDCap();
  computeWireArcs();
  computePinnedFloors();
  computeLatestOutputs();
  computeGateArcs();
  computeCrossExtra();
  computeLaunch();
  computeDriverShifts();
  computeEndpoints();
}

void Retime::resolveFlopDCap()
{
  flop_d_cap_ = 0.0f;
  sta::LibertyCell* liberty_cell
      = libertyCell(sta_, flop_d_mterm_->getMaster());
  if (liberty_cell != nullptr) {
    const sta::LibertyPort* d
        = liberty_cell->findLibertyPort(flop_d_mterm_->getConstName());
    if (d != nullptr) {
      flop_d_cap_ = sceneCap(scenes_.front(), d);
    }
  }
  if (flop_d_cap_ <= 0.0f) {
    logger_->warn(
        utl::RET,
        17,
        "No register D-pin capacitance found; using combinational arcs.");
  }
}

void Retime::computeWireArcs()
{
  const int n = graph_->size();
  fanin_.assign(n, {});
  fanout_.assign(n, {});
  for (int u = 0; u < n; ++u) {
    const std::vector<Edge>& edges = graph_->adj()[u];
    const std::vector<EdgePhys>& handles = graph_->phys()[u];
    const int edge_count = static_cast<int>(edges.size());
    for (int k = 0; k < edge_count; ++k) {
      odb::dbITerm* sink = handles[k].sink;
      const float wire = wireDelay(handles[k]);
      const float fallback = getGateArc(sink);
      std::vector<FaninEdge>& into = fanin_[edges[k].target];
      fanout_[u].push_back(
          {.gate = edges[k].target, .index = static_cast<int>(into.size())});
      into.push_back({
          .source = u,
          .weight = edges[k].weight,
          .wire = wire,
          .arc_comb = fallback,
          .arc_reg = fallback,
          .sink = sink,
          .launch = pinArrival(sink),
          .phys = &handles[k],
          .driver_shift = 0.0f,
          .cross_extra = 0.0f,
      });
    }
  }
}

float Retime::wireDelay(const EdgePhys& phys) const
{
  float wire = std::max(pinArrival(phys.sink) - pinArrival(phys.driver), 0.0f);
  for (odb::dbITerm* entry : phys.entries) {
    odb::dbITerm* source = entry->getNet()->getFirstOutput();
    if (source != nullptr) {
      wire += std::max(pinArrival(entry) - pinArrival(source), 0.0f);
    }
  }
  return wire;
}

float Retime::pinArrival(odb::dbITerm* db_pin) const
{
  const sta::Pin* pin = sta_->getDbNetwork()->dbToSta(db_pin);
  sta::Vertex* vertex = nullptr;
  sta::Vertex* bidirect_drvr = nullptr;
  sta_->ensureGraph()->pinVertices(pin, vertex, bidirect_drvr);
  if (vertex == nullptr) {
    return 0.0f;
  }
  return std::max(arrivalAt(vertex, sta::RiseFall::rise()),
                  arrivalAt(vertex, sta::RiseFall::fall()));
}

float Retime::arrivalAt(sta::Vertex* vertex, const sta::RiseFall* rf) const
{
  return sta::delayAsFloat(
      sta_->arrival(vertex, rf->asRiseFallBoth(), scenes_, sta::MinMax::max()));
}

float Retime::getGateArc(odb::dbITerm* input_pin) const
{
  const sta::Pin* pin = sta_->getDbNetwork()->dbToSta(input_pin);
  const sta::Graph* graph = sta_->ensureGraph();
  sta::Vertex* vertex = nullptr;
  sta::Vertex* bidirect_drvr = nullptr;
  graph->pinVertices(pin, vertex, bidirect_drvr);
  if (vertex == nullptr) {
    return 0.0f;
  }
  float best = 0.0f;
  sta::VertexOutEdgeIterator edge_iter(vertex, graph);
  while (edge_iter.hasNext()) {
    const sta::Edge* edge = edge_iter.next();
    if (edge->isWire()) {
      continue;
    }
    for (const sta::TimingArc* arc : edge->timingArcSet()->arcs()) {
      best = std::max(best,
                      sta::delayAsFloat(graph->arcDelay(edge, arc, dcalc_ap_)));
    }
  }
  return best;
}

void Retime::computePinnedFloors()
{
  const int n = graph_->size();
  pinned_floor_.assign(n, kUnreached);
  for (int u = 0; u < n; ++u) {
    if (!graph_->pinned()[u]) {
      continue;
    }
    for (odb::dbITerm* iterm : graph_->unmodeledInputs(u)) {
      pinned_floor_[u] = std::max(
          pinned_floor_[u],
          static_cast<double>(pinArrival(iterm) + getGateArc(iterm)));
    }
  }
}

void Retime::computeLatestOutputs()
{
  const int n = graph_->size();
  latest_output_.assign(n, nullptr);
  for (int v = 0; v < n; ++v) {
    latest_output_[v] = latestOutput(v);
  }
}

odb::dbITerm* Retime::latestOutput(const int v) const
{
  odb::dbITerm* latest = nullptr;
  for (odb::dbITerm* iterm : graph_->nodes()[v]->getITerms()) {
    if (iterm->getIoType() != odb::dbIoType::OUTPUT
        || iterm->getNet() == nullptr) {
      continue;
    }
    if (latest == nullptr || pinArrival(iterm) > pinArrival(latest)) {
      latest = iterm;
    }
  }
  return latest;
}

void Retime::computeGateArcs()
{
  const int n = graph_->size();
  for (int v = 0; v < n; ++v) {
    float load = 0.0f;
    for (const EdgePhys& phys : graph_->phys()[v]) {
      load += pinCap(phys.sink);
    }
    for (FaninEdge& fe : fanin_[v]) {
      fe.arc_comb = characterizeArc(v, fe.sink, load, fe.arc_comb);
      fe.arc_reg = characterizeArc(v, fe.sink, flop_d_cap_, fe.arc_reg);
    }
    calibrateArcs(v);
  }
}

float Retime::characterizeArc(const int v,
                              odb::dbITerm* sink,
                              const float load,
                              const float fallback,
                              const float forced_slew,
                              float* out_slew) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  const sta::MinMax* min_max = sta::MinMax::max();
  const sta::Scene* scene = scenes_.front();
  const sta::Sdc* sdc = scene->sdc();
  odb::dbInst* gate = graph_->nodes()[v];
  const sta::Pin* pin = network->dbToSta(sink);
  const sta::LibertyCell* cell = network->libertyCell(gate);
  const sta::LibertyPort* from = network->libertyPort(pin);
  // Per-instance PVT, then the SDC operating conditions, then the library
  // default, else nominal (what the delay calculator itself uses).
  const sta::Pvt* pvt = sdc->pvt(network->dbToSta(gate), min_max);
  if (pvt == nullptr) {
    pvt = sdc->operatingConditions(min_max);
  }
  if (pvt == nullptr && cell != nullptr) {
    pvt = cell->libertyLibrary()->defaultOperatingConditions();
  }
  if (cell == nullptr || from == nullptr) {
    return fallback;
  }
  const float in_slew = forced_slew >= 0.0f ? forced_slew : pinSlew(sta_, pin);
  float best = 0.0f;
  bool got = false;
  for (const sta::TimingArcSet* set : cell->timingArcSets(from, nullptr)) {
    if (set->role()->isTimingCheck()) {
      continue;
    }
    for (const sta::TimingArc* arc : set->arcs()) {
      const sta::GateTableModel* model = arc->gateTableModel(scene, min_max);
      if (model == nullptr) {
        continue;
      }
      float delay = 0.0f;
      float slew = 0.0f;
      model->gateDelay(pvt, in_slew, load, delay, slew);
      if (!got || delay > best) {
        best = delay;
        if (out_slew != nullptr) {
          *out_slew = slew;
        }
      }
      got = true;
    }
  }
  return got ? std::max(best, 0.0f) : fallback;
}

float Retime::pinCap(odb::dbITerm* pin) const
{
  const sta::LibertyPort* port = libertyPort(sta_, pin);
  return port != nullptr ? sceneCap(scenes_.front(), port) : 0.0f;
}

void Retime::calibrateArcs(const int t)
{
  odb::dbITerm* output = latest_output_[t];
  if (output == nullptr || fanin_[t].empty()) {
    return;
  }
  // No slide yet: the output is registered or not as STA timed it.
  const bool registered = outputRegistered(t);
  const double latest = modeledOutputArrival(t, registered);
  const double timed_arrival = pinArrival(output);
  // Without an arrival (a constant) there is nothing to compare.
  if (latest < kNoArrival || timed_arrival < kNoArrival) {
    return;
  }
  const double shift = timed_arrival - latest;
  for (FaninEdge& fe : fanin_[t]) {
    float& timed = registered ? fe.arc_reg : fe.arc_comb;
    float& other = registered ? fe.arc_comb : fe.arc_reg;
    // Without STA's arc for this input, the gate's shift, which is right
    // only for the input that sets the gate's arrival.
    const float target
        = staArc(fe.sink, output).value_or(timed + static_cast<float>(shift));
    other = std::max(other + target - timed, 0.0f);
    timed = std::max(target, 0.0f);
  }
}

double Retime::modeledOutputArrival(const int t, const bool registered) const
{
  double latest = kNegInf;
  for (odb::dbITerm* iterm : graph_->nodes()[t]->getITerms()) {
    if (!isSignalInput(iterm)) {
      continue;
    }
    const auto edge = std::ranges::find(fanin_[t], iterm, &FaninEdge::sink);
    double arc = 0.0;
    if (edge == fanin_[t].end()) {
      arc = getGateArc(iterm);
    } else {
      arc = registered ? edge->arc_reg : edge->arc_comb;
    }
    latest = std::max(latest, pinArrival(iterm) + arc);
  }
  return latest;
}

std::optional<float> Retime::staArc(odb::dbITerm* input,
                                    odb::dbITerm* output) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Graph* graph = sta_->ensureGraph();
  sta::Vertex* from = nullptr;
  sta::Vertex* bidirect_drvr = nullptr;
  graph->pinVertices(network->dbToSta(input), from, bidirect_drvr);
  sta::Vertex* to = graph->pinDrvrVertex(network->dbToSta(output));
  if (from == nullptr || to == nullptr) {
    return std::nullopt;
  }
  const float in_rise = arrivalAt(from, sta::RiseFall::rise());
  const float in_fall = arrivalAt(from, sta::RiseFall::fall());
  const float in_latest = std::max(in_rise, in_fall);
  if (in_latest < kNoArrival) {
    return std::nullopt;
  }
  const std::optional<float> via
      = arrivalThroughGate(graph, from, to, in_rise, in_fall, dcalc_ap_);
  if (!via) {
    return std::nullopt;
  }
  // No later than STA has the output itself.
  const float out_latest = std::max(arrivalAt(to, sta::RiseFall::rise()),
                                    arrivalAt(to, sta::RiseFall::fall()));
  return std::max(std::min(*via, out_latest) - in_latest, 0.0f);
}

void Retime::computeCrossExtra()
{
  // Read once per driver and per net, not per edge: one net can be on
  // thousands of edges.
  std::unordered_map<odb::dbITerm*, float> loads;
  const auto load_of = [&](odb::dbITerm* driver) {
    const auto [entry, is_new] = loads.try_emplace(driver);
    if (is_new) {
      entry->second = loadCap(driver);
    }
    return entry->second;
  };
  std::unordered_map<odb::dbNet*, NetReaders> readers;
  const auto readers_of = [&](odb::dbNet* net) {
    const auto [entry, is_new] = readers.try_emplace(net);
    if (is_new) {
      entry->second = netReaders(net);
    }
    return entry->second;
  };

  const int n = graph_->size();
  for (int t = 0; t < n; ++t) {
    for (FaninEdge& fe : fanin_[t]) {
      if (fe.weight == 0) {
        continue;
      }
      odb::dbITerm* source = edgeSource(fe);
      if (source == nullptr) {
        continue;
      }
      odb::dbITerm* last = fe.phys->driver;
      const float source_load = load_of(source);
      const float new_load = sourceLoadAfterSlide(
          fe, source_load, load_of(last), readers_of(last->getNet()));
      float slew = -1.0f;
      const float source_extra
          = slowdownAtLoad(fe.source, source_load, new_load, slew);
      const float gate_extra
          = slew >= 0.0f ? slowdownAtSlew(t, fe.sink, slew) : 0.0f;
      fe.cross_extra = source_extra + gate_extra;
    }
  }
}

odb::dbITerm* Retime::edgeSource(const FaninEdge& fe) const
{
  odb::dbITerm* source = fe.phys->driver;
  if (!fe.phys->flops.empty()) {
    odb::dbNet* net = graph_->chainNet(*fe.phys);
    source = net != nullptr ? net->getFirstOutput() : nullptr;
  }
  if (source == nullptr || source->getInst() != graph_->nodes()[fe.source]) {
    return nullptr;
  }
  return source;
}

Retime::NetReaders Retime::netReaders(odb::dbNet* net) const
{
  NetReaders readers;
  for (odb::dbITerm* reader : net->getITerms()) {
    if (reader->getIoType() == odb::dbIoType::INPUT) {
      readers.caps += pinCap(reader);
      ++readers.count;
    }
  }
  return readers;
}

float Retime::sourceLoadAfterSlide(const FaninEdge& fe,
                                   const float source_load,
                                   const float last_load,
                                   const NetReaders& readers) const
{
  // The gate takes its share of the wire the last register drove.
  const float wire = std::max(last_load - readers.caps, 0.0f);
  const float load
      = source_load + pinCap(fe.sink) + wire / std::max(readers.count, 1);
  if (readers.count > 1) {
    return load;
  }
  // The gate was the only reader: the first register comes off the net.
  return std::max(load - pinCap(fe.phys->entries.front()), 0.0f);
}

float Retime::loadCap(odb::dbITerm* driver) const
{
  return sta_->graphDelayCalc()->loadCap(sta_->getDbNetwork()->dbToSta(driver),
                                         sta_->cmdScene(),
                                         sta::MinMax::max());
}

float Retime::slowdownAtLoad(const int u,
                             const float load,
                             const float new_load,
                             float& slew) const
{
  float slowdown = 0.0f;
  slew = -1.0f;
  for (odb::dbITerm* input : graph_->nodes()[u]->getITerms()) {
    if (!isSignalInput(input)) {
      continue;
    }
    float new_slew = -1.0f;
    const float now = characterizeArc(u, input, load, -1.0f);
    const float then
        = characterizeArc(u, input, new_load, -1.0f, -1.0f, &new_slew);
    if (now >= 0.0f && then >= 0.0f) {
      slowdown = std::max(slowdown, then - now);
      slew = std::max(slew, new_slew);
    }
  }
  return slowdown;
}

float Retime::slowdownAtSlew(const int t,
                             odb::dbITerm* sink,
                             const float slew) const
{
  const float now = characterizeArc(t, sink, flop_d_cap_, -1.0f);
  const float then = characterizeArc(t, sink, flop_d_cap_, -1.0f, slew);
  return now >= 0.0f && then >= 0.0f ? then - now : 0.0f;
}

void Retime::computeLaunch()
{
  sampleLaunchOffset();
  const float offset = static_cast<float>(launch_offset_);
  const int n = graph_->size();
  new_launch_.assign(n, 0.0);
  launch_cell_.assign(n, nullptr);
  for (int u = 0; u < n; ++u) {
    // The register a slide across u builds drives the net u drives today.
    odb::dbITerm* output = firstOutput(u);
    const float clock_to_q
        = output != nullptr ? pickLaunchCell(u, loadCap(output)) : 0.0f;
    new_launch_[u] = offset + clock_to_q;
  }
}

void Retime::sampleLaunchOffset()
{
  const int n = graph_->size();
  clock_slew_ = 0.0f;
  launch_offset_ = 0.0;
  std::vector<float> offsets;
  std::unordered_set<odb::dbInst*> seen;
  for (int u = 0; u < n; ++u) {
    for (const EdgePhys& phys : graph_->phys()[u]) {
      for (odb::dbInst* flop : phys.flops) {
        if (seen.insert(flop).second) {
          sampleLaunch(flop, offsets);
        }
      }
    }
  }
  if (!offsets.empty()) {
    std::ranges::nth_element(offsets, offsets.begin() + offsets.size() / 2);
    launch_offset_ = offsets[offsets.size() / 2];
  }
}

void Retime::sampleLaunch(odb::dbInst* flop, std::vector<float>& offsets)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::LibertyCell* cell = libertyCell(sta_, flop->getMaster());
  for (odb::dbITerm* iterm : flop->getITerms()) {
    const sta::Pin* pin = network->dbToSta(iterm);
    const sta::LibertyPort* port = network->libertyPort(pin);
    if (port == nullptr) {
      continue;
    }
    if (port->isRegClk()) {
      clock_slew_ = clockPinSlew(sta_, pin);
    } else if (iterm->getIoType() == odb::dbIoType::OUTPUT
               && iterm->getNet() != nullptr) {
      const float table
          = registerClockToQ(sta_, cell, port, clock_slew_, loadCap(iterm));
      if (table >= 0.0f) {
        offsets.push_back(pinArrival(iterm) - table);
      }
    }
  }
}

odb::dbITerm* Retime::firstOutput(const int v) const
{
  for (odb::dbITerm* iterm : graph_->nodes()[v]->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::OUTPUT
        && iterm->getNet() != nullptr) {
      return iterm;
    }
  }
  return nullptr;
}

float Retime::pickLaunchCell(const int u, const float load)
{
  float best = -1.0f;
  for (sta::LibertyCell* cell : newRegisterCells(u)) {
    const float delay
        = registerClockToQ(sta_, cell, nullptr, clock_slew_, load);
    if (delay >= 0.0f && (best < 0.0f || delay < best)) {
      best = delay;
      launch_cell_[u] = cell;
    }
  }
  return std::max(best, 0.0f);
}

std::vector<sta::LibertyCell*> Retime::newRegisterCells(const int u) const
{
  std::vector<sta::LibertyCell*> cells;
  auto add = [&](odb::dbMaster* master) {
    sta::LibertyCell* cell = libertyCell(sta_, master);
    if (cell != nullptr && std::ranges::find(cells, cell) == cells.end()) {
      cells.push_back(cell);
    }
  };
  const std::vector<odb::dbInst*> moved = registersInto(u);
  const bool keeps = graph_->moveKeepsClass(moved, u);
  for (odb::dbInst* flop : moved) {
    if (keeps && keepsClass(graph_->regClass(flop))) {
      for (odb::dbMaster* master : cells_of_(flop->getMaster())) {
        add(master);
      }
    }
  }
  if (cells.empty()) {
    for (odb::dbMaster* master : base_cells_) {
      add(master);
    }
  }
  return cells;
}

std::vector<odb::dbInst*> Retime::registersInto(const int t) const
{
  std::vector<odb::dbInst*> registers;
  for (const FaninEdge& fe : fanin_[t]) {
    registers.insert(
        registers.end(), fe.phys->flops.begin(), fe.phys->flops.end());
  }
  return registers;
}

void Retime::computeDriverShifts()
{
  const int n = graph_->size();
  for (int t = 0; t < n; ++t) {
    for (FaninEdge& fe : fanin_[t]) {
      odb::dbITerm* source = edgeSource(fe);
      odb::dbITerm* latest = latest_output_[fe.source];
      if (source == nullptr || latest == nullptr) {
        continue;
      }
      const double source_arrival = pinArrival(source);
      const double latest_arrival = pinArrival(latest);
      if (source_arrival > kNoArrival && latest_arrival > kNoArrival) {
        fe.driver_shift = static_cast<float>(source_arrival - latest_arrival);
      }
    }
  }
}

void Retime::computeEndpoints()
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  const int n = graph_->size();
  new_setup_.assign(n, 0.0);
  orig_end_shift_.assign(n, kNegInf);
  fixed_end_shift_.assign(n, kNegInf);
  for (int v = 0; v < n; ++v) {
    odb::dbITerm* output = latest_output_[v];
    if (output == nullptr) {
      continue;
    }
    const double arrival = pinArrival(output);
    orig_end_shift_[v] = registerEndShift(v, arrival);
    fixed_end_shift_[v] = fixedEndShift(v, arrival);
    const float setup
        = registerSetupTime(sta_,
                            launch_cell_[v],
                            clock_slew_,
                            pinSlew(sta_, network->dbToSta(output)));
    new_setup_[v] = std::max(setup, 0.0f);
  }
}

double Retime::registerEndShift(const int v, const double arrival) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  double shift = kNegInf;
  for (const EdgePhys& phys : graph_->phys()[v]) {
    if (!phys.entries.empty()) {
      const sta::Pin* pin = network->dbToSta(phys.entries.front());
      shift = std::max(shift, endShiftAt(pin, arrival));
    }
  }
  return shift;
}

double Retime::fixedEndShift(const int v, const double arrival) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unordered_set<odb::dbITerm*> on_edges;
  for (const EdgePhys& phys : graph_->phys()[v]) {
    if (!phys.entries.empty()) {
      on_edges.insert(phys.entries.front());
    }
  }
  double shift = kNegInf;
  for (odb::dbITerm* output : graph_->nodes()[v]->getITerms()) {
    odb::dbNet* net = output->getNet();
    if (output->getIoType() != odb::dbIoType::OUTPUT || net == nullptr) {
      continue;
    }
    for (odb::dbBTerm* port : net->getBTerms()) {
      if (port->getIoType() == odb::dbIoType::OUTPUT) {
        shift = std::max(shift, endShiftAt(network->dbToSta(port), arrival));
      }
    }
    for (odb::dbITerm* load : net->getITerms()) {
      if (load->getIoType() == odb::dbIoType::INPUT
          && !on_edges.contains(load)) {
        shift = std::max(shift, endShiftAt(network->dbToSta(load), arrival));
      }
    }
  }
  return shift;
}

double Retime::endShiftAt(const sta::Pin* pin, const double arrival) const
{
  sta::Vertex* vertex = sta_->ensureGraph()->pinLoadVertex(pin);
  if (vertex == nullptr || !sta_->search()->isEndpoint(vertex)) {
    return kNegInf;
  }
  const float slack = sta::delayAsFloat(sta_->slack(
      vertex, sta::RiseFallBoth::riseFall(), scenes_, sta::MinMax::max()));
  return slack < sta::INF / 2 ? requiredTime() - slack - arrival : kNegInf;
}

}  // namespace ret
