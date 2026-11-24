// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Retime.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>
#include <vector>

#include "RetimingGraph.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace ret {

namespace {
// The arrival at a gate no path of that root reaches.
constexpr double kExcluded = -1e30;
}  // namespace

Retime::Retime(const RetimingGraph* graph,
               sta::dbSta* sta,
               utl::Logger* logger,
               odb::dbMTerm* flop_d_mterm,
               std::vector<odb::dbMaster*> base_cells,
               CellsOf cells_of)
    : graph_(graph),
      sta_(sta),
      logger_(logger),
      flop_d_mterm_(flop_d_mterm),
      base_cells_(std::move(base_cells)),
      cells_of_(std::move(cells_of))
{
}

void Retime::run(int max_moves)
{
  const int n = static_cast<int>(graph_->nodes().size());
  r_.assign(n, 0);
  moves_ = 0;
  arrivals_.assign(n, 0.0);
  arrivals_pi_.assign(n, kExcluded);
  scratch_.init(n);

  computeArcDelays();
  computeArrivals();
  if (hasRegisterPath()) {
    search(max_moves);
  }
}

float Retime::pinArrival(odb::dbITerm* db_pin)
{
  sta::Pin* pin = sta_->getDbNetwork()->dbToSta(db_pin);
  sta::Vertex* vertex = nullptr;
  sta::Vertex* bidirect_drvr = nullptr;
  sta_->ensureGraph()->pinVertices(pin, vertex, bidirect_drvr);
  if (vertex == nullptr) {
    return 0.0f;
  }
  const float rise
      = sta::delayAsFloat(sta_->arrival(vertex,
                                        sta::RiseFall::rise()->asRiseFallBoth(),
                                        sta_->scenes(),
                                        sta::MinMax::max()));
  const float fall
      = sta::delayAsFloat(sta_->arrival(vertex,
                                        sta::RiseFall::fall()->asRiseFallBoth(),
                                        sta_->scenes(),
                                        sta::MinMax::max()));
  return std::max(rise, fall);
}

float Retime::getGateArc(odb::dbITerm* input_pin)
{
  sta::Pin* pin = sta_->getDbNetwork()->dbToSta(input_pin);
  if (pin == nullptr) {
    return 0.0f;
  }
  sta::Graph* graph = sta_->ensureGraph();
  sta::Vertex* vertex = nullptr;
  sta::Vertex* bidirect_drvr = nullptr;
  graph->pinVertices(pin, vertex, bidirect_drvr);
  if (vertex == nullptr) {
    return 0.0f;
  }
  float best = 0.0f;
  sta::VertexOutEdgeIterator edge_iter(vertex, graph);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
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

float Retime::getPinSlew(sta::Pin* pin)
{
  sta::Graph* graph = sta_->ensureGraph();
  sta::Vertex* vertex = nullptr;
  sta::Vertex* bidirect_drvr = nullptr;
  graph->pinVertices(pin, vertex, bidirect_drvr);
  if (vertex == nullptr) {
    return 0.0f;
  }
  const float rise = sta::delayAsFloat(
      graph->slew(vertex, sta::RiseFall::rise(), dcalc_ap_));
  const float fall = sta::delayAsFloat(
      graph->slew(vertex, sta::RiseFall::fall(), dcalc_ap_));
  return std::max(rise, fall);
}

void Retime::computeArcDelays()
{
  const int n = static_cast<int>(graph_->nodes().size());
  sta_->updateTiming(false);
  gate_floor_.assign(n, 0.0);
  fanin_.assign(n, {});

  sta::Scene* scene = sta_->cmdScene();
  dcalc_ap_
      = scene != nullptr ? scene->dcalcAnalysisPtIndex(sta::MinMax::max()) : 0;

  for (int u = 0; u < n; ++u) {
    for (odb::dbITerm* iterm : graph_->nodes()[u]->getITerms()) {
      if (iterm->getIoType() == odb::dbIoType::INPUT) {
        gate_floor_[u]
            = std::max(gate_floor_[u], static_cast<double>(getGateArc(iterm)));
      }
    }
  }

  resolveFlopDCap();
  pi_source_.assign(n, 0.0f);
  for (int u = 0; u < n; ++u) {
    if (!graph_->pinned()[u]) {
      continue;
    }
    for (odb::dbITerm* iterm : graph_->nodes()[u]->getITerms()) {
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        pi_source_[u] = std::max(pi_source_[u], pinArrival(iterm));
      }
    }
  }
  computeWireArcs();
  computeCombArcs();
  computeSeqArcs();
  computeRegisterTiming();
}

void Retime::resolveFlopDCap()
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  flop_d_cap_ = 0.0f;
  if (flop_d_mterm_ != nullptr) {
    sta::Cell* cell = network->dbToSta(flop_d_mterm_->getMaster());
    sta::LibertyCell* liberty_cell
        = cell != nullptr ? network->libertyCell(cell) : nullptr;
    if (liberty_cell != nullptr) {
      sta::LibertyPort* d
          = liberty_cell->findLibertyPort(flop_d_mterm_->getConstName());
      if (d != nullptr) {
        flop_d_cap_ = d->capacitance();
      }
    }
  }
  if (flop_d_cap_ <= 0.0f) {
    logger_->warn(
        utl::RET,
        17,
        "No register D-pin capacitance resolved; the registered-output "
        "arc equals the combinational one.");
  }
}

float Retime::wireDelay(const EdgePhys& phys)
{
  float wire = std::max(pinArrival(phys.sink) - pinArrival(phys.driver), 0.0f);
  for (odb::dbInst* flop : phys.flops) {
    odb::dbITerm* d = graph_->dataPin(flop);
    odb::dbNet* net = d != nullptr ? d->getNet() : nullptr;
    odb::dbITerm* source = net != nullptr ? net->getFirstOutput() : nullptr;
    if (source != nullptr) {
      wire += std::max(pinArrival(d) - pinArrival(source), 0.0f);
    }
  }
  return wire;
}

void Retime::computeWireArcs()
{
  const int n = static_cast<int>(graph_->nodes().size());
  for (int u = 0; u < n; ++u) {
    const std::vector<Edge>& edges = graph_->adj()[u];
    const std::vector<EdgePhys>& handles = graph_->phys()[u];
    const int edge_count = static_cast<int>(edges.size());
    for (int k = 0; k < edge_count; ++k) {
      odb::dbITerm* sink = handles[k].sink;
      const float wire = wireDelay(handles[k]);
      const float fallback = getGateArc(sink);
      fanin_[edges[k].target].push_back({
          .u = u,
          .weight = edges[k].weight,
          .wire = wire,
          .arc_comb = fallback,
          .arc_reg = fallback,
          .sink = sink,
          .launch = pinArrival(sink),
          .phys = &handles[k],
          .unmap = 0.0f,
          .driver_shift = 0.0f,
          .cross_extra = 0.0f,
      });
    }
  }
}

void Retime::computeCombArcs()
{
  const int n = static_cast<int>(graph_->nodes().size());
  sta::dbNetwork* network = sta_->getDbNetwork();

  std::vector<float> load_comb(n, 0.0f);
  for (int v = 0; v < n; ++v) {
    for (const EdgePhys& h : graph_->phys()[v]) {
      sta::Pin* pin = network->dbToSta(h.sink);
      sta::LibertyPort* port
          = pin != nullptr ? network->libertyPort(pin) : nullptr;
      if (port != nullptr) {
        load_comb[v] += port->capacitance();
      }
    }
  }

  int total = 0;
  std::array<int, kArcCharStages> counts{};
  for (int v = 0; v < n; ++v) {
    for (FaninEdge& fe : fanin_[v]) {
      ++total;
      ArcCharStage stage = ArcCharStage::kOk;
      fe.arc_comb
          = characterizeArc(v, fe.sink, load_comb[v], fe.arc_comb, &stage);
      ++counts[static_cast<size_t>(stage)];
    }
  }
  debugPrint(
      logger_,
      utl::RET,
      "arcs",
      1,
      "load-aware arc on {}/{} edges; annotated arc kept on {} without a "
      "cell, {} without a port, {} without operating conditions, {} without "
      "a table model; register D-pin load {:.3g}",
      counts[static_cast<size_t>(ArcCharStage::kOk)],
      total,
      counts[static_cast<size_t>(ArcCharStage::kNoCell)],
      counts[static_cast<size_t>(ArcCharStage::kNoFromPort)],
      counts[static_cast<size_t>(ArcCharStage::kNoPvt)],
      counts[static_cast<size_t>(ArcCharStage::kNoTableModel)],
      flop_d_cap_);
}

void Retime::computeSeqArcs()
{
  const int n = static_cast<int>(graph_->nodes().size());
  for (int v = 0; v < n; ++v) {
    for (FaninEdge& fe : fanin_[v]) {
      fe.arc_reg
          = characterizeArc(v, fe.sink, flop_d_cap_, fe.arc_reg, nullptr);
    }
  }
}

float Retime::characterizeArc(int v,
                              odb::dbITerm* sink,
                              float load,
                              float fallback,
                              ArcCharStage* stage_out,
                              float forced_slew,
                              float* out_slew)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Scene* scene = sta_->cmdScene();
  const sta::MinMax* min_max = sta::MinMax::max();
  sta::Sdc* sdc = scene != nullptr ? scene->sdc() : nullptr;

  ArcCharStage stage = ArcCharStage::kOk;
  float best = 0.0f;
  bool got = false;

  sta::Pin* pin = network->dbToSta(sink);
  sta::LibertyCell* cell = network->libertyCell(graph_->nodes()[v]);
  sta::LibertyPort* from = pin != nullptr ? network->libertyPort(pin) : nullptr;
  // Per-instance PVT, then the SDC operating conditions, then the library
  // default (what the delay calculator itself uses).
  sta::Instance* inst = network->dbToSta(graph_->nodes()[v]);
  const sta::Pvt* pvt
      = (sdc != nullptr && inst != nullptr) ? sdc->pvt(inst, min_max) : nullptr;
  if (pvt == nullptr && sdc != nullptr) {
    pvt = sdc->operatingConditions(min_max);
  }
  if (pvt == nullptr && cell != nullptr) {
    pvt = cell->libertyLibrary()->defaultOperatingConditions();
  }
  if (cell == nullptr) {
    stage = ArcCharStage::kNoCell;
  } else if (from == nullptr) {
    stage = ArcCharStage::kNoFromPort;
  } else if (pvt == nullptr) {
    stage = ArcCharStage::kNoPvt;
  } else {
    const float in_slew = forced_slew >= 0.0f ? forced_slew : getPinSlew(pin);
    for (sta::TimingArcSet* set : cell->timingArcSets(from, nullptr)) {
      if (set->role()->isTimingCheck()) {
        continue;
      }
      for (sta::TimingArc* arc : set->arcs()) {
        const sta::GateTableModel* model
            = dynamic_cast<const sta::GateTableModel*>(arc->model());
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
    if (!got) {
      stage = ArcCharStage::kNoTableModel;
    }
  }
  if (stage_out != nullptr) {
    *stage_out = stage;
  }
  return got ? std::max(best, 0.0f) : fallback;
}

bool Retime::outputRegistered(int v) const
{
  for (const Edge& e : graph_->adj()[v]) {
    if (e.weight + r_[e.target] - r_[v] >= 1) {
      return true;
    }
  }
  return false;
}

bool Retime::isCombinational(int u, const Edge& e) const
{
  return e.weight + r_[e.target] - r_[u] == 0;
}

bool Retime::isCombinational(int t, const FaninEdge& fe) const
{
  return fe.weight + r_[t] - r_[fe.u] == 0;
}

void Retime::setArrival(int t)
{
  const bool registered = outputRegistered(t);
  bool root = true;
  double a = kExcluded;
  double a_pi = kExcluded;
  bool launched = false;
  for (const FaninEdge& fe : fanin_[t]) {
    const double arc = registered ? fe.arc_reg : fe.arc_comb;
    if (!isCombinational(t, fe)) {
      launched = true;
      a = std::max(a, launchArrival(t, fe) + arc);
      continue;
    }
    root = false;
    const double extra = crossingDelay(t, fe);
    a = std::max(a, arrivals_[fe.u] + fe.driver_shift + fe.wire + arc + extra);
    a_pi = std::max(
        a_pi, arrivals_pi_[fe.u] + fe.driver_shift + fe.wire + arc + extra);
  }
  if (root) {
    if (graph_->pinned()[t]) {
      arrivals_[t] = kExcluded;
    } else {
      arrivals_[t] = launched ? a : gate_floor_[t];
    }
    arrivals_pi_[t] = graph_->pinned()[t] ? pi_source_[t] : kExcluded;
  } else {
    arrivals_[t] = a;
    arrivals_pi_[t] = a_pi;
  }
}

void Retime::computeArrivals()
{
  const int n = static_cast<int>(graph_->nodes().size());
  std::ranges::fill(arrivals_, 0.0);
  std::ranges::fill(arrivals_pi_, kExcluded);
  std::ranges::fill(scratch_.indeg, 0);
  for (int t = 0; t < n; ++t) {
    for (const FaninEdge& fe : fanin_[t]) {
      if (isCombinational(t, fe)) {
        ++scratch_.indeg[t];
      }
    }
  }
  int head = 0;
  int tail = 0;
  for (int u = 0; u < n; ++u) {
    if (scratch_.indeg[u] == 0) {
      scratch_.queue[tail++] = u;
    }
  }
  while (head < tail) {
    const int u = scratch_.queue[head++];
    setArrival(u);
    for (const Edge& e : graph_->adj()[u]) {
      if (isCombinational(u, e) && --scratch_.indeg[e.target] == 0) {
        scratch_.queue[tail++] = e.target;
      }
    }
  }
}

bool Retime::hasRegisterPath() const
{
  return std::ranges::any_of(arrivals_,
                             [](double arrival) { return arrival > 0.0; });
}

void Retime::applySlide(int v)
{
  r_[v] -= 1;
  Scratch& s = scratch_;
  s.dirty.clear();
  auto mark = [&](int t) {
    if (!s.in_dirty[t]) {
      s.in_dirty[t] = 1;
      s.dirty.push_back(t);
    }
  };
  mark(v);
  for (const FaninEdge& fe : fanin_[v]) {
    mark(fe.u);
  }
  for (const Edge& e : graph_->adj()[v]) {
    mark(e.target);
  }
  size_t next = 0;
  while (next < s.dirty.size()) {
    const int u = s.dirty[next++];
    for (const Edge& e : graph_->adj()[u]) {
      if (isCombinational(u, e)) {
        mark(e.target);
      }
    }
  }

  s.saved_arrivals.clear();
  s.saved_arrivals_pi.clear();
  for (int t : s.dirty) {
    s.saved_arrivals.push_back(arrivals_[t]);
    s.saved_arrivals_pi.push_back(arrivals_pi_[t]);
    s.local_indeg[t] = 0;
    for (const FaninEdge& fe : fanin_[t]) {
      if (isCombinational(t, fe) && s.in_dirty[fe.u]) {
        ++s.local_indeg[t];
      }
    }
  }
  int head = 0;
  int tail = 0;
  for (int t : s.dirty) {
    if (s.local_indeg[t] == 0) {
      s.queue[tail++] = t;
    }
  }
  while (head < tail) {
    const int t = s.queue[head++];
    setArrival(t);
    s.in_dirty[t] = 0;
    for (const Edge& e : graph_->adj()[t]) {
      if (isCombinational(t, e) && s.in_dirty[e.target]
          && --s.local_indeg[e.target] == 0) {
        s.queue[tail++] = e.target;
      }
    }
  }
  for (int t : s.dirty) {
    if (s.in_dirty[t]) {  // on a combinational loop, as computeArrivals
      arrivals_[t] = 0.0;
      arrivals_pi_[t] = kExcluded;
      s.in_dirty[t] = 0;
    }
  }
}

void Retime::undoSlide(int v)
{
  r_[v] += 1;
  const Scratch& s = scratch_;
  for (size_t i = 0; i < s.dirty.size(); ++i) {
    arrivals_[s.dirty[i]] = s.saved_arrivals[i];
    arrivals_pi_[s.dirty[i]] = s.saved_arrivals_pi[i];
  }
}

bool Retime::forwardMovable(int v) const
{
  if (graph_->pinned()[v] || fanin_[v].empty()) {
    return false;
  }
  for (const Edge& e : graph_->adj()[v]) {
    if (e.weight > 0) {
      return false;
    }
  }
  for (const FaninEdge& fe : fanin_[v]) {
    if (fe.weight + r_[v] - r_[fe.u] < 1) {
      return false;
    }
  }
  return true;
}

}  // namespace ret
