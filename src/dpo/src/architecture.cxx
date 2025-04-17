// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "architecture.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <map>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "dpl/Objects.h"
#include "network.h"
#include "odb/db.h"
#include "odb/dbTransform.h"

namespace dpo {

struct compareRowBottom
{
  bool operator()(Architecture::Row* p, Architecture::Row* q) const
  {
    return p->getBottom() < q->getBottom();
  }
  bool operator()(Architecture::Row*& s, double i) const
  {
    return s->getBottom() < i;
  }
  bool operator()(double i, Architecture::Row*& s) const
  {
    return i < s->getBottom();
  }
};

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
  return std::lround(ndi->getHeight().v / (double) rows_[0]->getHeight());
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
int Architecture::postProcess(Network* network)
{
  // Sort the rows and assign ids.  Check for co-linear rows (sub-rows).
  // Right now, I am merging subrows back into single rows and adding
  // filler to block the gaps.
  //
  // It would be better to update the architecture to simply understand
  // subrows...

  xmin_ = std::numeric_limits<int>::max();
  xmax_ = std::numeric_limits<int>::lowest();
  ymin_ = std::numeric_limits<int>::max();
  ymax_ = std::numeric_limits<int>::lowest();

  // Sort rows.
  std::stable_sort(rows_.begin(), rows_.end(), compareRowBottom());

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
  std::vector<std::pair<int, int>> intervals;
  int count = 0;
  for (int r = 0; r < rows_.size();) {
    subrows.clear();
    subrows.push_back(rows_[r++]);
    while (r < rows_.size()
           && std::abs(rows_[r]->getBottom() - subrows[0]->getBottom()) == 0) {
      subrows.push_back(rows_[r++]);
    }

    // Convert subrows to intervals.
    intervals.clear();
    for (auto subrow : subrows) {
      const int lx = subrow->getLeft();
      const int rx = subrow->getRight();
      intervals.emplace_back(lx, rx);
    }
    std::sort(intervals.begin(), intervals.end());

    std::stack<std::pair<int, int>> s;
    s.push(intervals[0]);
    for (size_t i = 1; i < intervals.size(); i++) {
      std::pair<int, int> top = s.top();  // copy.
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
      const std::pair<int, int> temp = s.top();  // copy.
      intervals.push_back(temp);
      s.pop();
    }
    // Get intervals left to right.
    std::sort(intervals.begin(), intervals.end());

    // If more than one subrow, convert to a single row
    // and delete the unnecessary subrows.
    if (subrows.size() > 1) {
      const int lx = intervals.front().first;
      const int rx = intervals.back().second;
      subrows[0]->setNumSites((rx - lx) / subrows[0]->getSiteSpacing());
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
      const Node* ndi = network->createAndAddFillerNode(lx, yb, width, height);
      const std::string name = "FILLER_" + std::to_string(count);
      network->setNodeName(ndi->getId(), name);
      ++count;
    }
    for (size_t i = 1; i < intervals.size(); i++) {
      if (intervals[i].first > intervals[i - 1].second) {
        const DbuX lx{intervals[i - 1].second};
        const DbuX rx{intervals[i].first};
        const DbuX width{rx - lx};
        const Node* ndi
            = network->createAndAddFillerNode(lx, yb, width, height);
        const std::string name = "FILLER_" + std::to_string(count);
        network->setNodeName(ndi->getId(), name);
        ++count;
      }
    }
    if (xmax_ > intervals.back().second) {
      const DbuX lx{intervals.back().second};
      const DbuX rx{xmax_};
      const DbuX width{rx - lx};
      const Node* ndi = network->createAndAddFillerNode(lx, yb, width, height);
      const std::string name = "FILLER_" + std::to_string(count);
      network->setNodeName(ndi->getId(), name);
      ++count;
    }
  }
  // Replace original rows with new rows.
  rows_ = std::move(rows);
  // Sort rows (to be safe).
  std::stable_sort(rows_.begin(), rows_.end(), compareRowBottom());
  // Assign row ids.
  for (int r = 0; r < rows_.size(); r++) {
    rows_[r]->setId(r);
  }
  return count;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::find_closest_row(const DbuY y)
{
  // Given a position which is intended to be the bottom of a cell,
  // find its closest row.
  int r = 0;
  if (y > rows_[0]->getBottom()) {
    auto row_l
        = std::lower_bound(rows_.begin(), rows_.end(), y.v, compareRowBottom());
    if (row_l == rows_.end() || (*row_l)->getBottom() > y) {
      --row_l;
    }
    r = (int) (row_l - rows_.begin());
    if (r < rows_.size() - 1) {
      if (std::abs(rows_[r + 1]->getBottom() - y.v)
          < std::abs(rows_[r]->getBottom() - y.v)) {
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
      = std::lround(ndi->getHeight().v / (double) row->getHeight());
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
void Architecture::addCellPadding(Node* ndi, int leftPadding, int rightPadding)
{
  cellPaddings_[ndi->getId()] = {leftPadding, rightPadding};
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::getCellPadding(const Node* ndi,
                                  int& leftPadding,
                                  int& rightPadding) const
{
  auto it = cellPaddings_.find(ndi->getId());
  if (it == cellPaddings_.end()) {
    rightPadding = 0;
    leftPadding = 0;
    return false;
  }
  std::tie(leftPadding, rightPadding) = it->second;
  return true;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::addCellPadding(Node* ndi,
                                  DbuX leftPadding,
                                  DbuX rightPadding)
{
  cellPaddings_[ndi->getId()] = {leftPadding.v, rightPadding.v};
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::getCellPadding(const Node* ndi,
                                  DbuX& leftPadding,
                                  DbuX& rightPadding) const
{
  auto it = cellPaddings_.find(ndi->getId());
  if (it == cellPaddings_.end()) {
    rightPadding = DbuX{0};
    leftPadding = DbuX{0};
    return false;
  }
  leftPadding = DbuX{it->second.first};
  rightPadding = DbuX{it->second.second};
  return true;
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
      const auto it = cellPaddings_.find(leftNode->getId());
      if (it != cellPaddings_.end()) {
        separation += it->second.second;
      }
    }
    if (rightNode != nullptr) {
      const auto it = cellPaddings_.find(rightNode->getId());
      if (it != cellPaddings_.end()) {
        separation += it->second.first;
      }
    }
    retval = std::max(retval, separation);
  }
  return retval;
}

////////////////////////////////////////////////////////////////////////////////

}  // namespace dpo
