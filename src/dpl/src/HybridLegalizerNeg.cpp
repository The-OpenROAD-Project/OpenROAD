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

// HybridLegalizerNeg.cpp – negotiation pass and post-optimisation.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "HybridLegalizer.h"
#include "PlacementDRC.h"
#include "dpl/Opendp.h"
#include "graphics/DplObserver.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/network.h"
#include "utl/Logger.h"

namespace dpl {

// ===========================================================================
// runNegotiation – top-level negotiation driver
// ===========================================================================

void HybridLegalizer::runNegotiation(const std::vector<int>& illegalCells)
{
  // Seed with illegal cells and all movable neighbors within the search
  // window so the loop can create space organically.
  std::unordered_set<int> activeSet(illegalCells.begin(), illegalCells.end());

  for (int idx : illegalCells) {
    const HLCell& seed = cells_[idx];
    const int xlo = seed.x - horizWindow_;
    const int xhi = seed.x + seed.width + horizWindow_;
    const int ylo = seed.y - adjWindow_;
    const int yhi = seed.y + seed.height + adjWindow_;

    for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
      if (cells_[i].fixed) {
        continue;
      }
      const HLCell& nb = cells_[i];
      if (nb.x >= xlo && nb.x <= xhi && nb.y >= ylo && nb.y <= yhi) {
        activeSet.insert(i);
      }
    }
  }

  std::vector<int> active(activeSet.begin(), activeSet.end());

  // Phase 1 – all active cells rip-up every iteration (isolation point = 0).
  debugPrint(logger_,
             utl::DPL,
             "hybrid",
             1,
             "Negotiation phase 1: {} active cells, {} iterations.",
             active.size(),
             maxIterNeg_);

  for (int iter = 0; iter < maxIterNeg_; ++iter) {
    const int overflows = negotiationIter(active, iter, /*updateHistory=*/true);
    if (overflows == 0) {
      debugPrint(logger_,
                 utl::DPL,
                 "hybrid",
                 1,
                 "Negotiation phase 1 converged at iteration {}.",
                 iter);
      if(debug_observer_) {
        setDplPositions();
        pushHybridPixels();
        logger_->report("Pause after convergence at phase 1.");
        debug_observer_->redrawAndPause();
      }
      return;
    }
  }

  if(debug_observer_) {
    setDplPositions();
    pushHybridPixels();
    logger_->report("Pause before negotiation phase 2.");
    debug_observer_->redrawAndPause();
  }

  // Phase 2 – isolation point active: skip already-legal cells.
  debugPrint(logger_,
             utl::DPL,
             "hybrid",
             1,
             "Negotiation phase 2: isolation point active, {} iterations.",
             kMaxIterNeg2);

  for (int iter = 0; iter < kMaxIterNeg2; ++iter) {
    const int overflows
        = negotiationIter(active, iter + maxIterNeg_, /*updateHistory=*/true);
    if (overflows == 0) {
      debugPrint(logger_,
                 utl::DPL,
                 "hybrid",
                 1,
                 "Negotiation phase 2 converged at iteration {}.",
                 iter);
      if(debug_observer_) {
        setDplPositions();
        pushHybridPixels();
        logger_->report("Pause after convergence at phase 2.");
        debug_observer_->redrawAndPause();
      }
      return;
    }
  }

  // Non-convergence is reported by the caller (Opendp::detailedPlacement)
  // via numViolations(), which avoids registering a message ID in this file.
  debugPrint(logger_,
             utl::DPL,
             "hybrid",
             1,
             "Negotiation did not fully converge. Remaining violations: {}.",
             numViolations());
  if(debug_observer_) {
    setDplPositions();
    pushHybridPixels();
    logger_->report("Pause after non-convergence at negotiation phases 1 and 2.");
    debug_observer_->redrawAndPause();
  }
}

// ===========================================================================
// negotiationIter – one full rip-up/replace sweep over activeCells
// ===========================================================================

int HybridLegalizer::negotiationIter(std::vector<int>& activeCells,
                                     int iter,
                                     bool updateHistory)
{
  int moves_count = 0;
  sortByNegotiationOrder(activeCells);

  // for(auto cell : activeCells){
  //   logger_->report("Negotiation iter {}, cell {}, legal {}",
  //                   iter,
  //                   cell,
  //                   isCellLegal(cell));
  // }
  for (int idx : activeCells) {
    if (cells_[idx].fixed) {
      continue;
    }
    // Isolation point: skip legal cells during phase 2.
    if (iter >= kIsolationPt && isCellLegal(idx)) {
      continue;
    }
    ripUp(idx);
    const auto [bx, by] = findBestLocation(idx, iter);
    place(idx, bx, by);
    moves_count++;
    logger_->report("Negotiation iter {}, cell {}, moves {}, best location {}, {}",
                iter, cells_[idx].db_inst_->getName(), moves_count, bx, by);
  }

  // Count remaining overflows (grid overuse) AND DRC violations.
  // Both must reach zero for the negotiation to converge.
  // Also detect any non-active cells that have become DRC-illegal
  // (e.g. a move created a one-site gap with a neighbor outside the
  // active set) and pull them in so the negotiation can fix them.
  int totalOverflow = 0;
  std::unordered_set<int> activeSet(activeCells.begin(), activeCells.end());
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
  // Scan all movable cells for newly-created DRC violations outside the
  // current active set.  This handles cases where a move in the active
  // set created a one-site gap (or other DRC issue) with a bystander.
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    //TODO why skip fixed here?
    if (cells_[i].fixed || activeSet.contains(i)) {
      continue;
    }
    if (!isCellLegal(i)) {
      activeCells.push_back(i);
      activeSet.insert(i);
      ++totalOverflow;
    }
  }

  if (totalOverflow > 0 && updateHistory) {
    updateHistoryCosts();
    updateDrcHistoryCosts(activeCells);
    sortByNegotiationOrder(activeCells);
  }

  return totalOverflow;
}

// ===========================================================================
// ripUp / place
// ===========================================================================

void HybridLegalizer::ripUp(int cellIdx)
{
  eraseCellFromDplGrid(cellIdx);
  addUsage(cellIdx, -1);
}

void HybridLegalizer::place(int cellIdx, int x, int y)
{
  cells_[cellIdx].x = x;
  cells_[cellIdx].y = y;
  addUsage(cellIdx, 1);
  syncCellToDplGrid(cellIdx);  
  if (opendp_->iterative_debug_ && cells_[cellIdx].db_inst_ != nullptr) {
    std::string name = cells_[cellIdx].db_inst_->getName();
    if (name == "dpath.b_reg.out\\[13\\]$_DFFE_PP_") {
      pushHybridPixels();
      debug_observer_->redrawAndPause();
    }
  }
}

// ===========================================================================
// findBestLocation – enumerate candidates within the search window
// ===========================================================================

std::pair<int, int> HybridLegalizer::findBestLocation(int cellIdx,
                                                      int iter) const
{
  const HLCell& cell = cells_[cellIdx];

  auto bestCost = static_cast<double>(kInfCost);
  int bestX = cell.x;
  int bestY = cell.y;

  // Look up the DPL Node once for DRC checks (may be null if no
  // Opendp integration is available).
  Node* node = (opendp_ && opendp_->drc_engine_ && network_)
                   ? network_->getNode(cell.db_inst_)
                   : nullptr;

  // DRC penalty escalates with iteration count: early iterations are
  // lenient (cells can tolerate DRC violations to resolve overlaps first),
  // later iterations strongly penalise DRC violations to force resolution.
  const double kDrcPenalty = 1e3 * (1.0 + iter);

  // Helper: evaluate one candidate position.
  auto tryLocation = [&](int tx, int ty) {
    if (!inDie(tx, ty, cell.width, cell.height)) {
      return;
    }
    if (!isValidRow(ty, cell, tx)) {
      return;
    }
    if (!respectsFence(cellIdx, tx, ty)) {
      return;
    }
    double cost = negotiationCost(cellIdx, tx, ty);
    // Add a DRC penalty so clean positions are strongly preferred,
    // but a DRC-violating position can still be chosen if nothing
    // better is available (avoids infinite non-convergence).
    if (node != nullptr) {
      odb::dbOrientType targetOrient = node->getOrient();
      odb::dbSite* site = cell.db_inst_->getMaster()->getSite();
      if (site != nullptr) {
        auto orient = opendp_->grid_->getSiteOrientation(GridX{tx}, GridY{ty}, site);
        if (orient.has_value()) {
          targetOrient = orient.value();
        }
      }
      const int drcCount = opendp_->drc_engine_->countDRCViolations(
          node, GridX{tx}, GridY{ty}, targetOrient);
      cost += kDrcPenalty * drcCount;
    }
    if (cost < bestCost) {
      bestCost = cost;
      bestX = tx;
      bestY = ty;
    }
  };

  // Search around the initial (GP) position.
  for (int dy = -adjWindow_; dy <= adjWindow_; ++dy) {
    for (int dx = -horizWindow_; dx <= horizWindow_; ++dx) {
      tryLocation(cell.initX + dx, cell.initY + dy);
    }
  }

  // Also search around the current position — critical when the cell has
  // already been displaced far from initX and needs to explore its local
  // neighbourhood to resolve DRC violations (e.g. one-site gaps).
  if (cell.x != cell.initX || cell.y != cell.initY) {
    for (int dy = -adjWindow_; dy <= adjWindow_; ++dy) {
      for (int dx = -horizWindow_; dx <= horizWindow_; ++dx) {
        tryLocation(cell.x + dx, cell.y + dy);
      }
    }
  }

  // Also try snapping to the original position.
  const auto [sx, sy] = snapToLegal(cellIdx, cell.initX, cell.initY);
  tryLocation(sx, sy);

  return {bestX, bestY};
}

// ===========================================================================
// negotiationCost – Eq. 10 from the NBLG paper
//   Cost(x,y) = b(x,y) + Σ_grids h(g) * p(g)
// ===========================================================================

double HybridLegalizer::negotiationCost(int cellIdx, int x, int y) const
{
  const HLCell& cell = cells_[cellIdx];
  double cost = targetCost(cellIdx, x, y);

  const int xBegin = std::max(0, x - cell.padLeft);
  const int xEnd = std::min(gridW_, x + cell.width + cell.padRight);
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

double HybridLegalizer::targetCost(int cellIdx, int x, int y) const
{
  const HLCell& cell = cells_[cellIdx];
  const int disp = std::abs(x - cell.initX) + std::abs(y - cell.initY);
  return static_cast<double>(disp)
         + mf_ * static_cast<double>(std::max(0, disp - th_));
}

// ===========================================================================
// adaptivePf – Eq. 14 from the NBLG paper
//   pf = 1.0 + α * exp(−β * exp(−γ * (i − ith)))
// ===========================================================================

double HybridLegalizer::adaptivePf(int iter) const
{
  return 1.0 + kAlpha * std::exp(-kBeta * std::exp(-kGamma * (iter - kIth)));
}

// ===========================================================================
// updateHistoryCosts – Eq. 12 from the NBLG paper
//   h_new = h_old + hf * overuse
// ===========================================================================

void HybridLegalizer::updateHistoryCosts()
{
  for (int gy = 0; gy < gridH_; ++gy) {
    for (int gx = 0; gx < gridW_; ++gx) {
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

void HybridLegalizer::updateDrcHistoryCosts(
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
    Node* node = network_->getNode(cell.db_inst_);
    if (node == nullptr) {
      continue;
    }
    odb::dbOrientType orient = node->getOrient();
    odb::dbSite* site = cell.db_inst_->getMaster()->getSite();
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

void HybridLegalizer::sortByNegotiationOrder(std::vector<int>& indices) const
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

void HybridLegalizer::greedyImprove(int passes)
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

      int bestX = cell.x;
      int bestY = cell.y;
      int bestDisp = curDisp;

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
        const int txBegin = std::max(0, tx - cell.padLeft);
        const int txEnd = std::min(gridW_, tx + cell.width + cell.padRight);
        for (int dy = 0; dy < cell.height; ++dy) {
          for (int gx = txBegin; gx < txEnd; ++gx) {
            if (gridAt(gx, ty + dy).overuse() > 0) {
              return;
            }
          }
        }
        const int d = std::abs(tx - cell.initX) + std::abs(ty - cell.initY);
        if (d < bestDisp) {
          bestDisp = d;
          bestX = tx;
          bestY = ty;
        }
      };

      for (int dx = -horizWindow_; dx <= horizWindow_; ++dx) {
        tryLoc(cell.initX + dx, cell.y);
        tryLoc(cell.initX + dx, cell.y - cell.height);
        tryLoc(cell.initX + dx, cell.y + cell.height);
      }

      place(idx, bestX, bestY);
      if (bestDisp < curDisp) {
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

void HybridLegalizer::cellSwap()
{
  const int maxDisp = maxDisplacement();

  // Group movable cells by (height, width, railType).
  struct GroupKey
  {
    int height;
    int width;
    HLPowerRailType rail;
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
      groups[{cells_[i].height, cells_[i].width, cells_[i].railType}].push_back(
          i);
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
            = std::abs(cb.x - ca.initX) + std::abs(cb.y - ca.initY);
        const int newDispB
            = std::abs(ca.x - cb.initX) + std::abs(ca.y - cb.initY);
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

}  // namespace dpl
