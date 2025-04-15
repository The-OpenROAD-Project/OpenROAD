// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "DplObserver.h"
#include "dpl/Grid.h"
#include "dpl/Objects.h"
#include "dpl/Opendp.h"
#include "dpl/Padding.h"
#include "dpl/PlacementDRC.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"

// #define ODP_DEBUG

namespace dpl {

using std::max;
using std::min;
using std::numeric_limits;
using std::sort;
using std::string;
using std::vector;

using utl::DPL;

using utl::format_as;

std::string Opendp::printBgBox(
    const boost::geometry::model::box<bgPoint>& queryBox)
{
  return fmt::format("({0}, {1}) - ({2}, {3})",
                     queryBox.min_corner().x(),
                     queryBox.min_corner().y(),
                     queryBox.max_corner().x(),
                     queryBox.max_corner().y());
}
void Opendp::detailedPlacement()
{
  if (debug_observer_) {
    debug_observer_->startPlacement(block_);
  }

  placement_failures_.clear();
  initGrid();
  // Paint fixed cells.
  setFixedGridCells();
  // group mapping & x_axis dummycell insertion
  groupInitPixels2();
  // y axis dummycell insertion
  groupInitPixels();

  if (!groups_.empty()) {
    placeGroups();
  }
  place();

  if (debug_observer_) {
    debug_observer_->endPlacement();
  }
}

////////////////////////////////////////////////////////////////

void Opendp::placeGroups()
{
  groupAssignCellRegions();

  prePlaceGroups();
  prePlace();

  // naive placement method ( multi -> single )
  placeGroups2();
  for (Group& group : groups_) {
    // magic number alert
    for (int pass = 0; pass < 3; pass++) {
      int refine_count = groupRefine(&group);
      int anneal_count = anneal(&group);
      // magic number alert
      if (refine_count < 10 || anneal_count < 100) {
        break;
      }
    }
  }
}

void Opendp::prePlace()
{
  for (Node& cell : cells_) {
    const Rect* group_rect = nullptr;
    if (!cell.inGroup() && !cell.isPlaced()) {
      for (Group& group : groups_) {
        for (const Rect& rect : group.getRects()) {
          if (checkOverlap(&cell, rect)) {
            group_rect = &rect;
          }
        }
      }
      if (group_rect) {
        const DbuPt nearest = nearestPt(&cell, *group_rect);
        const GridPt legal = legalGridPt(&cell, nearest);
        if (mapMove(&cell, legal)) {
          cell.setHold(true);
        }
      }
    }
  }
}

bool Opendp::checkOverlap(const Node* cell, const DbuRect& rect) const
{
  const DbuPt init = initialLocation(cell, false);
  const DbuX x = init.x;
  const DbuY y = init.y;
  return x + cell->getWidth() > rect.xl && x < rect.xl
         && y + cell->getHeight() > rect.yl && y < rect.yh;
}

DbuPt Opendp::nearestPt(const Node* cell, const DbuRect& rect) const
{
  const DbuPt init = initialLocation(cell, false);
  const DbuX x = init.x;
  const DbuY y = init.y;

  DbuX temp_x = x;
  DbuY temp_y = y;

  const DbuX cell_width = cell->getWidth();
  if (checkOverlap(cell, rect)) {
    DbuX dist_x;
    DbuY dist_y;
    if (abs(x + cell_width - rect.xl) > abs(rect.xh - x)) {
      dist_x = abs(rect.xh - x);
      temp_x = rect.xh;
    } else {
      dist_x = abs(x - rect.xl);
      temp_x = rect.xl - cell_width;
    }
    if (abs(y + cell->getHeight() - rect.yl) > abs(rect.yh - y)) {
      dist_y = abs(rect.yh - y);
      temp_y = rect.yh;
    } else {
      dist_y = abs(y - rect.yl);
      temp_y = rect.yl - cell->getHeight();
    }
    if (dist_x.v < dist_y.v) {
      return {temp_x, y};
    }
    return {x, temp_y};
  }

  if (x < rect.xl) {
    temp_x = rect.xl;
  } else if (x + cell_width > rect.xh) {
    temp_x = rect.xh - cell_width;
  }

  if (y < rect.yl) {
    temp_y = rect.yl;
  } else if (y + cell->getHeight() > rect.yh) {
    temp_y = rect.yh - cell->getHeight();
  }

  return {temp_x, temp_y};
}

void Opendp::prePlaceGroups()
{
  for (Group& group : groups_) {
    for (Node* cell : group.getCells()) {
      if (!cell->isFixed() && !cell->isPlaced()) {
        int dist = numeric_limits<int>::max();
        bool in_group = false;
        const Rect* nearest_rect = nullptr;
        for (const Rect& rect : group.getRects()) {
          if (isInside(cell, rect)) {
            in_group = true;
          }
          int rect_dist = distToRect(cell, rect);
          if (rect_dist < dist) {
            dist = rect_dist;
            nearest_rect = &rect;
          }
        }
        if (!nearest_rect) {
          continue;  // degenerate case of empty group.regions
        }
        if (!in_group) {
          const DbuPt nearest = nearestPt(cell, *nearest_rect);
          const GridPt legal = legalGridPt(cell, nearest);
          if (mapMove(cell, legal)) {
            cell->setHold(true);
          }
        }
      }
    }
  }
}

bool Opendp::isInside(const Node* cell, const Rect& rect) const
{
  const DbuPt init = initialLocation(cell, false);
  const DbuX x = init.x;
  const DbuY y = init.y;
  return x >= rect.xMin() && x + cell->getWidth() <= rect.xMax()
         && y >= rect.yMin() && y + cell->getHeight() <= rect.yMax();
}

int Opendp::distToRect(const Node* cell, const Rect& rect) const
{
  const DbuPt init = initialLocation(cell, true);
  const DbuX x = init.x;
  const DbuY y = init.y;

  DbuX dist_x{0};
  DbuY dist_y{0};
  if (x < rect.xMin()) {
    dist_x = DbuX{rect.xMin()} - x;
  } else if (x + cell->getWidth() > rect.xMax()) {
    dist_x = x + cell->getWidth() - rect.xMax();
  }

  if (y < rect.yMin()) {
    dist_y = DbuY{rect.yMin()} - y;
  } else if (y + cell->getHeight() > rect.yMax()) {
    dist_y = y + cell->getHeight() - rect.yMax();
  }

  return sumXY(dist_x, dist_y);
}

class CellPlaceOrderLess
{
 public:
  explicit CellPlaceOrderLess(const Rect& core);
  bool operator()(const Node* cell1, const Node* cell2) const;

 private:
  int centerDist(const Node* cell) const;

  const int center_x_;
  const int center_y_;
};

CellPlaceOrderLess::CellPlaceOrderLess(const Rect& core)
    : center_x_((core.xMin() + core.xMax()) / 2),
      center_y_((core.yMin() + core.yMax()) / 2)
{
}

int CellPlaceOrderLess::centerDist(const Node* cell) const
{
  return sumXY(abs(cell->getLeft() - center_x_),
               abs(cell->getBottom() - center_y_));
}

bool CellPlaceOrderLess::operator()(const Node* cell1, const Node* cell2) const
{
  const int64_t area1 = cell1->area();
  const int64_t area2 = cell2->area();
  const int dist1 = centerDist(cell1);
  const int dist2 = centerDist(cell2);
  return area1 > area2
         || (area1 == area2
             && (dist1 < dist2
                 || (dist1 == dist2
                     && strcmp(cell1->getDbInst()->getConstName(),
                               cell2->getDbInst()->getConstName())
                            < 0)));
}

void Opendp::place()
{
  vector<Node*> sorted_cells;
  sorted_cells.reserve(cells_.size());

  for (Node& cell : cells_) {
    if (!(cell.isFixed() || cell.inGroup() || cell.isPlaced())) {
      sorted_cells.push_back(&cell);
      if (!grid_->cellFitsInCore(&cell)) {
        logger_->error(DPL,
                       15,
                       "instance {} does not fit inside the ROW core area.",
                       cell.name());
      }
    }
  }
  sort(sorted_cells.begin(),
       sorted_cells.end(),
       CellPlaceOrderLess(grid_->getCore()));

  // Place multi-row instances first.
  if (have_multi_row_cells_) {
    for (Node* cell : sorted_cells) {
      if (isMultiRow(cell)) {
        debugPrint(logger_,
                   DPL,
                   "place",
                   1,
                   "Placing multi-row cell {}",
                   cell->name());
        if (!mapMove(cell)) {
          shiftMove(cell);
        }
      }
    }
  }
  for (Node* cell : sorted_cells) {
    if (!isMultiRow(cell)) {
      if (!mapMove(cell)) {
        shiftMove(cell);
      }
    }
  }
}

void Opendp::placeGroups2()
{
  for (Group& group : groups_) {
    vector<Node*> group_cells;
    group_cells.reserve(cells_.size());
    for (Node* cell : group.getCells()) {
      if (!cell->isFixed() && !cell->isPlaced()) {
        group_cells.push_back(cell);
      }
    }
    sort(group_cells.begin(),
         group_cells.end(),
         CellPlaceOrderLess(grid_->getCore()));

    // Place multi-row cells in each group region.
    bool multi_pass = true;
    for (Node* cell : group_cells) {
      if (!cell->isFixed() && !cell->isPlaced()) {
        assert(cell->inGroup());
        if (isMultiRow(cell)) {
          multi_pass = mapMove(cell);
          if (!multi_pass) {
            break;
          }
        }
      }
    }
    bool single_pass = true;
    if (multi_pass) {
      // Place single-row cells in each group region.
      for (Node* cell : group_cells) {
        if (!cell->isFixed() && !cell->isPlaced()) {
          assert(cell->inGroup());
          if (!isMultiRow(cell)) {
            single_pass = mapMove(cell);
            if (!single_pass) {
              break;
            }
          }
        }
      }
    }

    if (!single_pass || !multi_pass) {
      // Erase group cells
      for (Node* cell : group.getCells()) {
        unplaceCell(cell);
      }

      // Determine brick placement by utilization.
      // magic number alert
      if (group.getUtil() > 0.95) {
        brickPlace1(&group);
      } else {
        brickPlace2(&group);
      }
    }
  }
}

// Place cells in group toward edges.
void Opendp::brickPlace1(const Group* group)
{
  const Rect& boundary = group->getBBox();
  vector<Node*> sorted_cells(group->getCells());

  sort(sorted_cells.begin(), sorted_cells.end(), [&](Node* cell1, Node* cell2) {
    return rectDist(cell1, boundary) < rectDist(cell2, boundary);
  });

  for (Node* cell : sorted_cells) {
    DbuX x;
    DbuY y;
    rectDist(cell, boundary, &x.v, &y.v);
    const GridPt legal = legalGridPt(cell, {x, y});
    // This looks for a site starting at the nearest corner in rect,
    // which seems broken. It should start looking at the nearest point
    // on the rect boundary. -cherry
    if (!mapMove(cell, legal)) {
      logger_->error(DPL, 16, "cannot place instance {}.", cell->name());
    }
  }
}

void Opendp::rectDist(const Node* cell,
                      const Rect& rect,
                      // Return values.
                      int* x,
                      int* y) const
{
  const DbuPt init = initialLocation(cell, false);
  const DbuX init_x = init.x;
  const DbuY init_y = init.y;

  if (init_x > (rect.xMin() + rect.xMax()) / 2) {
    *x = rect.xMax();
  } else {
    *x = rect.xMin();
  }

  if (init_y > (rect.yMin() + rect.yMax()) / 2) {
    *y = rect.yMax();
  } else {
    *y = rect.yMin();
  }
}

int Opendp::rectDist(const Node* cell, const Rect& rect) const
{
  int x, y;
  rectDist(cell, rect, &x, &y);
  const DbuPt init = initialLocation(cell, false);
  return sumXY(abs(init.x - x), abs(init.y - y));
}

// Place group cells toward region edges.
void Opendp::brickPlace2(const Group* group)
{
  vector<Node*> sorted_cells(group->getCells());

  sort(sorted_cells.begin(), sorted_cells.end(), [&](Node* cell1, Node* cell2) {
    return rectDist(cell1, *cell1->getRegion())
           < rectDist(cell2, *cell2->getRegion());
  });

  for (Node* cell : sorted_cells) {
    if (!cell->isHold()) {
      DbuX x;
      DbuY y;
      rectDist(cell, *cell->getRegion(), &x.v, &y.v);
      const GridPt legal = legalGridPt(cell, {x, y});
      // This looks for a site starting at the nearest corner in rect,
      // which seems broken. It should start looking at the nearest point
      // on the rect boundary. -cherry
      if (!mapMove(cell, legal)) {
        logger_->error(DPL, 17, "cannot place instance {}.", cell->name());
      }
    }
  }
}

int Opendp::groupRefine(const Group* group)
{
  vector<Node*> sort_by_disp(group->getCells());

  sort(sort_by_disp.begin(), sort_by_disp.end(), [&](Node* cell1, Node* cell2) {
    return (disp(cell1) > disp(cell2));
  });

  int count = 0;
  for (int i = 0; i < sort_by_disp.size() * group_refine_percent_; i++) {
    Node* cell = sort_by_disp[i];
    if (!cell->isHold() && !cell->isFixed()) {
      if (refineMove(cell)) {
        count++;
      }
    }
  }
  return count;
}

// This is NOT annealing. It is random swapping. -cherry
int Opendp::anneal(Group* group)
{
  srand(rand_seed_);
  int count = 0;

  // magic number alert
  for (int i = 0; i < 100 * group->getCells().size(); i++) {
    Node* cell1 = group->getCells()[rand() % group->getCells().size()];
    Node* cell2 = group->getCells()[rand() % group->getCells().size()];
    if (swapCells(cell1, cell2)) {
      count++;
    }
  }
  return count;
}

// Not called -cherry.
int Opendp::refine()
{
  vector<Node*> sorted;
  sorted.reserve(cells_.size());

  for (Node& cell : cells_) {
    if (!(cell.isFixed() || cell.isHold() || cell.inGroup())) {
      sorted.push_back(&cell);
    }
  }
  sort(sorted.begin(), sorted.end(), [&](Node* cell1, Node* cell2) {
    return disp(cell1) > disp(cell2);
  });

  int count = 0;
  for (int i = 0; i < sorted.size() * refine_percent_; i++) {
    Node* cell = sorted[i];
    if (!cell->isHold()) {
      if (refineMove(cell)) {
        count++;
      }
    }
  }
  return count;
}

////////////////////////////////////////////////////////////////

bool Opendp::mapMove(Node* cell)
{
  const GridPt init = legalGridPt(cell, true);
  return mapMove(cell, init);
}

bool Opendp::mapMove(Node* cell, const GridPt& grid_pt)
{
  debugPrint(logger_,
             DPL,
             "place",
             1,
             "Map move {} ({}, {}) to ({}, {})",
             cell->name(),
             cell->getLeft(),
             cell->getBottom(),
             grid_pt.x,
             grid_pt.y);
  const PixelPt pixel_pt = searchNearestSite(cell, grid_pt.x, grid_pt.y);
  debugPrint(logger_,
             DPL,
             "place",
             1,
             "Search Nearest Site {} ({}, {}) to ({}, {})",
             cell->name(),
             cell->getLeft(),
             cell->getBottom(),
             pixel_pt.x,
             pixel_pt.y);
  if (pixel_pt.pixel) {
    placeCell(cell, pixel_pt.x, pixel_pt.y);
    if (debug_observer_) {
      debug_observer_->placeInstance(cell->getDbInst());
    }
    return true;
  }
  return false;
}

void Opendp::shiftMove(Node* cell)
{
  const GridPt grid_pt = legalGridPt(cell, true);
  // magic number alert
  const GridY boundary_margin{3};
  const GridX margin_width{grid_->gridPaddedWidth(cell).v * boundary_margin.v};
  std::set<Node*> region_cells;
  for (GridX x = grid_pt.x - margin_width; x < grid_pt.x + margin_width; x++) {
    for (GridY y = grid_pt.y - boundary_margin; y < grid_pt.y + boundary_margin;
         y++) {
      Pixel* pixel = grid_->gridPixel(x, y);
      if (pixel) {
        Node* cell = pixel->cell;
        if (cell && !cell->isFixed()) {
          region_cells.insert(cell);
        }
      }
    }
  }

  // erase region cells
  for (Node* around_cell : region_cells) {
    if (cell->inGroup() == around_cell->inGroup()) {
      unplaceCell(around_cell);
    }
  }

  // place target cell
  if (!mapMove(cell)) {
    placement_failures_.push_back(cell);
  }

  // re-place erased cells
  for (Node* around_cell : region_cells) {
    if (cell->inGroup() == around_cell->inGroup() && !mapMove(around_cell)) {
      placement_failures_.push_back(cell);
    }
  }
}

bool Opendp::swapCells(Node* cell1, Node* cell2)
{
  if (cell1 != cell2 && !cell1->isHold() && !cell2->isHold()
      && cell1->getWidth() == cell2->getWidth()
      && cell1->getHeight() == cell2->getHeight() && !cell1->isFixed()
      && !cell2->isFixed()) {
    const int dist_change
        = distChange(cell1, cell2->getLeft(), cell2->getBottom())
          + distChange(cell2, cell1->getLeft(), cell1->getBottom());

    if (dist_change < 0) {
      const GridX grid_x1 = grid_->gridPaddedX(cell2);
      const GridY grid_y1 = grid_->gridSnapDownY(cell2);
      const GridX grid_x2 = grid_->gridPaddedX(cell1);
      const GridY grid_y2 = grid_->gridSnapDownY(cell1);

      unplaceCell(cell1);
      unplaceCell(cell2);
      placeCell(cell1, grid_x1, grid_y1);
      placeCell(cell2, grid_x2, grid_y2);
      return true;
    }
  }
  return false;
}

bool Opendp::refineMove(Node* cell)
{
  const GridPt grid_pt = legalGridPt(cell, true);
  const PixelPt pixel_pt = searchNearestSite(cell, grid_pt.x, grid_pt.y);

  if (pixel_pt.pixel) {
    if (abs(grid_pt.x - pixel_pt.x) > max_displacement_x_
        || abs(grid_pt.y - pixel_pt.y) > max_displacement_y_) {
      return false;
    }

    const int dist_change
        = distChange(cell,
                     gridToDbu(pixel_pt.x, grid_->getSiteWidth()),
                     grid_->gridYToDbu(pixel_pt.y));

    if (dist_change < 0) {
      unplaceCell(cell);
      placeCell(cell, pixel_pt.x, pixel_pt.y);
      return true;
    }
  }
  return false;
}

int Opendp::distChange(const Node* cell, const DbuX x, const DbuY y) const
{
  const DbuPt init = initialLocation(cell, false);
  const int cell_dist
      = sumXY(abs(cell->getLeft() - init.x), abs(cell->getBottom() - init.y));
  const int pt_dist = sumXY(abs(init.x - x), abs(init.y - y));
  return pt_dist - cell_dist;
}

////////////////////////////////////////////////////////////////

PixelPt Opendp::searchNearestSite(const Node* cell,
                                  const GridX x,
                                  const GridY y) const
{
  // Diamond search limits.
  GridX x_min = x - max_displacement_x_;
  GridX x_max = x + max_displacement_x_;
  GridY y_min = y - max_displacement_y_;
  GridY y_max = y + max_displacement_y_;

  // Restrict search to group boundary.
  Group* group = cell->getGroup();
  if (group) {
    // Map boundary to grid staying inside.
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
             "Search Nearest Site {} ({}, {}) bounds ({}-{}, {}-{})",
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
    bool operator>(const PQ_entry& other) const
    {
      return manhattan_distance > other.manhattan_distance;
    }
    bool operator==(const PQ_entry& other) const
    {
      return manhattan_distance == other.manhattan_distance;
    }
  };
  std::priority_queue<PQ_entry, std::vector<PQ_entry>, std::greater<PQ_entry>>
      positionsHeap;
  std::unordered_set<GridPt> visited;
  GridPt center{x, y};
  positionsHeap.push(PQ_entry{0, center});
  visited.insert(center);

  const vector<GridPt> neighbors = {{GridX(-1), GridY(0)},
                                    {GridX(1), GridY(0)},
                                    {GridX(0), GridY(-1)},
                                    {GridX(0), GridY(1)}};
  while (!positionsHeap.empty()) {
    const GridPt nearest = positionsHeap.top().p;
    positionsHeap.pop();

    if (canBePlaced(cell, nearest.x, nearest.y)) {
      return PixelPt(
          grid_->gridPixel(nearest.x, nearest.y), nearest.x, nearest.y);
    }

    // Put neighbors in the queue
    for (GridPt offset : neighbors) {
      GridPt neighbor = {nearest.x + offset.x, nearest.y + offset.y};
      // Check if it was already put in the queue
      if (visited.count(neighbor) > 0) {
        continue;
      }
      // Check limits
      if (neighbor.x < x_min || neighbor.x > x_max || neighbor.y < y_min
          || neighbor.y > y_max) {
        continue;
      }

      visited.insert(neighbor);
      positionsHeap.push(PQ_entry{calcDist(center, neighbor), neighbor});
    }
  }
  return PixelPt();
}

int Opendp::calcDist(GridPt p0, GridPt p1) const
{
  DbuY y_dist = abs(grid_->gridYToDbu(p0.y) - grid_->gridYToDbu(p1.y));
  DbuX x_dist = gridToDbu(abs(p0.x - p1.x), grid_->getSiteWidth());
  return sumXY(x_dist, y_dist);
}

bool Opendp::canBePlaced(const Node* cell, GridX bin_x, GridY bin_y) const
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

  const GridX x_end = bin_x + grid_->gridPaddedWidth(cell);
  const GridY y_end = bin_y + grid_->gridHeight(cell);

  if (debug_observer_) {
    debug_observer_->binSearch(cell, bin_x, bin_y, x_end, y_end);
  }
  return checkPixels(cell, bin_x, bin_y, x_end, y_end);
}

bool Opendp::checkRegionOverlap(const Node* cell,
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
  const bgBox queryBox(
      {gridToDbu(x, site_width).v, grid_->gridYToDbu(y).v},
      {gridToDbu(x_end, site_width).v - 1, grid_->gridYToDbu(y_end).v - 1});

  std::vector<bgBox> result;
  findOverlapInRtree(queryBox, result);

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
  // If the cell has a region, then the region's bounding box must
  // be fully contained by the cell's bounding box.
  return result.empty();
}

// Check all pixels are empty.
bool Opendp::checkPixels(const Node* cell,
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

  for (GridY y1 = y; y1 < y_end; y1++) {
    const bool first_row = (y1 == y);
    for (GridX x1 = x; x1 < x_end; x1++) {
      const Pixel* pixel = grid_->gridPixel(x1, y1);
      auto site = cell->getSite();
      if (pixel == nullptr || pixel->cell || !pixel->is_valid
          || (cell->inGroup() && pixel->group != cell->getGroup())
          || (!cell->inGroup() && pixel->group)
          || (first_row && pixel->sites.find(site) == pixel->sites.end())) {
        return false;
      }
    }
    if (disallow_one_site_gaps_) {
      // here we need to check for abutting first, if there is an abutting cell
      // then we continue as there is nothing wrong with it
      // if there is no abutting cell, we will then check cells at 1+ distances
      // we only need to check on the left and right sides
      const GridX x_begin = max(GridX{0}, x - 1);
      const GridY y_begin = max(GridY{0}, y - 1);
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
      for (GridY y = y_begin; y <= y_finish; ++y) {
        // left side
        if (!isAbutted(x_begin, y) && cellAtSite(x_begin - 1, y)) {
          return false;
        }
        // right side
        if (!isAbutted(x_finish, y) && cellAtSite(x_finish + 1, y)) {
          return false;
        }
      }
    }
  }
  const auto& orient = grid_->gridPixel(x, y)->sites.at(
      cell->getDbInst()->getMaster()->getSite());
  return drc_engine_->checkEdgeSpacing(cell, x, y, orient);
}

////////////////////////////////////////////////////////////////

// Legalize cell origin
//  inside the core
//  row site
DbuPt Opendp::legalPt(const Node* cell, const DbuPt& pt) const
{
  // Move inside core.
  const DbuX site_width = grid_->getSiteWidth();
  const DbuX core_x = std::clamp(
      pt.x,
      DbuX{0},
      gridToDbu(grid_->getRowSiteCount(), site_width) - cell->getWidth());
  // Align with row site.
  const GridX grid_x{divRound(core_x.v, site_width.v)};
  const DbuX legal_x{gridToDbu(grid_x, site_width)};
  // Align to row
  const DbuY core_y = std::clamp(
      pt.y, DbuY{0}, DbuY{grid_->getCore().yMax()} - cell->getHeight());
  const GridY grid_y = grid_->gridRoundY(core_y);
  DbuY legal_y = grid_->gridYToDbu(grid_y);

  return {legal_x, legal_y};
}

GridPt Opendp::legalGridPt(const Node* cell, const DbuPt& pt) const
{
  const DbuPt legal = legalPt(cell, pt);
  return GridPt(grid_->gridX(legal.x), grid_->gridSnapDownY(legal.y));
}

DbuPt Opendp::nearestBlockEdge(const Node* cell,
                               const DbuPt& legal_pt,
                               const Rect& block_bbox) const
{
  const DbuX legal_x = legal_pt.x;
  const DbuY legal_y = legal_pt.y;

  const DbuX x_min_dist = abs(legal_x - block_bbox.xMin());
  const DbuX x_max_dist
      = abs(DbuX{block_bbox.xMax()} - (legal_x + cell->getWidth()));
  const DbuY y_min_dist = abs(legal_y - block_bbox.yMin());
  const DbuY y_max_dist
      = abs(DbuY{block_bbox.yMax()} - (legal_y + cell->getHeight()));

  const int min_dist
      = std::min({x_min_dist.v, x_max_dist.v, y_min_dist.v, y_max_dist.v});

  if (min_dist == x_min_dist) {  // left of block
    return legalPt(cell,
                   {DbuX{block_bbox.xMin()} - cell->getWidth(), legal_pt.y});
  }
  if (min_dist == x_max_dist) {  // right of block
    return legalPt(cell, {DbuX{block_bbox.xMax()}, legal_pt.y});
  }
  if (min_dist == y_min_dist) {  // below block
    return legalPt(cell,
                   {legal_pt.x, DbuY{block_bbox.yMin() - cell->getHeight().v}});
  }
  // above block
  return legalPt(cell, {legal_pt.x, DbuY{block_bbox.yMax()}});
}

// Find the nearest valid site left/right/above/below, if any.
// The site doesn't need to be empty but mearly valid.  That should
// be a reasonable place to start the search.  Returns true if any
// site can be found.
bool Opendp::moveHopeless(const Node* cell, GridX& grid_x, GridY& grid_y) const
{
  GridX best_x = grid_x;
  GridY best_y = grid_y;
  int best_dist = std::numeric_limits<int>::max();
  const GridX site_count = grid_->getRowSiteCount();
  const GridY row_count = grid_->getRowCount();
  const DbuX site_width = grid_->getSiteWidth();

  for (GridX x = grid_x - 1; x >= 0; --x) {  // left
    if (grid_->pixel(grid_y, x).is_valid) {
      best_dist = gridToDbu(grid_x - x - 1, site_width).v;
      best_x = x;
      best_y = grid_y;
      break;
    }
  }
  for (GridX x = grid_x + 1; x < site_count; ++x) {  // right
    if (grid_->pixel(grid_y, x).is_valid) {
      const int dist = gridToDbu(x - grid_x, site_width).v - cell->getWidth().v;
      if (dist < best_dist) {
        best_dist = dist;
        best_x = x;
        best_y = grid_y;
      }
      break;
    }
  }
  for (GridY y = grid_y - 1; y >= 0; --y) {  // below
    if (grid_->pixel(y, grid_x).is_valid) {
      const int dist = (grid_->gridYToDbu(grid_y) - grid_->gridYToDbu(y)).v;
      if (dist < best_dist) {
        best_dist = dist;
        best_x = grid_x;
        best_y = y;
      }
      break;
    }
  }
  for (GridY y = grid_y + 1; y < row_count; ++y) {  // above
    if (grid_->pixel(y, grid_x).is_valid) {
      const int dist = (grid_->gridYToDbu(y) - grid_->gridYToDbu(grid_y)).v;
      if (dist < best_dist) {
        best_dist = dist;
        best_x = grid_x;
        best_y = y;
      }
      break;
    }
  }
  if (best_dist != std::numeric_limits<int>::max()) {
    grid_x = best_x;
    grid_y = best_y;
    return true;
  }
  return false;
}

void Opendp::initMacrosAndGrid()
{
  importDb();
  initGrid();
  setFixedGridCells();
}

void Opendp::convertDbToCell(dbInst* db_inst, Node& cell)
{
  cell.setDbInst(db_inst);
  Rect bbox = getBbox(db_inst);
  cell.setWidth(DbuX{bbox.dx()});
  cell.setHeight(DbuY{bbox.dy()});
  cell.setLeft(DbuX{bbox.xMin()});
  cell.setBottom(DbuY{bbox.yMin()});
  cell.setOrient(db_inst->getOrient());
}

DbuPt Opendp::pointOffMacro(const Node& cell)
{
  // Get cell position
  const DbuPt init = initialLocation(&cell, false);
  const Rect bbox(init.x.v,
                  init.y.v,
                  init.x.v + cell.getWidth().v,
                  init.y.v + cell.getHeight().v);

  const GridRect grid_box = grid_->gridCovering(bbox);

  Pixel* pixel1 = grid_->gridPixel(grid_box.xlo, grid_box.ylo);
  Pixel* pixel2 = grid_->gridPixel(grid_box.xhi, grid_box.ylo);
  Pixel* pixel3 = grid_->gridPixel(grid_box.xlo, grid_box.yhi);
  Pixel* pixel4 = grid_->gridPixel(grid_box.xhi, grid_box.yhi);

  Node* block = nullptr;
  if (pixel1 && pixel1->cell && pixel1->cell->isBlock()) {
    block = pixel1->cell;
  } else if (pixel2 && pixel2->cell && pixel2->cell->isBlock()) {
    block = pixel2->cell;
  } else if (pixel3 && pixel3->cell && pixel3->cell->isBlock()) {
    block = pixel3->cell;
  } else if (pixel4 && pixel4->cell && pixel4->cell->isBlock()) {
    block = pixel4->cell;
  }

  if (block && block->isBlock()) {
    // Get new legal position
    const Rect block_bbox(block->getLeft().v,
                          block->getBottom().v,
                          block->getLeft().v + block->getWidth().v,
                          block->getBottom().v + block->getHeight().v);
    return nearestBlockEdge(&cell, init, block_bbox);
  }
  return init;
}

void Opendp::legalCellPos(dbInst* db_inst)
{
  Node cell;
  convertDbToCell(db_inst, cell);
  // returns the initial position of the cell
  const DbuPt init_pos = initialLocation(&cell, false);
  // returns the modified position if the cell is in a macro
  const DbuPt legal_pt = pointOffMacro(cell);
  // return the modified position if the cell is outside the die
  const DbuPt new_pos = legalPt(&cell, legal_pt);

  if (init_pos == new_pos) {
    return;
  }

  // transform to grid Pos for align
  const GridPt legal_grid_pt{grid_->gridX(DbuX{new_pos.x}),
                             grid_->gridSnapDownY(DbuY{new_pos.y})};
  // Transform position on real position
  setGridPaddedLoc(&cell, legal_grid_pt.x, legal_grid_pt.y);
  // Set position of cell on db
  const Rect core = grid_->getCore();
  db_inst->setLocation(core.xMin() + cell.getLeft().v,
                       core.yMin() + cell.getBottom().v);
}

DbuPt Opendp::initialLocation(const Node* cell, const bool padded) const
{
  DbuPt loc;
  cell->getDbInst()->getLocation(loc.x.v, loc.y.v);
  loc.x -= grid_->getCore().xMin();
  if (padded) {
    loc.x -= gridToDbu(padding_->padLeft(cell), grid_->getSiteWidth());
  }
  loc.y -= grid_->getCore().yMin();
  return loc;
}

// Legalize pt origin for cell
//  inside the core
//  row site
//  not on top of a macro
//  not in a hopeless site
DbuPt Opendp::legalPt(const Node* cell, const bool padded) const
{
  if (cell->isFixed()) {
    logger_->critical(
        DPL, 26, "legalPt called on fixed cell {}.", cell->name());
  }

  const DbuPt init = initialLocation(cell, padded);
  DbuPt legal_pt = legalPt(cell, init);
  GridX grid_x = grid_->gridX(legal_pt.x);
  GridY grid_y = grid_->gridSnapDownY(legal_pt.y);

  Pixel* pixel = grid_->gridPixel(grid_x, grid_y);
  if (pixel) {
    // Move std cells off of macros.  First try the is_hopeless strategy
    if (pixel->is_hopeless && moveHopeless(cell, grid_x, grid_y)) {
      legal_pt = DbuPt(gridToDbu(grid_x, grid_->getSiteWidth()),
                       grid_->gridYToDbu(grid_y));
      pixel = grid_->gridPixel(grid_x, grid_y);
    }

    const Node* block = static_cast<Node*>(pixel->cell);

    // If that didn't do the job fall back on the old move to nearest
    // edge strategy.  This doesn't consider site availability at the
    // end used so it is secondary.
    if (block && block->isBlock()) {
      const Rect block_bbox(block->getLeft().v,
                            block->getBottom().v,
                            block->getLeft().v + block->getWidth().v,
                            block->getBottom().v + block->getHeight().v);
      if ((legal_pt.x + cell->getWidth()) >= block_bbox.xMin()
          && legal_pt.x <= block_bbox.xMax()
          && (legal_pt.y + cell->getHeight()) >= block_bbox.yMin()
          && legal_pt.y <= block_bbox.yMax()) {
        legal_pt = nearestBlockEdge(cell, legal_pt, block_bbox);
      }
    }
  }

  return legal_pt;
}

GridPt Opendp::legalGridPt(const Node* cell, const bool padded) const
{
  const DbuPt pt = legalPt(cell, padded);
  return GridPt(grid_->gridX(pt.x), grid_->gridSnapDownY(pt.y));
}

void Opendp::setGridPaddedLoc(Node* cell, const GridX x, const GridY y)
{
  cell->setLeft(gridToDbu(x + padding_->padLeft(cell), grid_->getSiteWidth()));
  cell->setBottom(grid_->gridYToDbu(y));
}
void Opendp::placeCell(Node* cell, const GridX x, const GridY y)
{
  grid_->paintPixel(cell, x, y);
  setGridPaddedLoc(cell, x, y);
  cell->setPlaced(true);
  cell->setOrient(grid_->gridPixel(x, y)->sites.at(
      cell->getDbInst()->getMaster()->getSite()));
}

void Opendp::unplaceCell(Node* cell)
{
  grid_->erasePixel(cell);
  cell->setPlaced(false);
  cell->setHold(false);
}

}  // namespace dpl
