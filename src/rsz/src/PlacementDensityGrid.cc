// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "rsz/PlacementDensityGrid.hh"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace rsz {

using odb::dbBlock;
using odb::dbInst;
using odb::dbIntProperty;
using odb::dbMaster;
using odb::dbPlacementStatus;
using odb::Rect;

namespace {

// The heat map's bin size before global placement has run.
constexpr int kDefaultBinMicrons = 10;
// The heat map coarsens its default bins to keep within this many a side.
constexpr int64_t kMaxDefaultBinsPerSide = 1024;
// Bounds the bin count however fine a bin size is asked for.
constexpr int64_t kMaxBins = int64_t{1} << 24;

// What the heat map counts with its default settings: placed instances
// other than fillers, pads and covers.
bool contributesToDensity(dbInst* inst, const dbPlacementStatus& status)
{
  if (!status.isPlaced()) {
    return false;
  }
  dbMaster* master = inst->getMaster();
  return !master->isFiller() && !master->isPad() && !master->isCover();
}

// A bin size, in DBU, that global placement published on block, or 0.
int64_t globalPlacementBinSize(dbBlock* block, const char* name)
{
  dbIntProperty* prop = dbIntProperty::find(block, name);
  return prop != nullptr ? prop->getValue() : 0;
}

}  // namespace

PlacementDensityGrid::PlacementDensityGrid(dbBlock* block,
                                           const int bin_width,
                                           const int bin_height)
    : block_(block),
      requested_bin_width_(bin_width),
      requested_bin_height_(bin_height)
{
  addOwner(block_);
}

PlacementDensity PlacementDensityGrid::getDensity(const Rect& region)
{
  if (stale_) {
    build();
    stale_ = false;
  }

  PlacementDensity density;
  if (occupied_area_.empty() || !region.overlaps(core_)) {
    return density;
  }
  const Rect inside = region.intersect(core_);
  density.region_area = inside.area();
  if (density.region_area == 0) {
    return density;
  }

  // Only the bins on the edges of the region can be partly inside it.
  const BinRange range = binsCovering(inside);
  const auto x_share = [&](const int x) {
    const Rect bin = getBinRect(x, range.y_lo);
    return static_cast<double>(std::min(bin.xMax(), inside.xMax())
                               - std::max(bin.xMin(), inside.xMin()))
           / bin.dx();
  };
  const auto y_share = [&](const int y) {
    const Rect bin = getBinRect(range.x_lo, y);
    return static_cast<double>(std::min(bin.yMax(), inside.yMax())
                               - std::max(bin.yMin(), inside.yMin()))
           / bin.dy();
  };
  const double lo_x_share = x_share(range.x_lo);
  const double hi_x_share = x_share(range.x_hi);

  double area = 0;
  for (int y = range.y_lo; y <= range.y_hi; y++) {
    const int64_t* row = &occupied_area_[binIndex(0, y)];
    double row_area = lo_x_share * static_cast<double>(row[range.x_lo]);
    if (range.x_hi > range.x_lo) {
      int64_t whole = 0;
      for (int x = range.x_lo + 1; x < range.x_hi; x++) {
        whole += row[x];
      }
      row_area += static_cast<double>(whole)
                  + (hi_x_share * static_cast<double>(row[range.x_hi]));
    }
    const bool edge = y == range.y_lo || y == range.y_hi;
    area += edge ? y_share(y) * row_area : row_area;
  }
  density.instance_area = area;
  return density;
}

Rect PlacementDensityGrid::getBinRect(const int x, const int y) const
{
  const int64_t lx = core_.xMin() + (static_cast<int64_t>(x) * bin_width_);
  const int64_t ly = core_.yMin() + (static_cast<int64_t>(y) * bin_height_);
  const int64_t ux = std::min<int64_t>(lx + bin_width_, core_.xMax());
  const int64_t uy = std::min<int64_t>(ly + bin_height_, core_.yMax());
  return Rect(static_cast<int>(lx),
              static_cast<int>(ly),
              static_cast<int>(ux),
              static_cast<int>(uy));
}

PlacementDensityGrid::BinRange PlacementDensityGrid::binsCovering(
    const Rect& rect) const
{
  // The upper edges are exclusive: a rect ending on a bin boundary does not
  // reach into the next bin.
  return {.x_lo = (rect.xMin() - core_.xMin()) / bin_width_,
          .y_lo = (rect.yMin() - core_.yMin()) / bin_height_,
          .x_hi = (rect.xMax() - 1 - core_.xMin()) / bin_width_,
          .y_hi = (rect.yMax() - 1 - core_.yMin()) / bin_height_};
}

void PlacementDensityGrid::build()
{
  core_ = block_->getCoreArea();
  bin_count_x_ = 0;
  bin_count_y_ = 0;
  occupied_area_.clear();
  inst_boxes_.clear();
  if (core_.dx() <= 0 || core_.dy() <= 0) {
    return;
  }

  int64_t width = 0;
  int64_t height = 0;
  pickBinSize(width, height);
  bin_width_ = static_cast<int>(width);
  bin_height_ = static_cast<int>(height);
  bin_count_x_ = static_cast<int>((core_.dx() + width - 1) / width);
  bin_count_y_ = static_cast<int>((core_.dy() + height - 1) / height);
  occupied_area_.assign(static_cast<size_t>(bin_count_x_) * bin_count_y_, 0);

  // Room for every instance id, and some to spare for the ones to come.
  const size_t inst_count = block_->getInsts().size();
  inst_boxes_.reserve(inst_count + (inst_count / 8) + 1);
  for (dbInst* inst : block_->getInsts()) {
    updateInst(inst, inst->getPlacementStatus());
  }
}

void PlacementDensityGrid::pickBinSize(int64_t& width, int64_t& height) const
{
  width = requested_bin_width_;
  height = requested_bin_height_;
  if (width <= 0 || height <= 0) {
    // As the heat map: the bins global placement last optimized on, or 10
    // microns before it has run.
    width = globalPlacementBinSize(block_, "gpl_bin_size_x");
    height = globalPlacementBinSize(block_, "gpl_bin_size_y");
    if (width <= 0 || height <= 0) {
      width = int64_t{kDefaultBinMicrons} * block_->getDbUnitsPerMicron();
      height = width;
    }
    while (core_.dx() > kMaxDefaultBinsPerSide * width
           || core_.dy() > kMaxDefaultBinsPerSide * height) {
      width *= 2;
      height *= 2;
    }
  }

  width = std::clamp<int64_t>(width, 1, core_.dx());
  height = std::clamp<int64_t>(height, 1, core_.dy());
  const auto bin_count = [&] {
    return ((core_.dx() + width - 1) / width)
           * ((core_.dy() + height - 1) / height);
  };
  while (bin_count() > kMaxBins) {
    width = std::min<int64_t>(2 * width, core_.dx());
    height = std::min<int64_t>(2 * height, core_.dy());
  }
}

void PlacementDensityGrid::updateInst(dbInst* inst,
                                      const dbPlacementStatus& status)
{
  if (occupied_area_.empty()) {
    return;
  }
  const uint32_t id = inst->getId();
  // Take back what the instance added, from the box recorded then.
  if (id < inst_boxes_.size()) {
    Rect& added = inst_boxes_[id];
    addBox(added, -1);
    added = Rect();
  }
  if (contributesToDensity(inst, status)) {
    Rect& added = recordFor(id);
    added = inst->getBBox()->getBox();
    addBox(added, 1);
  }
}

void PlacementDensityGrid::addBox(const Rect& box, const int sign)
{
  // Off the core, an instance is recorded but adds nothing.
  if (box.area() == 0 || !box.overlaps(core_)) {
    return;
  }
  const Rect inside = box.intersect(core_);
  const BinRange range = binsCovering(inside);
  for (int y = range.y_lo; y <= range.y_hi; y++) {
    for (int x = range.x_lo; x <= range.x_hi; x++) {
      occupied_area_[binIndex(x, y)]
          += sign * inside.intersect(getBinRect(x, y)).area();
    }
  }
}

Rect& PlacementDensityGrid::recordFor(const uint32_t inst_id)
{
  if (inst_id >= inst_boxes_.size()) {
    if (inst_id >= inst_boxes_.capacity()) {
      // New instances come a few at a time, so grow by an eighth rather than
      // doubling.
      inst_boxes_.reserve(std::max<size_t>(
          inst_id + 1, inst_boxes_.capacity() + (inst_boxes_.capacity() / 8)));
    }
    inst_boxes_.resize(inst_id + 1);
  }
  return inst_boxes_[inst_id];
}

////////////////////////////////////////////////////////////////
// dbBlockCallBackObj.  While stale_ is set, the coming build reads every
// instance afresh, so there is nothing to keep up to date.

void PlacementDensityGrid::inDbInstCreate(dbInst* inst)
{
  if (!stale_) {
    updateInst(inst, inst->getPlacementStatus());
  }
}

void PlacementDensityGrid::inDbInstDestroy(dbInst* inst)
{
  if (!stale_) {
    updateInst(inst, dbPlacementStatus(dbPlacementStatus::NONE));
  }
}

void PlacementDensityGrid::inDbInstPlacementStatusBefore(
    dbInst* inst,
    const dbPlacementStatus& status)
{
  if (!stale_) {
    updateInst(inst, status);
  }
}

void PlacementDensityGrid::inDbInstSwapMasterAfter(dbInst* inst)
{
  if (!stale_) {
    updateInst(inst, inst->getPlacementStatus());
  }
}

void PlacementDensityGrid::inDbPostMoveInst(dbInst* inst)
{
  if (!stale_) {
    updateInst(inst, inst->getPlacementStatus());
  }
}

void PlacementDensityGrid::inDbBlockSetCoreArea(dbBlock*)
{
  stale_ = true;
}

}  // namespace rsz
