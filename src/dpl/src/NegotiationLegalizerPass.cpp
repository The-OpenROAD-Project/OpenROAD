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
// printStuckSummary – shared formatter for per-iter and run-wide tallies
// ===========================================================================

void NegotiationLegalizer::printStuckSummary(
    const char* label,
    int no_cand_count,
    int same_pos_count,
    const std::unordered_map<int, int>& no_cand_by_height,
    const std::unordered_map<int, int>& same_pos_by_height) const
{
  if (no_cand_count == 0 && same_pos_count == 0) {
    return;
  }
  if (!logger_->debugCheck(utl::DPL, "negotiation", 1)) {
    return;
  }
  debugPrint(logger_,
             utl::DPL,
             "negotiation",
             1,
             "{} | no-valid-site {} | kept-at-current-position {}.",
             label,
             no_cand_count,
             same_pos_count);
  auto print_by_height
      = [&](const char* sub_label, const std::unordered_map<int, int>& m) {
          if (m.empty()) {
            return;
          }
          std::vector<std::pair<int, int>> entries(m.begin(), m.end());
          std::ranges::sort(entries, [](const auto& a, const auto& b) {
            return a.first < b.first;
          });
          for (const auto& [h, n] : entries) {
            debugPrint(logger_,
                       utl::DPL,
                       "negotiation",
                       1,
                       "  {} | height={} | {} occurrences.",
                       sub_label,
                       h,
                       n);
          }
        };
  print_by_height("no-valid-site:", no_cand_by_height);
  print_by_height("kept-at-current-position", same_pos_by_height);
}

// ===========================================================================
// runNegotiation – top-level negotiation driver
// ===========================================================================

void NegotiationLegalizer::runNegotiation(const std::vector<int>& illegalCells)
{
  // Reset stuck-cell tallies for this negotiation run.
  stuck_no_candidate_count_ = 0;
  stuck_same_pos_count_ = 0;
  stuck_no_candidate_by_height_.clear();
  stuck_same_pos_by_height_.clear();

  // Seed with illegal cells and all movable neighbors within the search
  // window so the loop can create space organically.
  std::unordered_set<int> active_set(illegalCells.begin(), illegalCells.end());

  // Build a spatial index once: bucket movable cells by their bottom row (y),
  // each bucket sorted by x.  Each seed then only scans the few rows in its
  // y-window and binary-searches the x-range, so the total cost is proportional
  // to the number of cells actually inside the search windows.
  std::vector<std::vector<std::pair<int, int>>> row_buckets(grid_h_);
  for (int i = 0; std::cmp_less(i, cells_.size()); ++i) {
    const NegCell& nb = cells_[i];
    if (nb.fixed || nb.y < 0 || nb.y >= grid_h_ || active_set.contains(i)) {
      continue;
    }
    row_buckets[nb.y].emplace_back(nb.x, i);
  }
  for (auto& bucket : row_buckets) {
    std::ranges::sort(bucket);
  }

  for (int idx : illegalCells) {
    const NegCell& seed = cells_[idx];
    const int site_window = effectiveSiteWindow(seed);
    const int row_cap = effectiveRowCap(seed);
    const int xlo = seed.x - site_window;
    const int xhi = seed.x + seed.width + site_window;
    const int ylo = std::max(0, seed.y - row_cap);
    const int yhi = std::min(grid_h_ - 1, seed.y + seed.height + row_cap);

    for (int yy = ylo; yy <= yhi; ++yy) {
      const auto& bucket = row_buckets[yy];
      auto it = std::ranges::lower_bound(  // NOLINT(misc-include-cleaner)
          bucket,
          xlo,
          {},
          [](const auto& p) { return p.first; });
      for (; it != bucket.end() && it->first <= xhi; ++it) {
        active_set.insert(it->second);
      }
    }
  }

  std::vector<int> active(active_set.begin(), active_set.end());

  debugPause("Pause before negotiation phase 1.");

  // Phase 1 – all active cells rip-up every iteration (isolation point = 0).
  debugPrint(logger_,
             utl::DPL,
             "negotiation",
             1,
             "Negotiation phase 1: {} active cells, {} iterations.",
             active.size(),
             max_iter_neg_);

  logger_->report("          |      Total |  Illegal |  Illegal");
  logger_->report("Iteration | Violations |    Cells |    Sites");
  logger_->report("---------------------------------------------");

  auto print_last_if_needed = [&]() {
    if (last_iter_ >= 0 && last_iter_ != last_printed_iter_) {
      logger_->report("{:>9} | {:>10} | {:>9} | {:>9}",
                      last_iter_,
                      last_violations_,
                      last_illegal_cells_,
                      last_illegal_sites_);
      last_printed_iter_ = last_iter_;
    }
  };

  int prev_violations = -1;
  int stall_count = 0;
  for (int iter = 0; iter < max_iter_neg_; ++iter) {
    const bool print_row = iter < 10 || iter % 10 == 0;
    const int phase_1_violations
        = negotiationIter(active, iter, /*updateHistory=*/true, print_row);
    if (print_row) {
      last_printed_iter_ = iter;
    }
    if (phase_1_violations == 0) {
      print_last_if_needed();
      logger_->report("Negotiation phase 1 converged at iteration {}.", iter);
      logger_->metric("negotiation__converge__phase_1__iteration", iter);
      printStuckSummary("Total stuck cells summary",
                        stuck_no_candidate_count_,
                        stuck_same_pos_count_,
                        stuck_no_candidate_by_height_,
                        stuck_same_pos_by_height_);
      debugPause("Pause after convergence at phase 1.");
      return;
    }
    if (phase_1_violations == prev_violations) {
      ++stall_count;
      if (stall_count == 3) {
        print_last_if_needed();
        std::vector<int> illegal_cells;
        for (int idx : active) {
          if (!cells_[idx].fixed && !isCellLegal(idx)) {
            illegal_cells.push_back(idx);
          }
        }
        logger_->warn(
            utl::DPL,
            700,
            "Negotiation phase 1: violations stuck at {} for 3 consecutive "
            "iterations.\nUsing old diamond search for {} remaining illegal "
            "cells.",
            phase_1_violations,
            illegal_cells.size());
        diamondRecovery(illegal_cells);
        break;
      }
    } else {
      stall_count = 0;
    }
    prev_violations = phase_1_violations;
  }
  print_last_if_needed();

  debugPause("Pause before negotiation phase 2.");

  if (debug_observer_) {
    debug_observer_->addNegotiationPhase2Marker(max_iter_neg_);
  }

  // Phase 2 – isolation point active: skip already-legal cells.
  logger_->report("Negotiation phase 2: isolation point active, {} iterations.",
                  kMaxIterNeg2);

  prev_violations = -1;
  stall_count = 0;
  for (int iter = 0; iter < kMaxIterNeg2; ++iter) {
    const int actual_iter = iter + max_iter_neg_;
    const bool print_row = actual_iter < 10 || actual_iter % 10 == 0;
    const int phase_2_violations = negotiationIter(
        active, actual_iter, /*updateHistory=*/true, print_row);
    if (print_row) {
      last_printed_iter_ = actual_iter;
    }
    if (phase_2_violations == 0) {
      print_last_if_needed();
      logger_->report("Negotiation phase 2 converged at iteration {}.", iter);
      logger_->metric("negotiation__converge__phase_2__iteration", iter);
      printStuckSummary("negotiation totals",
                        stuck_no_candidate_count_,
                        stuck_same_pos_count_,
                        stuck_no_candidate_by_height_,
                        stuck_same_pos_by_height_);
      debugPause("Pause after convergence at phase 2.");
      return;
    }
    if (phase_2_violations == prev_violations) {
      ++stall_count;
      if (stall_count == 3) {
        print_last_if_needed();
        std::vector<int> illegal_cells;
        for (int idx : active) {
          if (!cells_[idx].fixed && !isCellLegal(idx)) {
            illegal_cells.push_back(idx);
          }
        }
        logger_->warn(utl::DPL,
                      702,
                      "Negotiation phase 2: violations stuck at {} for 3 "
                      "consecutive iterations. Using diamond search for {} "
                      "remaining illegal cells.",
                      phase_2_violations,
                      illegal_cells.size());
        diamondRecovery(illegal_cells);
        break;
      }
    } else {
      stall_count = 0;
    }
    prev_violations = phase_2_violations;
  }
  print_last_if_needed();

  // Non-convergence is reported by the caller (Opendp::detailedPlacement)
  // via numViolations(), which avoids registering a message ID in this file.
  debugPrint(logger_,
             utl::DPL,
             "negotiation",
             1,
             "Negotiation did not fully converge. Remaining violations: {}.",
             numViolations());
  printStuckSummary("negotiation totals",
                    stuck_no_candidate_count_,
                    stuck_same_pos_count_,
                    stuck_no_candidate_by_height_,
                    stuck_same_pos_by_height_);
  debugPause("Pause after non-convergence at negotiation phases 1 and 2.");
}

// ===========================================================================
// negotiationIter – one full rip-up/replace sweep over activeCells
// ===========================================================================

int NegotiationLegalizer::negotiationIter(std::vector<int>& activeCells,
                                          int iter,
                                          bool updateHistory,
                                          bool print_row)
{
  if (debug_observer_) {
    debug_observer_->clearNegotiationSearchWindows();
  }

  current_iter_ = iter;
  current_iter_movers_.clear();

  // Reset findBestLocation profiling accumulators.
  prof_init_search_s_ = 0;
  prof_curr_search_s_ = 0;
  prof_filter_s_ = 0;
  prof_neg_cost_s_ = 0;
  prof_drc_s_ = 0;
  prof_candidates_evaluated_ = 0;
  prof_candidates_filtered_ = 0;

  // Reset per-iteration stuck-cell tallies.
  stuck_no_candidate_count_iter_ = 0;
  stuck_same_pos_count_iter_ = 0;
  stuck_no_candidate_by_height_iter_.clear();
  stuck_same_pos_by_height_iter_.clear();

  double sort_s{0}, rip_up_s{0}, find_best_s{0}, place_s{0};
  double sync_s{0}, violations_s{0}, bystander_s{0}, history_s{0};
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

  // Count remaining violations (grid overuse) AND DRC violations.
  // Both must reach zero for the negotiation to converge.
  // Also detect any non-active cells that have become DRC-illegal
  // (e.g. a move created a one-site gap with a neighbor outside the
  // active set) and pull them in so the negotiation can fix them.
  int totalViolations = 0;
  int illegalCellCount = 0;
  int illegalSiteCount = 0;
  std::unordered_set<int> active_set(activeCells.begin(), activeCells.end());
  {
    utl::DebugScopedTimer t(violations_s);
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
            const int overuse = gridAt(gx, cell.y + dy).overuse();
            totalViolations += overuse;
            if (overuse > 0) {
              ++illegalSiteCount;
            }
          }
        }
      }
      if (!isCellLegal(idx)) {
        ++totalViolations;
        ++illegalCellCount;
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
        ++totalViolations;
        ++illegalCellCount;
      }
    }
  }

  if (totalViolations > 0 && updateHistory) {
    utl::DebugScopedTimer t(history_s);
    updateHistoryCosts(activeCells);
    updateDrcHistoryCosts(activeCells);
    sortByNegotiationOrder(activeCells);
  }

  // Emit runtime profiling only on iterations that also print a table row
  // (first 10, then every 10th), to keep the two outputs aligned.
  if (print_row && logger_->debugCheck(utl::DPL, "negotiation_runtime", 1)) {
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
    const double violations_ms = to_ms(violations_s);
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
        "syncGrid {:.1f}ms ({:.0f}%), violationsCount {:.1f}ms ({:.0f}%), "
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
        violations_ms,
        pct(violations_ms),
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

  last_iter_ = iter;
  last_violations_ = totalViolations;
  last_illegal_cells_ = illegalCellCount;
  last_illegal_sites_ = illegalSiteCount;

  const std::string iter_label
      = "-> Stuck cells summary | iter " + std::to_string(iter);
  printStuckSummary(iter_label.c_str(),
                    stuck_no_candidate_count_iter_,
                    stuck_same_pos_count_iter_,
                    stuck_no_candidate_by_height_iter_,
                    stuck_same_pos_by_height_iter_);

  if (print_row) {
    logger_->report("{:>9} | {:>10} | {:>9} | {:>9}",
                    iter,
                    totalViolations,
                    illegalCellCount,
                    illegalSiteCount);
  }
  if (debug_observer_) {
    debug_observer_->addNegotiationViolationsPoint(
        iter, totalViolations, illegalCellCount, illegalSiteCount);
  }
  if (opendp_->iterative_debug_ && debug_observer_
      && iter >= opendp_->negotiation_debug_start_
      && ((iter - opendp_->negotiation_debug_start_)
              % opendp_->negotiation_debug_interval_
          == 0)) {
    commitNegotiationPosToDpl();
    pushNegotiationPixels();
    logger_->report("Pause after negotiation iteration {}.", iter);
    debug_observer_->redrawAndPause();
  }
  return totalViolations;
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
  const bool did_move = (x != cells_[cell_idx].x || y != cells_[cell_idx].y);
  cells_[cell_idx].x = x;
  cells_[cell_idx].y = y;
  addUsage(cell_idx, 1);
  syncCellToDplGrid(cell_idx);
  if (did_move && debug_observer_
      && (opendp_->iterative_debug_ || opendp_->deep_iterative_debug_)) {
    current_iter_movers_.insert(cells_[cell_idx].db_inst);
    debug_observer_->setCurrentIterMovers(current_iter_movers_);
  }
  if (opendp_->deep_iterative_debug_ && debug_observer_
      && current_iter_ >= opendp_->negotiation_debug_start_) {
    const odb::dbInst* debug_inst = debug_observer_->getDebugInstance();
    if (!debug_inst || cells_[cell_idx].db_inst == debug_inst) {
      pushNegotiationPixels();
      const NegCell& c = cells_[cell_idx];
      const int orig_x_dbu = die_xlo_ + c.init_x * site_width_;
      const int orig_y_dbu
          = die_ylo_ + opendp_->grid_->gridYToDbu(GridY{c.init_y}).v;
      const int tgt_x_dbu = die_xlo_ + c.x * site_width_;
      const int tgt_y_dbu = die_ylo_ + opendp_->grid_->gridYToDbu(GridY{c.y}).v;
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
// Search-window helpers – single source of truth for how the window scales
// with cell size and is capped by the user's max-displacement limits.
// ===========================================================================

int NegotiationLegalizer::effectiveSiteWindow(const NegCell& cell) const
{
  if (disable_window_extension_ || site_search_window_ == 0) {
    return std::min(site_search_window_, opendp_->max_displacement_x_);
  }
  return std::min(std::max(site_search_window_, cell.width),
                  opendp_->max_displacement_x_);
}

int NegotiationLegalizer::effectiveRowCap(const NegCell& cell) const
{
  if (disable_window_extension_) {
    return std::min(row_search_window_, opendp_->max_displacement_y_);
  }
  return std::min(cell.height * row_search_window_,
                  opendp_->max_displacement_y_);
}

std::pair<int, int> NegotiationLegalizer::horizontalWindowBounds(
    const NegCell& cell,
    int base_x,
    int target_y,
    int site_window) const
{
  // A "wall" is a macro/fixed-cell blockage (capacity == 0) or the core
  // boundary (off-die). Once the cell footprint anchored at target_x hits a
  // wall, nothing further in that direction is reachable.
  auto hitsWall = [&](int target_x) {
    if (!inDie(target_x, target_y, cell.width, cell.height)) {
      return true;
    }
    for (int dy = 0; dy < cell.height; ++dy) {
      for (int gx = target_x; gx < target_x + cell.width; ++gx) {
        if (gridAt(gx, target_y + dy).capacity == 0) {
          return true;
        }
      }
    }
    return false;
  };

  int left_avail = 0;
  for (int d = 1; d <= site_window; ++d) {
    if (hitsWall(base_x - d)) {
      break;
    }
    ++left_avail;
  }
  int right_avail = 0;
  for (int d = 1; d <= site_window; ++d) {
    if (hitsWall(base_x + d)) {
      break;
    }
    ++right_avail;
  }

  // Reach lost on one side (cut short by a wall) is added to the other side,
  // unless the user disabled this extension.
  const int left_deficit
      = disable_window_extension_ ? 0 : site_window - left_avail;
  const int right_deficit
      = disable_window_extension_ ? 0 : site_window - right_avail;
  int dx_lo = -(site_window + right_deficit);
  int dx_hi = site_window + left_deficit;

  // Hard cap limit
  dx_lo = std::max(dx_lo, -opendp_->max_displacement_x_);
  dx_hi = std::min(dx_hi, opendp_->max_displacement_x_);
  return {dx_lo, dx_hi};
}

NegotiationLegalizer::SearchWindow NegotiationLegalizer::buildSearchWindow(
    const NegCell& cell,
    int anchor_x,
    int anchor_y) const
{
  SearchWindow window;
  // Vertical reach: nearest valid rows around the anchor, extended past an
  // off-core wall onto the open side.
  window.rows = verticalWindowRows(
      cell, anchor_y, anchor_x, row_search_window_, effectiveRowCap(cell));
  // Horizontal reach: computed once at the anchor row, shifted away from any
  // macro/off-core wall onto the open side.
  std::tie(window.dx_lo, window.dx_hi) = horizontalWindowBounds(
      cell, anchor_x, anchor_y, effectiveSiteWindow(cell));
  return window;
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
  const double drc_penalty = drc_penalty_ * (1.0 + iter);

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
      cost += drc_penalty * drcCount;
    }
    ++prof_candidates_evaluated_;
    if (cost < best_cost) {
      best_cost = cost;
      best_x = tx;
      best_y = ty;
    }
  };

  // Search around the initial (GP) position. The window's rows and asymmetric
  // dx reach are already shifted away from macros/off-core walls (built once,
  // see buildSearchWindow).
  const SearchWindow init_window
      = buildSearchWindow(cell, cell.init_x, cell.init_y);
  {
    utl::DebugScopedTimer t(prof_init_search_s_);
    for (int ty : init_window.rows) {
      for (int dx = init_window.dx_lo; dx <= init_window.dx_hi; ++dx) {
        tryLocation(cell.init_x + dx, ty);
      }
    }
  }

  // TODO: check if this second call is actually impactful, maybe this impacts
  // runtime without actual better convergence, mostly considering first
  // iteration, maybe always skip this at first iteration.
  //
  // Also search around the current position — critical when the cell has
  // already been displaced far from init_x and needs to explore its local
  // neighbourhood to resolve DRC violations (e.g. one-site gaps).
  const bool displaced = (cell.x != cell.init_x || cell.y != cell.init_y);
  SearchWindow curr_window;
  if (displaced) {
    curr_window = buildSearchWindow(cell, cell.x, cell.y);
    utl::DebugScopedTimer t(prof_curr_search_s_);
    debugPrint(logger_,
               utl::DPL,
               "negotiation",
               2,
               "Searching at current position for {} (searching from inital "
               "position found no better solution).",
               cell.db_inst->getName());
    for (int ty : curr_window.rows) {
      for (int dx = curr_window.dx_lo; dx <= curr_window.dx_hi; ++dx) {
        tryLocation(cell.x + dx, ty);
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
    // Y extent reflects the actual sparse set of valid rows visited
    // (bounding rect: lowest to highest+1). Falls back to the seed row
    // when no valid rows were found within row_search_cap.
    auto y_range = [&](const std::vector<int>& rows, int fallback_y) {
      if (rows.empty()) {
        return std::pair{fallback_y, fallback_y + 1};
      }
      const auto [lo, hi] = std::ranges::minmax_element(rows);
      return std::pair{*lo, *hi + 1};
    };
    const auto [init_ylo, init_yhi] = y_range(init_window.rows, cell.init_y);
    const odb::Rect init_win(toX(cell.init_x + init_window.dx_lo),
                             toY(init_ylo),
                             toX(cell.init_x + init_window.dx_hi + 1),
                             toY(init_yhi));
    odb::Rect curr_win;
    if (displaced) {
      const auto [curr_ylo, curr_yhi] = y_range(curr_window.rows, cell.y);
      curr_win = odb::Rect(toX(cell.x + curr_window.dx_lo),
                           toY(curr_ylo),
                           toX(cell.x + curr_window.dx_hi + 1),
                           toY(curr_yhi));
    }
    debug_observer_->setNegotiationSearchWindow(
        cell.db_inst, init_win, curr_win);

    if (opendp_->deep_iterative_debug_
        && iter >= opendp_->negotiation_debug_start_
        && cell.db_inst == debug_observer_->getDebugInstance()) {
      const DbuX site_width = opendp_->grid_->getSiteWidth();
      odb::dbBlock* block = cell.db_inst->getBlock();
      const double inst_area_um2
          = block->dbuAreaToMicrons(cell.db_inst->getBBox()->getBox().area());
      logger_->report(
          "  Search window for {}: ll ({}, {}) ur ({}, {}) dbu, {:.3f} x "
          "{:.3f} um, area {:.3f} um^2 (instance area {:.3f} um^2).",
          cell.db_inst->getName(),
          init_win.xMin(),
          init_win.yMin(),
          init_win.xMax(),
          init_win.yMax(),
          block->dbuToMicrons(init_win.dx()),
          block->dbuToMicrons(init_win.dy()),
          block->dbuAreaToMicrons(init_win.area()),
          inst_area_um2);
      if (displaced) {
        logger_->report(
            "  current-position window ll ({}, {}) ur ({}, {}) dbu, {:.3f} "
            "x {:.3f} um, area {:.3f} um^2.",
            curr_win.xMin(),
            curr_win.yMin(),
            curr_win.xMax(),
            curr_win.yMax(),
            block->dbuToMicrons(curr_win.dx()),
            block->dbuToMicrons(curr_win.dy()),
            block->dbuAreaToMicrons(curr_win.area()));
      }
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

  if (logger_->debugCheck(utl::DPL, "negotiation", 2)) {
    if (best_cost == static_cast<double>(kInfCost)) {
      // Every candidate in the search window was filtered out (out-of-die,
      // invalid row, or fence violation).  The cell falls back to its current
      // position, which may already be illegal — a likely stuck-cell scenario.
      ++stuck_no_candidate_count_;
      ++stuck_no_candidate_by_height_[cell.height];
      ++stuck_no_candidate_count_iter_;
      ++stuck_no_candidate_by_height_iter_[cell.height];
      debugPrint(logger_,
                 utl::DPL,
                 "negotiation",
                 2,
                 "findBestLocation: no valid candidate found for cell '{}' "
                 "(iter {}, size {} rows x {} sites) — all {} candidate "
                 "filtered, cell may be stuck.",
                 cell.db_inst->getName(),
                 iter,
                 cell.height,
                 cell.width,
                 prof_candidates_filtered_);
    }

    else if (best_x == cell.x && best_y == cell.y) {
      // Valid sites are available, although the best choice is the current
      // position.
      ++stuck_same_pos_count_;
      ++stuck_same_pos_by_height_[cell.height];
      ++stuck_same_pos_count_iter_;
      ++stuck_same_pos_by_height_iter_[cell.height];
      debugPrint(logger_,
                 utl::DPL,
                 "negotiation",
                 1,
                 "Negotiation: best location for cell '{}' at iteration {} "
                 "(size {} rows x {} sites) is its current position.",
                 cell.db_inst->getName(),
                 iter,
                 cell.height,
                 cell.width);
    }
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

void NegotiationLegalizer::updateHistoryCosts(
    const std::vector<int>& activeCells)
{
  // Walk active-cell footprints instead of the full grid: every overused
  // pixel whose hist_cost is read has >= 2 overlapping cells, and at least
  // one of them is illegal (hence active). Dedupe shared pixels so each is
  // bumped once.
  hist_seen_pixels_.clear();
  for (int idx : activeCells) {
    const NegCell& cell = cells_[idx];
    if (cell.fixed) {
      continue;
    }
    const int xBegin = effXBegin(cell);
    const int xEnd = effXEnd(cell);
    for (int dy = 0; dy < cell.height; ++dy) {
      const int gy = cell.y + dy;
      for (int gx = xBegin; gx < xEnd; ++gx) {
        if (!gridExists(gx, gy)) {
          continue;
        }
        if (!hist_seen_pixels_.insert(gy * grid_w_ + gx).second) {
          continue;
        }
        Pixel& g = gridAt(gx, gy);
        const int ov = g.overuse();
        if (ov > 0) {
          g.hist_cost += kHfDefault * ov;
        }
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

  // Decorate-sort: compute each cell's overuse (a footprint scan) once into a
  // key, rather than recomputing it the O(log n) times per element a
  // comparison-time call would.  The comparator below yields identical results
  // to scoring (a, b) directly, so the resulting order is unchanged.
  struct SortKey
  {
    int overuse;
    int height;
    int width;
    int idx;
  };
  std::vector<SortKey> keys;
  keys.reserve(indices.size());
  for (int idx : indices) {
    keys.push_back(
        {cellOveruse(idx), cells_[idx].height, cells_[idx].width, idx});
  }

  std::ranges::sort(keys, [](const SortKey& a, const SortKey& b) {
    if (a.overuse != b.overuse) {
      return a.overuse > b.overuse;
    }
    if (a.height != b.height) {
      return a.height < b.height;
    }
    if (a.width != b.width) {
      return a.width < b.width;
    }
    return a.idx < b.idx;
  });

  for (size_t i = 0; i < keys.size(); ++i) {
    indices[i] = keys[i].idx;
  }
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

      auto tryLoc = [&](int target_x, int target_y) {
        if (!inDie(target_x, target_y, cell.width, cell.height)) {
          return;
        }
        if (!isValidRow(target_y, cell, target_x)) {
          return;
        }
        if (!respectsFence(idx, target_x, target_y)) {
          return;
        }
        // Only accept if no new overlap or padding violation is introduced.
        const int target_x_begin = std::max(0, target_x - cell.pad_left);
        const int target_x_end
            = std::min(grid_w_, target_x + cell.width + cell.pad_right);
        for (int dy = 0; dy < cell.height; ++dy) {
          for (int gx = target_x_begin; gx < target_x_end; ++gx) {
            if (gridAt(gx, target_y + dy).overuse() > 0) {
              return;
            }
          }
        }
        const int d = std::abs(target_x - cell.init_x)
                      + std::abs(target_y - cell.init_y);
        if (d < best_dist) {
          best_dist = d;
          best_x = target_x;
          best_y = target_y;
        }
      };

      const int site_window = effectiveSiteWindow(cell);
      const std::vector<int> rows = verticalWindowRows(
          cell, cell.y, cell.init_x, row_search_window_, effectiveRowCap(cell));
      for (int target_y : rows) {
        for (int dx = -site_window; dx <= site_window; ++dx) {
          tryLoc(cell.init_x + dx, target_y);
        }
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

  // Group movable cells by (height, width).  Power-rail compatibility of any
  // candidate swap is enforced below by isValidRow(), so it need not be part
  // of the grouping key.
  struct GroupKey
  {
    int height;
    int width;
    bool operator==(const GroupKey& o) const
    {
      return height == o.height && width == o.width;
    }
  };
  struct GroupKeyHash
  {
    size_t operator()(const GroupKey& k) const
    {
      return std::hash<int>()(k.height) ^ (std::hash<int>()(k.width) << 8);
    }
  };

  std::unordered_map<GroupKey, std::vector<int>, GroupKeyHash> groups;
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (!cells_[i].fixed) {
      groups[{cells_[i].height, cells_[i].width}].push_back(i);
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
      debugPrint(logger_,
                 utl::DPL,
                 "negotiation",
                 2,
                 "diamondRecovery: cell {} recovered at ({}, {}).",
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
