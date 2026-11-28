// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "LegalSiteFinder.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <queue>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "PlacementDRC.h"
#include "boost/geometry/geometry.hpp"
#include "boost/geometry/index/predicates.hpp"
#include "dpl/Opendp.h"
#include "graphics/DplObserver.h"
#include "infrastructure/Coordinates.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/architecture.h"
#include "odb/db.h"
#include "optimization/detailed_orient.h"
#include "util/symmetry.h"
#include "utl/Logger.h"

namespace dpl {

using std::max;
using std::min;
using utl::DPL;
using utl::format_as;  // NOLINT(misc-unused-using-decls)

LegalSiteFinder::LegalSiteFinder(utl::Logger* logger,
                                 const Grid* grid,
                                 Architecture* arch,
                                 const PlacementDRC* drc_engine,
                                 const Opendp::RtreeBox* regions_rtree,
                                 DplObserver* debug_observer,
                                 const Limits& limits)
    : logger_(logger),
      grid_(grid),
      arch_(arch),
      drc_engine_(drc_engine),
      regions_rtree_(regions_rtree),
      debug_observer_(debug_observer),
      limits_(limits)
{
}

PixelPt LegalSiteFinder::diamondSearch(const Node* cell,
                                       const GridX x,
                                       const GridY y) const
{
  if (debug_observer_) {
    debug_observer_->clearDiamondSearch(cell);
  }

  // DPL displacement budget.
  GridX x_min = x - limits_.max_displacement_x;
  GridX x_max = x + limits_.max_displacement_x;
  GridY y_min = y - limits_.max_displacement_y;
  GridY y_max = y + limits_.max_displacement_y;

  // Restrict search to group boundary.
  Group* group = cell->getGroup();
  if (group) {
    // Boundary to grid staying inside.
    const GridRect grid_boundary = grid_->gridWithin(group->getBBox());
    const GridPt min = grid_boundary.closestPtInside({x_min, y_min});
    const GridPt max = grid_boundary.closestPtInside({x_max, y_max});
    x_min = min.x;
    y_min = min.y;
    x_max = max.x;
    y_max = max.y;
  }

  // Clip limits to grid bounds.
  x_min = max(GridX{0}, x_min);
  y_min = max(GridY{0}, y_min);
  x_max = min(grid_->getRowSiteCount(), x_max);
  y_max = min(grid_->getRowCount(), y_max);
  debugPrint(logger_,
             DPL,
             "place",
             1,
             "Diamond search {} ({}, {}) bounds ({}-{}, {}-{})",
             cell->name(),
             x,
             y,
             x_min,
             x_max - 1,
             y_min,
             y_max - 1);

  struct PQ_entry
  {
    int manhattan_distance;
    GridPt p;
    int sequence;
    bool operator>(const PQ_entry& other) const
    {
      return std::tie(manhattan_distance, sequence)
             > std::tie(other.manhattan_distance, other.sequence);
    }
  };
  std::priority_queue<PQ_entry, std::vector<PQ_entry>, std::greater<PQ_entry>>
      positionsHeap;
  std::unordered_set<GridPt> visited;
  int sequence = 0;
  const GridPt center{x, y};
  positionsHeap.push(
      {.manhattan_distance = 0, .p = center, .sequence = sequence++});
  visited.insert(center);

  const std::vector<GridPt> neighbors = {
      {GridX(-1), GridY(0)},
      {GridX(1), GridY(0)},
      {GridX(0), GridY(-1)},
      {GridX(0), GridY(1)},
  };
  while (!positionsHeap.empty()) {
    const GridPt nearest = positionsHeap.top().p;
    positionsHeap.pop();

    if (canBePlaced(cell, nearest.x, nearest.y)) {
      return PixelPt(
          grid_->gridPixel(nearest.x, nearest.y), nearest.x, nearest.y);
    }

    // Put neighbors in the queue
    for (const GridPt& offset : neighbors) {
      const GridPt neighbor = {nearest.x + offset.x, nearest.y + offset.y};
      // Check if it was already put in the queue
      if (visited.contains(neighbor)) {
        continue;
      }
      // Check limits
      if (neighbor.x < x_min || neighbor.x > x_max || neighbor.y < y_min
          || neighbor.y > y_max) {
        continue;
      }

      visited.insert(neighbor);
      positionsHeap.push({
          .manhattan_distance = calcDist(center, neighbor),
          .p = neighbor,
          .sequence = sequence++,
      });
    }
  }
  return PixelPt();
}

int LegalSiteFinder::calcDist(const GridPt p0, const GridPt p1) const
{
  const DbuY y_dist = abs(grid_->gridYToDbu(p0.y) - grid_->gridYToDbu(p1.y));
  const DbuX x_dist = gridToDbu(abs(p0.x - p1.x), grid_->getSiteWidth());
  return sumXY(x_dist, y_dist);
}

bool LegalSiteFinder::canBePlaced(const Node* cell,
                                  const GridX bin_x,
                                  const GridY bin_y) const
{
  debugPrint(logger_,
             DPL,
             "place",
             3,
             " canBePlaced {} ({:4},{:4})",
             cell->name(),
             bin_x,
             bin_y);

  if (bin_y >= grid_->getRowCount()) {
    return false;
  }

  const GridX x_end = bin_x + grid_->gridWidth(cell);
  const GridY y_end
      = grid_->gridEndY(grid_->gridYToDbu(bin_y) + cell->getHeight());

  if (debug_observer_) {
    debug_observer_->binSearch(cell, bin_x, bin_y, x_end, y_end);
  }
  return checkPixels(cell, bin_x, bin_y, x_end, y_end);
}

bool LegalSiteFinder::checkRegionOverlap(const Node* cell,
                                         const GridX x,
                                         const GridY y,
                                         const GridX x_end,
                                         const GridY y_end) const
{
  // TODO: Investigate the caching of this function
  // it is called with the same cell and x,y,x_end,y_end multiple times
  debugPrint(logger_,
             DPL,
             "region",
             1,
             "Checking region overlap for cell {} at x[{} {}] and y[{} {}]",
             cell->name(),
             x,
             x_end,
             y,
             y_end);
  const DbuX site_width = grid_->getSiteWidth();
  const Opendp::bgBox queryBox(
      {gridToDbu(x, site_width).v, grid_->gridYToDbu(y).v},
      {gridToDbu(x_end, site_width).v - 1, grid_->gridYToDbu(y_end).v - 1});

  std::vector<Opendp::bgBox> result;
  regions_rtree_->query(boost::geometry::index::intersects(queryBox),
                        std::back_inserter(result));

  if (cell->getRegion()) {
    if (result.size() == 1) {
      // the queryBox must be fully contained in the region or else there
      // might be a part of the cell outside of any region
      return boost::geometry::covered_by(queryBox, result[0]);
    }
    // if we are here, then the overlap size is either 0 or > 1
    // both are invalid. The overlap size should be 1
    return false;
  }
  // A cell without a region must not overlap any region.
  return result.empty();
}

// Check all pixels are empty.
bool LegalSiteFinder::checkPixels(const Node* cell,
                                  const GridX x,
                                  const GridY y,
                                  const GridX x_end,
                                  const GridY y_end) const
{
  if (x_end > grid_->getRowSiteCount()) {
    return false;
  }
  if (!checkRegionOverlap(cell, x, y, x_end, y_end)) {
    return false;
  }

  odb::dbSite* site = cell->getSite();
  for (GridY y1 = y; y1 < y_end; y1++) {
    const bool first_row = (y1 == y);
    for (GridX x1 = x; x1 < x_end; x1++) {
      const Pixel* pixel = grid_->gridPixel(x1, y1);
      if (pixel == nullptr || pixel->cell || !pixel->is_valid) {
        return false;
      }
      const Group* pixel_group = grid_->pixelGroup(pixel);
      if ((cell->inGroup() && pixel_group != cell->getGroup())
          || (!cell->inGroup() && pixel_group != nullptr)
          || (first_row && !grid_->getSiteOrientation(x1, y1, site))) {
        return false;
      }
    }
  }

  if (limits_.disallow_one_site_gaps) {
    // here we need to check for abutting first, if there is an abutting
    // cell then we continue as there is nothing wrong with it if there is
    // no abutting cell, we will then check cells at 1+ distances we only
    // need to check on the left and right sides
    const GridX x_begin = max(GridX{0}, x - 1);
    const GridY y_begin = max(GridY{0}, y);
    // inclusive search, so we don't add 1 to the end
    const GridX x_finish = min(x_end, grid_->getRowSiteCount() - 1);
    const GridY y_finish = min(y_end, grid_->getRowCount() - 1);

    auto isAbutted = [this](const GridX x, const GridY y) {
      const Pixel* pixel = grid_->gridPixel(x, y);
      return (pixel == nullptr || pixel->cell);
    };

    auto cellAtSite = [this](const GridX x, const GridY y) {
      const Pixel* pixel = grid_->gridPixel(x, y);
      return (pixel != nullptr && pixel->cell);
    };
    for (GridY y = y_begin; y < y_finish; ++y) {
      // left side
      if (!isAbutted(x_begin, y) && cellAtSite(x_begin - 1, y)) {
        debugPrint(logger_,
                   DPL,
                   "one_site_gap",
                   1,
                   "One site gap left of {}  at ({}, {})",
                   cell->name(),
                   x,
                   y);
        return false;
      }
      // right side
      if (!isAbutted(x_finish, y) && cellAtSite(x_finish + 1, y)) {
        debugPrint(logger_,
                   DPL,
                   "one_site_gap",
                   1,
                   "One site gap right of {} at ({}, {})",
                   cell->name(),
                   x,
                   y);
        return false;
      }
    }
  }

  const auto orient = grid_->getSiteOrientation(x, y, site);
  if (!orient) {
    return false;
  }

  // Check for symmetry
  auto* const dbMaster = cell->getDbMaster();
  const unsigned masterSym = dpl::DetailedOrient::getMasterSymmetry(dbMaster);
  if (!checkMasterSym(masterSym, *orient)) {
    return false;
  }

  // For multi-row cells, the bottom-row site/orient check above only covers
  // the bottom row; it doesn't ensure the master's power pin stack lines up
  // with the PDN rail stack across the span.  Reject wrong-parity landings.
  if (cell->getMaster()->isMultiRow() && !checkRowPowerCompatible(cell, y)) {
    return false;
  }

  return drc_engine_->checkDRC(cell, x, y, *orient);
}

bool LegalSiteFinder::checkRowPowerCompatible(const Node* cell,
                                              const GridY y) const
{
  const int row_idx = arch_->find_closest_row(grid_->gridYToDbu(y));
  if (row_idx >= arch_->getNumRows()) {
    return false;
  }
  bool flip = false;
  return arch_->powerCompatible(cell, arch_->getRow(row_idx), flip);
}

bool LegalSiteFinder::checkMasterSym(const unsigned masterSym,
                                     const unsigned cellOri)
{
  using odb::dbOrientType;
  switch (cellOri) {
    case dbOrientType::R0:
      return true;
    case dbOrientType::MX:
      return (masterSym & Symmetry_X) != 0;
    case dbOrientType::MY:
      return (masterSym & Symmetry_Y) != 0;
    case dbOrientType::R180:
      return (masterSym & Symmetry_X) && (masterSym & Symmetry_Y);
    case dbOrientType::R90:
    case dbOrientType::R270:
      return (masterSym & Symmetry_ROT90) != 0;
    case dbOrientType::MXR90:
    case dbOrientType::MYR90:
      return (masterSym & Symmetry_ROT90) && (masterSym & Symmetry_X)
             && (masterSym & Symmetry_Y);
    default:
      return false;
  }
}

}  // namespace dpl
