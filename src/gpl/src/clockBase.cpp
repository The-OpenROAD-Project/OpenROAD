// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "clockBase.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Clock.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Sta.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace gpl {

using utl::GPL;

ClockBase::ClockBase() = default;

ClockBase::ClockBase(sta::dbSta* sta, odb::dbDatabase* db, utl::Logger* log)
    : sta_(sta), db_(db), log_(log)
{
}

ClockBase::~ClockBase() = default;

void ClockBase::initOverflowChk()
{
  overflow_done_.assign(overflows_.size(), false);
}

void ClockBase::setVirtualCtsOverflows(const std::vector<int>& overflows)
{
  overflows_ = overflows;
  std::ranges::sort(overflows_, std::greater<int>());
  initOverflowChk();
}

size_t ClockBase::getVirtualCtsOverflowSize() const
{
  return overflows_.size();
}

bool ClockBase::isVirtualCtsOverflow(float overflow)
{
  if (overflows_.empty()) {
    return false;
  }

  const int int_overflow = static_cast<int>(std::round(overflow * 100));

  // Trigger once for each threshold when overflow drops below it,
  // matching the same pattern used in TimingBase.
  if (int_overflow > overflows_[0]) {
    return false;
  }

  bool needs_run = false;
  for (size_t i = 0; i < overflows_.size(); ++i) {
    if (overflows_[i] > int_overflow) {
      if (!overflow_done_[i]) {
        overflow_done_[i] = true;
        needs_run = true;
      }
      continue;
    }
    return needs_run;
  }
  return needs_run;
}

bool ClockBase::executeVirtualCts()
{
  if (!sta_ || !db_) {
    return false;
  }

  // Remove any previous virtual insertions before building a fresh model.
  removeVirtualCts();

  const sta::ClockSeq& clocks = sta_->cmdSdc()->clocks();
  if (clocks.empty()) {
    log_->warn(GPL, 160, "Virtual CTS: no clocks defined in design. Skipping.");
    return false;
  }

  int total_insertions = 0;
  for (const sta::Clock* clk : clocks) {
    const size_t before = virtual_inserts_.size();
    buildVirtualTreeForClock(clk);
    total_insertions += static_cast<int>(virtual_inserts_.size() - before);
  }

  if (total_insertions == 0) {
    log_->warn(
        GPL, 161, "Virtual CTS: no register clock pins found. Skipping.");
    return false;
  }

  log_->info(GPL,
             162,
             "Virtual CTS: set {} virtual clock insertion delays.",
             total_insertions);
  return true;
}

void ClockBase::buildVirtualTreeForClock(const sta::Clock* clk)
{
  if (!clk) {
    return;
  }

  // Skip clocks with invalid or infinite periods (e.g. generated clocks
  // before propagation).
  const float period = clk->period();
  if (period <= 0.0f || std::isinf(period)) {
    return;
  }

  // Build a one-element ClockSet for the query.
  sta::ClockSet clk_set;
  clk_set.insert(const_cast<sta::Clock*>(clk));

  // Find all register clock sink pins for this clock.
  sta::PinSet sink_pins
      = sta_->findRegisterClkPins(&clk_set,
                                  sta::RiseFallBoth::riseFall(),
                                  /*registers=*/true,
                                  /*latches=*/true,
                                  sta_->cmdMode());

  if (sink_pins.empty()) {
    return;
  }

  // Collect sink positions.
  struct SinkInfo
  {
    const sta::Pin* pin;
    int x;
    int y;
  };
  std::vector<SinkInfo> sinks;
  sinks.reserve(sink_pins.size());

  for (const sta::Pin* pin : sink_pins) {
    int x = 0;
    int y = 0;
    if (getPinLocation(pin, x, y)) {
      sinks.push_back({pin, x, y});
    }
  }

  if (sinks.empty()) {
    return;
  }

  // With a single sink, there is no meaningful skew to assign.
  if (sinks.size() == 1) {
    return;
  }

  const int n = static_cast<int>(sinks.size());

  // -----------------------------------------------------------------------
  // Build a minimum spanning tree (Prim's, O(n^2)) using Manhattan distance.
  // This approximates the topology a balanced CTS would produce: sinks
  // connected by short branches get low relative skew; those on long
  // branches get higher skew.
  // -----------------------------------------------------------------------
  std::vector<int> mst_parent(n, -1);
  std::vector<double> mst_edge_dist(n, 0.0);
  std::vector<bool> in_mst(n, false);
  std::vector<double> key(n, std::numeric_limits<double>::max());
  key[0] = 0.0;

  for (int step = 0; step < n; ++step) {
    // Pick the minimum-key vertex not yet in the MST.
    int u = -1;
    for (int i = 0; i < n; ++i) {
      if (!in_mst[i] && (u == -1 || key[i] < key[u])) {
        u = i;
      }
    }
    in_mst[u] = true;

    // Update keys for remaining vertices.
    for (int v = 0; v < n; ++v) {
      if (!in_mst[v]) {
        const double d = std::abs(sinks[u].x - sinks[v].x)
                         + std::abs(sinks[u].y - sinks[v].y);
        if (d < key[v]) {
          key[v] = d;
          mst_parent[v] = u;
          mst_edge_dist[v] = d;
        }
      }
    }
  }

  // -----------------------------------------------------------------------
  // Root the MST at the node geometrically closest to the centroid of all
  // sinks.  This acts as the virtual clock source (H-tree hub).
  // -----------------------------------------------------------------------
  double sum_x = 0.0;
  double sum_y = 0.0;
  for (const auto& s : sinks) {
    sum_x += s.x;
    sum_y += s.y;
  }
  const double cx = sum_x / n;
  const double cy = sum_y / n;

  int root = 0;
  double best_centroid_dist = std::numeric_limits<double>::max();
  for (int i = 0; i < n; ++i) {
    const double d = std::abs(sinks[i].x - cx) + std::abs(sinks[i].y - cy);
    if (d < best_centroid_dist) {
      best_centroid_dist = d;
      root = i;
    }
  }

  // Build an undirected adjacency list from the MST edges.
  std::vector<std::vector<std::pair<int, double>>> adj(n);
  for (int i = 0; i < n; ++i) {
    if (mst_parent[i] != -1) {
      adj[mst_parent[i]].emplace_back(i, mst_edge_dist[i]);
      adj[i].emplace_back(mst_parent[i], mst_edge_dist[i]);
    }
  }

  // BFS from the root to compute each sink's path distance through the tree.
  std::vector<double> tree_dist(n, -1.0);
  std::queue<int> bfs;
  tree_dist[root] = 0.0;
  bfs.push(root);
  while (!bfs.empty()) {
    const int u = bfs.front();
    bfs.pop();
    for (auto& [v, w] : adj[u]) {
      if (tree_dist[v] < 0.0) {
        tree_dist[v] = tree_dist[u] + w;
        bfs.push(v);
      }
    }
  }

  // -----------------------------------------------------------------------
  // Normalize so the farthest sink in the tree gets exactly
  // max_skew_fraction_ * period of insertion delay.  All others scale
  // proportionally, preserving the relative skew structure.
  // -----------------------------------------------------------------------
  const double max_tree_dist = *std::ranges::max_element(tree_dist);

  if (max_tree_dist < 1.0) {
    // All sinks are co-located; no meaningful skew to assign.
    return;
  }

  const double scale
      = static_cast<double>(max_skew_fraction_) * period / max_tree_dist;

  sta::Sdc* sdc = sta_->cmdSdc();

  for (int i = 0; i < n; ++i) {
    const float delay = static_cast<float>(tree_dist[i] * scale);
    // Use setClockLatency (network latency) rather than setClockInsertion
    // (source latency).  During global placement the clock is ideal
    // (non-propagated), and OpenSTA only honours per-pin network latency
    // for ideal clocks; per-pin source latency is silently ignored.
    sta_->setClockLatency(const_cast<sta::Clock*>(clk),
                          const_cast<sta::Pin*>(sinks[i].pin),
                          sta::RiseFallBoth::riseFall(),
                          sta::MinMaxAll::all(),
                          delay,
                          sdc);
    virtual_inserts_.push_back({clk, sinks[i].pin});
  }
}

void ClockBase::removeVirtualCts()
{
  if (virtual_inserts_.empty()) {
    return;
  }

  sta::Sdc* sdc = sta_->cmdSdc();
  for (const auto& vi : virtual_inserts_) {
    sta_->removeClockLatency(vi.clk, vi.pin, sdc);
  }
  virtual_inserts_.clear();

  log_->info(GPL, 163, "Virtual CTS: removed virtual clock insertion delays.");
}

bool ClockBase::getPinLocation(const sta::Pin* pin, int& x, int& y) const
{
  if (!pin) {
    return false;
  }

  sta::dbNetwork* network = sta_->getDbNetwork();

  odb::dbITerm* iterm = nullptr;
  odb::dbBTerm* bterm = nullptr;
  odb::dbModITerm* moditerm = nullptr;
  network->staToDb(pin, iterm, bterm, moditerm);

  if (iterm) {
    odb::dbInst* inst = iterm->getInst();
    if (!inst || !inst->isPlaced()) {
      return false;
    }
    inst->getLocation(x, y);
    return true;
  }

  if (bterm) {
    // Clock port on the block boundary – use its placement location.
    int px = 0;
    int py = 0;
    if (bterm->getFirstPinLocation(px, py)) {
      x = px;
      y = py;
      return true;
    }
  }

  return false;
}

}  // namespace gpl
