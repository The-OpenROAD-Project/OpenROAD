// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

// Placement density queries.  Density is the instance area inside a region
// over the legal placement site area inside it, so it says how much room is
// left there for one more cell.  Nothing is cached: every call measures the
// placement as it currently stands.

#include <algorithm>
#include <cstdint>
#include <functional>

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

bool Opendp::hasPlacementSite(const odb::Rect& region) const
{
  densityBlock();
  return placeableArea(region) > 0;
}

void Opendp::reportPlacementDensity(const odb::Rect& region) const
{
  odb::dbBlock* block = densityBlock();
  if (region.dx() <= 0 || region.dy() <= 0) {
    logger_->error(DPL,
                   1401,
                   "Region ({:.3f}, {:.3f}) ({:.3f}, {:.3f}) is empty.",
                   block->dbuToMicrons(region.xMin()),
                   block->dbuToMicrons(region.yMin()),
                   block->dbuToMicrons(region.xMax()),
                   block->dbuToMicrons(region.yMax()));
  }

  // Called out separately so that a region with nowhere to put a cell reads
  // as such rather than as the 1.0 it otherwise reports.
  if (!hasPlacementSite(region)) {
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
