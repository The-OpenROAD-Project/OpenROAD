// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "odb/dbBlockCallBackObj.h"
#include "odb/geom.h"

namespace odb {
class dbBlock;
class dbInst;
class dbPlacementStatus;
}  // namespace odb

namespace rsz {

// How much of a region placed instances cover.
struct PlacementDensity
{
  // Area of the region inside the core, in DBU^2.
  int64_t region_area = 0;
  // Instance area inside the region, in DBU^2.  Exact over the bins the
  // region covers whole.  A bin it covers only part of adds that part of
  // its instance area, as if the area were spread evenly over the bin.
  double instance_area = 0;

  // The share of the region covered, which overlapping instances can take
  // above 1.
  double getDensity() const
  {
    return region_area > 0 ? instance_area / static_cast<double>(region_area)
                           : 0.0;
  }
};

// Placement density over the core of a block, counted as the GUI's
// placement density heat map counts it with its default settings: the core
// is cut into bins, and each placed instance adds the area it covers in
// each bin.  Taps and endcaps count; fillers, pads and covers do not, nor do
// placement blockages.  An instance of a child block counts as a whole.
//
// The heat map throws its whole map away whenever an instance changes, and
// rebuilds it on the next read.  This map is built by its first query, and
// odb callbacks then keep the bins current through every instance create,
// destroy, move, resize and placement status change.  So a query only costs
// a walk over the bins it covers, however much changed since the last one.
//
// The box each instance added is recorded, and an update takes back exactly
// that rather than what the db holds by then.  So the bins cannot drift,
// even when an edit reaches the db without a callback (journal undo restores
// an orientation or a placement status that way): that instance is just out
// of date until its next callback.
//
// Not thread safe: queries and db edits must not run concurrently.
class PlacementDensityMap : public odb::dbBlockCallBackObj
{
 public:
  // Bins of bin_width x bin_height DBU from the lower left corner of the
  // core.  A size of 0 takes the heat map's default, picked when the map is
  // built: the bins global placement last optimized on, or 10 microns before
  // it has run, coarsened if need be to at most 1024 bins a side.  The map
  // is built by the first query, so making one costs nothing until then.
  explicit PlacementDensityMap(odb::dbBlock* block,
                               int bin_width = 0,
                               int bin_height = 0);

  odb::dbBlock* getBlock() const { return block_; }

  // Area covered inside region, given in block coordinates.
  PlacementDensity getDensity(const odb::Rect& region);

  // The bins as of the last query.
  int getBinWidth() const { return bin_width_; }
  int getBinHeight() const { return bin_height_; }
  int getBinCountX() const { return bin_count_x_; }
  int getBinCountY() const { return bin_count_y_; }
  odb::Rect getBinRect(int x, int y) const;
  int64_t getBinInstanceArea(int x, int y) const
  {
    return bin_area_[binIndex(x, y)];
  }

  // dbBlockCallBackObj
  void inDbInstCreate(odb::dbInst* inst) override;
  void inDbInstDestroy(odb::dbInst* inst) override;
  void inDbInstPlacementStatusBefore(
      odb::dbInst* inst,
      const odb::dbPlacementStatus& status) override;
  void inDbInstSwapMasterAfter(odb::dbInst* inst) override;
  void inDbPostMoveInst(odb::dbInst* inst) override;
  void inDbBlockSetCoreArea(odb::dbBlock* block) override;

 private:
  // Inclusive bin index ranges.
  struct BinRange
  {
    int x_lo;
    int y_lo;
    int x_hi;
    int y_hi;
  };

  void build();
  void pickBinSize(int64_t& width, int64_t& height) const;

  // Brings what inst adds in line with status, the placement status it has
  // or is about to have.
  void updateInst(odb::dbInst* inst, const odb::dbPlacementStatus& status);
  // Adds the area box covers in each bin, or takes it away for a sign of -1.
  void addBox(const odb::Rect& box, int sign);
  odb::Rect& recordFor(uint32_t inst_id);

  size_t binIndex(int x, int y) const
  {
    return (static_cast<size_t>(y) * bin_count_x_) + x;
  }
  // The bins overlapping rect, which must lie in the core and have area.
  BinRange binsCovering(const odb::Rect& rect) const;

  odb::dbBlock* block_;
  const int requested_bin_width_;
  const int requested_bin_height_;

  // Set until the first query, and by a change to the core.  The next query
  // then builds the bins afresh.
  bool stale_ = true;

  odb::Rect core_;
  int bin_width_ = 1;
  int bin_height_ = 1;
  int bin_count_x_ = 0;
  int bin_count_y_ = 0;
  // The instance area inside each bin, row after row.
  std::vector<int64_t> bin_area_;
  // By dbInst id, the box each instance added to the bins; empty for the
  // instances the map does not count.
  std::vector<odb::Rect> inst_boxes_;
};

}  // namespace rsz
