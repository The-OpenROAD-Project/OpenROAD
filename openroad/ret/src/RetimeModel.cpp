// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The slack search of Retime: a delay model in which a path starts at what
// its register launches, gate arcs are calibrated to STA's and an endpoint
// is charged its setup, and a search that takes a slide by its slack there.

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
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
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace ret {

namespace {

// As in Retime.cpp.
constexpr double kExcluded = -1e30;
constexpr double kNegInf = -std::numeric_limits<double>::infinity();

constexpr float kNoArrival = -1e29f;

float arrivalAt(sta::dbSta* sta, sta::Vertex* vertex, const sta::RiseFall* rf)
{
  return sta::delayAsFloat(sta->arrival(
      vertex, rf->asRiseFallBoth(), sta->scenes(), sta::MinMax::max()));
}

float slackAtLoad(sta::dbSta* sta, sta::Graph* graph, const sta::Pin* pin)
{
  sta::Vertex* vertex = pin != nullptr ? graph->pinLoadVertex(pin) : nullptr;
  return vertex != nullptr
             ? sta::delayAsFloat(sta->slack(vertex, sta::MinMax::max()))
             : sta::INF;
}

// The latest arrival at `to` from `reached`, another output of the gate that
// drives it (a half adder's carry into its sum).
std::optional<float> arrivalThroughOutput(sta::Graph* graph,
                                          sta::Vertex* reached,
                                          sta::Vertex* to,
                                          const sta::RiseFall* rf,
                                          float at_reached,
                                          int ap)
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

// The latest arrival at `to` over the gate arcs out of `from`.
std::optional<float> arrivalThroughGate(sta::Graph* graph,
                                        sta::Vertex* from,
                                        sta::Vertex* to,
                                        float in_rise,
                                        float in_fall,
                                        int ap)
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

}  // namespace

void Retime::enableSlackSearch(std::vector<odb::dbMaster*> base_cells,
                               CellsOf cells_of,
                               bool fastest_cell)
{
  slack_search_ = true;
  base_cells_ = std::move(base_cells);
  cells_of_ = std::move(cells_of);
  fastest_cell_ = fastest_cell;
}

void Retime::computeSlackModel()
{
  const int n = static_cast<int>(graph_->nodes().size());
  unmap_on_slide_.assign(n, 0);
  new_launch_.assign(n, 0.0);
  launch_cell_.assign(n, nullptr);
  new_setup_.assign(n, 0.0);
  orig_end_shift_.assign(n, kNegInf);
  po_end_shift_.assign(n, kNegInf);
  latest_output_.assign(n, nullptr);
  clock_slew_ = 0.0f;
  launch_offset_ = 0.0;
  if (!slack_search_) {
    return;
  }
  for (int v = 0; v < n; ++v) {
    latest_output_[v] = latestOutput(v);
  }
  computeCrossExtra();
  computeLaunch();
  calibrateArcs();
  computeUnmapCost();
  computeEndpoints();
}

odb::dbITerm* Retime::latestOutput(int v)
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

odb::dbITerm* Retime::firstOutput(int v) const
{
  for (odb::dbITerm* iterm : graph_->nodes()[v]->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::OUTPUT
        && iterm->getNet() != nullptr) {
      return iterm;
    }
  }
  return nullptr;
}

odb::dbITerm* Retime::edgeSource(const FaninEdge& fe) const
{
  odb::dbITerm* source = fe.phys->driver;
  if (!fe.phys->flops.empty()) {
    odb::dbITerm* data = graph_->dataPin(fe.phys->flops.front());
    odb::dbNet* net = data != nullptr ? data->getNet() : nullptr;
    source = net != nullptr ? net->getFirstOutput() : nullptr;
  }
  if (source == nullptr || source->getInst() != graph_->nodes()[fe.u]) {
    return nullptr;
  }
  return source;
}

float Retime::pinCap(odb::dbITerm* pin) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  const sta::LibertyPort* port = network->libertyPort(network->dbToSta(pin));
  return port != nullptr ? port->capacitance() : 0.0f;
}

float Retime::loadCap(odb::dbITerm* driver) const
{
  return sta_->graphDelayCalc()->loadCap(sta_->getDbNetwork()->dbToSta(driver),
                                         sta_->cmdScene(),
                                         sta::MinMax::max());
}

double Retime::crossingDelay(int t, const FaninEdge& fe) const
{
  if (fe.weight == 0) {
    return 0.0;
  }
  return fe.cross_extra + (unmap_on_slide_[t] ? fe.unmap : 0.0);
}

void Retime::computeCrossExtra()
{
  if (sta_->cmdScene() == nullptr) {
    return;
  }
  const int n = static_cast<int>(graph_->nodes().size());
  for (int t = 0; t < n; ++t) {
    for (FaninEdge& fe : fanin_[t]) {
      if (fe.weight == 0 || fe.phys->flops.empty()
          || fe.phys->driver == nullptr) {
        continue;
      }
      odb::dbITerm* source = edgeSource(fe);
      if (source == nullptr) {
        continue;
      }
      float slew = -1.0f;
      const float source_extra = slowdownAtLoad(
          fe.u, loadCap(source), sourceLoadAfterSlide(fe, source), slew);
      const float gate_extra
          = slew >= 0.0f ? slowdownAtSlew(t, fe.sink, slew) : 0.0f;
      fe.cross_extra = source_extra + gate_extra;
    }
  }
}

float Retime::sourceLoadAfterSlide(const FaninEdge& fe, odb::dbITerm* source)
{
  float reader_caps = 0.0f;
  int readers = 0;
  for (odb::dbITerm* reader : fe.phys->driver->getNet()->getITerms()) {
    if (reader->getIoType() == odb::dbIoType::INPUT) {
      reader_caps += pinCap(reader);
      ++readers;
    }
  }
  // The gate takes its share of the wire the last register drove.
  const float wire = std::max(loadCap(fe.phys->driver) - reader_caps, 0.0f);
  const float load
      = loadCap(source) + pinCap(fe.sink) + wire / std::max(readers, 1);
  if (readers > 1) {
    return load;
  }
  // The gate was the only reader: the first register comes off the net.
  odb::dbITerm* data = graph_->dataPin(fe.phys->flops.front());
  return std::max(load - pinCap(data), 0.0f);
}

float Retime::slowdownAtLoad(int u, float load, float new_load, float& slew)
{
  float slowdown = 0.0f;
  slew = -1.0f;
  for (odb::dbITerm* input : graph_->nodes()[u]->getITerms()) {
    odb::dbNet* net = input->getNet();
    if (input->getIoType() != odb::dbIoType::INPUT || net == nullptr
        || net->getSigType().isSupply()) {
      continue;
    }
    float new_slew = -1.0f;
    const float now = characterizeArc(u, input, load, -1.0f, nullptr);
    const float then
        = characterizeArc(u, input, new_load, -1.0f, nullptr, -1.0f, &new_slew);
    if (now >= 0.0f && then >= 0.0f) {
      slowdown = std::max(slowdown, then - now);
      slew = std::max(slew, new_slew);
    }
  }
  return slowdown;
}

float Retime::slowdownAtSlew(int t, odb::dbITerm* sink, float slew)
{
  const float now = characterizeArc(t, sink, flop_d_cap_, -1.0f, nullptr);
  const float then
      = characterizeArc(t, sink, flop_d_cap_, -1.0f, nullptr, slew);
  return now >= 0.0f && then >= 0.0f ? then - now : 0.0f;
}

std::optional<float> Retime::staArc(odb::dbITerm* input, odb::dbITerm* output)
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
  const float in_rise = arrivalAt(sta_, from, sta::RiseFall::rise());
  const float in_fall = arrivalAt(sta_, from, sta::RiseFall::fall());
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
  const float out_latest = std::max(arrivalAt(sta_, to, sta::RiseFall::rise()),
                                    arrivalAt(sta_, to, sta::RiseFall::fall()));
  return std::max(std::min(*via, out_latest) - in_latest, 0.0f);
}

double Retime::modeledOutputArrival(int t, bool registered)
{
  double latest = kNegInf;
  for (odb::dbITerm* iterm : graph_->nodes()[t]->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (iterm->getIoType() != odb::dbIoType::INPUT || net == nullptr
        || net->getSigType().isSupply()) {
      continue;
    }
    double arc = -1.0;
    for (const FaninEdge& fe : fanin_[t]) {
      if (fe.sink == iterm) {
        arc = registered ? fe.arc_reg : fe.arc_comb;
        break;
      }
    }
    if (arc < 0.0) {
      arc = getGateArc(iterm);
    }
    latest = std::max(latest, pinArrival(iterm) + arc);
  }
  return latest;
}

void Retime::calibrateArcs()
{
  const int n = static_cast<int>(graph_->nodes().size());
  for (int t = 0; t < n; ++t) {
    odb::dbITerm* output = latest_output_[t];
    if (output == nullptr || fanin_[t].empty()) {
      continue;
    }
    // No slide yet: the output is registered or not as STA timed it.
    const bool registered = outputRegistered(t);
    const double latest = modeledOutputArrival(t, registered);
    const double shift = pinArrival(output) - latest;
    // An input without an arrival (a constant) leaves nothing to compare.
    if (latest == kNegInf || std::abs(shift) > 1.0) {
      continue;
    }
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
}

double Retime::launchArrival(int t, const FaninEdge& fe) const
{
  // Untouched ends: the registers on the edge are the ones STA timed.
  if (r_[fe.u] == 0 && r_[t] == 0) {
    return fe.launch;
  }
  return new_launch_[fe.u] + fe.wire;
}

bool Retime::slideKeepsClass(int t, std::vector<odb::dbInst*>& regs) const
{
  regs.clear();
  for (const FaninEdge& fe : fanin_[t]) {
    regs.insert(regs.end(), fe.phys->flops.begin(), fe.phys->flops.end());
  }
  if (regs.empty()) {
    return true;
  }
  const RegisterClass& first = graph_->regClass(regs[0]);
  for (odb::dbInst* reg : regs) {
    if (!sameRegisterClass(first, graph_->regClass(reg))) {
      return false;
    }
  }
  return first.kind != RegisterKind::kInverting || graph_->selfDual()[t];
}

float Retime::typicalGateDelay() const
{
  std::vector<float> arcs;
  for (const std::vector<FaninEdge>& fanin : fanin_) {
    for (const FaninEdge& fe : fanin) {
      if (fe.weight == 0) {
        arcs.push_back(fe.arc_comb);
      }
    }
  }
  if (arcs.empty()) {
    return 0.0f;
  }
  std::ranges::nth_element(arcs, arcs.begin() + arcs.size() / 2);
  return arcs[arcs.size() / 2];
}

int Retime::unmappedGates(const FaninEdge& fe) const
{
  int gates = 0;
  for (odb::dbInst* reg : fe.phys->flops) {
    const RegisterKind kind = graph_->regClass(reg).kind;
    if (kind == RegisterKind::kInverting) {
      gates = std::max(gates, 1);
    } else if (kind != RegisterKind::kPlain) {
      gates = std::max(gates, 2);
    }
  }
  return gates;
}

void Retime::computeUnmapCost()
{
  const float typical = typicalGateDelay();
  const int n = static_cast<int>(graph_->nodes().size());
  std::vector<odb::dbInst*> regs;
  for (int t = 0; t < n; ++t) {
    if (slideKeepsClass(t, regs)) {
      continue;
    }
    for (FaninEdge& fe : fanin_[t]) {
      const int gates = unmappedGates(fe);
      fe.unmap = static_cast<float>(gates) * typical;
      if (gates > 0) {
        unmap_on_slide_[t] = 1;
      }
    }
  }
}

double Retime::endShift(int v) const
{
  if (!slack_search_) {
    return 0.0;
  }
  const bool port = po_end_shift_[v] > kNegInf;
  double shift = port ? po_end_shift_[v] : 0.0;
  if (outputRegistered(v)) {
    // Untouched: the registers on the output are the ones STA timed, and
    // no endpoint when STA does not constrain them.
    const double registered = r_[v] == 0 ? orig_end_shift_[v] : new_setup_[v];
    shift = port ? std::max(shift, registered) : registered;
  }
  return shift;
}

void Retime::computeTails()
{
  const int n = static_cast<int>(graph_->nodes().size());
  tails_.assign(n, kNegInf);
  out_tails_.assign(n, kNegInf);
  std::vector<int> pending(n, 0);
  std::vector<int> ready;
  for (int u = 0; u < n; ++u) {
    for (const Edge& e : graph_->adj()[u]) {
      pending[u] += isCombinational(u, e) ? 1 : 0;
    }
    if (outputRegistered(u) || po_end_shift_[u] > kNegInf) {
      tails_[u] = endShift(u);
    }
    if (pending[u] == 0) {
      ready.push_back(u);
    }
  }
  // A gate is ready once every gate it feeds combinationally is done.
  for (size_t next = 0; next < ready.size(); ++next) {
    const int s = ready[next];
    const bool registered = outputRegistered(s);
    for (const FaninEdge& fe : fanin_[s]) {
      if (!isCombinational(s, fe)) {
        continue;
      }
      if (tails_[s] > kNegInf) {
        const double through = fe.wire + (registered ? fe.arc_reg : fe.arc_comb)
                               + crossingDelay(s, fe) + tails_[s];
        tails_[fe.u] = std::max(tails_[fe.u], fe.driver_shift + through);
        out_tails_[fe.u] = std::max(out_tails_[fe.u], through);
      }
      if (--pending[fe.u] == 0) {
        ready.push_back(fe.u);
      }
    }
  }
}

double Retime::requiredTime() const
{
  return launch_offset_ + graph_->clockPeriod();
}

double Retime::negativeSlackAt(int t) const
{
  if (!outputRegistered(t) && po_end_shift_[t] == kNegInf) {
    return 0.0;
  }
  const double arrival = std::max(arrivals_[t], arrivals_pi_[t]);
  if (arrival <= kExcluded / 2) {
    return 0.0;
  }
  const double slack = requiredTime() - (arrival + endShift(t));
  return std::min(slack, 0.0);
}

void Retime::sampleLaunchOffset()
{
  constexpr size_t kSamples = 400;
  const int n = static_cast<int>(graph_->nodes().size());
  std::vector<float> offsets;
  std::unordered_set<odb::dbInst*> seen;
  for (int u = 0; u < n && offsets.size() < kSamples; ++u) {
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
  sta::LibertyCell* cell
      = network->libertyCell(network->dbToSta(flop->getMaster()));
  for (odb::dbITerm* iterm : flop->getITerms()) {
    sta::Pin* pin = network->dbToSta(iterm);
    sta::LibertyPort* port
        = pin != nullptr ? network->libertyPort(pin) : nullptr;
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

std::vector<sta::LibertyCell*> Retime::newRegisterCells(int u) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::vector<sta::LibertyCell*> cells;
  auto add = [&](odb::dbMaster* master) {
    sta::LibertyCell* cell = network->libertyCell(network->dbToSta(master));
    if (cell != nullptr && std::ranges::find(cells, cell) == cells.end()) {
      cells.push_back(cell);
    }
  };
  std::vector<odb::dbInst*> moved;
  const bool keeps = slideKeepsClass(u, moved);
  for (odb::dbInst* flop : moved) {
    // Only an enable or inverting class the slide keeps is rebuilt from its
    // own cell; anything else from the base cell.
    const RegisterKind kind = graph_->regClass(flop).kind;
    if (keeps
        && (kind == RegisterKind::kEnable
            || kind == RegisterKind::kInverting)) {
      for (odb::dbMaster* master : cells_of_(flop->getMaster())) {
        add(master);
      }
    }
  }
  // Without the cell choice the register may be built from the base cell.
  if (cells.empty() || !fastest_cell_) {
    for (odb::dbMaster* master : base_cells_) {
      add(master);
    }
  }
  return cells;
}

float Retime::pickLaunchCell(int u, float load)
{
  float best = -1.0f;
  for (sta::LibertyCell* cell : newRegisterCells(u)) {
    const float delay
        = registerClockToQ(sta_, cell, nullptr, clock_slew_, load);
    const bool better = fastest_cell_ ? delay < best : delay > best;
    if (delay >= 0.0f && (best < 0.0f || better)) {
      best = delay;
      launch_cell_[u] = cell;
    }
  }
  return std::max(best, 0.0f);
}

void Retime::computeLaunch()
{
  if (sta_->cmdScene() == nullptr) {
    return;
  }
  sampleLaunchOffset();
  const float offset = static_cast<float>(launch_offset_);
  const int n = static_cast<int>(graph_->nodes().size());
  for (int u = 0; u < n; ++u) {
    // The register a slide across u builds drives the net u drives today.
    odb::dbITerm* output = firstOutput(u);
    const float clock_to_q
        = output != nullptr ? pickLaunchCell(u, loadCap(output)) : 0.0f;
    new_launch_[u] = offset + clock_to_q;
  }
}

void Retime::computeDriverShifts()
{
  const int n = static_cast<int>(graph_->nodes().size());
  for (int t = 0; t < n; ++t) {
    for (FaninEdge& fe : fanin_[t]) {
      odb::dbITerm* source = edgeSource(fe);
      odb::dbITerm* latest = latest_output_[fe.u];
      if (source == nullptr || latest == nullptr) {
        continue;
      }
      const double latest_arrival = pinArrival(latest);
      const double shift = pinArrival(source) - latest_arrival;
      if (std::abs(shift) < 1.0) {
        fe.driver_shift = static_cast<float>(shift);
      }
    }
  }
}

double Retime::registerEndShift(int v, double arrival)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Graph* graph = sta_->ensureGraph();
  double shift = kNegInf;
  for (const EdgePhys& phys : graph_->phys()[v]) {
    if (phys.flops.empty()) {
      continue;
    }
    odb::dbITerm* data = graph_->dataPin(phys.flops.front());
    const float slack = slackAtLoad(
        sta_, graph, data != nullptr ? network->dbToSta(data) : nullptr);
    if (slack < sta::INF / 2) {
      shift = std::max(shift, requiredTime() - slack - arrival);
    }
  }
  return shift;
}

double Retime::portEndShift(int v, double arrival)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Graph* graph = sta_->ensureGraph();
  double shift = kNegInf;
  for (odb::dbITerm* iterm : graph_->nodes()[v]->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (iterm->getIoType() != odb::dbIoType::OUTPUT || net == nullptr) {
      continue;
    }
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      if (bterm->getIoType() != odb::dbIoType::OUTPUT) {
        continue;
      }
      const float slack = slackAtLoad(sta_, graph, network->dbToSta(bterm));
      if (slack < sta::INF / 2) {
        shift = std::max(shift, requiredTime() - slack - arrival);
      }
    }
  }
  return shift;
}

void Retime::computeEndpoints()
{
  computeDriverShifts();
  sta::dbNetwork* network = sta_->getDbNetwork();
  const int n = static_cast<int>(graph_->nodes().size());
  for (int v = 0; v < n; ++v) {
    odb::dbITerm* output = latest_output_[v];
    if (output == nullptr) {
      continue;
    }
    const double arrival = pinArrival(output);
    orig_end_shift_[v] = registerEndShift(v, arrival);
    po_end_shift_[v] = portEndShift(v, arrival);
    const float setup = registerSetupTime(sta_,
                                          launch_cell_[v],
                                          clock_slew_,
                                          getPinSlew(network->dbToSta(output)));
    new_setup_[v] = std::max(setup, 0.0f);
  }
}

double Retime::slackOf(double arrival, double tail) const
{
  if (arrival <= kExcluded / 2 || tail == kNegInf) {
    return std::numeric_limits<double>::infinity();
  }
  return requiredTime() - (arrival + tail);
}

double Retime::slackThrough(int v) const
{
  return slackOf(std::max(arrivals_[v], arrivals_pi_[v]), tails_[v]);
}

Retime::ModelSlack Retime::modelSlack() const
{
  const int n = static_cast<int>(graph_->nodes().size());
  ModelSlack slack;
  for (int t = 0; t < n; ++t) {
    const double negative = negativeSlackAt(t);
    slack.worst = std::min(slack.worst, negative);
    slack.total += negative;
  }
  return slack;
}

Retime::SlideEffect Retime::slideEffect(int v)
{
  SlideEffect effect;
  effect.before = slackThrough(v);
  applySlide(v);
  effect.into = slackOf(std::max(arrivals_[v], arrivals_pi_[v]), endShift(v));
  const std::vector<int> changed = scratch_.dirty;
  for (const int t : changed) {
    effect.total_change += negativeSlackAt(t);
  }
  undoSlide(v);
  for (const int t : changed) {
    effect.total_change -= negativeSlackAt(t);
  }
  effect.out = slackOf(new_launch_[v], out_tails_[v]);
  return effect;
}

std::vector<std::pair<double, int>> Retime::failingSpots(
    const std::vector<char>& refused,
    bool& skipped) const
{
  const int n = static_cast<int>(graph_->nodes().size());
  std::vector<std::pair<double, int>> spots;
  for (int v = 0; v < n; ++v) {
    if (refused[v]) {
      skipped = true;
      continue;
    }
    if (!forwardMovable(v)) {
      continue;
    }
    const double slack = slackThrough(v);
    if (slack < 0.0) {
      spots.emplace_back(slack, v);
    }
  }
  std::ranges::sort(spots);
  return spots;
}

Retime::Pick Retime::pickSlide(double margin,
                               std::vector<char>& refused,
                               SlackTally& tally)
{
  const double worst = modelSlack().worst;
  Pick pick;
  int for_total = -1;
  for (const auto& [before, v] : failingSpots(refused, pick.skipped)) {
    const SlideEffect effect = slideEffect(v);
    const double after = std::min(effect.into, effect.out);
    if (after - before > margin) {
      if (effect.total_change < -margin) {
        ++tally.refused_for_total;
      } else {
        ++tally.for_spot;
        pick.gate = v;
        return pick;
      }
    }
    if (for_total < 0 && effect.total_change > margin
        && after >= worst - margin) {
      for_total = v;
    }
    if (v != for_total) {
      refused[v] = 1;
    }
  }
  if (for_total >= 0) {
    ++tally.for_total;
    pick.gate = for_total;
  }
  return pick;
}

void Retime::updateTails(std::vector<char>& refused)
{
  const std::vector<double> old_tails = tails_;
  const std::vector<double> old_out_tails = out_tails_;
  computeTails();
  if (old_tails.size() != tails_.size()) {
    return;
  }
  const int n = static_cast<int>(tails_.size());
  for (int v = 0; v < n; ++v) {
    if (old_tails[v] != tails_[v] || old_out_tails[v] != out_tails_[v]) {
      refused[v] = 0;
    }
  }
}

void Retime::reopenAround(const std::vector<int>& gates,
                          std::vector<char>& refused) const
{
  for (const int t : gates) {
    refused[t] = 0;
    for (const FaninEdge& fe : fanin_[t]) {
      refused[fe.u] = 0;
    }
    for (const Edge& e : graph_->adj()[t]) {
      refused[e.target] = 0;
    }
  }
}

void Retime::runBySlack(int max_moves)
{
  const int n = static_cast<int>(graph_->nodes().size());
  const int cap = max_moves <= 0 ? INT_MAX : max_moves;
  const double margin = 0.002 * graph_->clockPeriod();
  const ModelSlack start = modelSlack();
  SlackTally tally;
  int moves = 0;
  // Spots whose slide was refused and whose inputs have not changed since.
  std::vector<char> refused(n, 0);
  while (moves < cap) {
    updateTails(refused);
    const Pick pick = pickSlide(margin, refused, tally);
    if (pick.gate < 0) {
      // One more look at the refused spots, in case what a refusal rested on
      // has changed.
      if (!pick.skipped) {
        break;
      }
      std::ranges::fill(refused, 0);
      continue;
    }
    applySlide(pick.gate);
    ++moves;
    reportSlide(pick.gate, moves);
    reopenAround(scratch_.dirty, refused);
  }
  moves_ = moves;
  reportResult(moves, 0);
  reportSlackSearch(start, tally);
  if (logger_->debugCheck(utl::RET, "why", 1)) {
    computeTails();
    reportFailingEndpoints();
    reportStops(margin);
  }
}

void Retime::reportSlackSearch(const ModelSlack& start, const SlackTally& tally)
{
  if (quiet_) {
    return;
  }
  const ModelSlack end = modelSlack();
  auto time = [this](double seconds) {
    return sta::delayAsString(static_cast<float>(seconds), 4, sta_);
  };
  logger_->info(utl::RET,
                48,
                "Slack search: in the delay model, worst slack {} -> {}, "
                "total negative slack {} -> {}; {} slides taken for their "
                "spot, {} for the total, {} refused because the total would "
                "get worse.",
                time(start.worst),
                time(end.worst),
                time(start.total),
                time(end.total),
                tally.for_spot,
                tally.for_total,
                tally.refused_for_total);
}

std::unordered_set<const sta::Pin*> Retime::modeledEndpoints() const
{
  const int n = static_cast<int>(graph_->nodes().size());
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unordered_set<const sta::Pin*> pins;
  for (int u = 0; u < n; ++u) {
    for (const EdgePhys& phys : graph_->phys()[u]) {
      odb::dbITerm* data
          = phys.flops.empty() ? nullptr : graph_->dataPin(phys.flops.front());
      if (data != nullptr) {
        pins.insert(network->dbToSta(data));
      }
    }
    for (odb::dbITerm* iterm : graph_->nodes()[u]->getITerms()) {
      odb::dbNet* net = iterm->getNet();
      if (iterm->getIoType() != odb::dbIoType::OUTPUT || net == nullptr) {
        continue;
      }
      for (odb::dbBTerm* bterm : net->getBTerms()) {
        pins.insert(network->dbToSta(bterm));
      }
    }
  }
  return pins;
}

void Retime::reportFailingEndpoints() const
{
  enum Where
  {
    kModeled,
    kUndrivenData,
    kOffGraphData,
    kControlPin,
    kOffGraphPort,
    kOtherPin,
    kWheres,
  };
  static constexpr const char* kWhereText[kWheres] = {
      "endpoints the model reads (data pin behind a gate, output port)",
      "data pins of registers on the graph that no gate drives",
      "data pins of registers outside the graph",
      "other register pins (enable, reset)",
      "output ports no gate of the graph drives",
      "other pins",
  };
  sta::dbNetwork* network = sta_->getDbNetwork();
  const std::unordered_set<const sta::Pin*> modeled = modeledEndpoints();
  std::unordered_set<odb::dbInst*> on_graph;
  for (const std::vector<EdgePhys>& handles : graph_->phys()) {
    for (const EdgePhys& phys : handles) {
      on_graph.insert(phys.flops.begin(), phys.flops.end());
    }
  }
  std::array<int, kWheres> counts{};
  std::array<double, kWheres> totals{};
  int count = 0;
  double total = 0.0;
  for (sta::Vertex* vertex : sta_->endpoints()) {
    const float slack
        = sta::delayAsFloat(sta_->slack(vertex, sta::MinMax::max()));
    if (slack >= 0.0f) {
      continue;
    }
    const sta::Pin* pin = vertex->pin();
    odb::dbITerm* iterm = nullptr;
    odb::dbBTerm* bterm = nullptr;
    odb::dbModITerm* moditerm = nullptr;
    network->staToDb(pin, iterm, bterm, moditerm);
    Where where = kOtherPin;
    if (modeled.contains(pin)) {
      where = kModeled;
    } else if (bterm != nullptr) {
      where = kOffGraphPort;
    } else if (iterm != nullptr
               && iterm->getInst()->getMaster()->isSequential()) {
      odb::dbInst* reg = iterm->getInst();
      if (iterm != registerDataPin(sta_, reg)) {
        where = kControlPin;
      } else {
        where = on_graph.contains(reg) ? kUndrivenData : kOffGraphData;
      }
    }
    ++counts[where];
    totals[where] += slack;
    ++count;
    total += slack;
  }
  logger_->report(
      "retime why: STA has {} failing endpoints, total negative slack {}.",
      count,
      sta::delayAsString(static_cast<float>(total), 4, sta_));
  for (int where = 0; where < kWheres; ++where) {
    if (counts[where] > 0) {
      logger_->report(
          "retime why:   {:>6} endpoints, {:5.1f}% of the total: {}",
          counts[where],
          total < 0.0 ? 100.0 * totals[where] / total : 0.0,
          kWhereText[where]);
    }
  }
}

Retime::SlideStop Retime::slideStop(int s, double margin)
{
  if (forwardMovable(s)) {
    const SlideEffect effect = slideEffect(s);
    if (std::min(effect.into, effect.out) - effect.before > margin) {
      return effect.total_change < -margin ? SlideStop::kRefusedForTotal
                                           : SlideStop::kWouldHelp;
    }
    return effect.into <= effect.out ? SlideStop::kLengthenedPathLimits
                                     : SlideStop::kShortenedPathGainsLittle;
  }
  if (graph_->pinned()[s]) {
    return SlideStop::kPinnedGate;
  }
  if (fanin_[s].empty()) {
    return SlideStop::kNoFanin;
  }
  for (const Edge& e : graph_->adj()[s]) {
    if (e.weight > 0) {
      return SlideStop::kRegisteredOutput;
    }
  }
  return SlideStop::kUnregisteredInput;
}

int Retime::launchGate(int t) const
{
  const int n = static_cast<int>(graph_->nodes().size());
  int s = t;
  for (int step = 0; step < n; ++step) {
    const bool registered = outputRegistered(s);
    const FaninEdge* latest = nullptr;
    double latest_arrival = kExcluded;
    bool latest_launched = false;
    for (const FaninEdge& fe : fanin_[s]) {
      const double arc = registered ? fe.arc_reg : fe.arc_comb;
      const bool launched = !isCombinational(s, fe);
      const double arrival = launched
                                 ? launchArrival(s, fe) + arc
                                 : arrivals_[fe.u] + fe.driver_shift + fe.wire
                                       + arc + crossingDelay(s, fe);
      if (latest == nullptr || arrival > latest_arrival) {
        latest = &fe;
        latest_arrival = arrival;
        latest_launched = launched;
      }
    }
    if (latest == nullptr || latest_arrival <= kExcluded / 2) {
      return -1;
    }
    if (latest_launched) {
      return s;
    }
    s = latest->u;
  }
  return -1;
}

int Retime::pinnedStart(int t) const
{
  const int n = static_cast<int>(graph_->nodes().size());
  int s = t;
  for (int step = 0; step < n; ++step) {
    const bool registered = outputRegistered(s);
    int latest = -1;
    double latest_arrival = kExcluded / 2;
    for (const FaninEdge& fe : fanin_[s]) {
      if (!isCombinational(s, fe)) {
        continue;
      }
      const double arrival = arrivals_pi_[fe.u] + fe.driver_shift + fe.wire
                             + (registered ? fe.arc_reg : fe.arc_comb);
      if (arrival > latest_arrival) {
        latest = fe.u;
        latest_arrival = arrival;
      }
    }
    if (latest < 0) {
      break;
    }
    s = latest;
  }
  return s;
}

void Retime::reportStops(double margin)
{
  static constexpr const char* kStopText[kSlideStops] = {
      "the worst path starts at a port or at a gate that is never crossed",
      "a slide would help (the search stopped at its slide limit)",
      "a slide helps the spot but makes the total negative slack worse",
      "no gain: the path the slide lengthens (into the new register)",
      "no gain: the path the slide shortens gains too little",
      "the first gate is never crossed (reasons below)",
      "the first gate already has a register on an output",
      "the first gate has another input with no register in front of it",
      "the first gate has no input in the graph",
      "no register-to-register path traced in the model",
  };
  struct Tally
  {
    int count = 0;
    double total = 0.0;
  };
  const int n = static_cast<int>(graph_->nodes().size());
  std::array<Tally, kSlideStops> stops{};
  std::map<std::string, Tally> start_reasons;
  std::map<std::string, Tally> pinned_reasons;
  std::vector<int> root_stop(n, -1);
  Tally all;
  for (int t = 0; t < n; ++t) {
    const double negative = negativeSlackAt(t);
    if (negative >= 0.0) {
      continue;
    }
    SlideStop stop = SlideStop::kNoPath;
    if (arrivals_pi_[t] > arrivals_[t]) {
      stop = SlideStop::kStartsAtPinned;
      Tally& reason = start_reasons[graph_->pinReason(pinnedStart(t))];
      ++reason.count;
      reason.total += negative;
    } else if (const int s = launchGate(t); s >= 0) {
      if (root_stop[s] < 0) {
        root_stop[s] = static_cast<int>(slideStop(s, margin));
      }
      stop = static_cast<SlideStop>(root_stop[s]);
      if (stop == SlideStop::kPinnedGate) {
        Tally& reason = pinned_reasons[graph_->pinReason(s)];
        ++reason.count;
        reason.total += negative;
      }
    }
    Tally& tally = stops[static_cast<int>(stop)];
    ++tally.count;
    tally.total += negative;
    ++all.count;
    all.total += negative;
  }
  auto share = [&all](const Tally& part) {
    return all.total < 0.0 ? 100.0 * part.total / all.total : 0.0;
  };
  logger_->report(
      "retime why: the model has {} failing endpoints, total negative slack "
      "{}.  By what stops a slide at the start of each one's worst path:",
      all.count,
      sta::delayAsString(static_cast<float>(all.total), 4, sta_));
  for (int stop = 0; stop < kSlideStops; ++stop) {
    if (stops[stop].count > 0) {
      logger_->report(
          "retime why:   {:>6} endpoints, {:5.1f}% of the total: {}",
          stops[stop].count,
          share(stops[stop]),
          kStopText[stop]);
    }
  }
  for (const auto& [reason, tally] : start_reasons) {
    logger_->report(
        "retime why:     starts at a gate, {:>6} endpoints, {:5.1f}%: {}",
        tally.count,
        share(tally),
        reason);
  }
  for (const auto& [reason, tally] : pinned_reasons) {
    logger_->report("retime why:     never crossed, {:>6} endpoints: {}",
                    tally.count,
                    reason);
  }
}

}  // namespace ret
