// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

// Placement density queries.  Density is the instance area inside a region
// over the legal placement site area inside it, so it says how much room is
// left there for one more cell.  Nothing is cached: every call measures the
// placement as it currently stands.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "dpl/Opendp.h"
#include "infrastructure/Coordinates.h"
#include "infrastructure/Grid.h"
#include "infrastructure/InstanceIndex.h"
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

InstanceIndex::InstanceIndex(odb::dbBlock* block)
{
  extent_ = block->getDieArea();

  // Aim for a handful of instances per bucket: few enough that a small query
  // touches almost nothing, many enough that the per-bucket vector overhead
  // stays well under the instance data itself.
  constexpr int kInstsPerBucket = 8;
  const int inst_count = std::max<int>(1, block->getInsts().size());
  const double target_buckets
      = std::max(1.0, static_cast<double>(inst_count) / kInstsPerBucket);
  const double die_area = static_cast<double>(std::max(1, extent_.dx()))
                          * static_cast<double>(std::max(1, extent_.dy()));
  bucket_size_
      = std::max(1, static_cast<int>(std::sqrt(die_area / target_buckets)));
  count_x_ = std::max(1, ((extent_.dx() + bucket_size_) - 1) / bucket_size_);
  count_y_ = std::max(1, ((extent_.dy() + bucket_size_) - 1) / bucket_size_);
  buckets_.resize(static_cast<size_t>(count_x_) * count_y_);

  for (odb::dbInst* inst : block->getInsts()) {
    insert(inst);
  }

  addOwner(block);
}

int InstanceIndex::bucketIndex(const odb::Rect& bbox) const
{
  if (bbox.dx() > bucket_size_ || bbox.dy() > bucket_size_) {
    return kOversized;
  }
  // Instances can sit outside the die -- pads, or a cell not yet moved into
  // the core -- so clamp rather than reject.  A query clamps the same way,
  // which keeps them reachable from the edge buckets they land in.
  const int bx = std::clamp(
      (bbox.xMin() - extent_.xMin()) / bucket_size_, 0, count_x_ - 1);
  const int by = std::clamp(
      (bbox.yMin() - extent_.yMin()) / bucket_size_, 0, count_y_ - 1);
  return (by * count_x_) + bx;
}

int& InstanceIndex::slotOf(odb::dbInst* inst)
{
  const size_t id = inst->getId();
  if (id >= bucket_of_id_.size()) {
    bucket_of_id_.resize(id + 1, kAbsent);
  }
  return bucket_of_id_[id];
}

void InstanceIndex::insert(odb::dbInst* inst)
{
  int& slot = slotOf(inst);
  if (slot != kAbsent) {
    // Already in, so the callbacks did not pair up.  Take the erase now
    // rather than leave the instance in two buckets.
    erase(inst);
  }
  slot = bucketIndex(inst->getBBox()->getBox());
  if (slot == kOversized) {
    oversized_.push_back(inst);
  } else {
    buckets_[slot].push_back(inst);
  }
}

void InstanceIndex::erase(odb::dbInst* inst)
{
  int& slot = slotOf(inst);
  if (slot == kAbsent) {
    return;
  }
  std::vector<odb::dbInst*>& bucket
      = slot == kOversized ? oversized_ : buckets_[slot];
  const auto it = std::ranges::find(bucket, inst);
  if (it != bucket.end()) {
    // Order carries no meaning, so close the hole with the last entry.
    *it = bucket.back();
    bucket.pop_back();
  }
  slot = kAbsent;
}

void InstanceIndex::visit(
    const odb::Rect& region,
    const std::function<void(odb::dbInst* inst)>& visitor) const
{
  for (odb::dbInst* inst : oversized_) {
    visitor(inst);
  }

  const auto bucketX = [&](const int x) {
    return std::clamp((x - extent_.xMin()) / bucket_size_, 0, count_x_ - 1);
  };
  const auto bucketY = [&](const int y) {
    return std::clamp((y - extent_.yMin()) / bucket_size_, 0, count_y_ - 1);
  };

  // A bucketed instance extends at most one bucket up and to the right of
  // the one holding its lower left corner, so widening the low side by one
  // bucket catches everything that can reach into the region.
  const int x_begin = std::max(0, bucketX(region.xMin()) - 1);
  const int x_end = bucketX(region.xMax());
  const int y_begin = std::max(0, bucketY(region.yMin()) - 1);
  const int y_end = bucketY(region.yMax());

  for (int by = y_begin; by <= y_end; by++) {
    const int row = by * count_x_;
    for (int bx = x_begin; bx <= x_end; bx++) {
      for (odb::dbInst* inst : buckets_[row + bx]) {
        visitor(inst);
      }
    }
  }
}

////////////////////////////////////////////////////////////////

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

void Opendp::resetInstanceIndex()
{
  inst_index_.reset();
}

void Opendp::visitPlacedInstances(
    const odb::Rect& region,
    const std::function<void(const odb::Rect& bbox)>& visitor) const
{
  if (inst_index_ == nullptr) {
    inst_index_ = std::make_unique<InstanceIndex>(block_);
  }

  inst_index_->visit(region, [&visitor](odb::dbInst* inst) {
    if (!inst->getPlacementStatus().isPlaced()) {
      return;
    }
    // The same instances the GUI's placement density heat map counts on its
    // default settings: taps and endcaps in, fillers and IO out.  Macros
    // take their area like any other instance.
    odb::dbMaster* master = inst->getMaster();
    // A filler is removable, so the room it sits on is still room for a new
    // cell.
    if (master->isFiller()) {
      return;
    }
    // Pads and covers sit outside the rows, so they would be charged
    // against a region that has no placement site to hold them.
    if (master->isPad() || master->isCover()) {
      return;
    }
    // Read the location from the db rather than from the dpl network or from
    // anything the index cached, so a query made between optimization steps
    // sees where the cell is now.
    visitor(inst->getBBox()->getBox());
  });
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
  visitPlacedInstances(region, [&](const odb::Rect& bbox) {
    occupied += overlapArea(bbox, region);
  });

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
