// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "architecture.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <stack>
#include <utility>
#include <vector>

#include "Objects.h"
#include "Padding.h"
#include "dpl/Opendp.h"
#include "infrastructure/Coordinates.h"
#include "network.h"
#include "odb/db.h"
#include "odb/dbTransform.h"

namespace dpl {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::~Architecture()
{
  for (auto& row : rows_) {
    delete row;
  }
  rows_.clear();

  for (auto& region : regions_) {
    delete region;
  }
  regions_.clear();
}

void Architecture::clear()
{
  xmin_ = std::numeric_limits<DbuX>::max();
  xmax_ = std::numeric_limits<DbuX>::lowest();
  ymin_ = std::numeric_limits<DbuY>::max();
  ymax_ = std::numeric_limits<DbuY>::lowest();
  for (auto& row : rows_) {
    delete row;
  }
  rows_.clear();

  for (auto& region : regions_) {
    delete region;
  }
  regions_.clear();
  usePadding_ = false;
  padding_ = nullptr;
  site_width_ = DbuX{0};
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::isSingleHeightCell(const Node* ndi) const
{
  return getCellHeightInRows(ndi) == 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::isMultiHeightCell(const Node* ndi) const
{
  return getCellHeightInRows(ndi) != 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::getCellHeightInRows(const Node* ndi) const
{
  return std::lround(ndi->getHeight().v / (double) rows_[0]->getHeight().v);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Row* Architecture::createAndAddRow()
{
  auto row = new Row();
  rows_.push_back(row);
  return row;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Group* Architecture::createAndAddRegion()
{
  auto region = new Group();
  regions_.push_back(region);
  return region;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::postProcess(Network* network)
{
  // Sort the rows and assign ids.  Check for co-linear rows (sub-rows).
  // Right now, I am merging subrows back into single rows and adding
  // filler to block the gaps.
  //
  // It would be better to update the architecture to simply understand
  // subrows...

  xmin_ = std::numeric_limits<DbuX>::max();
  xmax_ = std::numeric_limits<DbuX>::lowest();
  ymin_ = std::numeric_limits<DbuY>::max();
  ymax_ = std::numeric_limits<DbuY>::lowest();

  // Sort rows.
  std::ranges::stable_sort(rows_, std::less{}, &Architecture::Row::getBottom);

  // Determine a box surrounding all the rows.
  for (auto row : rows_) {
    xmin_ = std::min(xmin_, row->getLeft());
    xmax_ = std::max(xmax_, row->getRight());
    ymin_ = std::min(ymin_, row->getBottom());
    ymax_ = std::max(ymax_, row->getTop());
  }

  // NEW.  Search for sub-rows.
  std::vector<Architecture::Row*> subrows;
  std::vector<Architecture::Row*> rows;
  std::vector<std::pair<DbuX, DbuX>> intervals;
  for (int r = 0; r < rows_.size();) {
    subrows.clear();
    subrows.push_back(rows_[r++]);
    while (r < rows_.size()
           && abs(rows_[r]->getBottom() - subrows[0]->getBottom()) == 0) {
      subrows.push_back(rows_[r++]);
    }

    // Convert subrows to intervals.
    intervals.clear();
    for (auto subrow : subrows) {
      const DbuX lx = subrow->getLeft();
      const DbuX rx = subrow->getRight();
      intervals.emplace_back(lx, rx);
    }
    std::ranges::sort(intervals);

    std::stack<std::pair<DbuX, DbuX>> s;
    s.push(intervals[0]);
    for (size_t i = 1; i < intervals.size(); i++) {
      std::pair<DbuX, DbuX> top = s.top();  // copy.
      if (top.second < intervals[i].first) {
        s.push(intervals[i]);  // new interval.
      } else {
        if (top.second < intervals[i].second) {
          top.second = intervals[i].second;  // extend interval.
        }
        s.pop();      // remove old.
        s.push(top);  // expanded interval.
      }
    }
    intervals.clear();
    while (!s.empty()) {
      const std::pair<DbuX, DbuX> temp = s.top();  // copy.
      intervals.push_back(temp);
      s.pop();
    }
    // Get intervals left to right.
    std::ranges::sort(intervals);

    // If more than one subrow, convert to a single row
    // and delete the unnecessary subrows.
    if (subrows.size() > 1) {
      const DbuX lx = intervals.front().first;
      const DbuX rx = intervals.back().second;
      subrows[0]->setNumSites(((rx - lx) / subrows[0]->getSiteSpacing()).v);
      subrows[0]->setSubRowOrigin(lx);

      // Delete un-needed rows.
      while (subrows.size() > 1) {
        Architecture::Row* ptr = subrows.back();
        subrows.pop_back();
        delete ptr;
      }
    }
    rows.push_back(subrows[0]);

    // Check for the insertion of filler.
    const DbuY height{subrows[0]->getHeight()};
    const DbuY yb{subrows[0]->getBottom()};
    if (xmin_ < intervals.front().first) {
      const DbuX lx{xmin_};
      const DbuX rx{intervals.front().first};
      const DbuX width{rx - lx};
      network->addFillerNode(lx, yb, width, height);
    }
    for (size_t i = 1; i < intervals.size(); i++) {
      if (intervals[i].first > intervals[i - 1].second) {
        const DbuX lx{intervals[i - 1].second};
        const DbuX rx{intervals[i].first};
        const DbuX width{rx - lx};
        network->addFillerNode(lx, yb, width, height);
      }
    }
    if (xmax_ > intervals.back().second) {
      const DbuX lx{intervals.back().second};
      const DbuX rx{xmax_};
      const DbuX width{rx - lx};
      network->addFillerNode(lx, yb, width, height);
    }
  }
  // Replace original rows with new rows.
  rows_ = std::move(rows);
  // Sort rows (to be safe).
  std::ranges::stable_sort(rows_, std::less{}, &Architecture::Row::getBottom);
  // Assign row ids.
  for (int r = 0; r < rows_.size(); r++) {
    rows_[r]->setId(r);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::find_closest_row(const DbuY y)
{
  // Given a position which is intended to be the bottom of a cell,
  // find its closest row.
  int r = 0;
  if (y > rows_[0]->getBottom()) {
    auto row_l = std::ranges::lower_bound(
        rows_, y, std::less{}, &Architecture::Row::getBottom);
    if (row_l == rows_.end() || (*row_l)->getBottom() > y) {
      --row_l;
    }
    r = (int) (row_l - rows_.begin());
    if (r < rows_.size() - 1) {
      if (abs(rows_[r + 1]->getBottom() - y) < abs(rows_[r]->getBottom() - y)) {
        ++r;
      }
    }
  }

  return r;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::powerCompatible(const Node* ndi,
                                   const Row* row,
                                   bool& flip) const
{
  // This routine assumes the node will be placed (start) in the provided row.
  // Based on this this routine determines if the node and the row are power
  // compatible.

  flip = false;

  // Number of spanned rows.
  const int spanned
      = std::lround(ndi->getHeight().v / (double) row->getHeight().v);
  const int lo = row->getId();
  const int hi = lo + spanned - 1;
  if (hi >= rows_.size()) {
    return false;  // off the top of the chip.
  }
  if (hi == lo) {
    // Single height cell.  Actually, is the check for a single height cell any
    // different than a multi height cell?  We could have power/ground at both
    // the top and the bottom...  However, I think this is beyond the current
    // goal...

    const int rowBot = rows_[lo]->getBottomPower();
    const int rowTop = rows_[hi]->getTopPower();

    const int ndBot = ndi->getBottomPower();
    const int ndTop = ndi->getTopPower();
    if ((ndBot == rowBot || ndBot == Architecture::Row::Power_UNK
         || rowBot == Architecture::Row::Power_UNK)
        && (ndTop == rowTop || ndTop == Architecture::Row::Power_UNK
            || rowTop == Architecture::Row::Power_UNK)) {
      // Power matches as it is.
      flip = false;
    } else {
      // Assume we need to flip.
      flip = true;
    }

    return true;
  }

  // Multi-height cell.
  const int rowBot = rows_[lo]->getBottomPower();
  const int rowTop = rows_[hi]->getTopPower();

  int ndBot = ndi->getBottomPower();
  int ndTop = ndi->getTopPower();

  if ((ndBot == rowBot || ndBot == Architecture::Row::Power_UNK
       || rowBot == Architecture::Row::Power_UNK)
      && (ndTop == rowTop || ndTop == Architecture::Row::Power_UNK
          || rowTop == Architecture::Row::Power_UNK)) {
    // The power at the top and bottom of the node match either because they
    // are the same or because something is not specified.  No need to flip
    // the node.
    flip = false;
    return true;
  }
  // Swap the node power rails and do the same check.  If we now get a match,
  // things are fine as long as the cell is flipped.
  std::swap(ndBot, ndTop);
  if ((ndBot == rowBot || ndBot == Architecture::Row::Power_UNK
       || rowBot == Architecture::Row::Power_UNK)
      && (ndTop == rowTop || ndTop == Architecture::Row::Power_UNK
          || rowTop == Architecture::Row::Power_UNK)) {
    flip = true;
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::getCellPadding(const Node* ndi,
                                  int& leftPadding,
                                  int& rightPadding) const
{
  leftPadding = dpl::gridToDbu(padding_->padLeft(ndi), site_width_).v;
  rightPadding = dpl::gridToDbu(padding_->padRight(ndi), site_width_).v;
  return true;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::getCellPadding(const Node* ndi,
                                  DbuX& leftPadding,
                                  DbuX& rightPadding) const
{
  leftPadding = dpl::gridToDbu(padding_->padLeft(ndi), site_width_);
  rightPadding = dpl::gridToDbu(padding_->padRight(ndi), site_width_);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::flipCellPadding(const Node* ndi)
{
  GridX left_padding = padding_->padLeft(ndi);
  GridX right_padding = padding_->padLeft(ndi);
  padding_->setPadding(ndi->getDbInst(), left_padding, right_padding);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::getCellSpacing(const Node* leftNode,
                                 const Node* rightNode) const
{
  // Return the required separation between the two cells.
  //
  // I've updated this as well to account for the situation where
  // one of the provided cells is null.  Even in the case of null,
  // I suppose that we should account for padding; e.g., we might
  // be at the end of a segment.
  int retval = 0;

  if (usePadding_) {
    // Separation is padding to the right of the left cell plus
    // the padding to the left of the right cell.

    int separation = 0;
    if (leftNode != nullptr) {
      DbuX left_padding, right_padding;
      getCellPadding(leftNode, left_padding, right_padding);
      separation += right_padding.v;
    }
    if (rightNode != nullptr) {
      DbuX left_padding, right_padding;
      getCellPadding(rightNode, left_padding, right_padding);
      separation += left_padding.v;
    }
    retval = std::max(retval, separation);
  }
  return retval;
}

////////////////////////////////////////////////////////////////////////////////

}  // namespace dpl
