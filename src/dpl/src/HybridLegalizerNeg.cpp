//////////////////////////////////////////////////////////////////////////////
// HybridLegalizerNeg.cpp
//
// Part 2: Negotiation pass + post-optimisation
//
// Only cells flagged as illegal by Abacus enter the negotiation loop,
// but their neighbors (within the bounding window) are also allowed to
// rip-up and improve during negotiation, so the loop can create space
// organically.
//////////////////////////////////////////////////////////////////////////////

#include "HybridLegalizer.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_set>

namespace dpl {

//============================================================================
// Negotiation pass — top level
//============================================================================

void HybridLegalizer::runNegotiation(const std::vector<int>& illegalCells) {
  // Seed active set: illegal cells + their spatial neighbors
  // (neighbors can move to create space)
  std::unordered_set<int> activeSet(illegalCells.begin(), illegalCells.end());

  // Expand to include cells within window of each illegal cell
  for (int idx : illegalCells) {
    const Cell& c = cells_[idx];
    int xlo = c.x - horizWindow_;
    int xhi = c.x + c.width  + horizWindow_;
    int ylo = c.y - adjWindow_;
    int yhi = c.y + c.height + adjWindow_;

    for (int i = 0; i < (int)cells_.size(); ++i) {
      if (cells_[i].fixed) continue;
      const Cell& n = cells_[i];
      if (n.x >= xlo && n.x <= xhi && n.y >= ylo && n.y <= yhi)
        activeSet.insert(i);
    }
  }

  std::vector<int> active(activeSet.begin(), activeSet.end());

  // Phase 1: I = 0 (all active cells rip-up every iteration)
  logger_->info(utl::DPL, 910,
    "Negotiation phase 1: {} active cells, {} iterations.",
    active.size(), maxIterNeg_);

  for (int iter = 0; iter < maxIterNeg_; ++iter) {
    int overflows = negotiationIter(active, iter, /*updateHistory=*/true);
    if (overflows == 0) {
      logger_->info(utl::DPL, 911,
        "Negotiation phase 1 converged at iteration {}.", iter);
      return;
    }
  }

  // Phase 2: I = kIsolationPt (skip already-legal cells)
  logger_->info(utl::DPL, 912,
    "Negotiation phase 2: isolation point active, {} iterations.", kMaxIterNeg2);

  for (int iter = 0; iter < kMaxIterNeg2; ++iter) {
    int overflows = negotiationIter(active, iter + maxIterNeg_,
                                    /*updateHistory=*/true);
    if (overflows == 0) {
      logger_->info(utl::DPL, 913,
        "Negotiation phase 2 converged at iteration {}.", iter);
      return;
    }
  }

  logger_->warn(utl::DPL, 914,
    "Negotiation did not fully converge. Remaining violations: {}.",
    numViolations());
}

//============================================================================
// Single negotiation iteration
//============================================================================

int HybridLegalizer::negotiationIter(std::vector<int>& activeCells,
                                      int iter,
                                      bool updateHistory) {
  // Sort by negotiation order each iteration
  sortByNegotiationOrder(activeCells);

  for (int idx : activeCells) {
    Cell& c = cells_[idx];
    if (c.fixed) continue;

    // Isolation point: skip legal cells in phase 2
    if (iter >= kIsolationPt && isCellLegal(idx)) continue;

    // Rip up and find best location
    ripUp(idx);
    auto [bx, by] = findBestLocation(idx);
    place(idx, bx, by);
  }

  // Count overflows
  int totalOverflow = 0;
  for (int idx : activeCells) {
    const Cell& c = cells_[idx];
    if (c.fixed) continue;
    for (int dy = 0; dy < c.height; ++dy)
      for (int dx = 0; dx < c.width; ++dx)
        if (gridExists(c.x+dx, c.y+dy))
          totalOverflow += gridAt(c.x+dx, c.y+dy).overuse();
  }

  if (totalOverflow > 0 && updateHistory) {
    updateHistoryCosts();
    sortByNegotiationOrder(activeCells);
  }

  return totalOverflow;
}

//============================================================================
// Rip-up: remove cell from grid
//============================================================================

void HybridLegalizer::ripUp(int cellIdx) {
  addUsage(cellIdx, -1);
}

//============================================================================
// Place cell and update grid
//============================================================================

void HybridLegalizer::place(int cellIdx, int x, int y) {
  cells_[cellIdx].x = x;
  cells_[cellIdx].y = y;
  addUsage(cellIdx, 1);
}

//============================================================================
// Find best location within search window
// Enumerates candidate grid locations, returns minimum negotiation cost.
//============================================================================

std::pair<int,int> HybridLegalizer::findBestLocation(int cellIdx) const {
  const Cell& c = cells_[cellIdx];

  double bestCost = static_cast<double>(kInfCost);
  int    bestX    = c.x;
  int    bestY    = c.y;

  // Search window: current row ± horizWindow_, adjacent rows ± adjWindow_
  // We search 3 rows: (y-1), y, (y+1) with different x ranges
  auto tryLocation = [&](int wx, int wy, int xRange) {
    for (int dx = -xRange; dx <= xRange; ++dx) {
      int tx = wx + dx;
      int ty = wy;
      if (!inDie(tx, ty, c.width, c.height))    continue;
      if (!isValidRow(ty, c))                    continue;
      if (!respectsFence(cellIdx, tx, ty))       continue;

      double cost = negotiationCost(cellIdx, tx, ty);
      if (cost < bestCost) {
        bestCost = cost;
        bestX = tx;
        bestY = ty;
      }
    }
  };

  // Current row — wider window
  tryLocation(c.initX, c.y,                horizWindow_);

  // Adjacent rows — narrower window
  tryLocation(c.initX, c.y - c.height,     adjWindow_);
  tryLocation(c.initX, c.y + c.height,     adjWindow_);

  // Also try snapping to initial position
  {
    auto [sx, sy] = snapToLegal(cellIdx, c.initX, c.initY);
    if (inDie(sx, sy, c.width, c.height) &&
        isValidRow(sy, c) &&
        respectsFence(cellIdx, sx, sy)) {
      double cost = negotiationCost(cellIdx, sx, sy);
      if (cost < bestCost) { bestCost = cost; bestX = sx; bestY = sy; }
    }
  }

  return {bestX, bestY};
}

//============================================================================
// Negotiation cost — Eq. 10 from NBLG paper
//   Cost(x,y) = b(x,y) + Σ h(gx,gy) * p(gx,gy)
//============================================================================

double HybridLegalizer::negotiationCost(int cellIdx, int x, int y) const {
  const Cell& c = cells_[cellIdx];

  double cost = targetCost(cellIdx, x, y);

  for (int dy = 0; dy < c.height; ++dy) {
    for (int dx = 0; dx < c.width; ++dx) {
      int gx = x + dx;
      int gy = y + dy;
      if (!gridExists(gx, gy)) {
        cost += kInfCost;
        continue;
      }
      const Grid& g = gridAt(gx, gy);
      if (g.capacity == 0) {
        cost += kInfCost;  // blockage
        continue;
      }
      // h * p  (congestion term)
      double usg = static_cast<double>(g.usage + 1); // +1 for this cell
      double cap = static_cast<double>(g.capacity);
      double p   = (usg / cap);  // simplified; full version uses pf
      cost += g.histCost * p;
    }
  }
  return cost;
}

//============================================================================
// Target (displacement) cost — Eq. 11 from NBLG
//   b(x,y) = δ + mf * max(δ - th, 0)
//============================================================================

double HybridLegalizer::targetCost(int cellIdx, int x, int y) const {
  const Cell& c = cells_[cellIdx];
  int disp = std::abs(x - c.initX) + std::abs(y - c.initY);
  return static_cast<double>(disp)
         + mf_ * std::max(0, disp - th_);
}

//============================================================================
// Adaptive penalty function pf — Eq. 14 from NBLG
//   pf = 1.0 + α * exp(-β * exp(-γ*(i - ith)))
//============================================================================

double HybridLegalizer::adaptivePf(int iter) const {
  return 1.0 + kAlpha * std::exp(-kBeta * std::exp(-kGamma * (iter - kIth)));
}

//============================================================================
// Update history costs — Eq. 12 from NBLG
//   h_new(x,y) = h_old(x,y) + hf * overuse(x,y)
//============================================================================

void HybridLegalizer::updateHistoryCosts() {
  for (auto& g : grid_) {
    int ov = g.overuse();
    if (ov > 0)
      g.histCost += kHfDefault * ov;
  }
}

//============================================================================
// Sort by negotiation order:
//   Primary:   total overuse descending  (most congested first)
//   Secondary: height descending          (taller cells later — NBLG inverts
//                                          this so larger cells are processed
//                                          after smaller ones have settled)
//   Tertiary:  width descending
//============================================================================

void HybridLegalizer::sortByNegotiationOrder(std::vector<int>& indices) const {
  // Compute per-cell overuse
  auto cellOveruse = [this](int idx) -> int {
    const Cell& c = cells_[idx];
    int ov = 0;
    for (int dy = 0; dy < c.height; ++dy)
      for (int dx = 0; dx < c.width; ++dx)
        if (gridExists(c.x+dx, c.y+dy))
          ov += gridAt(c.x+dx, c.y+dy).overuse();
    return ov;
  };

  std::sort(indices.begin(), indices.end(),
    [&](int a, int b) {
      int oa = cellOveruse(a);
      int ob = cellOveruse(b);
      if (oa != ob) return oa > ob;                           // more overuse first
      if (cells_[a].height != cells_[b].height)
        return cells_[a].height < cells_[b].height;           // smaller height first
      return cells_[a].width < cells_[b].width;
    });
}

//============================================================================
// Post-optimisation: greedy improve
// For each cell, try all locations in search window with congestion penalty
// set to infinity (no new overlaps allowed). Accept if displacement reduces.
//============================================================================

void HybridLegalizer::greedyImprove(int passes) {
  std::vector<int> order;
  for (int i = 0; i < (int)cells_.size(); ++i)
    if (!cells_[i].fixed) order.push_back(i);

  for (int p = 0; p < passes; ++p) {
    int improved = 0;
    for (int idx : order) {
      Cell& c = cells_[idx];
      int curDisp = c.displacement();

      ripUp(idx);

      // Search window — same as negotiation but only accept zero-overflow
      int bestX = c.x, bestY = c.y;
      int bestDisp = curDisp;

      auto tryLoc = [&](int tx, int ty) {
        if (!inDie(tx, ty, c.width, c.height))  return;
        if (!isValidRow(ty, c))                  return;
        if (!respectsFence(idx, tx, ty))         return;
        // Check no overflow would result
        for (int dy = 0; dy < c.height; ++dy)
          for (int dx = 0; dx < c.width; ++dx)
            if (gridAt(tx+dx, ty+dy).overuse() > 0) return;
        int d = std::abs(tx - c.initX) + std::abs(ty - c.initY);
        if (d < bestDisp) { bestDisp = d; bestX = tx; bestY = ty; }
      };

      for (int dx = -horizWindow_; dx <= horizWindow_; ++dx) {
        tryLoc(c.initX + dx, c.y);
        if (c.y >= c.height)         tryLoc(c.initX + dx, c.y - c.height);
        if (c.y + c.height < gridH_) tryLoc(c.initX + dx, c.y + c.height);
      }

      place(idx, bestX, bestY);
      if (bestDisp < curDisp) ++improved;
    }
    if (improved == 0) break;  // converged
  }
}

//============================================================================
// Post-optimisation: cell swap
// For cells of same height/width/rail type, try swapping positions if it
// reduces total displacement without increasing max displacement.
//============================================================================

void HybridLegalizer::cellSwap() {
  int maxDisp = maxDisplacement();

  // Group cells by (height, width, railType)
  struct Key {
    int h, w;
    PowerRailType r;
    bool operator==(const Key& o) const {
      return h==o.h && w==o.w && r==o.r;
    }
  };
  struct KeyHash {
    size_t operator()(const Key& k) const {
      return std::hash<int>()(k.h) ^ (std::hash<int>()(k.w) << 8)
             ^ (std::hash<int>()((int)k.r) << 16);
    }
  };

  std::unordered_map<Key, std::vector<int>, KeyHash> groups;
  for (int i = 0; i < (int)cells_.size(); ++i) {
    if (cells_[i].fixed) continue;
    groups[{cells_[i].height, cells_[i].width, cells_[i].railType}]
      .push_back(i);
  }

  // Within each group, try pairwise swaps
  for (auto& [key, grp] : groups) {
    for (int ii = 0; ii < (int)grp.size(); ++ii) {
      for (int jj = ii + 1; jj < (int)grp.size(); ++jj) {
        int a = grp[ii], b = grp[jj];
        Cell& ca = cells_[a];
        Cell& cb = cells_[b];

        int dispBefore = ca.displacement() + cb.displacement();

        int newDispA = std::abs(cb.x - ca.initX) + std::abs(cb.y - ca.initY);
        int newDispB = std::abs(ca.x - cb.initX) + std::abs(ca.y - cb.initY);
        int dispAfter  = newDispA + newDispB;

        // Accept swap if: total displacement reduces AND
        //                 neither new displacement exceeds current max
        if (dispAfter < dispBefore &&
            newDispA <= maxDisp && newDispB <= maxDisp &&
            respectsFence(a, cb.x, cb.y) &&
            respectsFence(b, ca.x, ca.y) &&
            isValidRow(cb.y, ca) &&
            isValidRow(ca.y, cb)) {
          // Swap
          ripUp(a); ripUp(b);
          std::swap(ca.x, cb.x);
          std::swap(ca.y, cb.y);
          place(a, ca.x, ca.y);
          place(b, cb.x, cb.y);
        }
      }
    }
  }
}

}  // namespace dpl
