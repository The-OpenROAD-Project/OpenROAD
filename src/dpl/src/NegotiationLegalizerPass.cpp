// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "NegotiationLegalizer.h"
#include "PlacementDRC.h"
#include "dpl/Opendp.h"
#include "graphics/DplObserver.h"
#include "infrastructure/Coordinates.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/network.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"
#include "utl/timer.h"

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
    const NegCell& seed = cells_[idx];
    const int xlo = seed.x - horiz_window_;
    const int xhi = seed.x + seed.width + horiz_window_;
    const int ylo = seed.y - adj_window_;
    const int yhi = seed.y + seed.height + adj_window_;

    for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
      if (cells_[i].fixed) {
        continue;
      }
      const NegCell& nb = cells_[i];
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
      logger_->metric("negotiation__converge__phase_1__iteration", iter);
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
      logger_->metric("negotiation__converge__phase_2__iteration", iter);
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
  if (debug_observer_) {
    debug_observer_->clearNegotiationSearchWindows();
  }

  // Reset findBestLocation profiling accumulators.
  prof_init_search_s_ = 0;
  prof_curr_search_s_ = 0;
  prof_filter_s_ = 0;
  prof_neg_cost_s_ = 0;
  prof_drc_s_ = 0;
  prof_candidates_evaluated_ = 0;
  prof_candidates_filtered_ = 0;

  double sort_s{0}, rip_up_s{0}, find_best_s{0}, place_s{0};
  double sync_s{0}, overflow_s{0}, bystander_s{0}, history_s{0};
  const utl::Timer total_iter_timer;

  int moves_count = 0;
  {
    utl::DebugScopedTimer t(sort_s);
    sortByNegotiationOrder(activeCells);
  }

  for (int idx : activeCells) {
    if (cells_[idx].fixed) {
      continue;
    }
    // Isolation point: skip legal cells during phase 2.
    if (iter >= kIsolationPt && isCellLegal(idx)) {
      continue;
    }
    int bx, by;
    {
      utl::DebugScopedTimer t(rip_up_s);
      ripUp(idx);
    }
    {
      utl::DebugScopedTimer t(find_best_s);
      std::tie(bx, by) = findBestLocation(idx, iter);
    }
    {
      utl::DebugScopedTimer t(place_s);
      place(idx, bx, by);
    }
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

  // Re-sync the DPL pixel grid before checking violations.  During the
  // rip-up/place loop above, overlapping cells can lose their pixel->cell
  // entry when a co-located cell is ripped up, leaving bystander cells
  // invisible to DRC checks.  A full re-sync restores every cell so the
  // violation scan below is accurate.
  {
    utl::DebugScopedTimer t(sync_s);
    syncAllCellsToDplGrid();
  }

  // Count remaining overflows (grid overuse) AND DRC violations.
  // Both must reach zero for the negotiation to converge.
  // Also detect any non-active cells that have become DRC-illegal
  // (e.g. a move created a one-site gap with a neighbor outside the
  // active set) and pull them in so the negotiation can fix them.
  int totalOverflow = 0;
  std::unordered_set<int> active_set(activeCells.begin(), activeCells.end());
  {
    utl::DebugScopedTimer t(overflow_s);
    for (int idx : activeCells) {
      if (cells_[idx].fixed) {
        continue;
      }
      const NegCell& cell = cells_[idx];
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
  }

  // Scan all movable cells for newly-created DRC violations outside the
  // current active set.  This handles cases where a move in the active
  // set created a one-site gap (or other DRC issue) with a bystander.
  {
    utl::DebugScopedTimer t(bystander_s);
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
  }

  if (totalOverflow > 0 && updateHistory) {
    utl::DebugScopedTimer t(history_s);
    updateHistoryCosts();
    updateDrcHistoryCosts(activeCells);
    sortByNegotiationOrder(activeCells);
  }

  if (logger_->debugCheck(utl::DPL, "negotiation_runtime", 1)) {
    const double total_ms = total_iter_timer.elapsed() * 1e3;
    auto pct = [total_ms](double ms_val) {
      return total_ms > 0 ? 100.0 * ms_val / total_ms : 0.0;
    };
    auto to_ms = [](double s) { return s * 1e3; };
    const double rip_up_ms = to_ms(rip_up_s);
    const double find_best_ms = to_ms(find_best_s);
    const double place_ms = to_ms(place_s);
    const double sort_ms = to_ms(sort_s);
    const double sync_ms = to_ms(sync_s);
    const double overflow_ms = to_ms(overflow_s);
    const double bystander_ms = to_ms(bystander_s);
    const double history_ms = to_ms(history_s);
    const double initSearchMs = prof_init_search_s_ * 1e3;
    const double currSearchMs = prof_curr_search_s_ * 1e3;
    const double filterMs = prof_filter_s_ * 1e3;
    const double negCostMs = prof_neg_cost_s_ * 1e3;
    const double drcMs = prof_drc_s_ * 1e3;
    const double overhead = find_best_ms - filterMs - negCostMs - drcMs;
    logger_->report(
        "  negotiationIter {} ({:.1f}ms, {} moves): "
        "sort {:.1f}ms ({:.0f}%), "
        "ripUp {:.1f}ms ({:.0f}%), findBest {:.1f}ms ({:.0f}%), place {:.1f}ms "
        "({:.0f}%), "
        "syncGrid {:.1f}ms ({:.0f}%), overflowCount {:.1f}ms ({:.0f}%), "
        "bystanderScan {:.1f}ms ({:.0f}%), historyUpdate {:.1f}ms ({:.0f}%)",
        iter,
        total_ms,
        moves_count,
        sort_ms,
        pct(sort_ms),
        rip_up_ms,
        pct(rip_up_ms),
        find_best_ms,
        pct(find_best_ms),
        place_ms,
        pct(place_ms),
        sync_ms,
        pct(sync_ms),
        overflow_ms,
        pct(overflow_ms),
        bystander_ms,
        pct(bystander_ms),
        history_ms,
        pct(history_ms));
    logger_->report(
        "    findBest by region ({} candidates, {} filtered): "
        "initSearch {:.1f}ms ({:.0f}%), currSearch {:.1f}ms ({:.0f}%)",
        prof_candidates_evaluated_,
        prof_candidates_filtered_,
        initSearchMs,
        pct(initSearchMs),
        currSearchMs,
        pct(currSearchMs));
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

void NegotiationLegalizer::ripUp(int cell_idx)
{
  eraseCellFromDplGrid(cell_idx);
  addUsage(cell_idx, -1);
}

void NegotiationLegalizer::place(int cell_idx, int x, int y)
{
  cells_[cell_idx].x = x;
  cells_[cell_idx].y = y;
  addUsage(cell_idx, 1);
  syncCellToDplGrid(cell_idx);
  if (opendp_->deep_iterative_debug_ && debug_observer_) {
    const odb::dbInst* debug_inst = debug_observer_->getDebugInstance();
    if (!debug_inst || cells_[cell_idx].db_inst == debug_inst) {
      pushNegotiationPixels();
      const NegCell& c = cells_[cell_idx];
      const int orig_x_dbu = die_xlo_ + c.init_x * site_width_;
      const int orig_y_dbu = die_ylo_ + c.init_y * row_height_;
      const int tgt_x_dbu = die_xlo_ + c.x * site_width_;
      const int tgt_y_dbu = die_ylo_ + c.y * row_height_;
      logger_->report(
          "Pause at placing of cell {}. orig=({},{}) target=({},{}) dbu. "
          "rowidx={}.",
          c.db_inst->getName(),
          orig_x_dbu,
          orig_y_dbu,
          tgt_x_dbu,
          tgt_y_dbu,
          c.y);
      debug_observer_->drawSelected(c.db_inst, !debug_inst);
    }
  }
}

// ===========================================================================
// findBestLocation – enumerate candidates within the search window
// ===========================================================================

std::pair<int, int> NegotiationLegalizer::findBestLocation(int cell_idx,
                                                           int iter) const
{
  const NegCell& cell = cells_[cell_idx];

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

  // Helper: evaluate one candidate position.
  auto tryLocation = [&](int tx, int ty) {
    {
      utl::DebugScopedTimer t(prof_filter_s_);
      if (!inDie(tx, ty, cell.width, cell.height) || !isValidRow(ty, cell, tx)
          || !respectsFence(cell_idx, tx, ty)) {
        ++prof_candidates_filtered_;
        return;
      }
    }

    double cost;
    {
      utl::DebugScopedTimer t(prof_neg_cost_s_);
      cost = negotiationCost(cell_idx, tx, ty);
    }

    // Add a DRC penalty so clean positions are strongly preferred,
    // but a DRC-violating position can still be chosen if nothing
    // better is available (avoids infinite non-convergence).
    if (node != nullptr) {
      utl::DebugScopedTimer t(prof_drc_s_);
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
    }
    ++prof_candidates_evaluated_;
    if (cost < best_cost) {
      best_cost = cost;
      best_x = tx;
      best_y = ty;
    }
  };

  // Search around the initial (GP) position.
  {
    utl::DebugScopedTimer t(prof_init_search_s_);
    for (int dy = -adj_window_; dy <= adj_window_; ++dy) {
      for (int dx = -horiz_window_; dx <= horiz_window_; ++dx) {
        tryLocation(cell.init_x + dx, cell.init_y + dy);
      }
    }
  }

  // Also search around the current position — critical when the cell has
  // already been displaced far from init_x and needs to explore its local
  // neighbourhood to resolve DRC violations (e.g. one-site gaps).
  if (cell.x != cell.init_x || cell.y != cell.init_y) {
    utl::DebugScopedTimer t(prof_curr_search_s_);
    for (int dy = -adj_window_; dy <= adj_window_; ++dy) {
      for (int dx = -horiz_window_; dx <= horiz_window_; ++dx) {
        tryLocation(cell.x + dx, cell.y + dy);
      }
    }
  }

  if (debug_observer_) {
    const odb::Rect core = opendp_->grid_->getCore();
    const DbuX sw = opendp_->grid_->getSiteWidth();
    const auto toX = [&](int gx) {
      return core.xMin() + gridToDbu(GridX{std::clamp(gx, 0, grid_w_)}, sw).v;
    };
    const auto toY = [&](int gy) {
      return core.yMin()
             + opendp_->grid_->gridYToDbu(GridY{std::clamp(gy, 0, grid_h_)}).v;
    };
    const odb::Rect init_win(toX(cell.init_x - horiz_window_),
                             toY(cell.init_y - adj_window_),
                             toX(cell.init_x + horiz_window_ + 1),
                             toY(cell.init_y + adj_window_ + 1));
    const bool displaced = (cell.x != cell.init_x || cell.y != cell.init_y);
    const odb::Rect curr_win = displaced
                                   ? odb::Rect(toX(cell.x - horiz_window_),
                                               toY(cell.y - adj_window_),
                                               toX(cell.x + horiz_window_ + 1),
                                               toY(cell.y + adj_window_ + 1))
                                   : odb::Rect();
    debug_observer_->setNegotiationSearchWindow(
        cell.db_inst, init_win, curr_win);
  }

  if (opendp_->deep_iterative_debug_ && debug_observer_) {
    const odb::dbInst* debug_inst = debug_observer_->getDebugInstance();
    if (cell.db_inst == debug_inst) {
      const DbuX site_width = opendp_->grid_->getSiteWidth();
      logger_->report("  Best location for {} is ({}, {}) with cost {}.",
                      cell.db_inst->getName(),
                      gridToDbu(GridX{best_x}, site_width).v,
                      opendp_->grid_->gridYToDbu(GridY{best_y}).v,
                      best_cost == static_cast<double>(kInfCost)
                          ? "inf"
                          : std::to_string(best_cost));
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

  if (best_cost == static_cast<double>(kInfCost)) {
    // Every candidate in the search window was filtered out (out-of-die,
    // invalid row, or fence violation).  The cell falls back to its current
    // position, which may already be illegal — a likely stuck-cell scenario.
    debugPrint(logger_,
               utl::DPL,
               "negotiation",
               1,
               "findBestLocation: no valid candidate found for cell '{}' "
               "(iter {}) — all {} candidates filtered, cell may be stuck.",
               cell.db_inst->getName(),
               iter,
               prof_candidates_filtered_);
  }

  return {best_x, best_y};
}

// ===========================================================================
// negotiationCost – Eq. 10 from the NBLG paper
//   Cost(x,y) = b(x,y) + Σ_grids h(g) * p(g)
// ===========================================================================

double NegotiationLegalizer::negotiationCost(int cell_idx, int x, int y) const
{
  const NegCell& cell = cells_[cell_idx];
  double cost = targetCost(cell_idx, x, y);

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

double NegotiationLegalizer::targetCost(int cell_idx, int x, int y) const
{
  const NegCell& cell = cells_[cell_idx];
  const int disp = std::abs(x - cell.init_x) + std::abs(y - cell.init_y);
  return static_cast<double>(disp)
         + max_disp_multiplier_
               * static_cast<double>(std::max(0, disp - max_disp_threshold_));
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
    const NegCell& cell = cells_[idx];
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
    const NegCell& cell = cells_[idx];
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
      NegCell& cell = cells_[idx];
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
        NegCell& ca = cells_[a];
        NegCell& cb = cells_[b];

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
    const NegCell& cell = cells_[idx];
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
