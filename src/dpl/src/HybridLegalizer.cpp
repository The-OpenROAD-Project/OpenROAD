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

// HybridLegalizer.cpp – initialisation, grid, Abacus pass, metrics.

#include "HybridLegalizer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "PlacementDRC.h"
#include "dpl/Opendp.h"
#include "graphics/DplObserver.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/Padding.h"
#include "infrastructure/network.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dpl {

// ===========================================================================
// FenceRegion
// ===========================================================================

bool FenceRegion::contains(int x, int y, int w, int h) const
{
  for (const auto& r : rects) {
    if (x >= r.xlo && y >= r.ylo && x + w <= r.xhi && y + h <= r.yhi) {
      return true;
    }
  }
  return false;
}

FenceRect FenceRegion::nearestRect(int cx, int cy) const
{
  assert(!rects.empty());
  const FenceRect* best = rects.data();
  int bestDist = std::numeric_limits<int>::max();
  for (const auto& r : rects) {
    const int dx = std::max({0, r.xlo - cx, cx - r.xhi});
    const int dy = std::max({0, r.ylo - cy, cy - r.yhi});
    const int d = dx + dy;
    if (d < bestDist) {
      bestDist = d;
      best = &r;
    }
  }
  return *best;
}

// ===========================================================================
// Constructor
// ===========================================================================

HybridLegalizer::HybridLegalizer(Opendp* opendp,
                                 odb::dbDatabase* db,
                                 utl::Logger* logger,
                                 const Padding* padding,
                                 DplObserver* debug_observer,
                                 Network* network)
    : opendp_(opendp), db_(db), logger_(logger), padding_(padding), debug_observer_(debug_observer), network_(network)
{
}

// ===========================================================================
// legalize – top-level entry point
// ===========================================================================

void HybridLegalizer::legalize()
{
  debugPrint(logger_,
             utl::DPL,
             "hybrid",
             1,
             "HybridLegalizer: starting legalization.");

  if (!initFromDb()) {
    return;
  }

  if (debug_observer_) {
    debug_observer_->startPlacement(db_->getChip()->getBlock());
  }
  if (debug_observer_) {
    setDplPositions();
    logger_->report("Pause before Abacus pass.");
    debug_observer_->redrawAndPause();
  }

  buildGrid();
  initFenceRegions();

  debugPrint(logger_,
             utl::DPL,
             "hybrid",
             1,
             "HybridLegalizer: {} cells, grid {}x{}.",
             cells_.size(),
             gridW_,
             gridH_);

  // --- Phase 1: Abacus (handles the majority of cells cheaply) -------------
  std::vector<int> illegal;
  if (run_abacus_) 
  {
    debugPrint(
        logger_, utl::DPL, "hybrid", 1, "HybridLegalizer: running Abacus pass.");
    illegal = runAbacus();

    debugPrint(logger_,
               utl::DPL,
               "hybrid",
               1,
               "HybridLegalizer: Abacus done, {} cells still illegal.",
               illegal.size());
  } else {
    debugPrint(
        logger_, utl::DPL, "hybrid", 1, "HybridLegalizer: skipping Abacus pass.");
    // Populate usage from initial coordinates
    for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
      if (!cells_[i].fixed) {
        addUsage(i, 1);
      }
    }
    // Sync all movable cells to the DPL Grid so PlacementDRC neighbour
    // lookups see the correct placement state.
    syncAllCellsToDplGrid();
    for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
      if (!cells_[i].fixed) {
        cells_[i].legal = isCellLegal(i);
        if (!cells_[i].legal) {
          illegal.push_back(i);
        }
      }
    }
  }

  if (debug_observer_) {
    setDplPositions();
    logger_->report("Pause after Abacus pass.");
    debug_observer_->redrawAndPause();
  }

  // --- Phase 2: Negotiation (fixes remaining violations) -------------------
  if (!illegal.empty()) {
    debugPrint(logger_,
               utl::DPL,
               "hybrid",
               1,
               "HybridLegalizer: negotiation pass on {} cells.",
               illegal.size());
    runNegotiation(illegal);
  }

  if (debug_observer_) {
    setDplPositions();
    logger_->report("Pause after negotiation pass.");
    debug_observer_->redrawAndPause();
  }

  // --- Phase 3: Post-optimisation ------------------------------------------
  debugPrint(
      logger_, utl::DPL, "hybrid", 1, "HybridLegalizer: post-optimisation.");
  // greedyImprove(5);
  // cellSwap();
  // greedyImprove(1);

  debugPrint(logger_,
             utl::DPL,
             "hybrid",
             1,
             "HybridLegalizer: done. AvgDisp={:.2f} MaxDisp={} Violations={}.",
             avgDisplacement(),
             maxDisplacement(),
             numViolations());

  // --- Write back to OpenDB ------------------------------------------------
  for (const auto& cell : cells_) {
    if (cell.fixed || cell.inst == nullptr) {
      continue;
    }
    const int dbX = dieXlo_ + cell.x * siteWidth_;
    const int dbY = dieYlo_ + cell.y * rowHeight_;
    cell.inst->setLocation(dbX, dbY);
    cell.inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    // Set orientation from the row so cells are properly flipped.
    if (opendp_ && opendp_->grid_) {
      odb::dbSite* site = cell.inst->getMaster()->getSite();
      if (site != nullptr) {
        auto orient = opendp_->grid_->getSiteOrientation(
            GridX{cell.x}, GridY{cell.y}, site);
        if (orient.has_value()) {
          cell.inst->setOrient(orient.value());
        }
      }
    }
  }
}

// ===========================================================================
// flushToDb – write current cell positions to ODB so the GUI reflects them
// ===========================================================================

void HybridLegalizer::flushToDb()
{
  for (const auto& cell : cells_) {
    if (cell.fixed || cell.inst == nullptr) {
      continue;
    }
    const int dbX = dieXlo_ + cell.x * siteWidth_;
    const int dbY = dieYlo_ + cell.y * rowHeight_;
    cell.inst->setLocation(dbX, dbY);
  }
}

// ===========================================================================
// setDplPositions – pass the positions to the DPL original structure (Node)
// ===========================================================================

void HybridLegalizer::setDplPositions()
{
  if (!network_) {
    return;
  }
  std::unordered_map<odb::dbInst*, Node*> inst_to_node;
  inst_to_node.reserve(network_->getNodes().size());
  for (const auto& node_ptr : network_->getNodes()) {
    if (node_ptr->getDbInst()) {
      inst_to_node[node_ptr->getDbInst()] = node_ptr.get();
    }
  }

  for (const auto& cell : cells_) {
    if (cell.fixed || cell.inst == nullptr) {
      continue;
    }
    auto it = inst_to_node.find(cell.inst);
    if (it != inst_to_node.end()) {
      const int coreX = cell.x * siteWidth_;
      const int coreY = cell.y * rowHeight_;
      it->second->setLeft(DbuX(coreX));
      it->second->setBottom(DbuY(coreY));
      it->second->setPlaced(true);

      // Update orientation to match the row.
      odb::dbSite* site = cell.inst->getMaster()->getSite();
      if (site != nullptr) {
        auto orient = opendp_->grid_->getSiteOrientation(
            GridX{cell.x}, GridY{cell.y}, site);
        if (orient.has_value()) {
          it->second->setOrient(orient.value());
        }
      }
    }
  }
}

// ===========================================================================
// Initialisation
// ===========================================================================

bool HybridLegalizer::initFromDb()
{
  if (db_->getChip() == nullptr) {
    return false;
  }
  auto* block = db_->getChip()->getBlock();
  if (block == nullptr) {
    return false;
  }

  const odb::Rect coreArea = block->getCoreArea();
  dieXlo_ = coreArea.xMin();
  dieYlo_ = coreArea.yMin();
  dieXhi_ = coreArea.xMax();
  dieYhi_ = coreArea.yMax();

  // Derive site dimensions from the first row.
  for (auto* row : block->getRows()) {
    auto* site = row->getSite();
    siteWidth_ = site->getWidth();
    rowHeight_ = site->getHeight();
    break;
  }
  assert(siteWidth_ > 0 && rowHeight_ > 0);

  // Assign power-rail types using row-index parity (VSS at even rows).
  // Replace with explicit LEF pg_pin parsing for advanced PDKs.
  const int numRows = (dieYhi_ - dieYlo_) / rowHeight_;
  rowRail_.clear();
  rowRail_.resize(numRows);
  for (int r = 0; r < numRows; ++r) {
    rowRail_[r] = (r % 2 == 0) ? HLPowerRailType::kVss : HLPowerRailType::kVdd;
  }

  // Build HLCell records from all placed instances.
  cells_.clear();
  cells_.reserve(block->getInsts().size());

  for (auto* inst : block->getInsts()) {
    const auto status = inst->getPlacementStatus();
    if (status == odb::dbPlacementStatus::NONE) {
      continue;
    }

    HLCell cell;
    cell.inst = inst;
    cell.fixed = (status == odb::dbPlacementStatus::FIRM
                  || status == odb::dbPlacementStatus::LOCKED
                  || status == odb::dbPlacementStatus::COVER);

    int dbX = 0;
    int dbY = 0;
    inst->getLocation(dbX, dbY);
    cell.initX = (dbX - dieXlo_) / siteWidth_;
    cell.initY = (dbY - dieYlo_) / rowHeight_;
    cell.x = cell.initX;
    cell.y = cell.initY;

    auto* master = inst->getMaster();
    cell.width = std::max(
        1,
        static_cast<int>(
            std::round(static_cast<double>(master->getWidth()) / siteWidth_)));
    cell.height = std::max(
        1,
        static_cast<int>(
            std::round(static_cast<double>(master->getHeight()) / rowHeight_)));

    cell.railType = inferRailType(cell.initY);
    // If the instance is currently flipped relative to the row's standard orientation,
    // its internal rail design is opposite of the row's bottom rail.
    if (opendp_ && opendp_->grid_) {
      auto siteOrient = opendp_->grid_->getSiteOrientation(GridX{cell.initX}, GridY{cell.initY}, master->getSite());
      if (siteOrient.has_value() && inst->getOrient() != siteOrient.value()) {
        cell.railType = (cell.railType == HLPowerRailType::kVss) ? HLPowerRailType::kVdd : HLPowerRailType::kVss;
      }
    }

    cell.flippable = master->getSymmetryX();  // X-symmetry allows vertical flip (MX)
    if (cell.height % 2 == 1) {
      // For 1-row cells, we usually assume they are flippable in most PDKs.
      cell.flippable = true;
    }

    if (padding_ != nullptr) {
      cell.padLeft = padding_->padLeft(inst).v;
      cell.padRight = padding_->padRight(inst).v;
    }

    cells_.push_back(cell);
  }

  return true;
}

HLPowerRailType HybridLegalizer::inferRailType(int rowIdx) const
{
  if (rowIdx >= 0 && rowIdx < static_cast<int>(rowRail_.size())) {
    return rowRail_[rowIdx];
  }
  return HLPowerRailType::kVss;
}

void HybridLegalizer::buildGrid()
{
  gridW_ = (dieXhi_ - dieXlo_) / siteWidth_;
  gridH_ = (dieYhi_ - dieYlo_) / rowHeight_;
  grid_.assign(static_cast<size_t>(gridW_) * gridH_, HLGrid{});

  // Mark blockages and record fixed-cell usage in one pass.
  // The padded range of fixed cells is also blocked so movable cells
  // cannot violate padding constraints relative to fixed instances.
  for (const HLCell& cell : cells_) {
    if (!cell.fixed) {
      continue;
    }
    const int xBegin = effXBegin(cell);
    const int xEnd = effXEnd(cell);
    for (int dy = 0; dy < cell.height; ++dy) {
      for (int gx = xBegin; gx < xEnd; ++gx) {
        const int gy = cell.y + dy;
        if (gridExists(gx, gy)) {
          gridAt(gx, gy).capacity = 0;
          // Physical footprint carries usage=1; padding slots do not.
          if (gx >= cell.x && gx < cell.x + cell.width) {
            gridAt(gx, gy).usage = 1;
          }
        }
      }
    }
  }
}

void HybridLegalizer::initFenceRegions()
{
  fences_.clear();  // Guard against double-population if legalize() is re-run.

  auto* block = db_->getChip()->getBlock();

  for (auto* region : block->getRegions()) {
    FenceRegion fr;
    fr.id = region->getId();

    for (auto* box : region->getBoundaries()) {
      FenceRect r;
      r.xlo = (box->xMin() - dieXlo_) / siteWidth_;
      r.ylo = (box->yMin() - dieYlo_) / rowHeight_;
      r.xhi = (box->xMax() - dieXlo_) / siteWidth_;
      r.yhi = (box->yMax() - dieYlo_) / rowHeight_;
      fr.rects.push_back(r);
    }

    if (!fr.rects.empty()) {
      fences_.push_back(std::move(fr));
    }
  }

  // Map each instance to its fence region (if any).
  for (auto& cell : cells_) {
    if (cell.inst == nullptr) {
      continue;
    }
    auto* region = cell.inst->getRegion();
    if (region == nullptr) {
      continue;
    }
    const int rid = region->getId();
    for (int fi = 0; fi < static_cast<int>(fences_.size()); ++fi) {
      if (fences_[fi].id == rid) {
        cell.fenceId = fi;
        break;
      }
    }
  }
}

// ===========================================================================
// HLGrid helpers
// ===========================================================================

void HybridLegalizer::addUsage(int cellIdx, int delta)
{
  const HLCell& cell = cells_[cellIdx];
  const int xBegin = effXBegin(cell);
  const int xEnd = effXEnd(cell);
  for (int dy = 0; dy < cell.height; ++dy) {
    for (int gx = xBegin; gx < xEnd; ++gx) {
      const int gy = cell.y + dy;
      if (gridExists(gx, gy)) {
        gridAt(gx, gy).usage += delta;
      }
    }
  }
}

// ===========================================================================
// DPL Grid synchronisation
// ===========================================================================

void HybridLegalizer::syncCellToDplGrid(int cellIdx)
{
  if (!opendp_ || !opendp_->grid_ || !network_) {
    return;
  }
  const HLCell& hlcell = cells_[cellIdx];
  if (hlcell.inst == nullptr) {
    return;
  }
  Node* node = network_->getNode(hlcell.inst);
  if (node == nullptr) {
    return;
  }
  // Update the Node's position to match the HLCell so Grid operations
  // (which read Node left/bottom) see the current placement.
  node->setLeft(DbuX(hlcell.x * siteWidth_));
  node->setBottom(DbuY(hlcell.y * rowHeight_));

  // Update orientation to match the row.
  odb::dbSite* site = hlcell.inst->getMaster()->getSite();
  if (site != nullptr) {
    auto orient = opendp_->grid_->getSiteOrientation(
        GridX{hlcell.x}, GridY{hlcell.y}, site);
    if (orient.has_value()) {
      node->setOrient(orient.value());
    }
  }

  opendp_->grid_->paintPixel(node, GridX{hlcell.x}, GridY{hlcell.y});
}

void HybridLegalizer::eraseCellFromDplGrid(int cellIdx)
{
  if (!opendp_ || !opendp_->grid_ || !network_) {
    return;
  }
  const HLCell& hlcell = cells_[cellIdx];
  if (hlcell.inst == nullptr) {
    return;
  }
  Node* node = network_->getNode(hlcell.inst);
  if (node == nullptr) {
    return;
  }
  // Ensure the Node's position matches the current HLCell position so
  // erasePixel clears the correct pixels (it reads gridCoveringPadded).
  node->setLeft(DbuX(hlcell.x * siteWidth_));
  node->setBottom(DbuY(hlcell.y * rowHeight_));
  opendp_->grid_->erasePixel(node);
}

void HybridLegalizer::syncAllCellsToDplGrid()
{
  if (!opendp_ || !opendp_->grid_ || !network_) {
    return;
  }
  // Clear all movable cells from the DPL Grid first, then repaint at
  // their current positions.  Fixed cells were already painted during
  // Opendp::setFixedGridCells() and should not be touched.
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (cells_[i].fixed || cells_[i].inst == nullptr) {
      continue;
    }
    Node* node = network_->getNode(cells_[i].inst);
    if (node == nullptr) {
      continue;
    }
    // Update Node position then erase whatever was previously painted.
    node->setLeft(DbuX(cells_[i].x * siteWidth_));
    node->setBottom(DbuY(cells_[i].y * rowHeight_));
    opendp_->grid_->erasePixel(node);
  }
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (cells_[i].fixed || cells_[i].inst == nullptr) {
      continue;
    }
    Node* node = network_->getNode(cells_[i].inst);
    if (node == nullptr) {
      continue;
    }
    // Set orientation to match the row, same as syncCellToDplGrid.
    odb::dbSite* site = cells_[i].inst->getMaster()->getSite();
    if (site != nullptr) {
      auto orient = opendp_->grid_->getSiteOrientation(
          GridX{cells_[i].x}, GridY{cells_[i].y}, site);
      if (orient.has_value()) {
        node->setOrient(orient.value());
      }
    }
    opendp_->grid_->paintPixel(node, GridX{cells_[i].x}, GridY{cells_[i].y});
  }
}

// ===========================================================================
// Constraint helpers
// ===========================================================================

bool HybridLegalizer::inDie(int x, int y, int w, int h) const
{
  return x >= 0 && y >= 0 && x + w <= gridW_ && y + h <= gridH_;
}

bool HybridLegalizer::isValidRow(int rowIdx, const HLCell& cell) const
{
  if (rowIdx < 0 || rowIdx + cell.height > gridH_) {
    return false;
  }
  const HLPowerRailType rowBot = rowRail_[rowIdx];
  if (cell.height % 2 == 1) {
    // Odd-height: bottom rail must match, or cell can be vertically flipped.
    return cell.flippable || (rowBot == cell.railType);
  }
  // Even-height: bottom boundary must be the correct rail type, and the
  // cell may only move by an even number of rows.
  return rowBot == cell.railType;
}

bool HybridLegalizer::respectsFence(int cellIdx, int x, int y) const
{
  const HLCell& cell = cells_[cellIdx];
  if (cell.fenceId < 0) {
    // Default region: must not overlap any named fence.
    for (const auto& fence : fences_) {
      if (fence.contains(x, y, cell.width, cell.height)) {
        return false;
      }
    }
    return true;
  }
  return fences_[cell.fenceId].contains(x, y, cell.width, cell.height);
}

std::pair<int, int> HybridLegalizer::snapToLegal(int cellIdx,
                                                 int x,
                                                 int y) const
{
  const HLCell& cell = cells_[cellIdx];
  x = std::max(0, std::min(x, gridW_ - cell.width));

  int bestRow = y;
  int bestDist = std::numeric_limits<int>::max();
  for (int r = 0; r + cell.height <= gridH_; ++r) {
    if (isValidRow(r, cell)) {
      const int d = std::abs(r - y);
      if (d < bestDist) {
        bestDist = d;
        bestRow = r;
      }
    }
  }
  return {x, bestRow};
}

// ===========================================================================
// Abacus pass
// ===========================================================================

std::vector<int> HybridLegalizer::runAbacus()
{
  // Build sorted order: ascending y then x.
  std::vector<int> order;
  order.reserve(cells_.size());
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (!cells_[i].fixed) {
      order.push_back(i);
    }
  }
  std::ranges::sort(order, [this](int a, int b) {
    if (cells_[a].y != cells_[b].y) {
      return cells_[a].y < cells_[b].y;
    }
    return cells_[a].x < cells_[b].x;
  });

  // Remove movable cell usage before replanting.
  for (int i : order) {
    addUsage(i, -1);
  }

  // Snap each cell to a legal row and group by that row.
  std::vector<std::vector<int>> byRow(gridH_);
  for (int i : order) {
    auto [sx, sy] = snapToLegal(i, cells_[i].x, cells_[i].y);
    cells_[i].x = sx;
    cells_[i].y = sy;
    if (sy >= 0 && sy < gridH_) {
      byRow[sy].push_back(i);
    }
  }

  // Run the Abacus sweep row by row.
  for (int r = 0; r < gridH_; ++r) {
    if (byRow[r].empty()) {
      continue;
    }
    std::ranges::sort(
        byRow[r], [this](int a, int b) { return cells_[a].x < cells_[b].x; });
    abacusRow(r, byRow[r]);
  }

  // Restore movable cell usage after placement.
  for (int i : order) {
    addUsage(i, 1);
  }

  // Sync to DPL Grid before DRC checks.
  syncAllCellsToDplGrid();

  // Collect still-illegal cells.
  std::vector<int> illegal;
  for (int i : order) {
    cells_[i].legal = isCellLegal(i);
    if (!cells_[i].legal) {
      illegal.push_back(i);
    }
  }
  return illegal;
}

void HybridLegalizer::abacusRow(int rowIdx, std::vector<int>& cellsInRow)
{
  std::vector<AbacusCluster> clusters;

  for (int idx : cellsInRow) {
    const HLCell& cell = cells_[idx];

    // Skip cells that violate fence or row constraints – negotiation handles
    // them later.
    if (!respectsFence(idx, cell.x, rowIdx) || !isValidRow(rowIdx, cell)) {
      continue;
    }

    AbacusCluster nc;
    nc.cellIndices.push_back(idx);
    const int effWidth = cell.width + cell.padLeft + cell.padRight;
    // Target the padded-left position so that cell.initX lands correctly.
    nc.optX = static_cast<double>(cell.initX - cell.padLeft);
    nc.totalWeight = 1.0;
    nc.totalQ = nc.optX;
    nc.totalWidth = effWidth;

    // Clamp to die boundary (padded width).
    nc.optX = std::max(
        0.0, std::min(nc.optX, static_cast<double>(gridW_ - effWidth)));
    clusters.push_back(std::move(nc));
    collapseClusters(clusters, rowIdx);
  }

  // Write solved positions back to cells_.
  for (const auto& cluster : clusters) {
    assignClusterPositions(cluster, rowIdx);
  }
}

void HybridLegalizer::collapseClusters(std::vector<AbacusCluster>& clusters,
                                       int /*rowIdx*/)
{
  while (clusters.size() >= 2) {
    AbacusCluster& last = clusters[clusters.size() - 1];
    AbacusCluster& prev = clusters[clusters.size() - 2];

    // Solve optimal position for the last cluster.
    last.optX = last.totalQ / last.totalWeight;
    last.optX = std::max(
        0.0,
        std::min(last.optX, static_cast<double>(gridW_ - last.totalWidth)));

    // If the last cluster overlaps the previous one, merge them.
    if (prev.optX + prev.totalWidth > last.optX) {
      prev.totalWeight += last.totalWeight;
      prev.totalQ += last.totalQ;
      prev.totalWidth += last.totalWidth;
      for (int idx : last.cellIndices) {
        prev.cellIndices.push_back(idx);
      }
      prev.optX = prev.totalQ / prev.totalWeight;
      prev.optX = std::max(
          0.0,
          std::min(prev.optX, static_cast<double>(gridW_ - prev.totalWidth)));
      clusters.pop_back();
    } else {
      break;
    }
  }

  // Re-solve the top cluster after any merge.
  if (!clusters.empty()) {
    AbacusCluster& top = clusters.back();
    top.optX = top.totalQ / top.totalWeight;
    top.optX = std::max(
        0.0, std::min(top.optX, static_cast<double>(gridW_ - top.totalWidth)));
  }
}

void HybridLegalizer::assignClusterPositions(const AbacusCluster& cluster,
                                             int rowIdx)
{
  // cluster.optX is the padded-left edge of the cluster.
  int paddedX = static_cast<int>(std::round(cluster.optX));
  paddedX = std::max(0, std::min(paddedX, gridW_ - cluster.totalWidth));

  for (int idx : cluster.cellIndices) {
    const int effWidth
        = cells_[idx].width + cells_[idx].padLeft + cells_[idx].padRight;
    // Physical left edge = padded-left edge + padLeft of this cell.
    cells_[idx].x = paddedX + cells_[idx].padLeft;
    cells_[idx].y = rowIdx;
    paddedX += effWidth;
    if (debug_observer_ && cells_[idx].inst != nullptr) {
      debug_observer_->placeInstance(cells_[idx].inst);
    }
  }
}

bool HybridLegalizer::isCellLegal(int cellIdx) const
{
  const HLCell& cell = cells_[cellIdx];
  if (!inDie(cell.x, cell.y, cell.width, cell.height)) {
    return false;
  }
  if (!isValidRow(cell.y, cell)) {
    return false;
  }
  if (!respectsFence(cellIdx, cell.x, cell.y)) {
    return false;
  }

  // Check placement DRCs (edge spacing, blocked layers, padding,
  // one-site gaps) against neighbours on the DPL Grid.
  if (opendp_ && opendp_->drc_engine_ && network_) {
    Node* node = network_->getNode(cell.inst);
    if (node != nullptr
        && !opendp_->drc_engine_->checkDRC(
            node, GridX{cell.x}, GridY{cell.y}, node->getOrient())) {
      return false;
    }
  }

  const int xBegin = effXBegin(cell);
  const int xEnd = effXEnd(cell);
  for (int dy = 0; dy < cell.height; ++dy) {
    for (int gx = xBegin; gx < xEnd; ++gx) {
      if (gridAt(gx, cell.y + dy).overuse() > 0) {
        return false;
      }
    }
  }
  return true;
}

// ===========================================================================
// Metrics
// ===========================================================================

double HybridLegalizer::avgDisplacement() const
{
  double sum = 0.0;
  int count = 0;
  for (const auto& cell : cells_) {
    if (!cell.fixed) {
      sum += cell.displacement();
      ++count;
    }
  }
  return count > 0 ? sum / count : 0.0;
}

int HybridLegalizer::maxDisplacement() const
{
  int mx = 0;
  for (const auto& cell : cells_) {
    if (!cell.fixed) {
      mx = std::max(mx, cell.displacement());
    }
  }
  return mx;
}

int HybridLegalizer::numViolations() const
{
  int count = 0;
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (!cells_[i].fixed && !isCellLegal(i)) {
      ++count;
    }
  }
  return count;
}

}  // namespace dpl
