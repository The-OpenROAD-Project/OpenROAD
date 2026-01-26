// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "detailed_manager.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "PlacementDRC.h"
#include "boost/format.hpp"
#include "boost/tokenizer.hpp"
#include "detailed_orient.h"
#include "infrastructure/Coordinates.h"
#include "infrastructure/architecture.h"
#include "infrastructure/detailed_segment.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "util/journal.h"
#include "util/symmetry.h"
#include "util/utility.h"
#include "utl/Logger.h"

using utl::DPL;

namespace dpl {

// Helper to check if a cell's master allows the orientation required by the row
static bool checkMasterSymmetry(Architecture* arch, const Node* nd, int rowId)
{
  const auto* rowPtr = arch->getRow(rowId);
  const unsigned rowOri = rowPtr->getOrient();

  auto* dbMaster = nd->getDbInst()->getMaster();
  unsigned masterSym = DetailedOrient::getMasterSymmetry(dbMaster);

  using odb::dbOrientType;
  if (rowOri == dbOrientType::R0 || rowOri == dbOrientType::MY) {
    return true;
  }
  if (rowOri == dbOrientType::MX || rowOri == dbOrientType::R180) {
    return (masterSym & Symmetry_X) != 0;
  }
  return false;
}

DetailedMgr::DetailedMgr(Architecture* arch,
                         Network* network,
                         Grid* grid,
                         PlacementDRC* drc_engine)
    : arch_(arch),
      network_(network),
      grid_(grid),
      drc_engine_(drc_engine),
      journal_(grid, this)
{
  singleRowHeight_ = arch_->getRow(0)->getHeight();
  numSingleHeightRows_ = arch_->getNumRows();

  // For random numbers...
  rng_ = std::make_unique<Placer_RNG>();
  rng_->seed(static_cast<unsigned>(1));

  // For limiting displacement...
  int limit = std::max(arch_->getWidth().v, arch_->getHeight().v) << 1;
  maxDispX_ = limit;
  maxDispY_ = limit;

  // For generating a move list...
  moveLimit_ = 100;
  // The purpose of this reverse map is to be able to remove the cell from
  // all segments that it has been placed into.  It only works (i.e., is
  // only up-to-date) if you use the proper routines to add and remove cells
  // to and from segments.
  reverseCellToSegs_.clear();
  reverseCellToSegs_.resize(network_->getNumNodes());

  recordOriginalPositions();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedMgr::~DetailedMgr()
{
  for (auto segment : segments_) {
    delete segment;
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::shuffle(std::vector<Node*>& nodes)
{
  Utility::random_shuffle(nodes.begin(), nodes.end(), rng_.get());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::setSeed(const int seed)
{
  logger_->info(DPL, 401, "Setting random seed to {:d}.", seed);
  rng_->seed(seed);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::setMaxDisplacement(const int x, const int y)
{
  const int limit = std::max(arch_->getWidth().v, arch_->getHeight().v) << 1;
  if (x != 0) {
    maxDispX_ = x * arch_->getRow(0)->getHeight().v;
  }
  maxDispX_ = std::min(maxDispX_, limit);
  if (y != 0) {
    maxDispY_ = y * arch_->getRow(0)->getHeight().v;
  }
  maxDispY_ = std::min(maxDispY_, limit);

  logger_->info(DPL,
                402,
                "Setting maximum displacement {:d} {:d} to "
                "{:d} {:d} units.",
                x,
                y,
                maxDispX_,
                maxDispY_);
}

void DetailedMgr::setDisallowOneSiteGaps(const bool disallowOneSiteGaps)
{
  disallowOneSiteGaps_ = disallowOneSiteGaps;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::internalError(const std::string& msg)
{
  logger_->error(
      DPL, 400, "Detailed improvement internal error: {:s}.", msg.c_str());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::findBlockages(const bool includeRouteBlockages)
{
  // Blockages come from filler, from fixed nodes (possibly with shapes) and
  // from larger macros which are now considered fixed...

  blockages_.clear();

  // Determine the single height segments and blockages.
  blockages_.resize(numSingleHeightRows_);

  for (Node* nd : fixedCells_) {
    if (nd->isTerminal()) {
      continue;
    }
    DbuX xmin = std::max(arch_->getMinX(), nd->getLeft());
    DbuX xmax = std::min(arch_->getMaxX(), nd->getRight());
    const DbuY ymin = std::max(arch_->getMinY(), nd->getBottom());
    const DbuY ymax = std::min(arch_->getMaxY(), nd->getTop());

    // HACK!  So a fixed cell might split a row into multiple
    // segments.  However, I don't take into account the
    // spacing or padding requirements of this cell!  This
    // means I could get an error later on.
    //
    // I don't think this is guaranteed to fix the problem,
    // but I suppose I can grab spacing/padding between this
    // cell and "no other cell" on either the left or the
    // right.  This might solve the problem since it will
    // make the blockage wider.
    int pad_left = arch_->getCellSpacing(nullptr, nd);
    int pad_right = arch_->getCellSpacing(nd, nullptr);

    for (int r = 0; r < numSingleHeightRows_; r++) {
      const DbuY yb = arch_->getRow(r)->getBottom();
      const DbuY yt = arch_->getRow(r)->getTop();

      if (ymin < yt && ymax > yb) {
        blockages_[r].emplace_back(xmin,
                                   xmax,
                                   DbuX{pad_left},
                                   DbuX{pad_right},
                                   BlockageType::FixedInstance);
      }
    }
  }

  for (int i = 0; i < network_->getNumBlockages(); i++) {
    const odb::Rect& blockage = network_->getBlockage(i);
    const DbuX xmin = std::max(arch_->getMinX(), DbuX{blockage.xMin()});
    const DbuX xmax = std::min(arch_->getMaxX(), DbuX{blockage.xMax()});
    const DbuY ymin = std::max(arch_->getMinY(), DbuY{blockage.yMin()});
    const DbuY ymax = std::min(arch_->getMaxY(), DbuY{blockage.yMax()});

    for (int r = 0; r < numSingleHeightRows_; r++) {
      const DbuY yb = arch_->getRow(r)->getBottom();
      const DbuY yt = arch_->getRow(r)->getTop();

      if (ymin < yt && ymax > yb) {
        blockages_[r].emplace_back(
            xmin, xmax, DbuX{0}, DbuX{0}, BlockageType::Placement);
      }
    }
  }

  // Sort blockages and merge.
  for (int r = 0; r < numSingleHeightRows_; r++) {
    auto& blockages = blockages_[r];
    if (blockages.empty()) {
      continue;
    }

    std::ranges::sort(blockages, compareBlockages());

    std::stack<Blockage> s;
    s.push(blockages[0]);
    for (int i = 1; i < blockages.size(); i++) {
      Blockage top = s.top();  // copy.
      if (top.getPaddedXMax() < blockages[i].getPaddedXMin()) {
        s.push(blockages[i]);  // new interval.
      } else {
        if (top.getPaddedXMax() < blockages[i].getPaddedXMax()) {
          top.pad_right = blockages[i].pad_right;
          top.x_max = blockages[i].getXMax();  // extend interval.
        }
        s.pop();      // remove old.
        s.push(top);  // expanded interval.
      }
    }

    blockages.clear();
    while (!s.empty()) {
      blockages.push_back(s.top());
      s.pop();
    }

    // Intervals need to be sorted, but they are currently in reverse order. Can
    // either resort or reverse.
    std::ranges::sort(blockages,
                      compareBlockages());  // Sort to get them left to right.
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::findSegments()
{
  // Create the segments into which movable cells are placed.  I do make
  // segment ends line up with sites and that segments don't extend off
  // the chip.
  auto core = grid_->getCore();
  logger_->info(DPL,
                322,
                "Image ({:d}, {:d}) - ({:d}, {:d})",
                arch_->getMinX().v + core.xMin(),
                arch_->getMinY().v + core.yMin(),
                arch_->getMaxX().v + core.xMin(),
                arch_->getMaxY().v + core.yMin());

  for (auto segment : segments_) {
    delete segment;
  }
  segments_.clear();

  int numSegments = 0;
  segsInRow_.resize(numSingleHeightRows_);
  for (int r = 0; r < numSingleHeightRows_; r++) {
    const DbuX lx = arch_->getRow(r)->getLeft();
    const DbuX rx = arch_->getRow(r)->getRight();

    segsInRow_[r] = std::vector<DetailedSeg*>();

    const int n = (int) blockages_[r].size();
    if (n == 0) {
      // Entire row free.

      const DbuX x1 = std::max(arch_->getMinX(), lx);
      const DbuX x2 = std::min(arch_->getMaxX(), rx);

      if (x2 > x1) {
        auto segment = new DetailedSeg();
        segment->setSegId(numSegments);
        segment->setRowId(r);
        segment->setMinX(x1);
        segment->setMaxX(x2);

        segsInRow_[r].push_back(segment);
        segments_.push_back(segment);

        ++numSegments;
      }
    } else {
      // Divide row.
      if (blockages_[r][0].getPaddedXMin() > std::max(arch_->getMinX(), lx)) {
        DbuX x1 = std::max(arch_->getMinX(), lx);
        DbuX x2 = std::min(std::min(arch_->getMaxX(), rx),
                           blockages_[r][0].getPaddedXMin());

        if (x2 > x1) {
          auto segment = new DetailedSeg();
          segment->setSegId(numSegments);
          segment->setRowId(r);
          segment->setMinX(x1);
          segment->setMaxX(x2);

          segsInRow_[r].push_back(segment);
          segments_.push_back(segment);

          ++numSegments;
        }
      }
      for (int i = 1; i < n; i++) {
        if (blockages_[r][i].getPaddedXMin()
            > blockages_[r][i - 1].getPaddedXMax()) {
          DbuX x1 = std::max(std::max(arch_->getMinX(), lx),
                             blockages_[r][i - 1].getPaddedXMax());
          DbuX x2 = std::min(std::min(arch_->getMaxX(), rx),
                             blockages_[r][i].getPaddedXMin());

          if (x2 > x1) {
            auto segment = new DetailedSeg();
            segment->setSegId(numSegments);
            segment->setRowId(r);
            segment->setMinX(x1);
            segment->setMaxX(x2);

            segsInRow_[r].push_back(segment);
            segments_.push_back(segment);

            ++numSegments;
          }
        }
      }
      if (blockages_[r][n - 1].getPaddedXMax()
          < std::min(arch_->getMaxX(), rx)) {
        DbuX x1 = std::min(std::min(arch_->getMaxX(), rx),
                           std::max(std::max(arch_->getMinX(), lx),
                                    blockages_[r][n - 1].getPaddedXMax()));
        DbuX x2 = std::min(arch_->getMaxX(), rx);

        if (x2 > x1) {
          auto segment = new DetailedSeg();
          segment->setSegId(numSegments);
          segment->setRowId(r);
          segment->setMinX(x1);
          segment->setMaxX(x2);

          segsInRow_[r].push_back(segment);
          segments_.push_back(segment);

          ++numSegments;
        }
      }
    }
  }

  // Here, we need to slice up the segments to account for regions.
  std::vector<std::vector<std::pair<DbuX, DbuX>>> intervals;
  for (int reg = 0; reg < arch_->getNumRegions(); reg++) {
    auto regPtr = arch_->getRegion(reg);

    findRegionIntervals(regPtr->getId(), intervals);

    for (int r = 0; r < numSingleHeightRows_; r++) {
      int n = (int) intervals[r].size();
      if (n == 0) {
        continue;
      }

      // Since the intervals do not overlap, I think the following is fine:
      // Pick an interval and pick a segment.  If the interval and segment
      // do not overlap, do nothing.  If the segment and the interval do
      // overlap, then there are cases.  Let <sl,sr> be the span of the
      // segment.  Let <il,ir> be the span of the interval.  Then:
      //
      // Case 1: il <= sl && ir >= sr: The interval entirely overlaps the
      //         segment.  So, we can simply change the segment's region
      //         type.
      // Case 2: il  > sl && ir >= sr: The segment needs to be split into
      //         two segments.  The left segment remains retains it's
      //         original type while the right segment is new and assigned
      //         to the region type.
      // Case 3: il <= sl && ir  < sr: Switch the meaning of left and right
      //         per case 2.
      // Case 4: il  > sl && ir  < sr: The original segment needs to be
      //         split into 2 with the original region type.  A new segment
      //         needs to be created with the new region type.

      for (size_t i = 0; i < intervals[r].size(); i++) {
        DbuX il = intervals[r][i].first;
        DbuX ir = intervals[r][i].second;
        for (size_t s = 0; s < segsInRow_[r].size(); s++) {
          DetailedSeg* segPtr = segsInRow_[r][s];

          DbuX sl = segPtr->getMinX();
          DbuX sr = segPtr->getMaxX();

          // Check for no overlap.
          if (ir <= sl) {
            continue;
          }
          if (il >= sr) {
            continue;
          }

          // Case 1:
          if (il <= sl && ir >= sr) {
            segPtr->setRegId(reg);
          }
          // Case 2:
          else if (il > sl && ir >= sr) {
            segPtr->setMaxX(il);

            auto newPtr = new DetailedSeg();
            newPtr->setSegId(numSegments);
            newPtr->setRowId(r);
            newPtr->setRegId(reg);
            newPtr->setMinX(il);
            newPtr->setMaxX(sr);

            segsInRow_[r].push_back(newPtr);
            segments_.push_back(newPtr);

            ++numSegments;
          }
          // Case 3:
          else if (ir < sr && il <= sl) {
            segPtr->setMinX(ir);

            auto newPtr = new DetailedSeg();
            newPtr->setSegId(numSegments);
            newPtr->setRowId(r);
            newPtr->setRegId(reg);
            newPtr->setMinX(sl);
            newPtr->setMaxX(ir);

            segsInRow_[r].push_back(newPtr);
            segments_.push_back(newPtr);

            ++numSegments;
          }
          // Case 4:
          else if (il > sl && ir < sr) {
            segPtr->setMaxX(il);

            auto newPtr = new DetailedSeg();
            newPtr->setSegId(numSegments);
            newPtr->setRowId(r);
            newPtr->setRegId(reg);
            newPtr->setMinX(il);
            newPtr->setMaxX(ir);

            segsInRow_[r].push_back(newPtr);
            segments_.push_back(newPtr);

            ++numSegments;

            newPtr = new DetailedSeg();
            newPtr->setSegId(numSegments);
            newPtr->setRowId(r);
            newPtr->setRegId(segPtr->getRegId());
            newPtr->setMinX(ir);
            newPtr->setMaxX(sr);

            segsInRow_[r].push_back(newPtr);
            segments_.push_back(newPtr);

            ++numSegments;
          } else {
            internalError("Unexpected problem while constructing segments");
          }
        }
      }
    }
  }

  // Make sure segment boundaries line up with sites.
  for (auto segment : segments_) {
    int rowId = segment->getRowId();

    DbuX originX = arch_->getRow(rowId)->getLeft();
    DbuX siteSpacing = arch_->getRow(rowId)->getSiteSpacing();

    int ix = ((segment->getMinX() - originX) / siteSpacing).v;
    if (originX + ix * siteSpacing.v < segment->getMinX()) {
      ++ix;
    }

    if (originX + ix * siteSpacing.v != segment->getMinX()) {
      segment->setMinX(originX + ix * siteSpacing);
    }

    ix = ((segment->getMaxX() - originX) / siteSpacing).v;
    if (originX + ix * siteSpacing.v != segment->getMaxX()) {
      segment->setMaxX(originX + ix * siteSpacing);
    }
  }

  // Create the structure for cells in segments.
  cellsInSeg_.clear();
  cellsInSeg_.resize(segments_.size());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedSeg* DetailedMgr::findClosestSegment(const Node* nd)
{
  // Find the closest segment for the cell which is wide enough to
  // accommodate the cell.

  // Guess at the closest row.  Assumes rows are stacked.
  const int row = arch_->find_closest_row(nd->getBottom());

  int dist1 = std::numeric_limits<int>::max();
  int dist2 = std::numeric_limits<int>::max();
  DetailedSeg* best1 = nullptr;  // closest segment...
  // closest segment which is wide enough to accomodate the cell...
  DetailedSeg* best2 = nullptr;

  // Segments in the current row...
  for (DetailedSeg* curr : segsInRow_[row]) {
    // Updated for regions.
    if (nd->getGroupId() != curr->getRegId()) {
      continue;
    }
    if (!checkMasterSymmetry(arch_, nd, curr->getRowId())) {
      continue;
    }

    // Work with left edge.
    const DbuX x1 = curr->getMinX();
    const DbuX x2 = curr->getMaxX() - nd->getWidth();
    const DbuX xx = std::max(x1, std::min(x2, nd->getLeft()));

    const DbuX hori = std::max(DbuX{0}, abs(xx - nd->getLeft()));

    const bool closer1 = hori < dist1;
    const bool closer2 = hori < dist2;
    const bool fits = nd->getWidth() <= (curr->getMaxX() - curr->getMinX());

    // Keep track of the closest segment.
    if (best1 == nullptr || (best1 != nullptr && closer1)) {
      best1 = curr;
      dist1 = hori.v;
    }
    // Keep track of the closest segment which is wide enough to accomodate the
    // cell.
    if (fits && (best2 == nullptr || (best2 != nullptr && closer2))) {
      best2 = curr;
      dist2 = hori.v;
    }
  }

  // Consider rows above and below the current row.
  for (int offset = 1; offset <= numSingleHeightRows_; offset++) {
    const int below = row - offset;
    DbuY vert = offset * singleRowHeight_;

    if (below >= 0) {
      // Consider the row if we could improve on either of the best segments we
      // are recording.
      if ((vert.v <= dist1 || vert.v <= dist2)) {
        for (DetailedSeg* curr : segsInRow_[below]) {
          // Updated for regions.
          if (nd->getGroupId() != curr->getRegId()) {
            continue;
          }
          if (!checkMasterSymmetry(arch_, nd, curr->getRowId())) {
            continue;
          }

          // Work with left edge.
          const DbuX x1 = curr->getMinX();
          const DbuX x2 = curr->getMaxX() - nd->getWidth();
          const DbuX xx = std::max(x1, std::min(x2, nd->getLeft()));

          const DbuX hori = std::max(DbuX{0}, abs(xx - nd->getLeft()));

          const bool closer1 = hori.v + vert.v < dist1;
          const bool closer2 = hori.v + vert.v < dist2;
          const bool fits
              = nd->getWidth() <= (curr->getMaxX() - curr->getMinX());

          // Keep track of the closest segment.
          if (best1 == nullptr || (best1 != nullptr && closer1)) {
            best1 = curr;
            dist1 = hori.v + vert.v;
          }
          // Keep track of the closest segment which is wide enough to
          // accomodate the cell.
          if (fits && (best2 == nullptr || (best2 != nullptr && closer2))) {
            best2 = curr;
            dist2 = hori.v + vert.v;
          }
        }
      }
    }

    const int above = row + offset;
    vert = offset * singleRowHeight_;

    if (above <= numSingleHeightRows_ - 1) {
      // Consider the row if we could improve on either of the best segments we
      // are recording.
      if ((vert <= dist1 || vert <= dist2)) {
        for (DetailedSeg* curr : segsInRow_[above]) {
          // Updated for regions.
          if (nd->getGroupId() != curr->getRegId()) {
            continue;
          }
          if (!checkMasterSymmetry(arch_, nd, curr->getRowId())) {
            continue;
          }

          // Work with left edge.
          const DbuX x1 = curr->getMinX();
          const DbuX x2 = curr->getMaxX() - nd->getWidth();
          const DbuX xx = std::max(x1, std::min(x2, nd->getLeft()));

          const DbuX hori = std::max(DbuX{0}, abs(xx - nd->getLeft()));

          const bool closer1 = hori.v + vert.v < dist1;
          const bool closer2 = hori.v + vert.v < dist2;
          const bool fits
              = nd->getWidth() <= (curr->getMaxX() - curr->getMinX());

          // Keep track of the closest segment.
          if (best1 == nullptr || (best1 != nullptr && closer1)) {
            best1 = curr;
            dist1 = hori.v + vert.v;
          }
          // Keep track of the closest segment which is wide enough to
          // accomodate the cell.
          if (fits && (best2 == nullptr || (best2 != nullptr && closer2))) {
            best2 = curr;
            dist2 = hori.v + vert.v;
          }
        }
      }
    }
  }

  return (best2) ? best2 : best1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::findClosestSpanOfSegmentsDfs(
    const Node* ndi,
    DetailedSeg* segPtr,
    const DbuX xmin,
    const DbuX xmax,
    const int bot,
    const int top,
    std::vector<DetailedSeg*>& stack,
    std::vector<std::vector<DetailedSeg*>>& candidates

)
{
  stack.push_back(segPtr);
  const int rowId = segPtr->getRowId();

  if (rowId < top) {
    for (DetailedSeg* seg : segsInRow_[rowId + 1]) {
      const DbuX overlap
          = std::min(xmax, seg->getMaxX()) - std::max(xmin, seg->getMinX());
      if (overlap > 0) {
        // Must find the reduced X-interval.
        const DbuX xl = std::max(xmin, seg->getMinX());
        const DbuX xr = std::min(xmax, seg->getMaxX());
        findClosestSpanOfSegmentsDfs(
            ndi, seg, xl, xr, bot, top, stack, candidates);
      }
    }
  } else {
    // Reaching this point should imply that we have a consecutive set of
    // segments which is potentially valid for placing the cell.
    const int spanned = top - bot + 1;
    if (stack.size() != spanned) {
      internalError("Multi-height cell spans an incorrect number of segments");
    }
    candidates.push_back(stack);
  }
  stack.pop_back();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::findClosestSpanOfSegments(Node* nd,
                                            std::vector<DetailedSeg*>& segments)
{
  // Intended for multi-height cells...  Finds the number of rows the cell
  // spans and then attempts to find a vector of segments (in different
  // rows) into which the cell can be assigned.

  const int spanned = arch_->getCellHeightInRows(nd);
  if (spanned <= 1) {
    return false;
  }

  double disp1 = std::numeric_limits<double>::max();
  double disp2 = std::numeric_limits<double>::max();

  std::vector<std::vector<DetailedSeg*>> candidates;
  std::vector<DetailedSeg*> stack;

  std::vector<DetailedSeg*> best1;  // closest.
  std::vector<DetailedSeg*> best2;  // closest that fits.

  // The efficiency of this is not good.  The information about overlapping
  // segments for multi-height cells could easily be precomputed for efficiency.
  bool flip = false;
  for (int r = 0; r < arch_->getNumRows(); r++) {
    // XXX: NEW! Check power compatibility of this cell with the row.  A
    // call to this routine will check both the bottom and the top rows
    // for power compatibility.
    if (!arch_->powerCompatible(nd, arch_->getRow(r), flip)) {
      continue;
    }

    if (!checkMasterSymmetry(arch_, nd, r)) {
      continue;
    }

    // Scan the segments in this row and look for segments in the required
    // number of rows above and below that result in non-zero interval.
    const int b = r;
    const int t = r + spanned - 1;
    if (t >= arch_->getRows().size()) {
      continue;
    }

    for (DetailedSeg* segPtr : segsInRow_[b]) {
      candidates.clear();
      stack.clear();

      findClosestSpanOfSegmentsDfs(nd,
                                   segPtr,
                                   segPtr->getMinX(),
                                   segPtr->getMaxX(),
                                   b,
                                   t,
                                   stack,
                                   candidates);
      if (candidates.empty()) {
        continue;
      }

      // Evaluate the candidate segments.  Determine the distance of the bottom
      // of the node to the bottom of the first segment.  Determine the overlap
      // in the interval in the X-direction and determine the required distance.

      for (auto& candidates_i : candidates) {
        // NEW: All of the segments must have the same region ID and that region
        // ID must be the same as the region ID of the cell.  If not, then we
        // are going to violate a fence region constraint.
        bool regionsOkay = true;
        for (DetailedSeg* segPtr : candidates_i) {
          if (segPtr->getRegId() != nd->getGroupId()) {
            regionsOkay = false;
          }
        }

        // XXX: Should region constraints be hard or soft?  If hard, there is
        // more change for failure!
        if (!regionsOkay) {
          continue;
        }

        DetailedSeg* segPtr = candidates_i[0];

        // Work with bottom edge.
        const DbuY ymin = arch_->getRow(segPtr->getRowId())->getBottom();
        const DbuY dy = abs(nd->getBottom() - ymin);

        DbuX xmin = segPtr->getMinX();
        DbuX xmax = segPtr->getMaxX();
        for (size_t j = 1; j < candidates_i.size(); j++) {
          segPtr = candidates_i[j];
          xmin = std::max(xmin, segPtr->getMinX());
          xmax = std::min(xmax, segPtr->getMaxX());
        }
        const DbuX width = xmax - xmin;

        // Still work with cell center.
        const DbuX ww = std::min(nd->getWidth(), width);
        const DbuX lx = xmin + ww / 2;
        const DbuX rx = xmax - ww / 2;
        const DbuX xc = nd->getCenterX();
        const DbuX xx = std::max(lx, std::min(rx, xc));
        const DbuX dx = abs(xc - xx);

        if (best1.empty() || (dx.v + dy.v < disp1)) {
          if (true) {
            best1 = candidates_i;
            disp1 = dx.v + dy.v;
          }
        }
        if (best2.empty() || (dx.v + dy.v < disp2)) {
          if (nd->getWidth() <= width) {
            best2 = candidates_i;
            disp2 = dx.v + dy.v;
          }
        }
      }
    }
  }

  segments.clear();
  if (!best2.empty()) {
    segments = std::move(best2);
    return true;
  }
  if (!best1.empty()) {
    segments = std::move(best1);
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::assignCellsToSegments(
    const std::vector<Node*>& nodesToConsider)
{
  // For the provided list of cells which are assumed movable, assign those
  // cells to segments.
  //
  // XXX: Multi height cells are assigned to multiple rows!  In other words,
  // a cell can exist in multiple rows.

  // Assign cells to segments.
  int nAssigned = 0;
  DbuX movementX{0};
  DbuY movementY{0};
  for (Node* nd : nodesToConsider) {
    const int nRowsSpanned = arch_->getCellHeightInRows(nd);

    if (nRowsSpanned == 1) {
      // Single height.
      DetailedSeg* segPtr = findClosestSegment(nd);
      if (segPtr == nullptr) {
        internalError("Unable to assign single height cell to segment");
      }

      const int rowId = segPtr->getRowId();
      const int segId = segPtr->getSegId();

      // Add to segment.
      addCellToSegment(nd, segId);
      ++nAssigned;

      // Move the cell's position into the segment.  Use left edge.
      const DbuX x1 = segPtr->getMinX();
      const DbuX x2 = segPtr->getMaxX() - nd->getWidth();
      const DbuX xx = std::max(x1, std::min(x2, nd->getLeft()));
      const DbuY yy = arch_->getRow(rowId)->getBottom();

      movementX += abs(nd->getLeft() - xx);
      movementY += abs(nd->getBottom() - yy);

      nd->setLeft(xx);
      nd->setBottom(yy);
    } else {
      // Multi height.
      std::vector<DetailedSeg*> segments;
      if (!findClosestSpanOfSegments(nd, segments)) {
        internalError("Unable to assign multi-height cell to segment");
      } else {
        if (segments.size() != nRowsSpanned) {
          internalError("Unable to assign multi-height cell to segment");
        }
        // NB: adding a cell to a segment does _not_ change its position.
        DetailedSeg* segPtr = segments[0];
        DbuX xmin = segPtr->getMinX();
        DbuX xmax = segPtr->getMaxX();
        for (auto seg : segments) {
          xmin = std::max(xmin, seg->getMinX());
          xmax = std::min(xmax, seg->getMaxX());
          addCellToSegment(nd, seg->getSegId());
        }
        ++nAssigned;

        segPtr = segments[0];
        const int rowId = segPtr->getRowId();

        // Work with left edge and bottom edge.
        const DbuX x1 = xmin;
        const DbuX x2 = xmax - nd->getWidth();
        const DbuX xx = std::max(x1, std::min(x2, nd->getLeft()));
        const DbuY yy = arch_->getRow(rowId)->getBottom();

        movementX += abs(nd->getLeft() - xx);
        movementY += abs(nd->getBottom() - yy);

        nd->setLeft(xx);
        nd->setBottom(yy);
      }
    }
    paintInGrid(nd);
  }
  // TODO: remove conversion back to double and update ok
  logger_->info(DPL,
                310,
                "Assigned {:d} cells into segments.  Movement in X-direction "
                "is {:f}, movement in Y-direction is {:f}.",
                nAssigned,
                (double) movementX.v,
                (double) movementY.v);
}

bool DetailedMgr::isInsideABlockage(const Node* nd, const DbuX position)
{
  const Architecture::Row* first_row = arch_->getRow(0);
  const DbuY single_height = first_row->getHeight();
  const DbuY rows_origin_y = first_row->getBottom();
  const int start_row
      = std::max(((nd->getBottom() - rows_origin_y) / single_height).v, 0);
  const int end_row
      = std::min(((nd->getTop() - rows_origin_y) / single_height).v,
                 numSingleHeightRows_ - 1);

  for (int r = start_row; r < end_row; r++) {
    auto it = std::lower_bound(
        blockages_[r].begin(),
        blockages_[r].end(),
        Blockage(position, position, DbuX{0}, DbuX{0}, BlockageType::None),
        [](const Blockage& block, const Blockage& target) {
          return block.getXMax() < target.getXMin();
        });

    if (it != blockages_[r].end()
        && (it->isFixedInstance() || it->isPlacement())
        && position >= it->getXMin() && position <= it->getXMax()) {
      return true;
    }
  }

  return false;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeCellFromSegment(const Node* nd, const int seg)
{
  // Removing a node from a segment means a few things...  It means: 1) removing
  // it from the cell list for the segment; 2) removing its width from the
  // segment utilization; 3) updating the required gaps between cells in the
  // segment.

  const DbuX width = nd->getWidth();

  const auto it
      = std::find(cellsInSeg_[seg].begin(), cellsInSeg_[seg].end(), nd);
  if (cellsInSeg_[seg].end() == it) {
    // Should not happen.
    internalError("Cell not found in expected segment");
  }

  // Remove this segment from the reverse map.
  const auto its = std::find(reverseCellToSegs_[nd->getId()].begin(),
                             reverseCellToSegs_[nd->getId()].end(),
                             segments_[seg]);
  if (reverseCellToSegs_[nd->getId()].end() == its) {
    // Should not happen.
    internalError("Cannot find segment for cell");
  }
  reverseCellToSegs_[nd->getId()].erase(its);

  cellsInSeg_[seg].erase(it);        // Removes the cell...
  segments_[seg]->remUtil(width.v);  // Removes the utilization...
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::addCellToSegment(Node* nd, const int seg)
{
  // Adding a node to a segment means a few things...  It means:
  // 1) adding it to the SORTED cell list for the segment;
  // 2) adding its width to the segment utilization;
  // 3) adding the required gaps between cells in the segment.

  // Need to figure out where the cell goes in the sorted list...
  const DbuX width = nd->getWidth();
  const DbuX x = nd->getCenterX();
  const auto it = std::lower_bound(
      cellsInSeg_[seg].begin(), cellsInSeg_[seg].end(), x, compareNodesX());
  if (it == cellsInSeg_[seg].end()) {
    // Cell is at the end of the segment.
    cellsInSeg_[seg].push_back(nd);    // Add the cell...
    segments_[seg]->addUtil(width.v);  // Adds the utilization...
  } else {
    cellsInSeg_[seg].insert(it, nd);   // Adds the cell...
    segments_[seg]->addUtil(width.v);  // Adds the utilization...
  }

  const auto its = std::find(reverseCellToSegs_[nd->getId()].begin(),
                             reverseCellToSegs_[nd->getId()].end(),
                             segments_[seg]);
  if (reverseCellToSegs_[nd->getId()].end() != its) {
    internalError("Segment already present in cell to segment map");
  }
  const int spanned = arch_->getCellHeightInRows(nd);
  if (reverseCellToSegs_[nd->getId()].size() >= spanned) {
    internalError("Cell to segment map incorrectly sized");
  }
  reverseCellToSegs_[nd->getId()].push_back(segments_[seg]);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::recordOriginalPositions()
{
  origBottom_.resize(network_->getNumNodes());
  origLeft_.resize(network_->getNumNodes());
  for (int i = 0; i < network_->getNumNodes(); i++) {
    const Node* nd = network_->getNode(i);
    origBottom_[nd->getId()] = nd->getBottom();
    origLeft_[nd->getId()] = nd->getLeft();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::restoreOriginalPositions()
{
  for (int i = 0; i < network_->getNumNodes(); i++) {
    Node* nd = network_->getNode(i);
    eraseFromGrid(nd);
    nd->setBottom(origBottom_[nd->getId()]);
    nd->setLeft(origLeft_[nd->getId()]);
    paintInGrid(nd);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectSingleHeightCells()
{
  // Routine to collect only the movable single height cells.
  //
  // XXX: This code also shifts cells to ensure that they are within the
  // placement area.  It also lines the cell up with its bottom row by
  // assuming rows are stacked continuously one on top of the other which
  // may or may not be a correct assumption.
  // Do I need to do any of this really?????????????????????????????????

  singleHeightCells_.clear();
  singleRowHeight_ = arch_->getRow(0)->getHeight();
  numSingleHeightRows_ = arch_->getNumRows();

  for (int i = 0; i < network_->getNumNodes(); i++) {
    Node* nd = network_->getNode(i);

    if (nd->isTerminal() || nd->isFixed()) {
      continue;
    }
    if (arch_->isMultiHeightCell(nd)) {
      continue;
    }

    singleHeightCells_.push_back(nd);
  }
  logger_->info(DPL,
                318,
                "Collected {:d} single height cells.",
                singleHeightCells_.size());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectMultiHeightCells()
{
  // Routine to collect only the movable multi height cells.
  //
  // XXX: This code also shifts cells to ensure that they are within the
  // placement area.  It also lines the cell up with its bottom row by
  // assuming rows are stacked continuously one on top of the other which
  // may or may not be a correct assumption.
  // Do I need to do any of this really?????????????????????????????????

  multiHeightCells_.clear();
  // Just in case...  Make the matrix for holding multi-height cells at
  // least large enough to hold single height cells (although we don't
  // even bothering storing such cells in this matrix).
  multiHeightCells_.resize(2);
  singleRowHeight_ = arch_->getRow(0)->getHeight();
  numSingleHeightRows_ = arch_->getNumRows();

  for (int i = 0; i < network_->getNumNodes(); i++) {
    Node* nd = network_->getNode(i);

    if (nd->isTerminal() || nd->isFixed() || arch_->isSingleHeightCell(nd)) {
      continue;
    }

    const int nRowsSpanned = arch_->getCellHeightInRows(nd);

    if (nRowsSpanned >= multiHeightCells_.size()) {
      multiHeightCells_.resize(nRowsSpanned + 1, std::vector<Node*>());
    }
    multiHeightCells_[nRowsSpanned].push_back(nd);
  }
  for (size_t i = 0; i < multiHeightCells_.size(); i++) {
    if (multiHeightCells_[i].empty()) {
      continue;
    }
    logger_->info(DPL,
                  319,
                  "Collected {:d} multi-height cells spanning {:d} rows.",
                  multiHeightCells_[i].size(),
                  i);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectFixedCells()
{
  // Fixed cells are used only to create blockages which, in turn, are used to
  // create obstacles.  Obstacles are then used to create the segments into
  // which cells can be placed.

  fixedCells_.clear();

  // Insert fixed items, shapes AND macrocells.
  for (int i = 0; i < network_->getNumNodes(); i++) {
    Node* nd = network_->getNode(i);

    if (nd->isFixed()) {
      fixedCells_.push_back(nd);
    }
  }

  logger_->info(DPL, 320, "Collected {:d} fixed cells.", fixedCells_.size());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectWideCells()
{
  // This is sort of a hack.  Some standard cells might be extremely wide and
  // based on how we set up segments (e.g., to take into account blockages of
  // different sorts), we might not be able to find a segment wide enough to
  // accomodate the cell.  In this case, we will not be able to resolve a bunch
  // of problems.
  //
  // My current solution is to (1) detect such cells; (2) recreate the segments
  // without blockages; (3) insert the wide cells into segments; (4) fix the
  // wide cells; (5) recreate the entire problem with the wide cells considered
  // as fixed.

  wideCells_.clear();
  for (int s = 0; s < segments_.size(); s++) {
    DetailedSeg* curr = segments_[s];

    const std::vector<Node*>& nodes = cellsInSeg_[s];
    for (Node* ndi : nodes) {
      if (ndi->getWidth() > curr->getMaxX() - curr->getMinX()) {
        wideCells_.push_back(ndi);
      }
    }
  }
  logger_->info(DPL, 321, "Collected {:d} wide cells.", wideCells_.size());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::cleanup()
{
  // Various cleanups.
  for (Node* ndi : wideCells_) {
    ndi->setFixed(false);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkOverlapInSegments()
{
  // Scan segments and check if adjacent cells overlap.  Do not
  // consider spacing or padding in this check.

  std::vector<Node*> temp;
  temp.reserve(network_->getNumNodes());

  int err_n = 0;
  // The following is for some printing if we need help finding a bug.
  // I don't want to print all the potential errors since that could
  // be too overwhelming.
  for (int s = 0; s < segments_.size(); s++) {
    const DbuX xmin = segments_[s]->getMinX();
    const DbuX xmax = segments_[s]->getMaxX();

    // To be safe, gather cells in each segment and re-sort them.
    temp = cellsInSeg_[s];
    std::ranges::sort(temp, compareNodesX());

    for (int j = 1; j < temp.size(); j++) {
      const Node* ndi = temp[j - 1];
      const Node* ndj = temp[j];

      const DbuX ri = ndi->getRight();
      const DbuX lj = ndj->getLeft();

      if (ri > lj) {
        // Overlap.
        err_n++;
      }
    }
    for (Node* ndi : temp) {
      if (ndi->getLeft() < xmin || ndi->getRight() > xmax) {
        // Out of range.
        err_n++;
      }
    }
  }

  logger_->info(DPL, 311, "Found {:d} overlaps between adjacent cells.", err_n);
  return err_n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::hasPlacementViolation(const Node* node) const
{
  return !drc_engine_->checkDRC(node);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkEdgeSpacingInSegments()
{
  // Check for spacing violations according to the spacing table.  Note
  // that there might not be a spacing table in which case we will
  // return no errors.  I should also check for padding errors although
  // we might not have any paddings either! :).

  std::vector<Node*> temp;
  temp.reserve(network_->getNumNodes());

  int dummyPadding = 0;
  int rightPadding = 0;
  int leftPadding = 0;

  int err_n = 0;
  int err_p = 0;
  for (int s = 0; s < segments_.size(); s++) {
    // To be safe, gather cells in each segment and re-sort them.
    temp = cellsInSeg_[s];
    std::ranges::sort(temp, compareNodesL());

    for (int j = 1; j < temp.size(); j++) {
      const Node* ndl = temp[j - 1];
      const Node* ndr = temp[j];

      const DbuX rlx_l = ndl->getRight();
      const DbuX llx_r = ndr->getLeft();

      const DbuX gap = llx_r - rlx_l;

      arch_->getCellPadding(ndl, dummyPadding, rightPadding);
      arch_->getCellPadding(ndr, leftPadding, dummyPadding);
      const int padding = leftPadding + rightPadding;

      if (hasPlacementViolation(ndl)) {
        logger_->report("Violation in {}", ndl->name());
        ++err_n;
      }
      if (gap < padding) {
        ++err_p;
      }
    }
  }

  logger_->info(
      DPL,
      312,
      "Found {:d} edge spacing violations and {:d} padding violations.",
      err_n,
      err_p);

  return err_n + err_p;
}

void DetailedMgr::getOneSiteGapViolationsPerSegment(
    std::vector<std::vector<int>>& violating_cells,
    bool fix_violations)
{
  // the pair is the segment and the cell index in that segment
  violating_cells.resize(segments_.size() + 1);
  for (auto segment : segments_) {
    // To be safe, gather cells in each segment and re-sort them.
    const int s = segment->getSegId();
    if (cellsInSeg_[s].size() < 2) {
      continue;
    }
    resortSegment(segment);
    // The idea here is to get the last X before the current cell,
    // it doesn't have to be the one directly before the current cell as
    // there might be two cells on the same x but different y.
    // if the difference between the last X and the current cell's X is equal to
    // 1-site then we might have a violation. we still need to check the
    // overlaps in the ys
    // to do this efficiently we can simply loop and check for overlaps in the
    // ys

    auto isInRange = [](DbuY value, DbuY min, DbuY max) -> bool {
      return min <= value && value <= max;
    };

    auto isOverlap =
        [&isInRange](DbuY bottom1, DbuY top1, DbuY bottom2, DbuY top2) -> bool {
      return isInRange(bottom1, bottom2, top2)
             || isInRange(bottom2, bottom1, top1);
    };

    Node* lastNode = cellsInSeg_[s][0];
    const DbuX one_site_gap = arch_->getRow(0)->getSiteWidth();
    std::vector<Node*> cellsAtLastX(1, cellsInSeg_[s][0]);

    for (int node_idx = 0; node_idx < cellsInSeg_[s].size(); node_idx++) {
      Node* nd = cellsInSeg_[s][node_idx];
      if (cellsInSeg_[s][node_idx]->getRight() != lastNode->getRight()) {
        // we have a new X
        // check if the difference in x is equal to one-site

        if (abs(nd->getLeft() - lastNode->getRight()) == one_site_gap) {
          // we might have a violation
          // check the ys
          for (auto cell : cellsAtLastX) {
            if (isOverlap(cell->getBottom(),
                          cell->getTop(),
                          nd->getBottom(),
                          nd->getTop())) {
              if (nd->isTerminal() || nd->isFixed()) {
                logger_->warn(DPL,
                              339,
                              "One-site gap violation detected with a "
                              "Fixed/Terminal cell {}",
                              nd->getId());
                violating_cells[s].push_back(nd->getId());
                continue;
              }
              if (arch_->isMultiHeightCell(nd)) {
                // TODO: Can be done
                logger_->warn(DPL,
                              340,
                              "One-site gap violation detected with a "
                              "multi-height cell {}",
                              nd->getId());
                violating_cells[s].push_back(nd->getId());
                continue;
              }
              if (fix_violations) {
                if (!fixOneSiteGapViolations(cell, one_site_gap, s, nd)) {
                  violating_cells[s].push_back(nd->getId());
                }
              } else {
                violating_cells[s].push_back(nd->getId());
              }
              break;
            }
          }
        }
        cellsAtLastX.clear();
      }
      if ((isInsideABlockage(nd, nd->getLeft() - one_site_gap)
           && !isInsideABlockage(nd, nd->getLeft()))
          || (isInsideABlockage(nd, nd->getRight() + one_site_gap)
              && !isInsideABlockage(nd, nd->getRight()))) {
        if (fix_violations) {
          if (!fixOneSiteGapViolations(nd,
                                       one_site_gap,
                                       s,
                                       nd)) {  // TODO: the first nd is wrong,
            // it should be the blockage cell
            violating_cells[s].push_back(nd->getId());
          }
        } else {
          violating_cells[s].push_back(nd->getId());
        }
      }
      cellsAtLastX.push_back(nd);
      lastNode = nd;
    }
  }
}

bool DetailedMgr::fixOneSiteGapViolations(Node* cell,
                                          DbuX one_site_gap,
                                          int segment,
                                          Node* violatingNode)
{
  // we try shifting to the left first
  if (shiftLeftHelper(
          violatingNode,
          violatingNode->getRight()
              - one_site_gap,  // Imagine we insert a cell that is one-site away
                               // from the right-end of the violating node
          segment,
          violatingNode)
      && verifyMove()) {
    acceptMove();
    clearMoveList();
    return true;
  }
  rejectMove();  // shift left failed, undo changes and try shift right
  if (shiftRightHelper(
          cell,
          cell->getLeft()
              + (one_site_gap * 2),  // assume we will insert the cell
                                     // before us at this location
          segment,
          violatingNode)
      && verifyMove()) {
    acceptMove();  // without this, the cells are not moved
    clearMoveList();
    return true;
  }
  rejectMove();
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkRegionAssignment()
{
  // Check cells are assigned (within) their proper regions.  This is sort
  // of a hack/cheat.  We assume that we have set up the segments correctly
  // and that all cells are in segments.  Multi-height cells can be in
  // multiple segments.
  //
  // Therefore, if we scan the segments and the cells have a region ID that
  // matches the region ID for the segment, the cell must be within its
  // region.  Note: This is not true if the cell is somehow outside of its
  // assigned segments.  However, that issue would be caught when checking
  // the segments themselves.

  std::vector<Node*> temp;
  temp.reserve(network_->getNumNodes());

  int err_n = 0;
  for (int s = 0; s < segments_.size(); s++) {
    // To be safe, gather cells in each segment and re-sort them.
    temp = cellsInSeg_[s];
    std::ranges::sort(temp, compareNodesL());

    for (const Node* ndi : temp) {
      if (ndi->getGroupId() != segments_[s]->getRegId()) {
        ++err_n;
      }
    }
  }

  logger_->info(DPL, 313, "Found {:d} cells in wrong regions.", err_n);

  return err_n;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkSiteAlignment()
{
  // Ensure that the left edge of each cell is aligned with a site.  We only
  // consider cells that are within segments.
  int err_n = 0;

  const DbuY singleRowHeight = getSingleRowHeight();
  for (int i = 0; i < network_->getNumNodes(); i++) {
    const Node* nd = network_->getNode(i);

    if (nd->isTerminal() || nd->isFixed()) {
      continue;
    }

    const DbuX xl = nd->getLeft();
    const DbuY yb = nd->getBottom();

    // Determine the spanned rows. XXX: Is this strictly correct?  It
    // assumes rows are continuous and that the bottom row lines up
    // with the bottom of the architecture.
    int rb = ((yb - arch_->getMinY()) / singleRowHeight).v;
    const int spanned = (nd->getHeight() / singleRowHeight).v;
    int rt = rb + spanned - 1;

    if (reverseCellToSegs_[nd->getId()].empty()) {
      continue;
    }
    if (reverseCellToSegs_[nd->getId()].size() != spanned) {
      internalError("Reverse cell map incorrectly sized.");
    }

    if (rb < 0 || rt >= arch_->getRows().size()) {
      // Either off the top of the bottom of the chip, so this is not
      // exactly an alignment problem, but still a problem so count it.
      ++err_n;
    }
    rb = std::max(rb, 0);
    rt = std::min(rt, (int) arch_->getRows().size() - 1);

    for (int r = rb; r <= rt; r++) {
      const DbuX originX = arch_->getRow(r)->getLeft();
      const DbuX siteSpacing = arch_->getRow(r)->getSiteSpacing();

      // XXX: Should I check the site to the left and right to avoid rounding
      // errors???
      const int sid = ((xl - originX) / siteSpacing).v;
      const DbuX xt = originX + sid * siteSpacing;
      if (xl != xt) {
        ++err_n;
      }
    }
  }
  logger_->info(DPL, 314, "Found {:d} site alignment problems.", err_n);
  return err_n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkRowAlignment()
{
  // Ensure that the bottom of each cell is aligned with a row.
  int err_n = 0;

  for (int i = 0; i < network_->getNumNodes(); i++) {
    const Node* nd = network_->getNode(i);

    if (nd->isTerminal() || nd->isFixed()) {
      continue;
    }

    const int rb = arch_->find_closest_row(nd->getBottom());
    const int rt = rb + arch_->getCellHeightInRows(nd) - 1;
    if (rb < 0 || rt >= arch_->getRows().size()) {
      // Apparently, off the bottom or top of hte chip.
      ++err_n;
      continue;
    }
    const DbuY ymin = arch_->getRow(rb)->getBottom();
    const DbuY ymax = arch_->getRow(rt)->getTop();
    if (abs(nd->getBottom() - ymin) != 0 || abs(nd->getTop() - ymax) != 0) {
      ++err_n;
    }
  }
  logger_->info(DPL, 315, "Found {:d} row alignment problems.", err_n);
  return err_n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::getCellSpacing(const Node* ndl, const Node* ndr)
{
  if (ndl == nullptr || ndr == nullptr) {
    return 0;
  }
  return arch_->getCellSpacing(ndl, ndl);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::getSpaceAroundCell(const int seg,
                                     int ix,
                                     DbuX& space,
                                     DbuX& larger,
                                     const int limit)
{
  // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the
  // bottom of the cell instead of the center of the cell.  Need to assign a
  // cell to multiple segments.

  const Node* ndi = cellsInSeg_[seg][ix];
  const Node* ndj = nullptr;
  const Node* ndk = nullptr;

  const int n = (int) cellsInSeg_[seg].size();
  const DbuX xmin = segments_[seg]->getMinX();
  const DbuX xmax = segments_[seg]->getMaxX();

  // Space to the immediate left and right of the cell.
  DbuX space_left{0};
  if (ix == 0) {
    space_left += (ndi->getLeft()) - xmin;
  } else {
    --ix;
    ndj = cellsInSeg_[seg][ix];

    space_left += (ndi->getLeft()) - (ndj->getRight());
    ++ix;
  }

  DbuX space_right{0};
  if (ix == n - 1) {
    space_right += xmax - (ndi->getRight());
  } else {
    ++ix;
    ndj = cellsInSeg_[seg][ix];
    space_right += (ndj->getLeft()) - (ndi->getRight());
  }
  space = space_left + space_right;

  // Space three cells 'limit' cells to the left and 'limit' cells to the right.
  if (ix < limit) {
    ndj = cellsInSeg_[seg][0];
    larger = ndj->getLeft() - xmin;
  } else {
    larger = DbuX{0};
  }
  for (int j = std::max(0, ix - limit); j <= std::min(n - 1, ix + limit); j++) {
    ndj = cellsInSeg_[seg][j];
    if (j < n - 1) {
      ndk = cellsInSeg_[seg][j + 1];
      larger += (ndk->getLeft()) - (ndj->getRight());
    } else {
      larger += xmax - (ndj->getRight());
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::getSpaceAroundCell(const int seg,
                                     int ix,
                                     DbuX& space_left,
                                     DbuX& space_right,
                                     DbuX& large_left,
                                     DbuX& large_right,
                                     const int limit)
{
  // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the
  // bottom of the cell instead of the center of the cell.  Need to assign a
  // cell to multiple segments.

  const Node* ndi = cellsInSeg_[seg][ix];
  const Node* ndj = nullptr;
  const Node* ndk = nullptr;

  const int n = (int) cellsInSeg_[seg].size();
  const DbuX xmin = segments_[seg]->getMinX();
  const DbuX xmax = segments_[seg]->getMaxX();

  // Space to the immediate left and right of the cell.
  space_left = DbuX{0};
  if (ix == 0) {
    space_left += ndi->getLeft() - xmin;
  } else {
    --ix;
    ndj = cellsInSeg_[seg][ix];
    space_left += ndi->getLeft() - (ndj->getRight());
    ++ix;
  }

  space_right = DbuX{0};
  if (ix == n - 1) {
    space_right += xmax - (ndi->getRight());
  } else {
    ++ix;
    ndj = cellsInSeg_[seg][ix];
    space_right += (ndj->getLeft()) - (ndi->getRight());
  }
  // Space three cells 'limit' cells to the left and 'limit' cells to the right.
  large_left = DbuX{0};
  if (ix < limit) {
    ndj = cellsInSeg_[seg][0];
    large_left = ndj->getLeft() - xmin;
  }
  for (int j = std::max(0, ix - limit); j < ix; j++) {
    ndj = cellsInSeg_[seg][j];
    ndk = cellsInSeg_[seg][j + 1];
    large_left += (ndk->getLeft()) - (ndj->getRight());
  }
  large_right = DbuX{0};
  for (int j = ix; j <= std::min(n - 1, ix + limit); j++) {
    ndj = cellsInSeg_[seg][j];
    if (j < n - 1) {
      ndk = cellsInSeg_[seg][j + 1];
      large_right += (ndk->getLeft()) - (ndj->getRight());
    } else {
      large_right += xmax - (ndj->getRight());
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::findRegionIntervals(
    const int regId,
    std::vector<std::vector<std::pair<DbuX, DbuX>>>& intervals)
{
  // Find intervals within each row that are spanned by the specified region.
  // We ignore the default region 0, since it is "everywhere".

  if (regId < 0 || regId >= arch_->getRegions().size()
      || arch_->getRegion(regId)->getId() != regId) {
    internalError("Improper region id");
  }
  auto regPtr = arch_->getRegion(regId);

  // Initialize.
  intervals.clear();
  intervals.resize(numSingleHeightRows_);

  // Look at the rectangles within the region.
  for (const auto& rect : regPtr->getRects()) {
    const DbuX xmin{rect.xMin()};
    const DbuX xmax{rect.xMax()};
    const DbuY ymin{rect.yMin()};
    const DbuY ymax{rect.yMax()};

    for (int r = 0; r < numSingleHeightRows_; r++) {
      const DbuY lb = arch_->getMinY() + r * singleRowHeight_;
      const DbuY ub = lb + singleRowHeight_;

      if (ymax >= ub && ymin <= lb) {
        // Blockage overlaps with the entire row span in the Y-dir... Sites
        // are possibly completely covered!

        const DbuX originX = arch_->getRow(r)->getLeft();
        const DbuX siteSpacing = arch_->getRow(r)->getSiteSpacing();

        const int i0 = ((xmin - originX) / siteSpacing).v;
        int i1 = ((xmax - originX) / siteSpacing).v;
        if (originX + i1 * siteSpacing != xmax) {
          ++i1;
        }

        if (i1 > i0) {
          intervals[r].emplace_back(originX + i0 * siteSpacing,
                                    originX + i1 * siteSpacing);
        }
      }
    }
  }

  // Sort intervals and merge.  We merge, since the region might have been
  // defined with rectangles that touch (so it is "wrong" to create an
  // artificial boundary).
  for (int r = 0; r < numSingleHeightRows_; r++) {
    if (intervals[r].empty()) {
      continue;
    }

    // Sort to get intervals left to right.
    std::sort(intervals[r].begin(), intervals[r].end(), compareIntervals());

    std::stack<std::pair<DbuX, DbuX>> s;
    s.push(intervals[r][0]);
    for (int i = 1; i < intervals[r].size(); i++) {
      std::pair<DbuX, DbuX> top = s.top();  // copy.
      if (top.second < intervals[r][i].first) {
        s.push(intervals[r][i]);  // new interval.
      } else {
        if (top.second < intervals[r][i].second) {
          top.second = intervals[r][i].second;  // extend interval.
        }
        s.pop();      // remove old.
        s.push(top);  // expanded interval.
      }
    }

    intervals[r].clear();
    while (!s.empty()) {
      intervals[r].push_back(s.top());
      s.pop();
    }

    // Sort to get them left to right.
    std::sort(intervals[r].begin(), intervals[r].end(), compareIntervals());
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::resortSegments()
{
  // Resort the nodes in the segments.  This might be required if we did
  // something to move cells around and broke the ordering.
  for (DetailedSeg* segPtr : segments_) {
    resortSegment(segPtr);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::resortSegment(DetailedSeg* segPtr)
{
  const int segId = segPtr->getSegId();
  std::stable_sort(
      cellsInSeg_[segId].begin(), cellsInSeg_[segId].end(), compareNodesX());
  segPtr->setUtil(0);
  for (const Node* ndi : cellsInSeg_[segId]) {
    segPtr->addUtil(ndi->getWidth().v);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeAllCellsFromSegments()
{
  // This routine removes _ALL_ cells from all segments.  It clears all
  // reverse maps and so forth.  Basically, it leaves things as if the
  // segments have all been created, but nothing has been inserted.
  for (DetailedSeg* segPtr : segments_) {
    const int segId = segPtr->getSegId();
    cellsInSeg_[segId].clear();
    segPtr->setUtil(0.0);
  }
  for (auto& reverseCellToSegs : reverseCellToSegs_) {
    reverseCellToSegs.clear();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::alignPos(const Node* ndi, DbuX& xi, const DbuX xl, DbuX xr)
{
  // Given a cell with a target location, xi, determine a
  // site-aligned position such that the cell falls
  // within the interval [xl,xr].
  //
  // This routine works with the left edge of the cell.

  const DbuX originX = arch_->getRow(0)->getLeft();
  const DbuX siteSpacing = arch_->getRow(0)->getSiteSpacing();
  const DbuX w = ndi->getWidth();

  xr -= w;  // [xl,xr] is now range for left edge of cell.

  // Left edge of cell within [xl,xr] closest to target.
  DbuX xp = std::max(xl, std::min(xr, xi));

  const DbuX ix = (xp - originX) / siteSpacing;
  xp = originX + ix * siteSpacing;  // Left edge aligned.

  if (xp < xl) {
    xp += siteSpacing;
  } else if (xp > xr) {
    xp -= siteSpacing;
  }

  if (xp < xl || xp > xr) {
    // Left edge out of range so cell will also be out of range.
    return false;
  }

  // Set new target.
  xi = xp;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::checkSiteOrientation(Node* node, DbuX x, DbuY y)
{
  odb::dbSite* site = node->getDbInst()->getMaster()->getSite();
  const auto grid_x = grid_->gridX(x);
  const auto grid_y = grid_->gridSnapDownY(y);
  return grid_->getSiteOrientation(grid_x, grid_y, site).has_value();
}

bool DetailedMgr::shift(std::vector<Node*>& cells,
                        std::vector<DbuX>& targetLeft,
                        std::vector<DbuX>& posLeft,
                        const DbuX leftLimit,
                        const DbuX rightLimit,
                        const int segId,
                        const int rowId)
{
  // Shift a set of ordered cells given target positions.
  // The final site-aligned positions are returned.  Only
  // works for a vector of single height cells.

  // Note: The segment id is not really required.  The
  // segment id is only required to be able to get the
  // origin and site spacing to align cells.

  const DbuX originX = arch_->getRow(rowId)->getLeft();
  const DbuX siteSpacing = arch_->getRow(rowId)->getSiteSpacing();
  const DbuX siteWidth = arch_->getRow(rowId)->getSiteWidth();

  // Number of cells.
  const int ncells = (int) cells.size();

  // Sites within the provided range.
  int i0 = ((leftLimit - originX) / siteSpacing).v;
  if (originX + i0 * siteSpacing < leftLimit) {
    ++i0;
  }
  int i1 = ((rightLimit - originX) / siteSpacing).v;
  if (originX + i1 * siteSpacing + siteWidth >= rightLimit) {
    --i1;
  }
  const int nsites = i1 - i0 + 1;

  // Get cell widths while accounting for spacing/padding.  We
  // ignore spacing/padding at the ends (should adjust left
  // and right edges prior to calling).  Convert spacing into
  // number of sites.
  // Change cell widths to be in terms of number of sites.
  std::vector<int> swid;
  swid.resize(ncells);
  std::ranges::fill(swid, 0);
  int rsites = 0;
  for (int i = 0; i < ncells; i++) {
    const Node* ndi = cells[i];
    DbuX width = ndi->getWidth();
    if (i != ncells - 1) {
      width += arch_->getCellSpacing(ndi, cells[i + 1]);
    }
    swid[i] = width.v / siteSpacing.v;
    rsites += swid[i];
  }
  if (rsites > nsites) {
    return false;
  }

  // Determine leftmost and rightmost site for each cell.
  std::vector<int> site_l, site_r;
  site_l.resize(ncells);
  site_r.resize(ncells);
  int k = i0;
  for (int i = 0; i < ncells; i++) {
    site_l[i] = k;
    k += swid[i];
  }
  k = i1 + 1;
  for (int i = ncells - 1; i >= 0; i--) {
    site_r[i] = k - swid[i];
    k = site_r[i];
    if (site_r[i] < site_l[i]) {
      return false;
    }
  }

  // Create tables.
  std::vector<std::vector<std::pair<int, int>>> prev;
  std::vector<std::vector<double>> tcost;
  std::vector<std::vector<double>> cost;
  tcost.resize(nsites + 1);
  prev.resize(nsites + 1);
  cost.resize(nsites + 1);
  for (size_t i = 0; i <= nsites; i++) {
    tcost[i].resize(ncells + 1);
    prev[i].resize(ncells + 1);
    cost[i].resize(ncells + 1);

    std::fill(
        tcost[i].begin(), tcost[i].end(), std::numeric_limits<double>::max());
    std::fill(prev[i].begin(), prev[i].end(), std::make_pair(-1, -1));
    std::fill(cost[i].begin(), cost[i].end(), 0.0);
  }

  // Fill in costs of cells to sites.
  for (int j = 1; j <= ncells; j++) {
    // Skip invalid sites.
    for (int i = 1; i <= nsites; i++) {
      // Cell will cover real sites from [site_id,site_id+width-1].

      const int site_id = i0 + i - 1;
      if (site_id < site_l[j - 1] || site_id > site_r[j - 1]) {
        continue;
      }

      // Figure out cell position if cell aligned to current site.
      const DbuX x = originX + site_id * siteSpacing;
      cost[i][j] = abs(x - targetLeft[j - 1]).v;
    }
  }

  // Fill in total costs.
  tcost[0][0] = 0.;
  for (int j = 1; j <= ncells; j++) {
    // Width info; for indexing.
    const int prev_wid = (j - 1 == 0) ? 1 : swid[j - 2];
    const int curr_wid = swid[j - 1];

    for (int i = 1; i <= nsites; i++) {
      // Current site is site_id and covers [site_id,site_id+width-1].
      const int site_id = i0 + i - 1;

      // Cost if site skipped.
      {
        const int ii = i - 1;
        const int jj = j;
        const double c = tcost[ii][jj];
        if (c < tcost[i][j]) {
          tcost[i][j] = c;
          prev[i][j] = {ii, jj};
        }
      }

      // Cost if site used; avoid if invalid (too far left or right).
      const int ii = i - prev_wid;
      const int jj = j - 1;
      if (!(ii < 0 || site_id + curr_wid - 1 > i1)) {
        const double c = tcost[ii][jj] + cost[i][j];
        if (c < tcost[i][j]) {
          tcost[i][j] = c;
          prev[i][j] = std::make_pair(ii, jj);
        }
      }
    }
  }

  // Test.
  {
    bool okay = false;
    std::pair<int, int> curr{nsites, ncells};
    while (curr.first != -1 && curr.second != -1) {
      if (curr.first == 0 && curr.second == 0) {
        okay = true;
      }
      curr = prev[curr.first][curr.second];
    }
    if (!okay) {
      // Odd.  Should not fail.
      return false;
    }
  }

  // Determine placement.
  {
    std::pair<int, int> curr{nsites, ncells};
    while (curr.first != -1 && curr.second != -1) {
      if (curr.first == 0 && curr.second == 0) {
        break;
      }
      const int curr_i = curr.first;   // Site.
      const int curr_j = curr.second;  // Cell.

      if (curr_j != prev[curr_i][curr_j].second) {
        // We've placed the cell at the site.
        const int ix = i0 + curr_i - 1;
        posLeft[curr_j - 1] = originX + ix * siteSpacing;
      }

      curr = prev[curr_i][curr_j];
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::shiftRightHelper(Node* ndi, DbuX xj, const int sj, Node* ndr)
{
  // Helper routine for shifting single height cells in a specified
  // segment to the right.
  //
  // We assume cell "ndi" is going to be positioned (left edge) at
  // "xj" within segment "sj".  The cell "ndr" is the cell which is
  // to the immediate right of "ndi" after the insertion.
  //
  // We will attempt to push cells starting at "ndr" to the right to
  // maintain no overlap, satisfy spacing, etc.

  const auto it
      = std::find(cellsInSeg_[sj].begin(), cellsInSeg_[sj].end(), ndr);
  if (cellsInSeg_[sj].end() == it) {
    // Error.
    return false;
  }
  int ix = (int) (it - cellsInSeg_[sj].begin());
  const int n = (int) cellsInSeg_[sj].size() - 1;

  const int rj = segments_[sj]->getRowId();
  const DbuX originX = arch_->getRow(rj)->getLeft();
  const DbuX siteSpacing = arch_->getRow(rj)->getSiteSpacing();

  // Shift single height cells to the right until we encounter some
  // sort of problem.
  while ((ix <= n)
         && (ndr->getLeft()
             < xj + ndi->getWidth() + arch_->getCellSpacing(ndi, ndr))) {
    if (arch_->getCellHeightInRows(ndr) != 1) {
      return false;
    }

    // Determine a proper site-aligned position for cell ndr.
    xj += ndi->getWidth();
    xj += arch_->getCellSpacing(ndi, ndr);

    const int site = (xj - originX / siteSpacing).v;

    DbuX sx = originX + site * siteSpacing;
    if (xj != sx) {
      // Might need to go another site to the right.
      if (xj > sx) {
        sx += siteSpacing;
      }
      if (xj != sx) {
        if (xj < sx) {
          xj = sx;
        }
      }
    }

    // Build the move list.
    if (!addToMoveList(ndr,
                       ndr->getLeft(),
                       ndr->getBottom(),
                       sj,
                       xj,
                       ndr->getBottom(),
                       sj)) {
      return false;
    }

    // Fail if we shift off end of segment.
    if (xj + ndr->getWidth() + arch_->getCellSpacing(ndr, nullptr)
        > segments_[sj]->getMaxX()) {
      return false;
    }

    if (ix == n) {
      // We shifted down to the last cell... Everything must be okay!
      break;
    }
    ndi = ndr;
    ndr = cellsInSeg_[sj][++ix];
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::shiftLeftHelper(Node* ndi, DbuX xj, const int sj, Node* ndl)
{
  // Helper routine for shifting single height cells in a specified
  // segment to the left.
  //
  // We assume cell "ndi" is going to be positioned (left edge) at
  // "xj" within segment "sj".  The cell "ndl" is the cell which is
  // to the immediate left of "ndi" after the insertion.
  //
  // We will attempt to push cells starting at "ndl" to the left to
  // maintain no overlap, satisfy spacing, etc.

  // Need the index of "ndl".
  const auto it
      = std::find(cellsInSeg_[sj].begin(), cellsInSeg_[sj].end(), ndl);
  if (cellsInSeg_[sj].end() == it) {
    return false;
  }
  int ix = (int) (it - cellsInSeg_[sj].begin());
  const int n = 0;

  const int rj = segments_[sj]->getRowId();
  const DbuX originX = arch_->getRow(rj)->getLeft();
  const DbuX siteSpacing = arch_->getRow(rj)->getSiteSpacing();

  // Shift single height cells to the left until we encounter some
  // sort of problem.
  while ((ix >= n)
         && (ndl->getRight() + arch_->getCellSpacing(ndl, ndi) > xj)) {
    if (arch_->getCellHeightInRows(ndl) != 1) {
      return false;
    }

    // Determine a proper site-aligned position for cell ndl.
    xj -= arch_->getCellSpacing(ndl, ndi);
    xj -= ndl->getWidth();

    const int site = (xj - originX / siteSpacing).v;

    const DbuX sx = originX + site * siteSpacing;
    if (xj != sx) {
      if (xj > sx) {
        xj = sx;
      }
    }

    // Build the move list.
    if (!addToMoveList(ndl,
                       ndl->getLeft(),
                       ndl->getBottom(),
                       sj,
                       xj,
                       ndl->getBottom(),
                       sj)) {
      return false;
    }

    // Fail if we shift off the end of a segment.
    if (xj - arch_->getCellSpacing(nullptr, ndl) < segments_[sj]->getMinX()) {
      return false;
    }
    if (ix == n) {
      // We shifted down to the last cell... Everything must be okay!
      break;
    }

    ndi = ndl;
    ndl = cellsInSeg_[sj][--ix];
  }
  return true;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::verifyMove()
{
  for (const auto& node : journal_.getAffectedNodes()) {
    if (hasPlacementViolation(node)) {
      rejectMove();
      return false;
    }
  }
  return true;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::tryMove(Node* ndi,
                          const DbuX xi,
                          const DbuY yi,
                          const int si,
                          const DbuX xj,
                          const DbuY yj,
                          const int sj)
{
  // Based on the input, call an appropriate routine to try
  // and generate a move.
  if (!checkMasterSymmetry(arch_, ndi, segments_[sj]->getRowId())) {
    rejectMove();
    return false;
  }

  if (arch_->getCellHeightInRows(ndi) == 1) {
    // Single height cell.
    if (si != sj) {
      // Different segment.
      if (tryMove1(ndi, xi, yi, si, xj, yj, sj)) {
        return verifyMove();
      }
    } else {
      // Same segment.
      if (tryMove2(ndi, xi, yi, si, xj, yj, sj)) {
        return verifyMove();
      }
    }
  } else {
    // Currently only a single, simple routine for trying to move
    // a multi-height cell.
    if (tryMove3(ndi, xi, yi, si, xj, yj, sj)) {
      return verifyMove();
    }
  }
  rejectMove();
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::trySwap(Node* ndi,
                          const DbuX xi,
                          const DbuY yi,
                          const int si,
                          const DbuX xj,
                          const DbuY yj,
                          const int sj)
{
  if (!checkMasterSymmetry(arch_, ndi, segments_[sj]->getRowId())) {
    rejectMove();
    return false;
  }
  if (trySwap1(ndi, xi, yi, si, xj, yj, sj)) {
    return verifyMove();
  }
  rejectMove();
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::tryMove1(Node* ndi,
                           const DbuX xi,
                           const DbuY yi,
                           const int si,
                           DbuX xj,
                           DbuY yj,
                           const int sj)
{
  // Try to move a single height cell to a new position in another segment.
  // Positions are understood to be positions for the left, bottom corner
  // of the cell.

  // Clear the move list.
  clearMoveList();

  // Reasons to fail.  Same or bogus segment, wrong region, or
  // not single height cell.
  const int spanned = arch_->getCellHeightInRows(ndi);
  if (sj == si || sj == -1 || ndi->getGroupId() != segments_[sj]->getRegId()
      || spanned != 1) {
    return false;
  }

  const int rj = segments_[sj]->getRowId();
  if (abs(yj - arch_->getRow(rj)->getBottom()) != 0) {
    // Weird.
    yj = arch_->getRow(rj)->getBottom();
  }

  // Find the cells to the left and to the right of the target location.
  Node* ndr = nullptr;
  Node* ndl = nullptr;
  if (!cellsInSeg_[sj].empty()) {
    auto it = std::lower_bound(cellsInSeg_[sj].begin(),
                               cellsInSeg_[sj].end(),
                               xj,
                               DetailedMgr::compareNodesX());

    if (it == cellsInSeg_[sj].end()) {
      // Nothing to the right of the target position.  But, there must be
      // something to the left since we know the segment is not empty.
      ndl = cellsInSeg_[sj].back();
    } else {
      ndr = *it;
      if (it != cellsInSeg_[sj].begin()) {
        --it;
        ndl = *it;
      }
    }
  }

  // What we do depends on if there are cells to the left or right.
  if (ndl == nullptr && ndr == nullptr) {
    // No left or right cell implies an empty segment.
    const DetailedSeg* segPtr = segments_[sj];

    // Reject if not enough space.
    const DbuX required{ndi->getWidth() + arch_->getCellSpacing(nullptr, ndi)
                        + arch_->getCellSpacing(ndi, nullptr)};
    if (required + segPtr->getUtil() > segPtr->getWidth()) {
      return false;
    }

    const DbuX lx = segPtr->getMinX() + arch_->getCellSpacing(nullptr, ndi);
    const DbuX rx = segPtr->getMaxX() - arch_->getCellSpacing(ndi, nullptr);
    if (!alignPos(ndi, xj, lx, rx)) {
      return false;
    }
    // Build the move list.
    if (!addToMoveList(ndi, ndi->getLeft(), ndi->getBottom(), si, xj, yj, sj)) {
      return false;
    }
    return true;
  }

  if (ndl != nullptr && ndr == nullptr) {
    // End of segment, cells to the left.
    const DetailedSeg* segPtr = segments_[sj];

    // Reject if not enough space.
    const DbuX required{ndi->getWidth() + arch_->getCellSpacing(ndl, ndi)
                        + arch_->getCellSpacing(ndi, nullptr)};
    if (required + segPtr->getUtil() > segPtr->getWidth()) {
      return false;
    }

    const DbuX lx = ndl->getRight() + arch_->getCellSpacing(ndl, ndi);
    const DbuX rx
        = segments_[sj]->getMaxX() - arch_->getCellSpacing(ndi, nullptr);
    if (!alignPos(ndi, xj, lx, rx)) {
      return false;
    }

    // Build the move list.
    if (!addToMoveList(ndi, ndi->getLeft(), ndi->getBottom(), si, xj, yj, sj)) {
      return false;
    }
    // Shift cells left if required.
    if (!shiftLeftHelper(ndi, xj, sj, ndl)) {
      return false;
    }
    return true;
  }

  if (ndl == nullptr && ndr != nullptr) {
    // left-end of segment, cells to the right.
    const DetailedSeg* segPtr = segments_[sj];

    // Reject if not enough space.
    const DbuX required{ndi->getWidth() + arch_->getCellSpacing(nullptr, ndi)
                        + arch_->getCellSpacing(ndi, ndr)};
    if (required + segPtr->getUtil() > segPtr->getWidth()) {
      return false;
    }

    const DbuX lx = segPtr->getMinX() + arch_->getCellSpacing(nullptr, ndi);

    const DbuX rx = ndr->getLeft() - arch_->getCellSpacing(ndi, ndr);
    if (!alignPos(ndi, xj, lx, rx)) {
      return false;
    }

    // Build the move list.
    if (!addToMoveList(ndi, ndi->getLeft(), ndi->getBottom(), si, xj, yj, sj)) {
      return false;
    }
    // Shift cells right if required.
    if (!shiftRightHelper(ndi, xj, sj, ndr)) {
      return false;
    }
    return true;
  }
  // (ndl != nullptr && ndr != nullptr)
  // In between two cells.
  const DetailedSeg* segPtr = segments_[sj];

  // Reject if not enough space.
  const DbuX required{ndi->getWidth() + arch_->getCellSpacing(ndl, ndi)
                      + arch_->getCellSpacing(ndi, ndr)
                      - arch_->getCellSpacing(ndl, ndr)};
  if (required + segPtr->getUtil() > segPtr->getWidth()) {
    return false;
  }

  const DbuX lx = ndl->getRight() + arch_->getCellSpacing(ndl, ndi);
  const DbuX rx = ndr->getLeft() - arch_->getCellSpacing(ndi, ndr);
  if (!alignPos(ndi, xj, lx, rx)) {
    return false;
  }

  // Build the move list.
  if (!addToMoveList(ndi, ndi->getLeft(), ndi->getBottom(), si, xj, yj, sj)) {
    return false;
  }
  // Shift cells right if required.
  if (!shiftRightHelper(ndi, xj, sj, ndr)) {
    return false;
  }
  // Shift cells left if necessary.
  if (!shiftLeftHelper(ndi, xj, sj, ndl)) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::tryMove2(Node* ndi,
                           const DbuX xi,
                           const DbuY yi,
                           const int si,
                           DbuX xj,
                           DbuY yj,
                           const int sj)
{
  // Very simple move within the same segment.
  // Nothing to move.
  clearMoveList();

  // Reasons to fail.  Different or bogus segment, wrong region, or
  // not single height cell.
  const int spanned = arch_->getCellHeightInRows(ndi);
  if (sj != si || sj == -1 || ndi->getGroupId() != segments_[sj]->getRegId()
      || spanned != 1) {
    return false;
  }

  const int rj = segments_[sj]->getRowId();
  if (abs(yj - arch_->getRow(rj)->getBottom()) != 0) {
    // Weird.
    yj = arch_->getRow(rj)->getBottom();
  }

  const int n = (int) cellsInSeg_[si].size() - 1;

  // Find closest cell to the right of the target location.  It's fine
  // to get "ndi" since we are just attempting a move to a new
  // location.
  const Node* ndj = nullptr;
  int ix_j = -1;
  if (!cellsInSeg_[sj].empty()) {
    auto it_j = std::lower_bound(cellsInSeg_[sj].begin(),
                                 cellsInSeg_[sj].end(),
                                 xj,
                                 DetailedMgr::compareNodesX());

    if (it_j == cellsInSeg_[sj].end()) {
      ndj = cellsInSeg_[sj].back();
      ix_j = (int) cellsInSeg_[sj].size() - 1;
    } else {
      ndj = *it_j;
      ix_j = (int) (it_j - cellsInSeg_[sj].begin());
    }
  }
  // We should find something...  At least "ndi"!
  if (ix_j == -1 || ndj == nullptr) {
    return false;
  }

  // Note that it is fine if ndj is the same as ndi; we are just trying
  // to move to a new position adjacent to some block.
  const Node* prev = (ix_j == 0) ? nullptr : cellsInSeg_[sj][ix_j - 1];
  const Node* next = (ix_j == n) ? nullptr : cellsInSeg_[sj][ix_j + 1];

  // Try to the left of ndj, then to the right.
  const DetailedSeg* segPtr = segments_[sj];

  // Try left.
  DbuX lx;
  if (prev) {
    lx = prev->getRight() + arch_->getCellSpacing(prev, ndi);
  } else {
    lx = segPtr->getMinX() + arch_->getCellSpacing(nullptr, ndi);
  }
  DbuX rx = ndj->getLeft() - arch_->getCellSpacing(ndi, ndj);
  if (ndi->getWidth() <= rx - lx) {
    if (!alignPos(ndi, xj, lx, rx)) {
      return false;
    }

    if (!addToMoveList(ndi, ndi->getLeft(), ndi->getBottom(), si, xj, yj, sj)) {
      return false;
    }
    return true;
  }

  // Try right.
  lx = ndj->getRight() + arch_->getCellSpacing(ndj, ndi);
  if (next) {
    rx = next->getLeft() - arch_->getCellSpacing(ndi, next);
  } else {
    rx = segPtr->getMaxX() - arch_->getCellSpacing(ndi, nullptr);
  }

  if (ndi->getWidth() <= rx - lx) {
    if (!alignPos(ndi, xj, lx, rx)) {
      return false;
    }

    if (!addToMoveList(ndi, ndi->getLeft(), ndi->getBottom(), si, xj, yj, sj)) {
      return false;
    }
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::tryMove3(Node* ndi,
                           const DbuX xi,
                           const DbuY yi,
                           const int si,
                           DbuX xj,
                           DbuY yj,
                           const int sj)
{
  clearMoveList();

  // Code to try and move a multi-height cell to another location.  Simple
  // in that it only looks for gaps.

  // Ensure multi-height, although I think this code should work for single
  // height cells too.
  const int spanned = arch_->getCellHeightInRows(ndi);
  if (spanned <= 1 || spanned != reverseCellToSegs_[ndi->getId()].size()) {
    return false;
  }

  // Turn the target location into a set of rows.  The target position
  // in the y-direction should be the target position for the bottom
  // of the cell which should also correspond to the row in which the
  // segment is found.
  int rb = segments_[sj]->getRowId();
  while (rb + spanned >= arch_->getRows().size()) {
    --rb;
  }
  // We might need to adjust the target position if we needed to move
  // the rows "down"...
  const int rt = rb + spanned - 1;  // Cell would occupy rows [rb,rt].

  bool flip = false;
  if (!arch_->powerCompatible(ndi, arch_->getRow(rb), flip)) {
    return false;
  }

  // Next find the segments based on the targeted x location.  We might be
  // outside of our region or there could be a blockage.  So, we need a flag.
  std::vector<int> segs;
  for (int r = rb; r <= rt; r++) {
    bool gotSeg = false;
    for (int s = 0; s < segsInRow_[r].size() && !gotSeg; s++) {
      const DetailedSeg* segPtr = segsInRow_[r][s];
      if (segPtr->getRegId() == ndi->getGroupId()) {
        if (xj >= segPtr->getMinX() && xj <= segPtr->getMaxX()) {
          gotSeg = true;
          segs.push_back(segPtr->getSegId());
        }
      }
    }
    if (!gotSeg) {
      break;
    }
  }
  // Extra check.
  if (segs.size() != spanned) {
    return false;
  }

  // So, the goal is to try and move the cell into the segments contained within
  // the "segs" vector.  Determine if there is space.  To do this, we loop over
  // the segments and look for the cell to the right of the target location.  We
  // then grab the cell to the left.  We can determine if the the gap is large
  // enough.
  DbuX xmin = std::numeric_limits<DbuX>::lowest();
  DbuX xmax = std::numeric_limits<DbuX>::max();
  for (auto seg : segs) {
    const DetailedSeg* segPtr = segments_[seg];
    const int segId = segPtr->getSegId();
    const Node* left = nullptr;
    const Node* rite = nullptr;

    if (!cellsInSeg_[segId].empty()) {
      auto it_j = std::lower_bound(cellsInSeg_[segId].begin(),
                                   cellsInSeg_[segId].end(),
                                   xj,
                                   DetailedMgr::compareNodesX());
      if (it_j == cellsInSeg_[segId].end()) {
        // Nothing to the right; the last cell in the row will be on the left.
        left = cellsInSeg_[segId].back();

        // If the cell on the left turns out to be the current cell, then we
        // can assume this cell is not there and look to the left "one cell
        // more".
        if (left == ndi) {
          if (it_j != cellsInSeg_[segId].begin()) {
            --it_j;
            left = *it_j;
            ++it_j;
          } else {
            left = nullptr;
          }
        }
      } else {
        rite = *it_j;
        if (it_j != cellsInSeg_[segId].begin()) {
          --it_j;
          left = *it_j;
          if (left == ndi) {
            if (it_j != cellsInSeg_[segId].begin()) {
              --it_j;
              left = *it_j;
              ++it_j;
            } else {
              left = nullptr;
            }
          }
          ++it_j;
        }
      }
    }

    // If the left or the right cells are the same as the current cell, then
    // we aren't moving.
    if (ndi == left || ndi == rite) {
      return false;
    }

    DbuX lx = (left == nullptr) ? segPtr->getMinX() : left->getRight();
    DbuX rx = (rite == nullptr) ? segPtr->getMaxX() : rite->getLeft();
    if (left != nullptr) {
      lx += arch_->getCellSpacing(left, ndi);
    }
    if (rite != nullptr) {
      rx -= arch_->getCellSpacing(ndi, rite);
    }
    if (ndi->getWidth() <= rx - lx) {
      // The cell will fit without moving the left and right cell.
      xmin = std::max(xmin, lx);
      xmax = std::min(xmax, rx);
    } else {
      // The cell will not fit in between the left and right cell
      // in this segment.  So, we cannot faciliate the single move.
      return false;
    }
  }

  // Here, we can fit.
  if (ndi->getWidth() <= xmax - xmin) {
    if (!alignPos(ndi, xj, xmin, xmax)) {
      return false;
    }

    std::vector<int> old_segs;
    old_segs.reserve(reverseCellToSegs_[ndi->getId()].size());
    for (const auto seg : reverseCellToSegs_[ndi->getId()]) {
      old_segs.push_back(seg->getSegId());
    }

    if (!addToMoveList(ndi,
                       ndi->getLeft(),
                       ndi->getBottom(),
                       old_segs,
                       xj,
                       arch_->getRow(rb)->getBottom(),
                       segs)) {
      return false;
    }
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::trySwap1(Node* ndi,
                           DbuX xi,
                           const DbuY yi,
                           const int si,
                           DbuX xj,
                           const DbuY yj,
                           const int sj)
{
  // Tries to swap cell "ndi" with another cell, "ndj", which it finds
  // near the target.  No cell shifting is involved; only the two cells
  // are considered.  So, it is a very simple swap.  It also only works
  // for single height cells.

  clearMoveList();

  Node* ndj = nullptr;
  if (!cellsInSeg_[sj].empty()) {
    const auto it_j = std::lower_bound(cellsInSeg_[sj].begin(),
                                       cellsInSeg_[sj].end(),
                                       xj,
                                       DetailedMgr::compareNodesX());
    if (it_j == cellsInSeg_[sj].end()) {
      ndj = cellsInSeg_[sj].back();
    } else {
      ndj = *it_j;
    }
  }
  if (ndj == ndi || ndj == nullptr) {
    return false;
  }
  if (!checkMasterSymmetry(arch_, ndj, segments_[si]->getRowId())) {
    return false;
  }

  if (arch_->getCellHeightInRows(ndi) != 1
      || arch_->getCellHeightInRows(ndj) != 1) {
    return false;
  }

  // Determine the indices of the cells in their respective
  // segments.  Determine if cells are adjacent.
  const auto it_i
      = std::find(cellsInSeg_[si].begin(), cellsInSeg_[si].end(), ndi);
  const int ix_i = (int) (it_i - cellsInSeg_[si].begin());

  const auto it_j
      = std::find(cellsInSeg_[sj].begin(), cellsInSeg_[sj].end(), ndj);
  const int ix_j = (int) (it_j - cellsInSeg_[sj].begin());

  const bool adjacent
      = ((si == sj) && (ix_i + 1 == ix_j || ix_j + 1 == ix_i)) ? true : false;

  const Node* prev;
  const Node* next;
  int n;
  DbuX lx, rx;
  if (!adjacent) {
    // Determine if "ndi" can fit into the gap created
    // by removing "ndj" and visa-versa.
    n = (int) cellsInSeg_[si].size() - 1;
    next = (ix_i == n) ? nullptr : cellsInSeg_[si][ix_i + 1];
    prev = (ix_i == 0) ? nullptr : cellsInSeg_[si][ix_i - 1];
    rx = segments_[si]->getMaxX();
    if (next) {
      rx = next->getLeft();
    }
    rx -= arch_->getCellSpacing(ndj, next);

    lx = segments_[si]->getMinX();
    if (prev) {
      lx = prev->getRight();
    }
    lx += arch_->getCellSpacing(prev, ndj);

    if (ndj->getWidth() > (rx - lx)) {
      // Cell "ndj" will not fit into gap created by removing "ndi".
      return false;
    }

    // Determine aligned position for "ndj" in spot created by
    // removing "ndi".
    if (!alignPos(ndj, xi, lx, rx)) {
      return false;
    }

    n = (int) cellsInSeg_[sj].size() - 1;
    next = (ix_j == n) ? nullptr : cellsInSeg_[sj][ix_j + 1];
    prev = (ix_j == 0) ? nullptr : cellsInSeg_[sj][ix_j - 1];
    rx = segments_[sj]->getMaxX();
    if (next) {
      rx = next->getLeft();
    }
    rx -= arch_->getCellSpacing(ndi, next);

    lx = segments_[sj]->getMinX();
    if (prev) {
      lx = prev->getRight();
    }
    lx += arch_->getCellSpacing(prev, ndi);

    if (ndi->getWidth() > (rx - lx)) {
      // Cell "ndi" will not fit into gap created by removing "ndj".
      return false;
    }

    // Determine aligned position for "ndi" in spot created by
    // removing "ndj".
    if (!alignPos(ndi, xj, lx, rx)) {
      return false;
    }
    const DbuX x1 = ndi->getLeft();
    const DbuY y1 = ndi->getBottom();
    const DbuX x2 = ndj->getLeft();
    const DbuY y2 = ndj->getBottom();
    // Build move list.
    if (!addToMoveList(ndi, x1, y1, si, xj, y2, sj)) {
      return false;
    }
    if (!addToMoveList(ndj, x2, y2, sj, xi, y1, si)) {
      paintInGrid(ndj);
      return false;
    }
    return true;
  }

  // Same row and adjacent.
  if (ix_i + 1 == ix_j) {
    // cell "ndi" is left of cell "ndj".
    n = (int) cellsInSeg_[sj].size() - 1;
    next = (ix_j == n) ? nullptr : cellsInSeg_[sj][ix_j + 1];
    prev = (ix_i == 0) ? nullptr : cellsInSeg_[si][ix_i - 1];

    rx = segments_[sj]->getMaxX();
    if (next) {
      rx = next->getLeft();
    }
    rx -= arch_->getCellSpacing(ndi, next);

    lx = segments_[si]->getMinX();
    if (prev) {
      lx = prev->getRight();
    }
    lx += arch_->getCellSpacing(prev, ndj);

    if (ndj->getWidth() + ndi->getWidth() + arch_->getCellSpacing(ndj, ndi)
        > (rx - lx)) {
      return false;
    }

    // Shift...
    std::vector<Node*> cells;
    std::vector<DbuX> targetLeft;
    std::vector<DbuX> posLeft;
    cells.push_back(ndj);
    targetLeft.push_back(xi);
    posLeft.emplace_back(0);
    cells.push_back(ndi);
    targetLeft.push_back(xj);
    posLeft.emplace_back(0);
    int ri = segments_[si]->getRowId();
    if (!shift(cells, targetLeft, posLeft, lx, rx, si, ri)) {
      return false;
    }
    xi = posLeft[0];
    xj = posLeft[1];
  } else if (ix_j + 1 == ix_i) {
    // cell "ndj" is left of cell "ndi".
    n = (int) cellsInSeg_[si].size() - 1;
    next = (ix_i == n) ? nullptr : cellsInSeg_[si][ix_i + 1];
    prev = (ix_j == 0) ? nullptr : cellsInSeg_[sj][ix_j - 1];

    rx = segments_[si]->getMaxX();
    if (next) {
      rx = next->getLeft();
    }
    rx -= arch_->getCellSpacing(ndj, next);

    lx = segments_[sj]->getMinX();
    if (prev) {
      lx = prev->getRight();
    }
    lx += arch_->getCellSpacing(prev, ndi);

    if (ndi->getWidth() + ndj->getWidth() + arch_->getCellSpacing(ndi, ndj)
        > (rx - lx)) {
      return false;
    }

    // Shift...
    std::vector<Node*> cells;
    std::vector<DbuX> targetLeft;
    std::vector<DbuX> posLeft;
    cells.push_back(ndi);
    targetLeft.push_back(xj);
    posLeft.emplace_back(0);
    cells.push_back(ndj);
    targetLeft.push_back(xi);
    posLeft.emplace_back(0);
    int ri = segments_[si]->getRowId();
    if (!shift(cells, targetLeft, posLeft, lx, rx, si, ri)) {
      return false;
    }
    xj = posLeft[0];
    xi = posLeft[1];
  } else {
    // Shouldn't get here.
    return false;
  }

  const DbuX x1 = ndi->getLeft();
  const DbuY y1 = ndi->getBottom();
  const DbuX x2 = ndj->getLeft();
  const DbuY y2 = ndj->getBottom();
  // Build move list.

  if (!addToMoveList(ndi, x1, y1, si, xj, y2, sj)) {
    return false;
  }
  if (!addToMoveList(ndj, x2, y2, sj, xi, y1, si)) {
    paintInGrid(ndj);
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::clearMoveList()
{
  journal_.clear();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::addToMoveList(Node* ndi,
                                const DbuX curLeft,
                                const DbuY curBottom,
                                const int curSeg,
                                const DbuX newLeft,
                                const DbuY newBottom,
                                const int newSeg)
{
  // Limit maximum number of cells that can move at once.
  if (journal_.size() >= moveLimit_) {
    return false;
  }

  if (!checkSiteOrientation(ndi, newLeft, newBottom)) {
    return false;
  }

  // Easy to observe displacement limit if using the
  // manager to compose a move list.  We can check
  // only here whether or not a cell will violate its
  // displacement limit.
  const DbuY dy = abs(newBottom - ndi->getOrigBottom());
  const DbuX dx = abs(newLeft - ndi->getOrigLeft());
  if (dx > maxDispX_ || dy > maxDispY_) {
    return false;
  }
  // commit move and add to journal
  eraseFromGrid(ndi);
  if (curSeg >= 0) {
    removeCellFromSegment(ndi, curSeg);
  }
  ndi->setLeft(newLeft);
  ndi->setBottom(newBottom);
  paintInGrid(ndi);
  if (newSeg >= 0) {
    addCellToSegment(ndi, newSeg);
  }

  MoveCellAction action(
      ndi, curLeft, curBottom, newLeft, newBottom, true, {curSeg}, {newSeg});
  journal_.addAction(action);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::addToMoveList(Node* ndi,
                                const DbuX curLeft,
                                const DbuY curBottom,
                                const std::vector<int>& curSegs,
                                const DbuX newLeft,
                                const DbuY newBottom,
                                const std::vector<int>& newSegs)
{
  // Most number of cells that can move.
  if (journal_.size() >= moveLimit_) {
    return false;
  }

  if (!checkSiteOrientation(ndi, newLeft, newBottom)) {
    return false;
  }

  // commit move and add to journal
  eraseFromGrid(ndi);
  for (const auto& curSeg : curSegs) {
    removeCellFromSegment(ndi, curSeg);
  }
  ndi->setLeft(newLeft);
  ndi->setBottom(newBottom);
  paintInGrid(ndi);
  for (const auto& newSeg : newSegs) {
    addCellToSegment(ndi, newSeg);
  }
  MoveCellAction action(
      ndi, curLeft, curBottom, newLeft, newBottom, true, curSegs, newSegs);
  journal_.addAction(action);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::acceptMove()
{
  clearMoveList();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::rejectMove()
{
  journal_.undo();
  clearMoveList();
}

////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::eraseFromGrid(Node* node)
{
  grid_->erasePixel(node);
}
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::paintInGrid(Node* node)
{
  const auto grid_x = grid_->gridX(node);
  const auto grid_y = grid_->gridSnapDownY(node);
  odb::dbSite* site = node->getDbInst()->getMaster()->getSite();

  const auto orientation
      = grid_->getSiteOrientation(grid_x, grid_y, site).value();
  grid_->paintPixel(node, grid_x, grid_y);
  node->adjustCurrOrient(orientation);
}
}  // namespace dpl
