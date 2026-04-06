///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The OpenROAD Authors
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

// NegotiationLegalizerPass.cpp – negotiation pass and post-optimisation.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <ranges>
#include <ratio>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "NegotiationLegalizer.h"
#include "PlacementDRC.h"
#include "dpl/Opendp.h"
#include "graphics/DplObserver.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/network.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace dpl {

// ===========================================================================
// runNegotiation – top-level negotiation driver
// ===========================================================================

void NegotiationLegalizer::runNegotiation(const std::vector<int>& illegalCells)
{
  // Seed with illegal cells and all movable neighbors within the search
  // window so the loop can create space organically.
  std::unordered_set<int> active_set(illegalCells.begin(), illegalCells.end());

  for (int idx : illegalCells) {
    const HLCell& seed = cells_[idx];
    const int xlo = seed.x - horiz_window_;
    const int xhi = seed.x + seed.width + horiz_window_;
    const int ylo = seed.y - adj_window_;
    const int yhi = seed.y + seed.height + adj_window_;

    for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
      if (cells_[i].fixed) {
        continue;
      }
      const HLCell& nb = cells_[i];
      if (nb.x >= xlo && nb.x <= xhi && nb.y >= ylo && nb.y <= yhi) {
        active_set.insert(i);
      }
    }
  }

  std::vector<int> active(active_set.begin(), active_set.end());

  // Phase 1 – all active cells rip-up every iteration (isolation point = 0).
  debugPrint(logger_,
             utl::DPL,
             "negotiation",
             1,
             "Negotiation phase 1: {} active cells, {} iterations.",
             active.size(),
             max_iter_neg_);

  int prev_overflows = -1;
  int stall_count = 0;
  for (int iter = 0; iter < max_iter_neg_; ++iter) {
    debugPrint(logger_,
               utl::DPL,
               "negotiation",
               1,
               "Starting phase 1 negotiation iteration {} ({} active cells)",
               iter,
               active.size());
    const int phase_1_overflows
        = negotiationIter(active, iter, /*updateHistory=*/true);
    if (phase_1_overflows == 0) {
      debugPrint(logger_,
                 utl::DPL,
                 "negotiation",
                 1,
                 "Negotiation phase 1 converged at iteration {}.",
                 iter);
      logger_->metric("HL__converge__phase_1__iteration", iter);
      debugPause("Pause after convergence at phase 1.");
      return;
    }
    if (phase_1_overflows == prev_overflows) {
      ++stall_count;
      if (stall_count == 3) {
        logger_->warn(
            utl::DPL,
            700,
            "Negotiation phase 1: overflow stuck at {} for 3 consecutive "
            "iterations.\nUsing old diamond search for remaining cells.",
            phase_1_overflows);
        diamondRecovery(active);
        break;
      }
    } else {
      stall_count = 0;
    }
    prev_overflows = phase_1_overflows;
  }

  debugPause("Pause before negotiation phase 2.");

  // Phase 2 – isolation point active: skip already-legal cells.
  debugPrint(logger_,
             utl::DPL,
             "negotiation",
             1,
             "Negotiation phase 2: isolation point active, {} iterations.",
             kMaxIterNeg2);

  prev_overflows = -1;
  stall_count = 0;
  for (int iter = 0; iter < kMaxIterNeg2; ++iter) {
    debugPrint(logger_,
               utl::DPL,
               "negotiation",
               1,
               "Starting phase 2 negotiation iteration {} (+{} phase 1 "
               "iterations) ({} active cells)",
               iter,
               max_iter_neg_,
               active.size());
    const int phase_2_overflows
        = negotiationIter(active, iter + max_iter_neg_, /*updateHistory=*/true);
    if (phase_2_overflows == 0) {
      debugPrint(logger_,
                 utl::DPL,
                 "negotiation",
                 1,
                 "Negotiation phase 2 converged at iteration {}.",
                 iter);
      logger_->metric("HL__converge__phase_2__iteration", iter);
      debugPause("Pause after convergence at phase 2.");
      return;
    }
    if (phase_2_overflows == prev_overflows) {
      ++stall_count;
      if (stall_count == 3) {
        logger_->warn(utl::DPL,
                      702,
                      "Negotiation phase 2: overflow stuck at {} for 3 "
                      "consecutive iterations.",
                      phase_2_overflows);
        diamondRecovery(active);
        break;
      }
    } else {
      stall_count = 0;
    }
    prev_overflows = phase_2_overflows;
  }

  // Non-convergence is reported by the caller (Opendp::detailedPlacement)
  // via numViolations(), which avoids registering a message ID in this file.
  debugPrint(logger_,
             utl::DPL,
             "negotiation",
             1,
             "Negotiation did not fully converge. Remaining violations: {}.",
             numViolations());
  debugPause("Pause after non-convergence at negotiation phases 1 and 2.");
}

// ===========================================================================
// negotiationIter – one full rip-up/replace sweep over activeCells
// ===========================================================================

int NegotiationLegalizer::negotiationIter(std::vector<int>& activeCells,
                                          int iter,
                                          bool updateHistory)
{
  using Clock = std::chrono::steady_clock;
  const auto t0 = Clock::now();

  // Reset findBestLocation profiling accumulators.
  prof_init_search_ns_ = 0;
  prof_curr_search_ns_ = 0;
  prof_snap_ns_ = 0;
  prof_filter_ns_ = 0;
  prof_neg_cost_ns_ = 0;
  prof_drc_ns_ = 0;
  prof_candidates_evaluated_ = 0;
  prof_candidates_filtered_ = 0;

  int moves_count = 0;
  sortByNegotiationOrder(activeCells);

  const auto t1 = Clock::now();

  double rip_up_ms = 0, find_best_ms = 0, place_ms = 0;
  for (int idx : activeCells) {
    if (cells_[idx].fixed) {
      continue;
    }
    // Isolation point: skip legal cells during phase 2.
    if (iter >= kIsolationPt && isCellLegal(idx)) {
      continue;
    }
    auto ta = Clock::now();
    ripUp(idx);
    auto tb = Clock::now();
    const auto [bx, by] = findBestLocation(idx, iter);
    auto tc = Clock::now();
    place(idx, bx, by);
    auto td = Clock::now();
    rip_up_ms += std::chrono::duration<double, std::milli>(tb - ta).count();
    find_best_ms += std::chrono::duration<double, std::milli>(tc - tb).count();
    place_ms += std::chrono::duration<double, std::milli>(td - tc).count();
    moves_count++;
    debugPrint(logger_,
               utl::DPL,
               "negotiation",
               2,
               "Negotiation iter {}, cell {}, moves {}, best location {}, {}",
               iter,
               cells_[idx].db_inst->getName(),
               moves_count,
               bx,
               by);
  }

  const auto t2 = Clock::now();

  // Re-sync the DPL pixel grid before checking violations.  During the
  // rip-up/place loop above, overlapping cells can lose their pixel->cell
  // entry when a co-located cell is ripped up, leaving bystander cells
  // invisible to DRC checks.  A full re-sync restores every cell so the
  // violation scan below is accurate.
  syncAllCellsToDplGrid();

  const auto t3 = Clock::now();

  // Count remaining overflows (grid overuse) AND DRC violations.
  // Both must reach zero for the negotiation to converge.
  // Also detect any non-active cells that have become DRC-illegal
  // (e.g. a move created a one-site gap with a neighbor outside the
  // active set) and pull them in so the negotiation can fix them.
  int totalOverflow = 0;
  std::unordered_set<int> active_set(activeCells.begin(), activeCells.end());
  for (int idx : activeCells) {
    if (cells_[idx].fixed) {
      continue;
    }
    const HLCell& cell = cells_[idx];
    const int xBegin = effXBegin(cell);
    const int xEnd = effXEnd(cell);
    for (int dy = 0; dy < cell.height; ++dy) {
      for (int gx = xBegin; gx < xEnd; ++gx) {
        if (gridExists(gx, cell.y + dy)) {
          totalOverflow += gridAt(gx, cell.y + dy).overuse();
        }
      }
    }
    if (!isCellLegal(idx)) {
      ++totalOverflow;
    }
  }

  const auto t4 = Clock::now();

  // Scan all movable cells for newly-created DRC violations outside the
  // current active set.  This handles cases where a move in the active
  // set created a one-site gap (or other DRC issue) with a bystander.
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (cells_[i].fixed || active_set.contains(i)) {
      continue;
    }
    if (!isCellLegal(i)) {
      activeCells.push_back(i);
      active_set.insert(i);
      ++totalOverflow;
    }
  }

  const auto t5 = Clock::now();

  if (totalOverflow > 0 && updateHistory) {
    updateHistoryCosts();
    updateDrcHistoryCosts(activeCells);
    sortByNegotiationOrder(activeCells);
  }

  const auto t6 = Clock::now();

  auto ms = [](auto a, auto b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
  };

  if (logger_->debugCheck(utl::DPL, "negotiation_runtime", 1)) {
    const double totalMs = ms(t0, t6);
    auto pct
        = [&](double v) { return totalMs > 0 ? 100.0 * v / totalMs : 0.0; };
    const double sortMs = ms(t0, t1);
    const double syncMs = ms(t2, t3);
    const double overflowMs = ms(t3, t4);
    const double bystanderMs = ms(t4, t5);
    const double historyMs = ms(t5, t6);
    const double initSearchMs = prof_init_search_ns_ / 1e6;
    const double currSearchMs = prof_curr_search_ns_ / 1e6;
    const double snapMs = prof_snap_ns_ / 1e6;
    const double filterMs = prof_filter_ns_ / 1e6;
    const double negCostMs = prof_neg_cost_ns_ / 1e6;
    const double drcMs = prof_drc_ns_ / 1e6;
    const double overhead = find_best_ms - filterMs - negCostMs - drcMs;
    logger_->report(
        "  negotiationIter {} ({:.1f}ms, {} moves): "
        "sort {:.1f}ms ({:.0f}%), "
        "ripUp {:.1f}ms ({:.0f}%), findBest {:.1f}ms ({:.0f}%), place {:.1f}ms "
        "({:.0f}%), "
        "syncGrid {:.1f}ms ({:.0f}%), overflowCount {:.1f}ms ({:.0f}%), "
        "bystanderScan {:.1f}ms ({:.0f}%), historyUpdate {:.1f}ms ({:.0f}%)",
        iter,
        totalMs,
        moves_count,
        sortMs,
        pct(sortMs),
        rip_up_ms,
        pct(rip_up_ms),
        find_best_ms,
        pct(find_best_ms),
        place_ms,
        pct(place_ms),
        syncMs,
        pct(syncMs),
        overflowMs,
        pct(overflowMs),
        bystanderMs,
        pct(bystanderMs),
        historyMs,
        pct(historyMs));
    logger_->report(
        "    findBest by region ({} candidates, {} filtered): "
        "initSearch {:.1f}ms ({:.0f}%), currSearch {:.1f}ms ({:.0f}%), "
        "snap {:.1f}ms ({:.0f}%)",
        prof_candidates_evaluated_,
        prof_candidates_filtered_,
        initSearchMs,
        pct(initSearchMs),
        currSearchMs,
        pct(currSearchMs),
        snapMs,
        pct(snapMs));
    logger_->report(
        "    findBest by function: "
        "filter {:.1f}ms ({:.0f}%), negCost {:.1f}ms ({:.0f}%), "
        "drc {:.1f}ms ({:.0f}%), overhead {:.1f}ms ({:.0f}%)",
        filterMs,
        pct(filterMs),
        negCostMs,
        pct(negCostMs),
        drcMs,
        pct(drcMs),
        overhead,
        pct(overhead));
  }

  logger_->report(
      "Negotiation iteration {}: total overflow {}.", iter, totalOverflow);
  if (opendp_->iterative_debug_ && debug_observer_) {
    setDplPositions();
    pushNegotiationPixels();
    logger_->report("Pause after negotiation iteration {}.", iter);
    debug_observer_->redrawAndPause();
  }
  return totalOverflow;
}

// ===========================================================================
// ripUp / place
// ===========================================================================

void NegotiationLegalizer::ripUp(int cellIdx)
{
  eraseCellFromDplGrid(cellIdx);
  addUsage(cellIdx, -1);
}

void NegotiationLegalizer::place(int cellIdx, int x, int y)
{
  cells_[cellIdx].x = x;
  cells_[cellIdx].y = y;
  addUsage(cellIdx, 1);
  syncCellToDplGrid(cellIdx);
  if (opendp_->deep_iterative_debug_ && debug_observer_) {
    const odb::dbInst* debug_inst = debug_observer_->getDebugInstance();
    if (!debug_inst || cells_[cellIdx].db_inst == debug_inst) {
      pushNegotiationPixels();
      logger_->report("Pause at placing of cell {}.",
                      cells_[cellIdx].db_inst->getName());
      debug_observer_->drawSelected(cells_[cellIdx].db_inst);
    }
  }
}

// ===========================================================================
// findBestLocation – enumerate candidates within the search window
// ===========================================================================

std::pair<int, int> NegotiationLegalizer::findBestLocation(int cellIdx,
                                                           int iter) const
{
  const HLCell& cell = cells_[cellIdx];

  auto best_cost = static_cast<double>(kInfCost);
  int best_x = cell.x;
  int best_y = cell.y;

  // Look up the DPL Node once for DRC checks (may be null if no
  // Opendp integration is available).
  Node* node = (opendp_ && opendp_->drc_engine_ && network_)
                   ? network_->getNode(cell.db_inst)
                   : nullptr;

  // DRC penalty escalates with iteration count: early iterations are
  // lenient (cells can tolerate DRC violations to resolve overlaps first),
  // later iterations strongly penalise DRC violations to force resolution.
  const double kDrcPenalty = 1e3 * (1.0 + iter);

  using Clock = std::chrono::steady_clock;
  Clock::duration local_filter_time{};
  Clock::duration local_neg_cost_time{};
  Clock::duration local_drc_time{};

  // Helper: evaluate one candidate position.
  auto tryLocation = [&](int tx, int ty) {
    const auto t_filter = Clock::now();
    if (!inDie(tx, ty, cell.width, cell.height) || !isValidRow(ty, cell, tx)
        || !respectsFence(cellIdx, tx, ty)) {
      local_filter_time += Clock::now() - t_filter;
      ++prof_candidates_filtered_;
      return;
    }
    local_filter_time += Clock::now() - t_filter;

    const auto t_neg = Clock::now();
    double cost = negotiationCost(cellIdx, tx, ty);
    local_neg_cost_time += Clock::now() - t_neg;

    // Add a DRC penalty so clean positions are strongly preferred,
    // but a DRC-violating position can still be chosen if nothing
    // better is available (avoids infinite non-convergence).
    if (node != nullptr) {
      const auto t_drc = Clock::now();
      odb::dbOrientType targetOrient = node->getOrient();
      odb::dbSite* site = cell.db_inst->getMaster()->getSite();
      if (site != nullptr) {
        auto orient
            = opendp_->grid_->getSiteOrientation(GridX{tx}, GridY{ty}, site);
        if (orient.has_value()) {
          targetOrient = orient.value();
        }
      }
      const int drcCount = opendp_->drc_engine_->countDRCViolations(
          node, GridX{tx}, GridY{ty}, targetOrient);
      cost += kDrcPenalty * drcCount;
      local_drc_time += Clock::now() - t_drc;
    }
    ++prof_candidates_evaluated_;
    if (cost < best_cost) {
      best_cost = cost;
      best_x = tx;
      best_y = ty;
    }
  };

  // Search around the initial (GP) position.
  const auto tInitStart = Clock::now();
  for (int dy = -adj_window_; dy <= adj_window_; ++dy) {
    for (int dx = -horiz_window_; dx <= horiz_window_; ++dx) {
      tryLocation(cell.init_x + dx, cell.init_y + dy);
    }
  }
  const auto tInitEnd = Clock::now();

  // Also search around the current position — critical when the cell has
  // already been displaced far from init_x and needs to explore its local
  // neighbourhood to resolve DRC violations (e.g. one-site gaps).
  const auto tCurrStart = Clock::now();
  if (cell.x != cell.init_x || cell.y != cell.init_y) {
    for (int dy = -adj_window_; dy <= adj_window_; ++dy) {
      for (int dx = -horiz_window_; dx <= horiz_window_; ++dx) {
        tryLocation(cell.x + dx, cell.y + dy);
      }
    }
  }
  const auto tCurrEnd = Clock::now();

  prof_init_search_ns_
      += std::chrono::duration<double, std::nano>(tInitEnd - tInitStart)
             .count();
  prof_curr_search_ns_
      += std::chrono::duration<double, std::nano>(tCurrEnd - tCurrStart)
             .count();
  prof_filter_ns_
      += std::chrono::duration<double, std::nano>(local_filter_time).count();
  prof_neg_cost_ns_
      += std::chrono::duration<double, std::nano>(local_neg_cost_time).count();
  prof_drc_ns_
      += std::chrono::duration<double, std::nano>(local_drc_time).count();

  if (opendp_->deep_iterative_debug_ && debug_observer_) {
    const odb::dbInst* debug_inst = debug_observer_->getDebugInstance();
    if (cell.db_inst == debug_inst) {
      logger_->report("  Best location for {} is ({}, {}) with cost {}.",
                      cell.db_inst->getName(),
                      best_x,
                      best_y,
                      best_cost);
      if (node != nullptr) {
        odb::dbOrientType targetOrient = node->getOrient();
        odb::dbSite* site = cell.db_inst->getMaster()->getSite();
        if (site != nullptr) {
          auto orient = opendp_->grid_->getSiteOrientation(
              GridX{best_x}, GridY{best_y}, site);
          if (orient.has_value()) {
            targetOrient = orient.value();
          }
        }
        const int drcCount = opendp_->drc_engine_->countDRCViolations(
            node, GridX{best_x}, GridY{best_y}, targetOrient);
        logger_->report("  DRC violations at best location: {}.", drcCount);
      }
    }
  }
  return {best_x, best_y};
}

// ===========================================================================
// negotiationCost – Eq. 10 from the NBLG paper
//   Cost(x,y) = b(x,y) + Σ_grids h(g) * p(g)
// ===========================================================================

double NegotiationLegalizer::negotiationCost(int cellIdx, int x, int y) const
{
  const HLCell& cell = cells_[cellIdx];
  double cost = targetCost(cellIdx, x, y);

  const int xBegin = std::max(0, x - cell.pad_left);
  const int xEnd = std::min(grid_w_, x + cell.width + cell.pad_right);
  for (int dy = 0; dy < cell.height; ++dy) {
    for (int gx = xBegin; gx < xEnd; ++gx) {
      const int gy = y + dy;
      if (!gridExists(gx, gy)) {
        cost += kInfCost;
        continue;
      }
      const Pixel& g = gridAt(gx, gy);
      if (g.capacity == 0) {
        cost += kInfCost;  // blockage
        continue;
      }
      // Congestion term: h * p  (Eq. 10).
      // Usage is incremented by 1 to account for this cell being placed.
      const auto usageWithCell = static_cast<double>(g.usage + 1);
      const auto cap = static_cast<double>(g.capacity);
      const double penalty = usageWithCell / cap;
      cost += g.hist_cost * penalty;
    }
  }
  return cost;
}

// ===========================================================================
// targetCost – Eq. 11 from the NBLG paper
//   b(x,y) = δ + mf * max(δ − th, 0)
// ===========================================================================

double NegotiationLegalizer::targetCost(int cellIdx, int x, int y) const
{
  const HLCell& cell = cells_[cellIdx];
  const int disp = std::abs(x - cell.init_x) + std::abs(y - cell.init_y);
  return static_cast<double>(disp)
         + mf_ * static_cast<double>(std::max(0, disp - th_));
}

// ===========================================================================
// adaptivePf – Eq. 14 from the NBLG paper
//   pf = 1.0 + α * exp(−β * exp(−γ * (i − ith)))
// ===========================================================================

double NegotiationLegalizer::adaptivePf(int iter) const
{
  return 1.0 + kAlpha * std::exp(-kBeta * std::exp(-kGamma * (iter - kIth)));
}

// ===========================================================================
// updateHistoryCosts – Eq. 12 from the NBLG paper
//   h_new = h_old + hf * overuse
// ===========================================================================

void NegotiationLegalizer::updateHistoryCosts()
{
  for (int gy = 0; gy < grid_h_; ++gy) {
    for (int gx = 0; gx < grid_w_; ++gx) {
      Pixel& g = gridAt(gx, gy);
      const int ov = g.overuse();
      if (ov > 0) {
        g.hist_cost += kHfDefault * ov;
      }
    }
  }
}

// ===========================================================================
// updateDrcHistoryCosts – bump history on pixels occupied by cells that
// have DRC violations, so the negotiation loop builds pressure to move
// them away from DRC-problematic positions over iterations.
// ===========================================================================

void NegotiationLegalizer::updateDrcHistoryCosts(
    const std::vector<int>& activeCells)
{
  if (!opendp_ || !opendp_->drc_engine_ || !network_) {
    return;
  }
  for (int idx : activeCells) {
    if (cells_[idx].fixed) {
      continue;
    }
    const HLCell& cell = cells_[idx];
    Node* node = network_->getNode(cell.db_inst);
    if (node == nullptr) {
      continue;
    }
    odb::dbOrientType orient = node->getOrient();
    odb::dbSite* site = cell.db_inst->getMaster()->getSite();
    if (site != nullptr) {
      auto o = opendp_->grid_->getSiteOrientation(
          GridX{cell.x}, GridY{cell.y}, site);
      if (o.has_value()) {
        orient = o.value();
      }
    }
    const int drcCount = opendp_->drc_engine_->countDRCViolations(
        node, GridX{cell.x}, GridY{cell.y}, orient);
    if (drcCount > 0) {
      const int xBegin = effXBegin(cell);
      const int xEnd = effXEnd(cell);
      for (int dy = 0; dy < cell.height; ++dy) {
        for (int gx = xBegin; gx < xEnd; ++gx) {
          if (gridExists(gx, cell.y + dy)) {
            gridAt(gx, cell.y + dy).hist_cost += kHfDefault * drcCount;
          }
        }
      }
    }
  }
}

// ===========================================================================
// sortByNegotiationOrder
//   Primary:   total overuse descending  (most congested processed first)
//   Secondary: height ascending           (smaller cells settle before larger)
//   Tertiary:  width ascending
// ===========================================================================

void NegotiationLegalizer::sortByNegotiationOrder(
    std::vector<int>& indices) const
{
  auto cellOveruse = [this](int idx) {
    const HLCell& cell = cells_[idx];
    int ov = 0;
    const int xBegin = effXBegin(cell);
    const int xEnd = effXEnd(cell);
    for (int dy = 0; dy < cell.height; ++dy) {
      for (int gx = xBegin; gx < xEnd; ++gx) {
        if (gridExists(gx, cell.y + dy)) {
          ov += gridAt(gx, cell.y + dy).overuse();
        }
      }
    }
    return ov;
  };

  std::ranges::sort(indices, [&](int a, int b) {
    const int oa = cellOveruse(a);
    const int ob = cellOveruse(b);
    if (oa != ob) {
      return oa > ob;
    }
    if (cells_[a].height != cells_[b].height) {
      return cells_[a].height < cells_[b].height;
    }
    return cells_[a].width < cells_[b].width;
  });
}

// ===========================================================================
// greedyImprove – post-optimisation: reduce displacement without creating
// new overlaps.
// ===========================================================================

void NegotiationLegalizer::greedyImprove(int passes)
{
  std::vector<int> order;
  order.reserve(cells_.size());
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (!cells_[i].fixed) {
      order.push_back(i);
    }
  }

  for (int pass = 0; pass < passes; ++pass) {
    int improved = 0;

    for (int idx : order) {
      HLCell& cell = cells_[idx];
      const int curDisp = cell.displacement();

      ripUp(idx);

      int best_x = cell.x;
      int best_y = cell.y;
      int best_dist = curDisp;

      auto tryLoc = [&](int tx, int ty) {
        if (!inDie(tx, ty, cell.width, cell.height)) {
          return;
        }
        if (!isValidRow(ty, cell, tx)) {
          return;
        }
        if (!respectsFence(idx, tx, ty)) {
          return;
        }
        // Only accept if no new overlap or padding violation is introduced.
        const int txBegin = std::max(0, tx - cell.pad_left);
        const int txEnd = std::min(grid_w_, tx + cell.width + cell.pad_right);
        for (int dy = 0; dy < cell.height; ++dy) {
          for (int gx = txBegin; gx < txEnd; ++gx) {
            if (gridAt(gx, ty + dy).overuse() > 0) {
              return;
            }
          }
        }
        const int d = std::abs(tx - cell.init_x) + std::abs(ty - cell.init_y);
        if (d < best_dist) {
          best_dist = d;
          best_x = tx;
          best_y = ty;
        }
      };

      for (int dx = -horiz_window_; dx <= horiz_window_; ++dx) {
        tryLoc(cell.init_x + dx, cell.y);
        tryLoc(cell.init_x + dx, cell.y - cell.height);
        tryLoc(cell.init_x + dx, cell.y + cell.height);
      }

      place(idx, best_x, best_y);
      if (best_dist < curDisp) {
        ++improved;
      }
    }

    if (improved == 0) {
      break;
    }
  }
}

// ===========================================================================
// cellSwap – swap pairs of same-type cells when total displacement decreases
// without exceeding the current maximum displacement.
// ===========================================================================

void NegotiationLegalizer::cellSwap()
{
  const int maxDisp = maxDisplacement();

  // Group movable cells by (height, width, rail_type).
  struct GroupKey
  {
    int height;
    int width;
    NLPowerRailType rail;
    bool operator==(const GroupKey& o) const
    {
      return height == o.height && width == o.width && rail == o.rail;
    }
  };
  struct GroupKeyHash
  {
    size_t operator()(const GroupKey& k) const
    {
      return std::hash<int>()(k.height) ^ (std::hash<int>()(k.width) << 8)
             ^ (std::hash<int>()(static_cast<int>(k.rail)) << 16);
    }
  };

  std::unordered_map<GroupKey, std::vector<int>, GroupKeyHash> groups;
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (!cells_[i].fixed) {
      groups[{cells_[i].height, cells_[i].width, cells_[i].rail_type}]
          .push_back(i);
    }
  }

  for (auto& [key, grp] : groups) {
    for (int ii = 0; ii < static_cast<int>(grp.size()); ++ii) {
      for (int jj = ii + 1; jj < static_cast<int>(grp.size()); ++jj) {
        const int a = grp[ii];
        const int b = grp[jj];
        HLCell& ca = cells_[a];
        HLCell& cb = cells_[b];

        const int dispBefore = ca.displacement() + cb.displacement();
        const int newDispA
            = std::abs(cb.x - ca.init_x) + std::abs(cb.y - ca.init_y);
        const int newDispB
            = std::abs(ca.x - cb.init_x) + std::abs(ca.y - cb.init_y);
        const int dispAfter = newDispA + newDispB;

        if (dispAfter >= dispBefore) {
          continue;
        }
        if (newDispA > maxDisp || newDispB > maxDisp) {
          continue;
        }
        if (!respectsFence(a, cb.x, cb.y) || !respectsFence(b, ca.x, ca.y)) {
          continue;
        }
        if (!isValidRow(cb.y, ca, cb.x) || !isValidRow(ca.y, cb, ca.x)) {
          continue;
        }

        ripUp(a);
        ripUp(b);
        std::swap(ca.x, cb.x);
        std::swap(ca.y, cb.y);
        place(a, ca.x, ca.y);
        place(b, cb.x, cb.y);
      }
    }
  }
}

// ===========================================================================
// diamondRecovery – break stalls by trying the Opendp BFS diamond search on
// each illegal cell.  Unlike findBestLocation (bounded rectangular window),
// the diamond search expands outward in physical Manhattan order until it
// finds a pixel where canBePlaced() passes, so it can escape local congestion
// pockets that the rectangular window cannot resolve.
// ===========================================================================

void NegotiationLegalizer::diamondRecovery(const std::vector<int>& activeCells)
{
  if (!opendp_ || !network_) {
    return;
  }
  int recovered = 0;
  for (int idx : activeCells) {
    if (cells_[idx].fixed || isCellLegal(idx)) {
      continue;
    }
    const HLCell& cell = cells_[idx];
    Node* node = network_->getNode(cell.db_inst);
    if (node == nullptr) {
      continue;
    }
    // diamondSearch uses canBePlaced which reads the DPL pixel grid, so the
    // cell must be ripped up first to avoid blocking itself.
    ripUp(idx);
    const PixelPt pt
        = opendp_->diamondSearch(node, GridX{cell.x}, GridY{cell.y});
    if (pt.pixel) {
      place(idx, pt.x.v, pt.y.v);
      logger_->report("diamondRecovery: cell {} recovered at ({}, {}).",
                      cell.db_inst->getName(),
                      pt.x.v,
                      pt.y.v);
      ++recovered;
    } else {
      // No legal site found — restore at current position so the negotiation
      // loop can keep trying.
      place(idx, cell.x, cell.y);
    }
  }
  logger_->report("diamondRecovery: recovered {}/{} stuck cells.",
                  recovered,
                  activeCells.size());
}

}  // namespace dpl
