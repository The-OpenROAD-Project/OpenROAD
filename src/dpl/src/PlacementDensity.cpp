// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

// Placement density queries.  Density is the instance area inside a region
// over the legal placement site area inside it, so it says how much room is
// left there for one more cell.  Nothing is cached: every call measures the
// placement as it currently stands.

#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include "dpl/Opendp.h"
#include "infrastructure/Coordinates.h"
#include "infrastructure/Grid.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dpl {

using utl::DPL;

namespace {

int64_t overlapArea(const odb::Rect& a, const odb::Rect& b)
{
  const int64_t dx
      = std::min(a.xMax(), b.xMax()) - std::max(a.xMin(), b.xMin());
  const int64_t dy
      = std::min(a.yMax(), b.yMax()) - std::max(a.yMin(), b.yMin());
  return (dx > 0 && dy > 0) ? dx * dy : 0;
}

double areaDensity(const int64_t occupied, const int64_t placeable)
{
  if (placeable <= 0) {
    // Nothing fits where there is no site, so report full rather than empty.
    return 1.0;
  }
  return std::min(
      1.0, static_cast<double>(occupied) / static_cast<double>(placeable));
}

// Index of the interval of edges holding v, clamped to the edge list.
int edgeIndex(const std::vector<int>& edges, const int v)
{
  const int last = static_cast<int>(edges.size()) - 2;
  if (v <= edges.front()) {
    return 0;
  }
  if (v >= edges.back()) {
    return last;
  }
  const auto it = std::ranges::upper_bound(edges, v);
  return static_cast<int>(it - edges.begin()) - 1;
}

// Turns GCell start edges into a strictly increasing edge list closed at
// end, so GCell i spans [edges[i], edges[i + 1]).
std::vector<int> closeEdges(std::vector<int> lines, const int end)
{
  std::ranges::sort(lines);
  std::vector<int> edges;
  edges.reserve(lines.size() + 1);
  for (const int line : lines) {
    if (line < end && (edges.empty() || line != edges.back())) {
      edges.push_back(line);
    }
  }
  edges.push_back(end);
  return edges;
}

}  // namespace

odb::dbBlock* Opendp::densityBlock() const
{
  if (block_ == nullptr) {
    logger_->error(DPL,
                   1400,
                   "initPlacementGrid() must run before querying placement "
                   "density.");
  }
  return block_;
}

void Opendp::visitPlacementSites(
    const odb::Rect& region,
    const std::function<void(const odb::Rect& site)>& visitor) const
{
  // The grid is indexed off the core's lower left corner.
  odb::Rect local = region;
  local.moveDelta(-core_.xMin(), -core_.yMin());

  const int site_width = grid_->getSiteWidth().v;
  const GridX x_begin = std::max(GridX{0}, grid_->gridX(DbuX{local.xMin()}));
  const GridX x_end
      = std::min(grid_->getRowSiteCount(), grid_->gridEndX(DbuX{local.xMax()}));
  const GridY y_begin
      = std::max(GridY{0}, grid_->gridSnapDownY(DbuY{local.yMin()}));
  const GridY y_end
      = std::min(grid_->getRowCount(), grid_->gridEndY(DbuY{local.yMax()}));

  for (GridY y = y_begin; y < y_end; y++) {
    const int row_lo = core_.yMin() + grid_->gridYToDbu(y).v;
    const int row_hi = core_.yMin() + grid_->gridYToDbu(y + 1).v;
    for (GridX x = x_begin; x < x_end; x++) {
      const Pixel* pixel = grid_->gridPixel(x, y);
      if (pixel == nullptr || !pixel->is_valid) {
        continue;
      }
      const int site_lo = core_.xMin() + (x.v * site_width);
      visitor(odb::Rect(site_lo, row_lo, site_lo + site_width, row_hi));
    }
  }
}

void Opendp::visitPlacedInstances(
    const std::function<void(const odb::Rect& bbox)>& visitor) const
{
  // Read the locations from the db rather than from the dpl network, so a
  // query made between optimization steps sees where the cells are now.
  for (odb::dbInst* inst : block_->getInsts()) {
    // Pads and cover cells sit outside the rows and take no placement area;
    // fillers and macros take theirs like any other instance.
    if (!inst->getPlacementStatus().isPlaced()
        || !inst->getMaster()->isCoreAutoPlaceable()) {
      continue;
    }
    visitor(inst->getBBox()->getBox());
  }
}

void Opendp::getGCellEdges(std::vector<int>& x_edges,
                           std::vector<int>& y_edges) const
{
  odb::dbGCellGrid* gcell_grid = block_->getGCellGrid();
  if (gcell_grid == nullptr) {
    logger_->error(DPL,
                   1401,
                   "The design has no GCell grid. Run global routing before "
                   "querying GCell placement density.");
  }

  std::vector<int> x_lines;
  std::vector<int> y_lines;
  gcell_grid->getGridX(x_lines);
  gcell_grid->getGridY(y_lines);

  // dbGCellGrid holds the GCell start edges only; the last GCell of each
  // row and column runs to the die boundary.
  const odb::Rect die = block_->getDieArea();
  x_edges = closeEdges(std::move(x_lines), die.xMax());
  y_edges = closeEdges(std::move(y_lines), die.yMax());

  if (x_edges.size() < 2 || y_edges.size() < 2) {
    logger_->error(DPL, 1402, "The design's GCell grid is empty.");
  }
}

int64_t Opendp::placeableArea(const odb::Rect& region) const
{
  int64_t area = 0;
  visitPlacementSites(region, [&](const odb::Rect& site) {
    area += overlapArea(site, region);
  });
  return area;
}

double Opendp::getPlacementDensity(const odb::Rect& region) const
{
  densityBlock();

  const int64_t placeable = placeableArea(region);
  if (placeable <= 0) {
    return areaDensity(0, 0);
  }

  int64_t occupied = 0;
  visitPlacedInstances(
      [&](const odb::Rect& bbox) { occupied += overlapArea(bbox, region); });

  return areaDensity(occupied, placeable);
}

std::vector<GCellDensity> Opendp::getGCellDensities(
    const odb::Rect& region) const
{
  densityBlock();
  if (region.dx() <= 0 || region.dy() <= 0) {
    return {};
  }

  std::vector<int> x_edges;
  std::vector<int> y_edges;
  getGCellEdges(x_edges, y_edges);

  const odb::Rect grid_extent(
      x_edges.front(), y_edges.front(), x_edges.back(), y_edges.back());
  if (!region.overlaps(grid_extent)) {
    return {};
  }
  const odb::Rect search = region.intersect(grid_extent);

  // The GCells overlapping the region.  A GCell owns its lower edge and not
  // its upper one, hence the -1 on the high side.
  const int x_begin = edgeIndex(x_edges, search.xMin());
  const int x_end = edgeIndex(x_edges, search.xMax() - 1);
  const int y_begin = edgeIndex(y_edges, search.yMin());
  const int y_end = edgeIndex(y_edges, search.yMax() - 1);
  const int64_t count_x = x_end - x_begin + 1;
  const int64_t count_y = y_end - y_begin + 1;

  const auto gcellRect = [&](const int gx, const int gy) {
    return odb::Rect(
        x_edges[gx], y_edges[gy], x_edges[gx + 1], y_edges[gy + 1]);
  };
  const odb::Rect bounds(gcellRect(x_begin, y_begin).ll(),
                         gcellRect(x_end, y_end).ur());

  // Both areas are gathered in one pass each, splitting whatever straddles a
  // GCell boundary between the GCells it covers.
  std::vector<int64_t> placeable(count_x * count_y, 0);
  std::vector<int64_t> occupied(count_x * count_y, 0);
  const auto addArea = [&](std::vector<int64_t>& bins, const odb::Rect& rect) {
    if (!rect.overlaps(bounds)) {
      return;
    }
    const odb::Rect clipped = rect.intersect(bounds);
    const int gx_lo = edgeIndex(x_edges, clipped.xMin());
    const int gx_hi = edgeIndex(x_edges, clipped.xMax() - 1);
    const int gy_lo = edgeIndex(y_edges, clipped.yMin());
    const int gy_hi = edgeIndex(y_edges, clipped.yMax() - 1);
    for (int gy = gy_lo; gy <= gy_hi; gy++) {
      for (int gx = gx_lo; gx <= gx_hi; gx++) {
        bins[((gy - y_begin) * count_x) + (gx - x_begin)]
            += overlapArea(clipped, gcellRect(gx, gy));
      }
    }
  };

  visitPlacementSites(bounds,
                      [&](const odb::Rect& site) { addArea(placeable, site); });
  visitPlacedInstances([&](const odb::Rect& bbox) { addArea(occupied, bbox); });

  std::vector<GCellDensity> densities;
  densities.reserve(count_x * count_y);
  for (int gy = y_begin; gy <= y_end; gy++) {
    for (int gx = x_begin; gx <= x_end; gx++) {
      const int i = ((gy - y_begin) * count_x) + (gx - x_begin);
      densities.push_back({.gcell = gcellRect(gx, gy),
                           .density = areaDensity(occupied[i], placeable[i])});
    }
  }
  return densities;
}

odb::Rect Opendp::getGCellRegion(const odb::Point& pt, const int radius) const
{
  odb::dbBlock* block = densityBlock();
  if (radius < 0) {
    logger_->error(DPL, 1404, "GCell radius {} is negative.", radius);
  }

  std::vector<int> x_edges;
  std::vector<int> y_edges;
  getGCellEdges(x_edges, y_edges);

  const odb::Rect grid_extent(
      x_edges.front(), y_edges.front(), x_edges.back(), y_edges.back());
  if (!grid_extent.intersects(pt)) {
    logger_->error(DPL,
                   1403,
                   "Point ({:.3f}, {:.3f}) is outside the GCell grid.",
                   block->dbuToMicrons(pt.x()),
                   block->dbuToMicrons(pt.y()));
  }

  const int count_x = static_cast<int>(x_edges.size()) - 1;
  const int count_y = static_cast<int>(y_edges.size()) - 1;
  const int gx = edgeIndex(x_edges, pt.x());
  const int gy = edgeIndex(y_edges, pt.y());

  const odb::Rect window(x_edges[std::max(0, gx - radius)],
                         y_edges[std::max(0, gy - radius)],
                         x_edges[std::min(count_x, gx + radius + 1)],
                         y_edges[std::min(count_y, gy + radius + 1)]);

  // Only the core holds placement sites, so the part of the window outside
  // it would count instances sitting in the margin against no site at all.
  // Rect's constructor normalizes, turning a window that misses the core
  // into a plausible looking rect, so check before clipping.
  if (!window.overlaps(core_)) {
    return {};
  }
  return window.intersect(core_);
}

void Opendp::reportGCellDensity(const odb::Point& pt, const int radius) const
{
  odb::dbBlock* block = densityBlock();
  const odb::Rect region = getGCellRegion(pt, radius);

  if (region.dx() <= 0 || region.dy() <= 0) {
    logger_->report(
        "GCell region around ({:.3f}, {:.3f}) with radius {} holds no core "
        "area.",
        block->dbuToMicrons(pt.x()),
        block->dbuToMicrons(pt.y()),
        radius);
    return;
  }

  logger_->report(
      "GCell region ({:.3f}, {:.3f}) ({:.3f}, {:.3f}) placement density {:.3f}",
      block->dbuToMicrons(region.xMin()),
      block->dbuToMicrons(region.yMin()),
      block->dbuToMicrons(region.xMax()),
      block->dbuToMicrons(region.yMax()),
      getPlacementDensity(region));
}

void Opendp::reportPlacementDensity(const odb::Rect& region) const
{
  odb::dbBlock* block = densityBlock();
  if (region.dx() <= 0 || region.dy() <= 0) {
    logger_->error(DPL,
                   1405,
                   "Region ({:.3f}, {:.3f}) ({:.3f}, {:.3f}) is empty.",
                   block->dbuToMicrons(region.xMin()),
                   block->dbuToMicrons(region.yMin()),
                   block->dbuToMicrons(region.xMax()),
                   block->dbuToMicrons(region.yMax()));
  }

  // Separated out so that an empty region reads as empty rather than as the
  // 1.0 a region with nowhere to put a cell reports.
  if (placeableArea(region) <= 0) {
    logger_->report(
        "Region ({:.3f}, {:.3f}) ({:.3f}, {:.3f}) holds no "
        "placement site.",
        block->dbuToMicrons(region.xMin()),
        block->dbuToMicrons(region.yMin()),
        block->dbuToMicrons(region.xMax()),
        block->dbuToMicrons(region.yMax()));
    return;
  }

  logger_->report(
      "Region ({:.3f}, {:.3f}) ({:.3f}, {:.3f}) placement density {:.3f}",
      block->dbuToMicrons(region.xMin()),
      block->dbuToMicrons(region.yMin()),
      block->dbuToMicrons(region.xMax()),
      block->dbuToMicrons(region.yMax()),
      getPlacementDensity(region));
}

}  // namespace dpl
