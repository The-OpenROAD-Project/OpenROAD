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
class dbBlockage;
class dbInst;
class dbMaster;
class dbPlacementStatus;
class dbRow;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace rsz {

// How much of a region placed instances cover, and how much of it no
// standard cell can use.
struct PlacementDensity
{
  // Area of the region inside the core, in DBU^2.
  int64_t region_area = 0;
  // Instance area inside the region, in DBU^2.  Exact over the bins the
  // region covers whole.  A bin it covers only part of adds that part of
  // its instance area, as if the area were spread evenly over the bin.
  double instance_area = 0;
  // Area inside the region where no standard cell can be placed, in DBU^2;
  // see PlacementDensityGrid.  The area under macros is instance area, so
  // the two do not overlap once the placement is legal.  Shared out over
  // partly covered bins as instance_area is.
  double blocked_area = 0;

  // The share of the region covered, which overlapping instances can take
  // above 1.  As in the heat map, blocked area plays no part.
  double getDensity() const
  {
    return region_area > 0 ? instance_area / static_cast<double>(region_area)
                           : 0.0;
  }
  // The area left for new standard cells.  It goes below 0 where instances
  // overfill the area they can use, or sit in blocked area, as cells can
  // before legalization.
  double getFreeArea() const
  {
    return static_cast<double>(region_area) - blocked_area - instance_area;
  }
};

// Placement density over the core of a block: the core is cut into bins, by
// default the global router's gcells, and each placed instance adds the
// area it covers in each bin.  Instances count as the GUI's placement
// density heat map counts them with its default settings: taps and endcaps
// count; fillers, pads and covers do not.  An instance of a child block
// counts as a whole.
//
// Each bin also holds its blocked area, where no standard cell can be
// placed: off the rows, under placement blockages, and in the halos of
// placed instances.  Soft blockages and soft halos only bind global
// placement, so they do not block; partial blockages block whole, as in
// detailed placement.  The area under a macro, or under an instance with a
// hard halo, is left out, since it is instance area; so with a legal
// placement, no area is both.  The blocked area is counted when the grid is
// built, and an edit to a row, a blockage, a macro or an instance with a
// hard halo has the next query build the grid afresh.  odb has no callback
// for a change to a halo or to a blockage's settings, so the grid does not
// see one until then.
//
// The heat map throws its whole map away whenever an instance changes, and
// rebuilds it on the next read.  This grid is built by ensureBuilt(), and
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
// The queries are const and only read, so several threads can make them at
// once, as when moves are scored in parallel; but not while the db is edited
// or the grid is built.
class PlacementDensityGrid : public odb::dbBlockCallBackObj
{
 public:
  // Bins that are the global router's gcells as the block defines them when
  // the grid is built: squares of block->getGCellTileSize() from the lower
  // left corner of the die.  So the density of a gcell is exact.  The router
  // stretches its last column and row of gcells to the edge of the die, and
  // each of those gcells takes up to two bins, four in the corner.  The tile
  // size needs the block's routing layers and their tracks.
  //
  // Making a grid costs nothing until ensureBuilt() builds it.
  PlacementDensityGrid(odb::dbBlock* block, utl::Logger* logger);
  // Bins of bin_width x bin_height DBU, at least 1, on a grid through
  // origin, which can lie outside the core.
  PlacementDensityGrid(odb::dbBlock* block,
                       utl::Logger* logger,
                       const odb::Point& origin,
                       int bin_width,
                       int bin_height);

  odb::dbBlock* getBlock() const { return block_; }

  // Builds the grid if it is out of date: before the first query, and after
  // an edit that has it build afresh.  Otherwise it costs nothing, so call it
  // after editing the db and before querying.
  void ensureBuilt();

  // Area covered inside region, given in block coordinates.  A query on a
  // grid that is out of date is an error.
  PlacementDensity getDensity(const odb::Rect& region) const;
  // Area covered inside the global router's gcell holding point, so exact.
  // A point off the die counts as in the nearest gcell.  With bins other
  // than gcells, the area covered inside the bin holding point.
  PlacementDensity getDensity(const odb::Point& point) const;

  // The bins as of the last build.
  int getBinWidth() const { return bin_width_; }
  int getBinHeight() const { return bin_height_; }
  int getBinCountX() const { return bin_count_x_; }
  int getBinCountY() const { return bin_count_y_; }
  // Bin (x, y) clipped to the core.
  odb::Rect getBinRect(int x, int y) const;
  int64_t getBinOccupiedArea(int x, int y) const
  {
    return occupied_area_[binIndex(x, y)];
  }
  int64_t getBinBlockedArea(int x, int y) const
  {
    return blocked_area_.empty() ? 0 : blocked_area_[binIndex(x, y)];
  }

  // dbBlockCallBackObj
  void inDbInstCreate(odb::dbInst* inst) override;
  void inDbInstDestroy(odb::dbInst* inst) override;
  void inDbInstPlacementStatusBefore(
      odb::dbInst* inst,
      const odb::dbPlacementStatus& status) override;
  void inDbInstSwapMasterBefore(odb::dbInst* inst,
                                odb::dbMaster* master) override;
  void inDbInstSwapMasterAfter(odb::dbInst* inst) override;
  void inDbPostMoveInst(odb::dbInst* inst) override;
  void inDbBlockageCreate(odb::dbBlockage* blockage) override;
  void inDbBlockageDestroy(odb::dbBlockage* blockage) override;
  void inDbRowCreate(odb::dbRow* row) override;
  void inDbRowDestroy(odb::dbRow* row) override;
  void inDbBlockSetDieArea(odb::dbBlock* block) override;
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
  // Fills blocked_area_, given the hard halos of the placed instances, and
  // the area of the counted ones that is their own, not blocked.
  void buildBlockedArea(const std::vector<odb::Rect>& halos,
                        const std::vector<odb::Rect>& own_areas);

  // Brings the grid up to date after an edit to inst, whose placement
  // status is or is about to be status.  An edit that moves the blocked area
  // has the next query build the grid afresh instead.
  void instEdited(odb::dbInst* inst, const odb::dbPlacementStatus& status);
  // Brings what inst, of master, adds in line with status.
  void updateInst(odb::dbInst* inst,
                  odb::dbMaster* master,
                  const odb::dbPlacementStatus& status);
  // Adds the area box covers in each bin to bins, or takes it away for a
  // sign of -1.
  void addBox(std::vector<int64_t>& bins, const odb::Rect& box, int sign);
  // The sum of bins over region, which must lie in the core and have area.
  // A bin partly inside it adds that share of its value.
  double sumBins(const std::vector<int64_t>& bins,
                 const odb::Rect& region) const;
  odb::Rect& recordFor(uint32_t inst_id);

  size_t binIndex(int x, int y) const
  {
    return (static_cast<size_t>(y) * bin_count_x_) + x;
  }
  // The bins overlapping rect, which must lie in the core and have area.
  BinRange binsCovering(const odb::Rect& rect) const;
  // The router's gcell holding point, or the bin with other bins, not
  // clipped to the core.
  odb::Rect cellHolding(const odb::Point& point) const;

  odb::dbBlock* block_;
  utl::Logger* logger_;
  // The grid of bins asked for; a width of 0 asks for the router's gcells.
  const odb::Point requested_origin_;
  const int requested_bin_width_;
  const int requested_bin_height_;

  // Set until the first build, and by an edit that has the grid build
  // afresh.  ensureBuilt() then builds the bins.
  bool stale_ = true;

  // The die, which the router's gcells cover, and the core.
  odb::Rect die_;
  odb::Rect core_;
  // The lower left corner of bin (0, 0) before it is clipped to the core:
  // the grid point nearest the core's lower left corner from below and from
  // the left.
  odb::Point bin_origin_;
  int bin_width_ = 1;
  int bin_height_ = 1;
  int bin_count_x_ = 0;
  int bin_count_y_ = 0;
  // The instance area inside each bin, row after row.
  std::vector<int64_t> occupied_area_;
  // The blocked area inside each bin, row after row; empty when there is
  // none anywhere.
  std::vector<int64_t> blocked_area_;
  // By dbInst id, the box each instance added to the bins; empty for the
  // instances the grid does not count.
  std::vector<odb::Rect> inst_boxes_;
};

}  // namespace rsz
