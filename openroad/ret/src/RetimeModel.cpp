// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
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
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace ret {

namespace {

constexpr float kNoArrival = -1e29f;
constexpr double kMaxArrivalShift = 1.0;
constexpr double kMarginFraction = 0.002;

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

}  // namespace

void Retime::computeRegisterTiming()
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
  for (int v = 0; v < n; ++v) {
    latest_output_[v] = latestOutput(v);
  }
  computeCrossExtra();
  computeLaunch();
  calibrateArcs();
  computeUnmapCost();
  computeDriverShifts();
  computeEndpoints();
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

odb::dbITerm* Retime::edgeSource(const FaninEdge& fe) const
{
  odb::dbITerm* source = fe.phys->driver;
  if (!fe.phys->flops.empty()) {
    odb::dbNet* net = graph_->chainNet(*fe.phys);
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

double Retime::crossingDelay(const int t, const FaninEdge& fe) const
{
  if (fe.weight == 0) {
    return 0.0;
  }
  return fe.cross_extra + (unmap_on_slide_[t] ? fe.unmap : 0.0);
}

void Retime::computeCrossExtra()
{
  const int n = static_cast<int>(graph_->nodes().size());
  for (int t = 0; t < n; ++t) {
    for (FaninEdge& fe : fanin_[t]) {
      if (fe.weight == 0) {
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

float Retime::sourceLoadAfterSlide(const FaninEdge& fe,
                                   odb::dbITerm* source) const
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
  return std::max(load - pinCap(fe.phys->entries.front()), 0.0f);
}

float Retime::slowdownAtLoad(const int u,
                             const float load,
                             const float new_load,
                             float& slew) const
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

double Retime::modeledOutputArrival(const int t, const bool registered) const
{
  double latest = kNegInf;
  for (odb::dbITerm* iterm : graph_->nodes()[t]->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (iterm->getIoType() != odb::dbIoType::INPUT || net == nullptr
        || net->getSigType().isSupply()) {
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
    if (latest == kNegInf || std::abs(shift) > kMaxArrivalShift) {
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

double Retime::launchArrival(const int t, const FaninEdge& fe) const
{
  // Untouched ends: the registers on the edge are the ones STA timed.
  if (r_[fe.u] == 0 && r_[t] == 0) {
    return fe.launch;
  }
  return new_launch_[fe.u] + fe.wire;
}

bool Retime::slideKeepsClass(const int t, std::vector<odb::dbInst*>& regs) const
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

double Retime::endShift(const int v) const
{
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

double Retime::negativeSlackAt(const int t) const
{
  if (!outputRegistered(t) && po_end_shift_[t] == kNegInf) {
    return 0.0;
  }
  return std::min(
      slackOf(std::max(arrivals_[t], arrivals_pinned_[t]), endShift(t)), 0.0);
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

std::vector<sta::LibertyCell*> Retime::newRegisterCells(const int u) const
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

void Retime::computeLaunch()
{
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
      if (std::abs(shift) < kMaxArrivalShift) {
        fe.driver_shift = static_cast<float>(shift);
      }
    }
  }
}

double Retime::registerEndShift(const int v, const double arrival) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Graph* graph = sta_->ensureGraph();
  double shift = kNegInf;
  for (const EdgePhys& phys : graph_->phys()[v]) {
    if (phys.entries.empty()) {
      continue;
    }
    const float slack
        = slackAtLoad(sta_, graph, network->dbToSta(phys.entries.front()));
    if (slack < sta::INF / 2) {
      shift = std::max(shift, requiredTime() - slack - arrival);
    }
  }
  return shift;
}

double Retime::portEndShift(const int v, const double arrival) const
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
    const float setup
        = registerSetupTime(sta_,
                            launch_cell_[v],
                            clock_slew_,
                            pinSlew(sta_, network->dbToSta(output)));
    new_setup_[v] = std::max(setup, 0.0f);
  }
}

double Retime::slackOf(const double arrival, const double tail) const
{
  if (arrival <= kUnreached / 2 || tail == kNegInf) {
    return std::numeric_limits<double>::infinity();
  }
  return requiredTime() - (arrival + tail);
}

double Retime::slackThrough(const int v) const
{
  return slackOf(std::max(arrivals_[v], arrivals_pinned_[v]), tails_[v]);
}

double Retime::worstSlack() const
{
  const int n = static_cast<int>(graph_->nodes().size());
  double worst = 0.0;
  for (int t = 0; t < n; ++t) {
    worst = std::min(worst, negativeSlackAt(t));
  }
  return worst;
}

Retime::SlideEffect Retime::slideEffect(const int v)
{
  SlideEffect effect;
  applySlide(v);
  effect.into
      = slackOf(std::max(arrivals_[v], arrivals_pinned_[v]), endShift(v));
  const std::vector<int>& changed = scratch_.dirty;
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

std::vector<std::pair<double, int>> Retime::failingGates(
    const std::vector<char>& refused,
    bool& skipped) const
{
  const int n = static_cast<int>(graph_->nodes().size());
  std::vector<std::pair<double, int>> gates;
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
      gates.emplace_back(slack, v);
    }
  }
  std::ranges::sort(gates);
  return gates;
}

Retime::Pick Retime::pickSlide(const double margin, std::vector<char>& refused)
{
  const double worst = worstSlack();
  Pick pick;
  int for_total = -1;
  for (const auto& [before, v] : failingGates(refused, pick.skipped)) {
    const SlideEffect effect = slideEffect(v);
    const double after = std::min(effect.into, effect.out);
    if (after - before > margin && effect.total_change >= -margin) {
      pick.gate = v;
      return pick;
    }
    if (for_total < 0 && effect.total_change > margin
        && after >= worst - margin) {
      for_total = v;
    }
    if (v != for_total) {
      refused[v] = 1;
    }
  }
  pick.gate = for_total;
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

void Retime::search(const int max_moves)
{
  const int n = static_cast<int>(graph_->nodes().size());
  const int cap = max_moves <= 0 ? INT_MAX : max_moves;
  const double margin = kMarginFraction * graph_->clockPeriod();
  // Gates whose slide was refused and whose inputs have not changed since.
  std::vector<char> refused(n, 0);
  while (moves_ < cap) {
    updateTails(refused);
    const Pick pick = pickSlide(margin, refused);
    if (pick.gate < 0) {
      // Retry the refused gates before giving up.
      if (!pick.skipped) {
        break;
      }
      std::ranges::fill(refused, 0);
      continue;
    }
    applySlide(pick.gate);
    ++moves_;
    debugPrint(logger_,
               utl::RET,
               "search",
               1,
               "slide {} across {}",
               moves_,
               graph_->nodes()[pick.gate]->getName());
    reopenAround(scratch_.dirty, refused);
  }
}

}  // namespace ret
