// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The search: the slides, and the arrivals and tails they change.

#include "Retime.h"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

#include "RegisterUtils.h"
#include "RetimingGraph.h"
#include "odb/db.h"
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

void Retime::Scratch::init(const int n)
{
  queue.assign(n, 0);
  local_indeg.assign(n, 0);
  in_dirty.assign(n, 0);
  dirty.clear();
  dirty.reserve(n);
  saved_arrivals.reserve(n);
}

void Retime::run(const int max_moves, const double margin)
{
  const int n = graph_->size();
  r_.assign(n, 0);
  slides_.clear();
  gains_.clear();
  arrivals_.assign(n, 0.0);
  scratch_.init(n);

  buildModel();
  computeArrivals();
  search(max_moves, margin);
  rankSlides();
}

void Retime::FailingGates::init(const int n)
{
  order.clear();
  slacks.assign(n, 0.0);
}

void Retime::FailingGates::set(const int gate, const double slack)
{
  const double negative = std::min(slack, 0.0);
  if (negative == slacks[gate]) {
    return;
  }
  if (slacks[gate] < 0.0) {
    order.erase(FailingGate{.slack = slacks[gate], .gate = gate});
  }
  if (negative < 0.0) {
    order.insert(FailingGate{.slack = negative, .gate = gate});
  }
  slacks[gate] = negative;
}

void Retime::updateFailing()
{
  for (const int v : scratch_.dirty) {
    negative_.set(v, negativeSlackAt(v));
    failing_.set(v, forwardMovable(v) ? slackThrough(v) : 0.0);
  }
}

void Retime::search(const int max_moves, const double margin_fraction)
{
  const int cap = max_moves <= 0 ? INT_MAX : max_moves;
  const double margin = margin_fraction * graph_->clockPeriod();
  const int n = graph_->size();
  failing_.init(n);
  negative_.init(n);
  computeTails();
  updateFailing();  // computeTails marked every gate
  while (getMoves() < cap) {
    double gain = 0.0;
    const int v = pickSlide(margin, gain);
    if (v < 0) {
      break;
    }
    applySlide(v);
    updateFailing();
    slides_.push_back(v);
    gains_.push_back(gain);
    debugPrint(logger_,
               utl::RET,
               "search",
               1,
               "slide {} across {}",
               getMoves(),
               graph_->nodes()[v]->getName());
    updateTails(v);
    updateFailing();
  }
}

int Retime::pickSlide(const double margin, double& gain)
{
  const double worst
      = negative_.order.empty() ? 0.0 : negative_.order.begin()->slack;
  for (const auto& [before, v] : failing_.order) {
    const SlideEffect effect = slideEffect(v);
    const double after = std::min(effect.into, effect.out);
    const bool harms = effect.total_change < -margin || after < worst - margin;
    const bool helps = after - before > margin || effect.total_change > margin;
    if (helps && !harms) {
      gain = effect.total_change;
      return v;
    }
  }
  return -1;
}

std::vector<std::vector<int>> Retime::slideNeeds() const
{
  const int count = getMoves();
  std::vector<std::vector<int>> needs(count);
  // Per gate, the slides across it so far.
  std::vector<std::vector<int>> across(graph_->nodes().size());
  for (int i = 0; i < count; ++i) {
    const int v = slides_[i];
    const int nth = static_cast<int>(across[v].size()) + 1;
    if (nth > 1) {
      needs[i].push_back(across[v].back());
    }
    for (const FaninEdge& fe : fanin_[v]) {
      // The edge held fe.weight registers; slides across fe.source brought the
      // rest.
      const int from_source = nth - fe.weight;
      if (from_source > 0) {
        needs[i].push_back(across[fe.source][from_source - 1]);
      }
    }
    across[v].push_back(i);
  }
  return needs;
}

std::vector<int> Retime::pathOf(const int end,
                                const std::vector<std::vector<int>>& needs)
{
  std::vector<int> path = {end};
  for (size_t next = 0; next < path.size(); ++next) {
    for (const int needed : needs[path[next]]) {
      if (std::ranges::find(path, needed) == path.end()) {
        path.push_back(needed);
      }
    }
  }
  std::ranges::sort(path);
  return path;
}

void Retime::rankSlides()
{
  const int count = getMoves();
  const std::vector<std::vector<int>> needs = slideNeeds();
  std::vector<bool> needed(count, false);
  for (const std::vector<int>& of_slide : needs) {
    for (const int i : of_slide) {
      needed[i] = true;
    }
  }
  std::vector<std::vector<int>> paths;
  for (int i = 0; i < count; ++i) {
    if (!needed[i]) {
      paths.push_back(pathOf(i, needs));
    }
  }
  std::vector<int> ranked;
  for (const int i : rankPaths(paths, gains_)) {
    ranked.push_back(slides_[i]);
  }
  slides_ = std::move(ranked);
}

std::vector<int> rankPaths(const std::vector<std::vector<int>>& paths,
                           const std::vector<double>& gains)
{
  const int path_count = static_cast<int>(paths.size());
  std::vector<std::vector<int>> paths_of(gains.size());
  for (int p = 0; p < path_count; ++p) {
    for (const int i : paths[p]) {
      paths_of[i].push_back(p);
    }
  }

  // An entry is stale once a slide of its path got listed: the path is
  // entered again with what is left of its gain.
  struct Entry
  {
    double gain;
    int path;
    int version;
  };
  const auto later = [](const Entry& a, const Entry& b) {
    return a.gain < b.gain || (a.gain == b.gain && a.path > b.path);
  };
  std::priority_queue<Entry, std::vector<Entry>, decltype(later)> queue(later);
  std::vector<int> version(path_count, 0);
  std::vector<bool> listed(gains.size(), false);
  const auto enqueue = [&](const int p) {
    double gain = 0.0;
    bool open = false;
    for (const int i : paths[p]) {
      if (!listed[i]) {
        gain += gains[i];
        open = true;
      }
    }
    if (open) {
      queue.push({.gain = gain, .path = p, .version = version[p]});
    }
  };
  for (int p = 0; p < path_count; ++p) {
    enqueue(p);
  }

  std::vector<int> ranked;
  while (!queue.empty()) {
    const Entry best = queue.top();
    queue.pop();
    if (best.version != version[best.path]) {
      continue;
    }
    std::vector<int> changed;
    for (const int i : paths[best.path]) {
      if (!listed[i]) {
        listed[i] = true;
        ranked.push_back(i);
        changed.insert(changed.end(), paths_of[i].begin(), paths_of[i].end());
      }
    }
    sortUnique(changed);
    for (const int p : changed) {
      ++version[p];
      enqueue(p);
    }
  }
  return ranked;
}

Retime::SlideEffect Retime::slideEffect(const int v)
{
  SlideEffect effect;
  applySlide(v);
  effect.into = slackOf(arrivals_[v], endShift(v));
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

double Retime::negativeSlackAt(const int t) const
{
  if (!outputRegistered(t) && fixed_end_shift_[t] == kNegInf) {
    return 0.0;
  }
  return std::min(slackOf(arrivals_[t], endShift(t)), 0.0);
}

double Retime::slackThrough(const int v) const
{
  return slackOf(arrivals_[v], tails_[v]);
}

double Retime::slackOf(const double arrival, const double tail) const
{
  if (arrival <= kUnreached / 2 || tail == kNegInf) {
    return std::numeric_limits<double>::infinity();
  }
  return requiredTime() - (arrival + tail);
}

double Retime::requiredTime() const
{
  return launch_offset_ + graph_->clockPeriod();
}

double Retime::endShift(const int v) const
{
  const bool fixed = fixed_end_shift_[v] > kNegInf;
  double shift = fixed ? fixed_end_shift_[v] : 0.0;
  if (outputRegistered(v)) {
    // Untouched: the registers on the output are the ones STA timed, and
    // no endpoint when STA does not constrain them.
    const double registered = r_[v] == 0 ? orig_end_shift_[v] : new_setup_[v];
    shift = fixed ? std::max(shift, registered) : registered;
  }
  return shift;
}

bool Retime::forwardMovable(const int v) const
{
  return !graph_->pinned()[v] && !fanin_[v].empty()
         && std::ranges::none_of(graph_->adj()[v],
                                 [](const Edge& e) { return e.weight > 0; })
         && std::ranges::all_of(fanin_[v], [&](const FaninEdge& fe) {
              return registersOn(v, fe) >= 1;
            });
}

bool Retime::outputRegistered(const int v) const
{
  return std::ranges::any_of(
      graph_->adj()[v], [&](const Edge& e) { return registersOn(v, e) >= 1; });
}

bool Retime::isCombinational(const int u, const Edge& e) const
{
  return registersOn(u, e) == 0;
}

bool Retime::isCombinational(const int t, const FaninEdge& fe) const
{
  return registersOn(t, fe) == 0;
}

void Retime::applySlide(const int v)
{
  r_[v] -= 1;
  scratch_.dirty.clear();
  mark(v);
  for (const FaninEdge& fe : fanin_[v]) {
    mark(fe.source);
  }
  for (const Edge& e : graph_->adj()[v]) {
    mark(e.target);
  }
  markDownstream();
  propagateArrivals();
}

void Retime::undoSlide(const int v)
{
  r_[v] += 1;
  const Scratch& s = scratch_;
  for (size_t i = 0; i < s.dirty.size(); ++i) {
    arrivals_[s.dirty[i]] = s.saved_arrivals[i];
  }
}

void Retime::computeArrivals()
{
  markAll();
  propagateArrivals();
}

void Retime::propagateArrivals()
{
  Scratch& s = scratch_;
  s.saved_arrivals.clear();
  for (const int t : s.dirty) {
    s.saved_arrivals.push_back(arrivals_[t]);
  }
  visitMarked(/*forward=*/true, [this](const int t) { setArrival(t); });
  for (const int t : s.dirty) {
    if (s.in_dirty[t]) {  // on a combinational loop
      arrivals_[t] = 0.0;
      s.in_dirty[t] = 0;
    }
  }
}

void Retime::setArrival(const int t)
{
  const bool registered = outputRegistered(t);
  double a = kUnreached;
  for (const FaninEdge& fe : fanin_[t]) {
    const double arc = registered ? fe.arc_reg : fe.arc_comb;
    if (!isCombinational(t, fe)) {
      a = std::max(a, launchArrival(t, fe) + arc);
      continue;
    }
    const double extra = crossingDelay(fe);
    a = std::max(
        a, arrivals_[fe.source] + fe.driver_shift + fe.wire + arc + extra);
  }
  arrivals_[t] = std::max(a, pinned_floor_[t]);
}

double Retime::launchArrival(const int t, const FaninEdge& fe) const
{
  // Untouched ends: the registers on the edge are the ones STA timed.
  if (r_[fe.source] == 0 && r_[t] == 0) {
    return fe.launch;
  }
  return new_launch_[fe.source] + fe.wire;
}

double Retime::crossingDelay(const FaninEdge& fe) const
{
  return fe.weight == 0 ? 0.0 : fe.cross_extra;
}

void Retime::computeTails()
{
  const int n = graph_->size();
  tails_.assign(n, kNegInf);
  out_tails_.assign(n, kNegInf);
  markAll();
  loops_ = !propagateTails();
}

void Retime::updateTails(const int v)
{
  if (loops_) {
    computeTails();
    return;
  }
  scratch_.dirty.clear();
  mark(v);
  for (const FaninEdge& fe : fanin_[v]) {
    mark(fe.source);
  }
  markUpstream();
  propagateTails();
}

bool Retime::propagateTails()
{
  Scratch& s = scratch_;
  visitMarked(/*forward=*/false, [this](const int u) { setTail(u); });
  bool all_visited = true;
  for (const int u : s.dirty) {
    if (s.in_dirty[u]) {
      setTail(u);
      all_visited = false;
    }
  }
  for (const int u : s.dirty) {
    s.in_dirty[u] = 0;
  }
  return all_visited;
}

void Retime::setTail(const int u)
{
  const bool endpoint = outputRegistered(u) || fixed_end_shift_[u] > kNegInf;
  double tail = endpoint ? endShift(u) : kNegInf;
  double out_tail = kNegInf;
  for (const auto& [s, k] : fanout_[u]) {
    const FaninEdge& fe = fanin_[s][k];
    // A gate still marked is on a combinational loop: it is not done.
    if (!isCombinational(s, fe) || scratch_.in_dirty[s]
        || tails_[s] == kNegInf) {
      continue;
    }
    const float arc = outputRegistered(s) ? fe.arc_reg : fe.arc_comb;
    const double through = fe.wire + arc + crossingDelay(fe) + tails_[s];
    tail = std::max(tail, fe.driver_shift + through);
    out_tail = std::max(out_tail, through);
  }
  tails_[u] = tail;
  out_tails_[u] = out_tail;
}

void Retime::mark(const int t)
{
  Scratch& s = scratch_;
  if (!s.in_dirty[t]) {
    s.in_dirty[t] = 1;
    s.dirty.push_back(t);
  }
}

void Retime::markAll()
{
  scratch_.dirty.clear();
  const int n = graph_->size();
  for (int t = 0; t < n; ++t) {
    mark(t);
  }
}

void Retime::markDownstream()
{
  // mark() appends to the list being read.
  size_t next = 0;
  while (next < scratch_.dirty.size()) {
    const int u = scratch_.dirty[next++];
    for (const Edge& e : graph_->adj()[u]) {
      if (isCombinational(u, e)) {
        mark(e.target);
      }
    }
  }
}

void Retime::markUpstream()
{
  size_t next = 0;
  while (next < scratch_.dirty.size()) {
    const int t = scratch_.dirty[next++];
    for (const FaninEdge& fe : fanin_[t]) {
      if (isCombinational(t, fe)) {
        mark(fe.source);
      }
    }
  }
}

void Retime::visitMarked(const bool forward,
                         const std::function<void(int)>& visit)
{
  Scratch& s = scratch_;
  for (const int t : s.dirty) {
    s.local_indeg[t] = 0;
    if (forward) {
      for (const FaninEdge& fe : fanin_[t]) {
        if (isCombinational(t, fe) && s.in_dirty[fe.source]) {
          ++s.local_indeg[t];
        }
      }
    } else {
      for (const Edge& e : graph_->adj()[t]) {
        if (isCombinational(t, e) && s.in_dirty[e.target]) {
          ++s.local_indeg[t];
        }
      }
    }
  }
  int head = 0;
  int tail = 0;
  for (const int t : s.dirty) {
    if (s.local_indeg[t] == 0) {
      s.queue[tail++] = t;
    }
  }
  const auto release = [&](const int t) {
    if (s.in_dirty[t] && --s.local_indeg[t] == 0) {
      s.queue[tail++] = t;
    }
  };
  while (head < tail) {
    const int t = s.queue[head++];
    visit(t);
    s.in_dirty[t] = 0;
    if (forward) {
      for (const Edge& e : graph_->adj()[t]) {
        if (isCombinational(t, e)) {
          release(e.target);
        }
      }
    } else {
      for (const FaninEdge& fe : fanin_[t]) {
        if (isCombinational(t, fe)) {
          release(fe.source);
        }
      }
    }
  }
}

}  // namespace ret
