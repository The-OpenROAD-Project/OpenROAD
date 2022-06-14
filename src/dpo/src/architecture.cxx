///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "architecture.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <stack>
#include <vector>

#include "network.h"
#include "router.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Architecture() : useSpacingTable_(false), usePadding_(false)
{
  xmin_ = std::numeric_limits<int>::max();
  xmax_ = std::numeric_limits<int>::lowest();
  ymin_ = std::numeric_limits<int>::max();
  ymax_ = std::numeric_limits<int>::lowest();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::~Architecture()
{
  clear_edge_type();

  for (size_t r = 0; r < rows_.size(); r++) {
    delete rows_[r];
  }
  rows_.clear();

  for (size_t r = 0; r < regions_.size(); r++) {
    delete regions_[r];
  }
  regions_.clear();

  clearSpacingTable();
}

void Architecture::clearSpacingTable()
{
  for (size_t i = 0; i < cellSpacings_.size(); i++) {
    delete cellSpacings_[i];
  }
  cellSpacings_.clear();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::isSingleHeightCell(Node* ndi) const
{
  int spanned = (int) (ndi->getHeight() / rows_[0]->getHeight() + 0.5);
  return spanned == 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::isMultiHeightCell(Node* ndi) const
{
  int spanned = (int) (ndi->getHeight() / rows_[0]->getHeight() + 0.5);
  return spanned != 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::getCellHeightInRows(Node* ndi) const
{
  int spanned = (int) (ndi->getHeight() / rows_[0]->getHeight() + 0.5);
  return spanned;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct compareIntervals
{
  bool operator()(std::pair<T, T> i1, std::pair<T, T> i2) const
  {
    if (i1.first == i2.first) {
      return i1.second < i2.second;
    }
    return i1.first < i2.first;
  }
};
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Row* Architecture::createAndAddRow()
{
  Architecture::Row* ptr = new Row();
  rows_.push_back(ptr);
  return ptr;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Region* Architecture::createAndAddRegion()
{
  Architecture::Region* ptr = new Region();
  regions_.push_back(ptr);
  return ptr;
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
  std::stable_sort(
      rows_.begin(), rows_.end(), Architecture::Row::compareRowBottom());

  // Determine a box surrounding all the rows.
  for (int r = 0; r < rows_.size(); r++) {
    Architecture::Row* row = rows_[r];

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
    subrows.erase(subrows.begin(), subrows.end());
    subrows.push_back(rows_[r++]);
    while (r < rows_.size()
           && std::abs(rows_[r]->getBottom() - subrows[0]->getBottom()) == 0) {
      subrows.push_back(rows_[r++]);
    }

    // Convert subrows to intervals.
    intervals.erase(intervals.begin(), intervals.end());
    for (size_t i = 0; i < subrows.size(); i++) {
      int lx = subrows[i]->getLeft();
      int rx = subrows[i]->getRight();
      intervals.push_back(std::make_pair(lx, rx));
    }
    std::sort(intervals.begin(), intervals.end(), compareIntervals<int>());

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
    intervals.erase(intervals.begin(), intervals.end());
    while (!s.empty()) {
      std::pair<int, int> temp = s.top();  // copy.
      intervals.push_back(temp);
      s.pop();
    }
    // Get intervals left to right.
    std::sort(intervals.begin(), intervals.end(), compareIntervals<int>());

    // If more than one subrow, convert to a single row
    // and delete the unnecessary subrows.
    if (subrows.size() > 1) {
      int lx = intervals.front().first;
      int rx = intervals.back().second;
      subrows[0]->setNumSites((int) ((rx - lx) / subrows[0]->getSiteSpacing()));

      // Delete un-needed rows.
      while (subrows.size() > 1) {
        Architecture::Row* ptr = subrows.back();
        subrows.pop_back();
        delete ptr;
      }
    }
    rows.push_back(subrows[0]);

    // Check for the insertion of filler.  Hmm.
    // How do we set the id of the filler here?
    int height = subrows[0]->getHeight();
    int yb = subrows[0]->getBottom();
    if (xmin_ < intervals.front().first) {
      int lx = xmin_;
      int rx = intervals.front().first;
      int width = rx - lx;
      Node* ndi = network->createAndAddFillerNode(lx, yb, width, height);
      std::string name = "FILLER_" + std::to_string(count);
      network->setNodeName(ndi->getId(), name);
      ++count;
    }
    for (size_t i = 1; i < intervals.size(); i++) {
      if (intervals[i].first > intervals[i - 1].second) {
        int lx = intervals[i - 1].second;
        int rx = intervals[i].first;
        int width = rx - lx;
        Node* ndi = network->createAndAddFillerNode(lx, yb, width, height);
        std::string name = "FILLER_" + std::to_string(count);
        network->setNodeName(ndi->getId(), name);
        ++count;
      }
    }
    if (xmax_ > intervals.back().second) {
      int lx = intervals.back().second;
      int rx = xmax_;
      int width = rx - lx;
      Node* ndi = network->createAndAddFillerNode(lx, yb, width, height);
      std::string name = "FILLER_" + std::to_string(count);
      network->setNodeName(ndi->getId(), name);
      ++count;
    }
  }
  // Replace original rows with new rows.
  rows_.erase(rows_.begin(), rows_.end());
  rows_.insert(rows_.end(), rows.begin(), rows.end());
  // Sort rows (to be safe).
  std::stable_sort(
      rows_.begin(), rows_.end(), Architecture::Row::compareRowBottom());
  // Assign row ids.
  for (int r = 0; r < rows_.size(); r++) {
    rows_[r]->setId(r);
  }
  return count;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::find_closest_row(int y)
{
  // Given a position which is intended to be the bottom of a cell,
  // find its closest row.
  int r = 0;
  if (y > rows_[0]->getBottom()) {
    std::vector<Architecture::Row*>::iterator row_l;
    row_l = std::lower_bound(
        rows_.begin(), rows_.end(), y, Architecture::Row::compareRowBottom());
    if (row_l == rows_.end() || (*row_l)->getBottom() > y) {
      --row_l;
    }
    r = (int) (row_l - rows_.begin());
    if (r < rows_.size() - 1) {
      if (std::abs(rows_[r + 1]->getBottom() - y)
          < std::abs(rows_[r]->getBottom() - y)) {
        ++r;
      }
    }
  }

  return r;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::power_compatible(Node* ndi, Row* row, bool& flip)
{
  // This routine assumes the node will be placed (start) in the provided row.
  // Based on this this routine determines if the node and the row are power
  // compatible.

  flip = false;

  int spanned = (int) ((ndi->getHeight() / row->getHeight())
                       + 0.5);  // Number of spanned rows.
  int lo = row->getId();
  int hi = lo + spanned - 1;
  if (hi >= rows_.size())
    return false;  // off the top of the chip.
  if (hi == lo) {
    // Single height cell.  Actually, is the check for a single height cell any
    // different than a multi height cell?  We could have power/ground at both
    // the top and the bottom...  However, I think this is beyond the current
    // goal...

    int rowBot = rows_[lo]->getBottomPower();
    int rowTop = rows_[hi]->getTopPower();

    int ndBot = ndi->getBottomPower();
    int ndTop = ndi->getTopPower();
    if ((ndBot == rowBot || ndBot == RowPower_UNK || rowBot == RowPower_UNK)
        && (ndTop == rowTop || ndTop == RowPower_UNK
            || rowTop == RowPower_UNK)) {
      // Power matches as it is.
      flip = false;
    } else {
      // Assume we need to flip.
      flip = true;
    }

    return true;
  } else {
    // Multi-height cell.
    int rowBot = rows_[lo]->getBottomPower();
    int rowTop = rows_[hi]->getTopPower();

    int ndBot = ndi->getBottomPower();
    int ndTop = ndi->getTopPower();

    if ((ndBot == rowBot || ndBot == RowPower_UNK || rowBot == RowPower_UNK)
        && (ndTop == rowTop || ndTop == RowPower_UNK
            || rowTop == RowPower_UNK)) {
      // The power at the top and bottom of the node match either because they
      // are the same or because something is not specified.  No need to flip
      // the node.
      flip = false;
      return true;
    }
    // Swap the node power rails and do the same check.  If we now get a match,
    // things are fine as long as the cell is flipped.
    std::swap(ndBot, ndTop);
    if ((ndBot == rowBot || ndBot == RowPower_UNK || rowBot == RowPower_UNK)
        && (ndTop == rowTop || ndTop == RowPower_UNK
            || rowTop == RowPower_UNK)) {
      flip = true;
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::addCellSpacingUsingTable(int firstEdge,
                                            int secondEdge,
                                            int sep)
{
  cellSpacings_.push_back(
      new Architecture::Spacing(firstEdge, secondEdge, sep));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::addCellPadding(Node* ndi, int leftPadding, int rightPadding)
{
  cellPaddings_[ndi->getId()] = std::make_pair(leftPadding, rightPadding);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::getCellPadding(Node* ndi,
                                  int& leftPadding,
                                  int& rightPadding)
{
  std::map<int, std::pair<int, int>>::iterator it;
  if (cellPaddings_.end() == (it = cellPaddings_.find(ndi->getId()))) {
    rightPadding = 0;
    leftPadding = 0;
    return false;
  }
  rightPadding = it->second.second;
  leftPadding = it->second.first;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::getCellSpacing(Node* leftNode, Node* rightNode)
{
  // Return the required separation between the two cells.  We use
  // either spacing tables or padding information, or both.  If
  // we use both, then we return the largest spacing.
  //
  // I've updated this as well to account for the situation where
  // one of the provided cells is null.  Even in the case of null,
  // I suppose that we should account for padding; e.g., we might
  // be at the end of a segment.
  int retval = 0;
  if (useSpacingTable_) {
    // Don't need this if one of the cells is null.
    int i1 = (leftNode == 0) ? -1 : leftNode->getRightEdgeType();
    int i2 = (rightNode == 0) ? -1 : rightNode->getLeftEdgeType();
    retval = std::max(retval, getCellSpacingUsingTable(i1, i2));
  }
  if (usePadding_) {
    // Separation is padding to the right of the left cell plus
    // the padding to the left of the right cell.
    std::map<int, std::pair<int, int>>::iterator it;

    int separation = 0;
    if (leftNode != 0) {
      if (cellPaddings_.end() != (it = cellPaddings_.find(leftNode->getId()))) {
        separation += it->second.second;
      }
    }
    if (rightNode != 0) {
      if (cellPaddings_.end()
          != (it = cellPaddings_.find(rightNode->getId()))) {
        separation += it->second.first;
      }
    }
    retval = std::max(retval, separation);
  }
  return retval;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::getCellSpacingUsingTable(int i1, int i2)
{
  // In the event that one of the left or right indices is
  // void (-1), return the worst possible spacing.  This
  // is very pessimistic, but will ensure all issues are
  // resolved.

  int spacing = 0;

  if (i1 == -1 && i2 == -1) {
    for (int i = 0; i < cellSpacings_.size(); i++) {
      Architecture::Spacing* ptr = cellSpacings_[i];
      spacing = std::max(spacing, ptr->getSeparation());
    }
    return spacing;
  } else if (i1 == -1) {
    for (int i = 0; i < cellSpacings_.size(); i++) {
      Architecture::Spacing* ptr = cellSpacings_[i];
      if (ptr->getFirstEdge() == i2 || ptr->getSecondEdge() == i2) {
        spacing = std::max(spacing, ptr->getSeparation());
      }
    }
    return spacing;
  } else if (i2 == -1) {
    for (int i = 0; i < cellSpacings_.size(); i++) {
      Architecture::Spacing* ptr = cellSpacings_[i];
      if (ptr->getFirstEdge() == i1 || ptr->getSecondEdge() == i1) {
        spacing = std::max(spacing, ptr->getSeparation());
      }
    }
    return spacing;
  }

  for (int i = 0; i < cellSpacings_.size(); i++) {
    Architecture::Spacing* ptr = cellSpacings_[i];
    if ((ptr->getFirstEdge() == i1 && ptr->getSecondEdge() == i2)
        || (ptr->getFirstEdge() == i2 && ptr->getSecondEdge() == i1)) {
      spacing = std::max(spacing, ptr->getSeparation());
    }
  }
  return spacing;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::clear_edge_type()
{
  for (int i = 0; i < edgeTypes_.size(); i++) {
    if (edgeTypes_[i].first != 0)
      delete[] edgeTypes_[i].first;
  }
  edgeTypes_.clear();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::init_edge_type()
{
  clear_edge_type();
  char* newName = new char[strlen("DEFAULT") + 1];
  strcpy(newName, "DEFAULT");
  edgeTypes_.push_back(std::pair<char*, int>(newName, EDGETYPE_DEFAULT));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::add_edge_type(const char* name)
{
  for (int i = 0; i < edgeTypes_.size(); i++) {
    std::pair<char*, int>& temp = edgeTypes_[i];
    if (strcmp(temp.first, name) == 0) {
      // Edge type already exists.
      return temp.second;
    }
  }
  char* newName = new char[strlen(name) + 1];
  strcpy(newName, name);
  int n = (int) edgeTypes_.size();
  edgeTypes_.push_back(std::pair<char*, int>(newName, n));
  return n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Row::Row()
    : id_(-1),
      rowLoc_(0),
      rowHeight_(0),
      subRowOrigin_(0),
      siteSpacing_(0),
      siteWidth_(0),
      numSites_(0),
      siteOrient_(0),
      siteSymmetry_(0),
      powerTop_(RowPower_UNK),
      powerBot_(RowPower_UNK)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Row::~Row()
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Spacing::Spacing(int i1, int i2, int sep)
    : i1_(i1), i2_(i2), sep_(sep)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Region::Region()
    : id_(-1),
      xmin_(std::numeric_limits<int>::max()),
      ymin_(std::numeric_limits<int>::max()),
      xmax_(std::numeric_limits<int>::lowest()),
      ymax_(std::numeric_limits<int>::lowest())
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Region::~Region()
{
  rects_.clear();
}

}  // namespace dpo
