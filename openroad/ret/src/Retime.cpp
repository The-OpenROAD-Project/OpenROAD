// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Retime.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include "RegisterTiming.h"
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

Retime::Retime(const RetimingGraph* graph,
               sta::dbSta* sta,
               utl::Logger* logger,
               odb::dbMTerm* flop_d_mterm,
               CellsOf cells_of)
    : graph_(graph),
      sta_(sta),
      logger_(logger),
      flop_d_mterm_(flop_d_mterm),
      cells_of_(std::move(cells_of)),
      base_cells_(cells_of_(flop_d_mterm->getMaster()))
{
}

void Retime::run(int max_moves)
{
  const int n = static_cast<int>(graph_->nodes().size());
  r_.assign(n, 0);
  moves_ = 0;
  arrivals_.assign(n, 0.0);
  arrivals_pinned_.assign(n, kUnreached);
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
  return std::max(arrivalAt(vertex, sta::RiseFall::rise()),
                  arrivalAt(vertex, sta::RiseFall::fall()));
}

float Retime::arrivalAt(sta::Vertex* vertex, const sta::RiseFall* rf) const
{
  return sta::delayAsFloat(sta_->arrival(
      vertex, rf->asRiseFallBoth(), sta_->scenes(), sta::MinMax::max()));
}

float Retime::getGateArc(odb::dbITerm* input_pin)
{
  sta::Pin* pin = sta_->getDbNetwork()->dbToSta(input_pin);
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

void Retime::computeArcDelays()
{
  const int n = static_cast<int>(graph_->nodes().size());
  sta_->updateTiming(false);
  gate_floor_.assign(n, 0.0);
  fanin_.assign(n, {});

  dcalc_ap_ = sta_->cmdScene()->dcalcAnalysisPtIndex(sta::MinMax::max());

  for (int u = 0; u < n; ++u) {
    for (odb::dbITerm* iterm : graph_->nodes()[u]->getITerms()) {
      if (iterm->getIoType() == odb::dbIoType::INPUT) {
        gate_floor_[u]
            = std::max(gate_floor_[u], static_cast<double>(getGateArc(iterm)));
      }
    }
  }

  resolveFlopDCap();
  pinned_source_.assign(n, 0.0f);
  for (int u = 0; u < n; ++u) {
    if (!graph_->pinned()[u]) {
      continue;
    }
    for (odb::dbITerm* iterm : graph_->nodes()[u]->getITerms()) {
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        pinned_source_[u] = std::max(pinned_source_[u], pinArrival(iterm));
      }
    }
  }
  computeWireArcs();
  computeGateArcs();
  computeRegisterTiming();
}

void Retime::resolveFlopDCap()
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  flop_d_cap_ = 0.0f;
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
  if (flop_d_cap_ <= 0.0f) {
    logger_->warn(
        utl::RET,
        17,
        "No register D-pin capacitance found; using combinational arcs.");
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

void Retime::computeGateArcs()
{
  const int n = static_cast<int>(graph_->nodes().size());
  std::vector<float> load_comb(n, 0.0f);
  for (int v = 0; v < n; ++v) {
    for (const EdgePhys& h : graph_->phys()[v]) {
      load_comb[v] += pinCap(h.sink);
    }
  }

  for (int v = 0; v < n; ++v) {
    for (FaninEdge& fe : fanin_[v]) {
      fe.arc_comb = characterizeArc(v, fe.sink, load_comb[v], fe.arc_comb);
      fe.arc_reg = characterizeArc(v, fe.sink, flop_d_cap_, fe.arc_reg);
    }
  }
}

float Retime::characterizeArc(int v,
                              odb::dbITerm* sink,
                              float load,
                              float fallback,
                              float forced_slew,
                              float* out_slew)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  const sta::MinMax* min_max = sta::MinMax::max();
  sta::Sdc* sdc = sta_->cmdScene()->sdc();
  odb::dbInst* gate = graph_->nodes()[v];
  sta::Pin* pin = network->dbToSta(sink);
  sta::LibertyCell* cell = network->libertyCell(gate);
  sta::LibertyPort* from = network->libertyPort(pin);
  // Per-instance PVT, then the SDC operating conditions, then the library
  // default (what the delay calculator itself uses).
  const sta::Pvt* pvt = sdc->pvt(network->dbToSta(gate), min_max);
  if (pvt == nullptr) {
    pvt = sdc->operatingConditions(min_max);
  }
  if (pvt == nullptr && cell != nullptr) {
    pvt = cell->libertyLibrary()->defaultOperatingConditions();
  }
  if (cell == nullptr || from == nullptr || pvt == nullptr) {
    return fallback;
  }
  const float in_slew = forced_slew >= 0.0f ? forced_slew : pinSlew(sta_, pin);
  float best = 0.0f;
  bool got = false;
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
  return got ? std::max(best, 0.0f) : fallback;
}

bool Retime::outputRegistered(int v) const
{
  return std::ranges::any_of(graph_->adj()[v], [&](const Edge& e) {
    return e.weight + r_[e.target] - r_[v] >= 1;
  });
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
  double a = kUnreached;
  double a_pinned = kUnreached;
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
    a_pinned = std::max(
        a_pinned, arrivals_pinned_[fe.u] + fe.driver_shift + fe.wire + arc + extra);
  }
  if (root) {
    if (graph_->pinned()[t]) {
      arrivals_[t] = kUnreached;
    } else {
      arrivals_[t] = launched ? a : gate_floor_[t];
    }
    arrivals_pinned_[t] = graph_->pinned()[t] ? pinned_source_[t] : kUnreached;
  } else {
    arrivals_[t] = a;
    arrivals_pinned_[t] = a_pinned;
  }
}

void Retime::computeArrivals()
{
  const int n = static_cast<int>(graph_->nodes().size());
  std::vector<int> indeg(n, 0);
  for (int t = 0; t < n; ++t) {
    for (const FaninEdge& fe : fanin_[t]) {
      if (isCombinational(t, fe)) {
        ++indeg[t];
      }
    }
  }
  int head = 0;
  int tail = 0;
  for (int u = 0; u < n; ++u) {
    if (indeg[u] == 0) {
      scratch_.queue[tail++] = u;
    }
  }
  while (head < tail) {
    const int u = scratch_.queue[head++];
    setArrival(u);
    for (const Edge& e : graph_->adj()[u]) {
      if (isCombinational(u, e) && --indeg[e.target] == 0) {
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
  s.saved_arrivals_pinned.clear();
  for (int t : s.dirty) {
    s.saved_arrivals.push_back(arrivals_[t]);
    s.saved_arrivals_pinned.push_back(arrivals_pinned_[t]);
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
      arrivals_pinned_[t] = kUnreached;
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
    arrivals_pinned_[s.dirty[i]] = s.saved_arrivals_pinned[i];
  }
}

bool Retime::forwardMovable(int v) const
{
  return !graph_->pinned()[v] && !fanin_[v].empty()
         && std::ranges::none_of(graph_->adj()[v],
                                 [](const Edge& e) { return e.weight > 0; })
         && std::ranges::all_of(fanin_[v], [&](const FaninEdge& fe) {
              return fe.weight + r_[v] - r_[fe.u] >= 1;
            });
}

}  // namespace ret
