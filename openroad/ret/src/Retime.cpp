// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Retime.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <limits>
#include <string>
#include <unordered_set>
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
// Arrival of a path rooted at a pinned gate: a forward move can only
// lengthen it, so it never becomes the worst arrival the search lowers.
constexpr double kExcluded = -1e30;
constexpr double kNegInf = -std::numeric_limits<double>::infinity();

constexpr double kEps = 1e-15;

constexpr const char* kSlideTableRule
    = "----------------------------------------"
      "---------------------------";
}  // namespace

Retime::Retime(const RetimingGraph* graph,
               sta::dbSta* sta,
               utl::Logger* logger,
               odb::dbMTerm* flop_d_mterm)
    : graph_(graph), sta_(sta), logger_(logger), flop_d_mterm_(flop_d_mterm)
{
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
  computeSlackModel();
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
      if (slack_search_) {
        launched = true;
        a = std::max(a, launchArrival(t, fe) + arc);
      }
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

double Retime::slide(int v)
{
  applySlide(v);
  return worstArrival();
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

double Retime::worstArrival()
{
  const int n = static_cast<int>(graph_->nodes().size());
  double worst = 0.0;
  worst_pi_ = kExcluded;
  for (int v = 0; v < n; ++v) {
    worst = std::max(worst, arrivals_[v]);
    worst_pi_ = std::max(worst_pi_, arrivals_pi_[v]);
  }
  return worst;
}

void Retime::markCriticalCone(double worst)
{
  constexpr double kSlackBand = 1e-3;
  const int n = static_cast<int>(graph_->nodes().size());
  critical_threshold_ = worst * (1.0 - kSlackBand);

  std::ranges::fill(in_critical_cone_, 0);
  int head = 0;
  int tail = 0;
  for (int v = 0; v < n; ++v) {
    if (arrivals_[v] > critical_threshold_) {
      in_critical_cone_[v] = 1;
      scratch_.queue[tail++] = v;
    }
  }
  while (head < tail) {
    const int t = scratch_.queue[head++];
    for (const FaninEdge& fe : fanin_[t]) {
      if (isCombinational(t, fe) && !in_critical_cone_[fe.u]) {
        in_critical_cone_[fe.u] = 1;
        scratch_.queue[tail++] = fe.u;
      }
    }
  }
}

double Retime::excess(double arrival) const
{
  return arrival > critical_threshold_ ? arrival - critical_threshold_ : 0.0;
}

double Retime::arrivalAfterSlide(int v) const
{
  double arrival = kNegInf;
  for (const FaninEdge& fe : fanin_[v]) {
    if (fe.weight + r_[v] - r_[fe.u] == 1) {
      arrival = std::max(arrival, arrivals_[fe.u] + fe.wire + fe.arc_reg);
    }
  }
  if (arrival == kNegInf) {
    return graph_->pinned()[v] ? kExcluded : gate_floor_[v];
  }
  return arrival;
}

void Retime::collectSlideCone(int v)
{
  scratch_.cone.clear();
  auto visit = [&](int u) {
    for (const Edge& e : graph_->adj()[u]) {
      const int t = e.target;
      if (e.weight + r_[t] - r_[u] == 0 && in_critical_cone_[t]
          && !scratch_.in_cone[t]) {
        scratch_.in_cone[t] = 1;
        scratch_.cone.push_back(t);
      }
    }
  };
  visit(v);
  size_t next = 0;
  while (next < scratch_.cone.size()) {
    visit(scratch_.cone[next++]);
  }
}

void Retime::sweepSlideCone(int v)
{
  for (int t : scratch_.cone) {
    scratch_.local_indeg[t] = 0;
  }
  for (int t : scratch_.cone) {
    for (const FaninEdge& fe : fanin_[t]) {
      if (fe.weight + r_[t] - r_[fe.u] != 0 || fe.u == v) {
        continue;
      }
      if (scratch_.in_cone[fe.u]) {
        ++scratch_.local_indeg[t];
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
    double a = kNegInf;
    for (const FaninEdge& fe : fanin_[t]) {
      if (fe.weight + r_[t] - r_[fe.u] != 0 || fe.u == v) {
        continue;
      }
      const double src
          = scratch_.in_cone[fe.u] ? scratch_.local_arr[fe.u] : arrivals_[fe.u];
      const double arc = registered ? fe.arc_reg : fe.arc_comb;
      a = std::max(a, src + fe.wire + arc);
    }
    if (a == kNegInf) {
      a = graph_->pinned()[t] ? kExcluded : gate_floor_[t];
    }
    scratch_.local_arr[t] = a;
    for (const Edge& e : graph_->adj()[t]) {
      const int s = e.target;
      if (e.weight + r_[s] - r_[t] != 0 || !scratch_.in_cone[s]) {
        continue;
      }
      if (--scratch_.local_indeg[s] == 0) {
        scratch_.queue[tail++] = s;
      }
    }
  }
}

double Retime::estimateSlideDelta(int v)
{
  const double v_delta = excess(arrivalAfterSlide(v)) - excess(arrivals_[v]);
  collectSlideCone(v);
  if (scratch_.cone.empty()) {
    return v_delta;
  }
  sweepSlideCone(v);
  double cone_delta = 0.0;
  for (int t : scratch_.cone) {
    cone_delta += excess(scratch_.local_arr[t]) - excess(arrivals_[t]);
    scratch_.in_cone[t] = 0;
  }
  return v_delta + cone_delta;
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

void Retime::run(int max_moves)
{
  const int n = static_cast<int>(graph_->nodes().size());
  r_.assign(n, 0);
  moves_ = 0;
  arrivals_.assign(n, 0.0);
  arrivals_pi_.assign(n, kExcluded);
  in_critical_cone_.assign(n, 0);
  scratch_.init(n);

  computeArcDelays();
  computeArrivals();
  double worst = worstArrival();
  markCriticalCone(worst);
  pi_limit_ = std::max(static_cast<double>(graph_->clockPeriod()), worst_pi_);
  if (worst <= 0.0) {
    if (!quiet_) {
      logger_->info(utl::RET, 21, "No register-to-register path to optimize.");
    }
    return;
  }
  if (slack_search_) {
    runBySlack(max_moves);
    return;
  }

  const int cap = max_moves <= 0 ? INT_MAX : max_moves;
  int moves = 0;
  int pi_rejects = 0;
  bool accepted = true;
  while (accepted && moves < cap) {
    accepted = false;
    for (int v = 0; v < n && !accepted; ++v) {
      if (in_critical_cone_[v] && forwardMovable(v)
          && estimateSlideDelta(v) < -kEps) {
        const double trial = slide(v);
        const bool pi_ok = worst_pi_ <= pi_limit_ + kEps;
        if (!pi_ok) {
          ++pi_rejects;
        }
        if (pi_ok && trial < worst - kEps) {
          worst = trial;
          ++moves;
          reportSlide(v, moves);
          markCriticalCone(worst);
          accepted = true;
        } else {
          undoSlide(v);
        }
      }
    }
  }
  moves_ = moves;
  reportResult(moves, pi_rejects);
}

void Retime::reportSlide(int v, int moves)
{
  if (quiet_) {
    return;
  }
  std::string net_name = "?";
  for (odb::dbITerm* iterm : graph_->nodes()[v]->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      if (iterm->getNet() != nullptr) {
        net_name = iterm->getNet()->getName();
      }
      break;
    }
  }
  int max_wr = 0;
  for (const Edge& e : graph_->adj()[v]) {
    max_wr = std::max(max_wr, e.weight + r_[e.target] - r_[v]);
  }
  std::unordered_set<odb::dbInst*> moved_flops;
  for (const FaninEdge& fe : fanin_[v]) {
    const std::vector<Edge>& edges = graph_->adj()[fe.u];
    const int edge_count = static_cast<int>(edges.size());
    for (int k = 0; k < edge_count; ++k) {
      if (edges[k].target == v) {
        const std::vector<odb::dbInst*>& flops = graph_->phys()[fe.u][k].flops;
        moved_flops.insert(flops.begin(), flops.end());
      }
    }
  }
  if (moves == 1) {
    logger_->report("");
    logger_->report(" Slide | Flops | Sinks | Max wr | {:<20} | fanout net",
                    "gate");
    logger_->report(kSlideTableRule);
  }
  logger_->report("{:>6} | {:>5} | {:>5} | {:>6} | {:<20} | {}",
                  moves,
                  moved_flops.size(),
                  graph_->adj()[v].size(),
                  max_wr,
                  graph_->nodes()[v]->getName(),
                  net_name);
}

void Retime::reportResult(int moves, int pi_rejects)
{
  if (quiet_) {
    return;
  }
  if (moves == 0) {
    logger_->info(utl::RET, 16, "Retime found no slides.");
  } else {
    logger_->report(kSlideTableRule);
    logger_->info(utl::RET, 23, "Retime found {} slides.", moves);
  }
  if (pi_rejects > 0) {
    logger_->info(utl::RET,
                  19,
                  "{} candidate moves were rejected because they would push "
                  "an input path past the clock period.",
                  pi_rejects);
  }
}

}  // namespace ret
