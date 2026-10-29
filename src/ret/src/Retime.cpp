// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "Retime.h"

#include <algorithm>
#include <climits>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "sta/TimingRole.hh"
#include "utl/Logger.h"

namespace ret {

Retime::Retime(RetimingGraph* graph,
               sta::dbSta* sta,
               utl::Logger* logger,
               odb::dbMaster* flop_master)
    : graph_(graph), sta_(sta), logger_(logger), flop_master_(flop_master)
{
}

// Late arrival at a pin, worst of rise and fall over all scenes.
float Retime::pinArrival(odb::dbITerm* db_pin)
{
  sta::Pin* pin = sta_->getDbNetwork()->dbToSta(db_pin);
  sta::Vertex* vertex;
  sta::Vertex* bidirect_drvr;
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

// Largest annotated STA arc delay from an input pin through its gate.
float Retime::getGateArc(odb::dbITerm* input_pin)
{
  sta::Pin* pin = sta_->getDbNetwork()->dbToSta(input_pin);
  if (pin == nullptr) {
    return 0.0f;
  }
  sta::Graph* graph = sta_->ensureGraph();
  sta::Vertex* vertex;
  sta::Vertex* bidirect_drvr;
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
  sta::Vertex* vertex;
  sta::Vertex* bidirect_drvr;
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
  const int n = static_cast<int>(graph_->nodes_.size());
  gate_floor_.assign(n, 0.0);
  fanin_.assign(n, {});

  sta::Scene* scene = sta_->cmdScene();
  dcalc_ap_ = scene ? scene->dcalcAnalysisPtIndex(sta::MinMax::max()) : 0;

  // Per-gate floor: the largest annotated arc through any of its input pins.
  for (int u = 0; u < n; u++) {
    for (odb::dbITerm* iterm : graph_->nodes_[u]->getITerms()) {
      if (iterm->getIoType() == odb::dbIoType::INPUT) {
        gate_floor_[u]
            = std::max(gate_floor_[u], static_cast<double>(getGateArc(iterm)));
      }
    }
  }

  resolveFlopDCap();
  computeWireArcs();
  computeCombArcs();
  computeSeqArcs();
}

// Register D-pin load for the registered-output arc variant.
void Retime::resolveFlopDCap()
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  flop_d_cap_ = 0.0f;
  if (flop_master_ != nullptr) {
    sta::Cell* cell = network->dbToSta(flop_master_);
    sta::LibertyCell* liberty_cell
        = cell ? network->libertyCell(cell) : nullptr;
    if (liberty_cell != nullptr) {
      sta::LibertyPort* d = liberty_cell->findLibertyPort("D");
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

// Fanin skeleton: wire delay from the STA arrivals plus the annotated arc as
// the fallback for both variants.
void Retime::computeWireArcs()
{
  const int n = static_cast<int>(graph_->nodes_.size());
  for (int u = 0; u < n; u++) {
    const std::vector<Edge>& edges = graph_->adj_[u];
    const std::vector<EdgePhys>& handles = graph_->phys_[u];
    for (int k = 0; k < static_cast<int>(edges.size()); k++) {
      odb::dbITerm* sink = handles[k].sink;
      float wire = pinArrival(sink) - pinArrival(handles[k].driver);
      if (wire < 0.0f) {
        wire = 0.0f;
      }
      const float fallback = getGateArc(sink);
      fanin_[edges[k].target].push_back(
          {u, edges[k].weight, wire, fallback, fallback, sink});
    }
  }
}

// Combinational-output arc: the gate arc under the sum of its sink pin caps.
void Retime::computeCombArcs()
{
  const int n = static_cast<int>(graph_->nodes_.size());
  sta::dbNetwork* network = sta_->getDbNetwork();

  std::vector<float> load_comb(n, 0.0f);
  for (int v = 0; v < n; v++) {
    for (const EdgePhys& h : graph_->phys_[v]) {
      sta::Pin* pin = network->dbToSta(h.sink);
      sta::LibertyPort* port = pin ? network->libertyPort(pin) : nullptr;
      if (port != nullptr) {
        load_comb[v] += port->capacitance();
      }
    }
  }

  int total = 0;
  int ok = 0;
  int no_cell = 0;
  int no_port = 0;
  int no_pvt = 0;
  int no_model = 0;
  for (int v = 0; v < n; v++) {
    for (FaninEdge& fe : fanin_[v]) {
      total++;
      ArcCharStage stage = ArcCharStage::ok;
      fe.arc_comb
          = characterizeArc(v, fe.sink, load_comb[v], fe.arc_comb, &stage);
      switch (stage) {
        case ArcCharStage::ok:
          ok++;
          break;
        case ArcCharStage::no_cell:
          no_cell++;
          break;
        case ArcCharStage::no_from_port:
          no_port++;
          break;
        case ArcCharStage::no_pvt:
          no_pvt++;
          break;
        case ArcCharStage::no_table_model:
          no_model++;
          break;
      }
    }
  }
  logger_->info(
      utl::RET,
      18,
      "Load-aware arc on {}/{} edges (fell back to the annotated arc: "
      "{} no cell, {} no port, {} no operating conditions, {} no table "
      "model); register D-pin load {:.3g}",
      ok,
      total,
      no_cell,
      no_port,
      no_pvt,
      no_model,
      flop_d_cap_);
}

// Registered-output arc: the same characterization under the D-pin load.
void Retime::computeSeqArcs()
{
  const int n = static_cast<int>(graph_->nodes_.size());
  for (int v = 0; v < n; v++) {
    for (FaninEdge& fe : fanin_[v]) {
      fe.arc_reg
          = characterizeArc(v, fe.sink, flop_d_cap_, fe.arc_reg, nullptr);
    }
  }
}

// Gate arc through v from input pin `sink` under `load`, from the liberty NLDM
// table at the pin's real input slew; `fallback` when any ingredient is
// missing.
float Retime::characterizeArc(int v,
                              odb::dbITerm* sink,
                              float load,
                              float fallback,
                              ArcCharStage* stage_out)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Scene* scene = sta_->cmdScene();
  const sta::MinMax* min_max = sta::MinMax::max();
  sta::Sdc* sdc = scene ? scene->sdc() : nullptr;

  ArcCharStage stage = ArcCharStage::ok;
  float best = 0.0f;
  bool got = false;

  sta::Pin* pin = network->dbToSta(sink);
  sta::LibertyCell* cell = network->libertyCell(graph_->nodes_[v]);
  sta::LibertyPort* from = pin ? network->libertyPort(pin) : nullptr;
  // Per-instance PVT, then the SDC operating conditions, then the library
  // default (what the delay calculator itself uses).
  sta::Instance* inst = network->dbToSta(graph_->nodes_[v]);
  const sta::Pvt* pvt = (sdc && inst) ? sdc->pvt(inst, min_max) : nullptr;
  if (pvt == nullptr && sdc != nullptr) {
    pvt = sdc->operatingConditions(min_max);
  }
  if (pvt == nullptr && cell != nullptr) {
    pvt = cell->libertyLibrary()->defaultOperatingConditions();
  }
  if (cell == nullptr) {
    stage = ArcCharStage::no_cell;
  } else if (from == nullptr) {
    stage = ArcCharStage::no_from_port;
  } else if (pvt == nullptr) {
    stage = ArcCharStage::no_pvt;
  } else {
    const float in_slew = getPinSlew(pin);
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
        best = std::max(best, delay);
        got = true;
      }
    }
    if (!got) {
      stage = ArcCharStage::no_table_model;
    }
  }
  if (stage_out != nullptr) {
    *stage_out = stage;
  }
  return got ? best : fallback;
}

// True when any fanout edge of v carries a register under the current r.
bool Retime::outputRegistered(int v) const
{
  for (const Edge& e : graph_->adj_[v]) {
    if (e.weight + r_[e.target] - r_[v] >= 1) {
      return true;
    }
  }
  return false;
}

// Longest-path sweep over the w_r == 0 subgraph.  A gate's arrival is the max
// over its combinational fanins of (fanin arrival + wire + the gate's arc
// variant for the current configuration).
void Retime::computeArrivals()
{
  const int n = static_cast<int>(graph_->nodes_.size());
  std::fill(arrivals_.begin(), arrivals_.end(), 0.0);
  std::fill(scratch_.indeg.begin(), scratch_.indeg.end(), 0);
  for (int t = 0; t < n; t++) {
    for (const FaninEdge& fe : fanin_[t]) {
      if (fe.weight + r_[t] - r_[fe.u] == 0) {
        scratch_.indeg[t]++;
      }
    }
  }
  int head = 0;
  int tail = 0;
  for (int u = 0; u < n; u++) {
    if (scratch_.indeg[u] == 0) {
      arrivals_[u] = gate_floor_[u];
      scratch_.queue[tail++] = u;
    }
  }
  while (head < tail) {
    const int u = scratch_.queue[head++];
    for (const Edge& e : graph_->adj_[u]) {
      if (e.weight + r_[e.target] - r_[u] != 0) {
        continue;
      }
      const int s = e.target;
      if (--scratch_.indeg[s] == 0) {
        const bool registered = outputRegistered(s);
        double a = 0.0;
        for (const FaninEdge& fe : fanin_[s]) {
          if (fe.weight + r_[s] - r_[fe.u] != 0) {
            continue;
          }
          const double arc = registered ? fe.arc_reg : fe.arc_comb;
          a = std::max(a, arrivals_[fe.u] + fe.wire + arc);
        }
        arrivals_[s] = a;
        scratch_.queue[tail++] = s;
      }
    }
  }
}

// Fills arrivals_, marks the near-critical cone (gates within kSlackBand of
// the worst plus their combinational fanin cone) and returns the worst arrival.
double Retime::markCriticalCone()
{
  constexpr double kSlackBand = 1e-3;
  const int n = static_cast<int>(graph_->nodes_.size());
  computeArrivals();

  double worst = 0.0;
  for (int v = 0; v < n; v++) {
    worst = std::max(worst, arrivals_[v]);
  }
  critical_threshold_ = worst * (1.0 - kSlackBand);

  std::fill(in_critical_cone_.begin(), in_critical_cone_.end(), 0);
  int head = 0;
  int tail = 0;
  for (int v = 0; v < n; v++) {
    if (arrivals_[v] > critical_threshold_) {
      in_critical_cone_[v] = 1;
      scratch_.queue[tail++] = v;
    }
  }
  while (head < tail) {
    const int t = scratch_.queue[head++];
    for (const FaninEdge& fe : fanin_[t]) {
      if (fe.weight + r_[t] - r_[fe.u] != 0) {
        continue;
      }
      if (!in_critical_cone_[fe.u]) {
        in_critical_cone_[fe.u] = 1;
        scratch_.queue[tail++] = fe.u;
      }
    }
  }
  return worst;
}

// Predicted change of the worst-arrival excess for the slide r[v] -= 1,
// without mutating r_.  After the slide v's output is registered, so v uses
// the registered-output arc.  Only the near-critical cone can move the worst,
// so the recompute is restricted to it.
double Retime::estimateSlideDelta(int v)
{
  const double kNeg = -std::numeric_limits<double>::infinity();
  auto excess = [&](double a) {
    return a > critical_threshold_ ? a - critical_threshold_ : 0.0;
  };

  double new_v = kNeg;
  for (const FaninEdge& fe : fanin_[v]) {
    if (fe.weight + r_[v] - r_[fe.u] == 1) {
      new_v = std::max(new_v, arrivals_[fe.u] + fe.wire + fe.arc_reg);
    }
  }
  if (new_v == kNeg) {
    new_v = gate_floor_[v];
  }
  const double v_delta = excess(new_v) - excess(arrivals_[v]);

  scratch_.cone.clear();
  for (const Edge& e : graph_->adj_[v]) {
    const int t = e.target;
    if (e.weight + r_[t] - r_[v] == 0 && in_critical_cone_[t]
        && !scratch_.in_cone[t]) {
      scratch_.in_cone[t] = 1;
      scratch_.cone.push_back(t);
    }
  }
  if (scratch_.cone.empty()) {
    return v_delta;
  }
  for (size_t i = 0; i < scratch_.cone.size(); i++) {
    const int u = scratch_.cone[i];
    for (const Edge& e : graph_->adj_[u]) {
      const int t = e.target;
      if (e.weight + r_[t] - r_[u] == 0 && in_critical_cone_[t]
          && !scratch_.in_cone[t]) {
        scratch_.in_cone[t] = 1;
        scratch_.cone.push_back(t);
      }
    }
  }

  for (int t : scratch_.cone) {
    scratch_.local_indeg[t] = 0;
  }
  for (int t : scratch_.cone) {
    for (const FaninEdge& fe : fanin_[t]) {
      if (fe.weight + r_[t] - r_[fe.u] != 0 || fe.u == v) {
        continue;
      }
      if (scratch_.in_cone[fe.u]) {
        scratch_.local_indeg[t]++;
      }
    }
  }
  int head = 0;
  int tail = 0;
  for (int t : scratch_.cone) {
    if (scratch_.local_indeg[t] == 0) {
      scratch_.queue[tail++] = t;
    }
  }
  while (head < tail) {
    const int t = scratch_.queue[head++];
    const bool registered = outputRegistered(t);
    double a = kNeg;
    for (const FaninEdge& fe : fanin_[t]) {
      if (fe.weight + r_[t] - r_[fe.u] != 0 || fe.u == v) {
        continue;
      }
      const double src
          = scratch_.in_cone[fe.u] ? scratch_.local_arr[fe.u] : arrivals_[fe.u];
      const double arc = registered ? fe.arc_reg : fe.arc_comb;
      a = std::max(a, src + fe.wire + arc);
    }
    if (a == kNeg) {
      a = gate_floor_[t];
    }
    scratch_.local_arr[t] = a;
    for (const Edge& e : graph_->adj_[t]) {
      const int s = e.target;
      if (e.weight + r_[s] - r_[t] != 0 || !scratch_.in_cone[s]) {
        continue;
      }
      if (--scratch_.local_indeg[s] == 0) {
        scratch_.queue[tail++] = s;
      }
    }
  }

  double cone_delta = 0.0;
  for (int t : scratch_.cone) {
    cone_delta += excess(scratch_.local_arr[t]) - excess(arrivals_[t]);
    scratch_.in_cone[t] = 0;
  }
  return v_delta + cone_delta;
}

// Anchor the model to STA once: the effective period is the model's worst
// arrival plus the STA worst slack, capped at the clock period.
void Retime::calibratePeriod()
{
  eff_period_ = graph_->clock_period_;
  computeArrivals();
  double model_max = 0.0;
  for (double a : arrivals_) {
    model_max = std::max(model_max, a);
  }
  const float worst_slack
      = sta::delayAsFloat(sta_->worstSlack(sta::MinMax::max()));
  if (worst_slack > -1e6f && worst_slack < 1e6f) {
    eff_period_ = std::min(model_max + static_cast<double>(worst_slack),
                           static_cast<double>(graph_->clock_period_));
  }
  logger_->info(utl::RET,
                19,
                "Clock period {:.4g}, STA worst slack {:.4g}, effective period "
                "{:.4g}",
                graph_->clock_period_,
                worst_slack,
                eff_period_);
}

// r[v] -= 1 is legal when v is not pinned, no fanout edge already holds a
// register in the original netlist, and every fanin edge carries one.
bool Retime::forwardMovable(int v) const
{
  if (graph_->pinned_[v] || fanin_[v].empty()) {
    return false;
  }
  for (const Edge& e : graph_->adj_[v]) {
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

// When nothing can be accepted the usual cause is a worst path whose first
// gate is pinned; say so instead of reporting zero slides silently.
void Retime::reportWorstPathStart()
{
  const int n = static_cast<int>(graph_->nodes_.size());
  int t = 0;
  for (int v = 0; v < n; v++) {
    if (arrivals_[v] > arrivals_[t]) {
      t = v;
    }
  }
  for (int step = 0; step < n; step++) {
    const bool registered = outputRegistered(t);
    int best_u = -1;
    double best = -1.0;
    for (const FaninEdge& fe : fanin_[t]) {
      if (fe.weight + r_[t] - r_[fe.u] != 0) {
        continue;
      }
      const double arc = registered ? fe.arc_reg : fe.arc_comb;
      const double cand = arrivals_[fe.u] + fe.wire + arc;
      if (cand > best) {
        best = cand;
        best_u = fe.u;
      }
    }
    if (best_u < 0) {
      break;
    }
    t = best_u;
  }
  if (graph_->pinned_[t]) {
    logger_->info(utl::RET,
                  22,
                  "The worst path starts at pinned gate {}; its arrival cannot "
                  "be lowered by a forward register move.",
                  graph_->nodes_[t]->getName());
  }
}

void Retime::run(int max_moves, bool stop_at_period)
{
  const int n = static_cast<int>(graph_->nodes_.size());
  stop_at_period_ = stop_at_period;
  r_.assign(n, 0);
  arrivals_.assign(n, 0.0);
  in_critical_cone_.assign(n, 0);
  scratch_.init(n);

  computeArcDelays();
  calibratePeriod();

  double cur = markCriticalCone();
  logger_->info(
      utl::RET,
      20,
      "Start worst arrival {:.4g} (model slack {:.4g} vs period {:.4g})",
      cur,
      graph_->clock_period_ - cur,
      graph_->clock_period_);
  if (cur <= 0.0) {
    logger_->info(utl::RET, 21, "No timing to optimize.");
    return;
  }
  reportWorstPathStart();

  const int cap = max_moves <= 0 ? INT_MAX : max_moves;
  const double eps = 1e-15;
  int moves = 0;

  logger_->report("");
  logger_->report(
      " Slide |  Worst arr | Flops | Sinks | Max wr | gate -> fanout net");
  logger_->report(
      "-------+------------+-------+-------+--------+---------------------");

  while (moves < cap && !(stop_at_period_ && cur <= eff_period_)) {
    bool accepted = false;
    for (int v = 0; v < n; v++) {
      if (!in_critical_cone_[v] || !forwardMovable(v)) {
        continue;
      }
      if (estimateSlideDelta(v) >= -eps) {
        continue;  // cheap cone estimate says no improvement
      }
      r_[v] -= 1;
      const double trial = markCriticalCone();  // exact recompute decides
      if (trial < cur - eps) {
        cur = trial;
        moves++;
        std::string net_name = "?";
        for (odb::dbITerm* iterm : graph_->nodes_[v]->getITerms()) {
          if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
            if (iterm->getNet() != nullptr) {
              net_name = iterm->getNet()->getName();
            }
            break;
          }
        }
        int max_wr = 0;
        for (const Edge& e : graph_->adj_[v]) {
          max_wr = std::max(max_wr, e.weight + r_[e.target] - r_[v]);
        }
        std::unordered_set<odb::dbInst*> moved_flops;
        for (const FaninEdge& fe : fanin_[v]) {
          const std::vector<Edge>& edges = graph_->adj_[fe.u];
          for (int k = 0; k < static_cast<int>(edges.size()); k++) {
            if (edges[k].target == v) {
              const std::vector<odb::dbInst*>& flops
                  = graph_->phys_[fe.u][k].flops;
              moved_flops.insert(flops.begin(), flops.end());
            }
          }
        }
        logger_->report("{:>6} | {:>10.4g} | {:>5} | {:>5} | {:>6} | {} -> {}",
                        moves,
                        cur,
                        moved_flops.size(),
                        graph_->adj_[v].size(),
                        max_wr,
                        graph_->nodes_[v]->getName(),
                        net_name);
        accepted = true;
        break;  // first improvement: restart the scan on the new cone
      }
      r_[v] += 1;
      markCriticalCone();
    }
    if (!accepted) {
      break;
    }
  }

  logger_->report(
      "-------+------------+-------+-------+--------+---------------------");
  logger_->info(
      utl::RET,
      23,
      "Final worst arrival {:.4g} (model slack {:.4g}) after {} slides",
      cur,
      graph_->clock_period_ - cur,
      moves);
}

}  // namespace ret
