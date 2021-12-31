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
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <vector>
#include <stack>

#include "architecture.h"
#include "network.h"
#include "router.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Architecture() : m_useSpacingTable(false), m_usePadding(false) {
  m_xmin = std::numeric_limits<double>::max();
  m_xmax = -std::numeric_limits<double>::max();
  m_ymin = std::numeric_limits<double>::max();
  m_ymax = -std::numeric_limits<double>::max();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::~Architecture() {
  clear_edge_type();

  for (unsigned r = 0; r < m_rows.size(); r++) {
    delete m_rows[r];
  }
  m_rows.clear();

  for (unsigned r = 0; r < m_regions.size(); r++) {
    delete m_regions[r];
  }
  m_regions.clear();

  clearSpacingTable();
}

void Architecture::clearSpacingTable() {
  for (size_t i = 0; i < m_cellSpacings.size(); i++) {
    delete m_cellSpacings[i];
  }
  m_cellSpacings.clear();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::isSingleHeightCell(Node* ndi) const {
  int spanned = (int)(ndi->getHeight() / m_rows[0]->getHeight() + 0.5);
  return spanned == 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::isMultiHeightCell(Node* ndi) const {
  int spanned = (int)(ndi->getHeight() / m_rows[0]->getHeight() + 0.5);
  return spanned != 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::getCellHeightInRows(Node* ndi) const {
  int spanned = (int)(ndi->getHeight() / m_rows[0]->getHeight() + 0.5);
  return spanned;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct compareIntervals {
  bool operator()(std::pair<double, double> i1,
                  std::pair<double, double> i2) const {
    if (i1.first == i2.first) {
      return i1.second < i2.second;
    }
    return i1.first < i2.first;
  }
};
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Row* Architecture::createAndAddRow() {
  Architecture::Row* ptr = new Row();
  m_rows.push_back(ptr);
  return ptr;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Region* Architecture::createAndAddRegion() {
  Architecture::Region* ptr = new Region();
  m_regions.push_back(ptr);
  return ptr;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::postProcess( Network* network ) {
  // Sort the rows and assign ids.  Check for co-linear rows (sub-rows).
  // Right now, I am merging subrows back into single rows and adding
  // filler to block the gaps.
  //
  // It would be better to update the architecture to simply understand
  // subrows...

  m_xmin = std::numeric_limits<double>::max();
  m_xmax = -std::numeric_limits<double>::max();
  m_ymin = std::numeric_limits<double>::max();
  m_ymax = -std::numeric_limits<double>::max();

  double height, width, x, y;
  double lx, rx, yb, yt;

  // Sort rows.
  std::stable_sort(m_rows.begin(), m_rows.end(), SortRow());

  // Determine a box surrounding all the rows.
  for (int r = 0; r < m_rows.size(); r++) {
    Architecture::Row* row = m_rows[r];

    lx = row->getLeft();
    rx = row->getRight();

    yb = row->getBottom();
    yt = yb + row->getHeight();

    m_xmin = std::min(m_xmin, lx);
    m_xmax = std::max(m_xmax, rx);
    m_ymin = std::min(m_ymin, yb);
    m_ymax = std::max(m_ymax, yt);
  }

  // NEW.  Search for sub-rows.
  std::vector<Architecture::Row*> subrows;
  std::vector<Architecture::Row*> rows;
  std::vector<std::pair<double,double> > intervals;
  for (int r = 0; r < m_rows.size(); ) {
    subrows.erase(subrows.begin(), subrows.end());
    subrows.push_back(m_rows[r++]);
    while (r < m_rows.size() && std::fabs(m_rows[r]->getBottom()-subrows[0]->getBottom()) < 1.0e-3) {
      subrows.push_back(m_rows[r++]);
    }

    // Convert subrows to intervals.
    intervals.erase(intervals.begin(),intervals.end());
    for (size_t i = 0; i < subrows.size(); i++) {
      lx = subrows[i]->getLeft();
      rx = subrows[i]->getRight();
      intervals.push_back(std::make_pair(lx,rx));
    }
    std::sort(intervals.begin(), intervals.end(), compareIntervals());

    std::stack<std::pair<double, double> > s;
    s.push(intervals[0]);
    for (size_t i = 1; i < intervals.size(); i++) {
      std::pair<double, double> top = s.top();  // copy.
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
      std::pair<double, double> temp = s.top();  // copy.
      intervals.push_back(temp);
      s.pop();
    }
    // Get intervals left to right.
    std::sort(intervals.begin(), intervals.end(), compareIntervals());

    // If more than one subrow, convert to a single row
    // and delete the unnecessary subrows.
    if (subrows.size() > 1) {
      lx = intervals.front().first;
      rx = intervals.back().second;
      subrows[0]->setNumSites((rx -lx) / subrows[0]->getSiteSpacing());
      rx = lx + subrows[0]->getNumSites() * subrows[0]->getSiteSpacing();

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
    height = subrows[0]->getHeight();
    y = subrows[0]->getCenterY();
    if (m_xmin < intervals.front().first) {
      lx = m_xmin;
      rx = intervals.front().first;
      width = rx-lx;
      x = 0.5*(lx + rx);
      network->createAndAddFillerNode(x, y, width, height);
    }
    for (size_t i = 1; i < intervals.size(); i++) {
      if( intervals[i].first > intervals[i-1].second) {
        lx = intervals[i-1].second;
        rx = intervals[i].first;
        width = rx-lx;
        x = 0.5*(lx + rx);
        network->createAndAddFillerNode(x, y, width, height);
      }
    }
    if (m_xmax > intervals.back().second) {
      lx = intervals.back().second;
      rx = m_xmax;
      width = rx-lx;
      x = 0.5*(lx + rx);
      network->createAndAddFillerNode(x, y, width, height);
    }
  }
  // Replace original rows with new rows.
  m_rows.erase( m_rows.begin(), m_rows.end() );
  m_rows.insert( m_rows.end(), rows.begin(), rows.end() );
  // Sort rows (to be safe).
  std::stable_sort(m_rows.begin(), m_rows.end(), SortRow());
  // Assign row ids.
  for (int r = 0; r < m_rows.size(); r++) {
    m_rows[r]->setId(r);
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::find_closest_row(double y) {
  // Given a "y" (intended to be the bottom of a cell), find the closest row.
  int r = 0;
  if (y > m_rows[0]->getBottom()) {
    std::vector<Architecture::Row*>::iterator row_l;
    row_l = std::lower_bound(m_rows.begin(), m_rows.end(), y,
                             Architecture::Row::compare_row());
    if (row_l == m_rows.end() || (*row_l)->getBottom() > y) {
      --row_l;
    }
    r = row_l - m_rows.begin();

    // Should we check actual distance?  The row we find is the one that the
    // bottom of the cell (specified in "y") overlaps with.  But, it could be
    // true that the bottom of the cell is actually closer to the row above...
    if (r < m_rows.size() - 1) {
      if (std::fabs(m_rows[r + 1]->getBottom() - y) <
          std::fabs(m_rows[r]->getBottom() - y)) {
        // Actually, the cell is closer to the row above the one found...
        ++r;
      }
    }
  }

  return r;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Architecture::compute_overlap(double xmin1, double xmax1, double ymin1,
                                     double ymax1, double xmin2, double xmax2,
                                     double ymin2, double ymax2) {
  if (xmin1 >= xmax2) return 0.0;
  if (xmax1 <= xmin2) return 0.0;
  if (ymin1 >= ymax2) return 0.0;
  if (ymax1 <= ymin2) return 0.0;
  double ww = std::min(xmax1, xmax2) - std::max(xmin1, xmin2);
  double hh = std::min(ymax1, ymax2) - std::max(ymin1, ymin2);
  return ww * hh;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::power_compatible(Node* ndi, Row* row, bool& flip) {
  // This routine assumes the node will be placed (start) in the provided row.
  // Based on this this routine determines if the node and the row are power
  // compatible.

  flip = false;


  int spanned =
      (int)((ndi->getHeight() / row->getHeight()) + 0.5);  // Number of spanned rows.
  int lo = row->getId();
  int hi = lo + spanned - 1;
  if (hi >= m_rows.size()) return false;  // off the top of the chip.
  if (hi == lo) {
    // Single height cell.  Actually, is the check for a single height cell any
    // different than a multi height cell?  We could have power/ground at both
    // the top and the bottom...  However, I think this is beyond the current
    // goal...

    int rowBot = m_rows[lo]->getPowerBottom();
    int rowTop = m_rows[hi]->getPowerTop();

    int ndBot = ndi->getBottomPower();
    int ndTop = ndi->getTopPower();
    if ((ndBot == rowBot || ndBot == RowPower_UNK || rowBot == RowPower_UNK) &&
        (ndTop == rowTop || ndTop == RowPower_UNK || rowTop == RowPower_UNK)) {
      // Power matches as it is.
      flip = false;
    } else {
      // Assume we need to flip.
      flip = true;
    }

    return true;
  } else {
    // Multi-height cell.
    int rowBot = m_rows[lo]->getPowerBottom();
    int rowTop = m_rows[hi]->getPowerTop();

    int ndBot = ndi->getBottomPower();
    int ndTop = ndi->getTopPower();

    if ((ndBot == rowBot || ndBot == RowPower_UNK || rowBot == RowPower_UNK) &&
        (ndTop == rowTop || ndTop == RowPower_UNK || rowTop == RowPower_UNK)) {
      // The power at the top and bottom of the node match either because they
      // are the same or because something is not specified.  No need to flip
      // the node.
      flip = false;
      return true;
    }
    // Swap the node power rails and do the same check.  If we now get a match,
    // things are fine as long as the cell is flipped.
    std::swap(ndBot, ndTop);
    if ((ndBot == rowBot || ndBot == RowPower_UNK || rowBot == RowPower_UNK) &&
        (ndTop == rowTop || ndTop == RowPower_UNK || rowTop == RowPower_UNK)) {
      flip = true;
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::addCellSpacingUsingTable(int firstEdge, int secondEdge,
                                            double sep) {
  m_cellSpacings.push_back(
      new Architecture::Spacing(firstEdge, secondEdge, sep));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::addCellPadding(Node* ndi, double leftPadding,
                                  double rightPadding) {
  m_cellPaddings[ndi->getId()] = std::make_pair(leftPadding, rightPadding);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Architecture::getCellPadding(Node* ndi, double& leftPadding,
                                  double& rightPadding) {
  std::map<int, std::pair<double, double> >::iterator it;
  if (m_cellPaddings.end() == (it = m_cellPaddings.find(ndi->getId()))) {
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
double Architecture::getCellSpacing(Node* leftNode, Node* rightNode) {
  // Return the required separation between the two cells.  We use
  // either spacing tables or padding information, or both.  If
  // we use both, then we return the largest spacing.
  //
  // I've updated this as well to account for the situation where
  // one of the provided cells is null.  Even in the case of null,
  // I suppose that we should account for padding; e.g., we might
  // be at the end of a segment.
  double retval = 0.;
  if (m_useSpacingTable) {
    // Don't need this if one of the cells is null.
    int i1 = (leftNode == 0) ? -1 : leftNode->getRightEdgeType();
    int i2 = (rightNode == 0) ? -1 : rightNode->getLeftEdgeType();
    retval = std::max(retval, getCellSpacingUsingTable(i1, i2));
  }
  if (m_usePadding) {
    // Separation is padding to the right of the left cell plus
    // the padding to the left of the right cell.
    std::map<int, std::pair<double, double> >::iterator it;

    double separation = 0.;
    if (leftNode != 0) {
      if (m_cellPaddings.end() !=
          (it = m_cellPaddings.find(leftNode->getId()))) {
        separation += it->second.second;
      }
    }
    if (rightNode != 0) {
      if (m_cellPaddings.end() !=
          (it = m_cellPaddings.find(rightNode->getId()))) {
        separation += it->second.first;
      }
    }
    retval = std::max(retval, separation);
  }
  return retval;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Architecture::getCellSpacingUsingTable(int i1, int i2) {
  // In the event that one of the left or right indices is
  // void (-1), return the worst possible spacing.  This
  // is very pessimistic, but will ensure all issues are
  // resolved.

  double spacing = 0.0;

  if (i1 == -1 && i2 == -1) {
    for (int i = 0; i < m_cellSpacings.size(); i++) {
      Architecture::Spacing* ptr = m_cellSpacings[i];
      spacing = std::max(spacing, ptr->getSeparation());
    }
    return spacing;
  } else if (i1 == -1) {
    for (int i = 0; i < m_cellSpacings.size(); i++) {
      Architecture::Spacing* ptr = m_cellSpacings[i];
      if (ptr->getFirstEdge() == i2 || ptr->getSecondEdge() == i2) {
        spacing = std::max(spacing, ptr->getSeparation());
      }
    }
    return spacing;
  } else if (i2 == -1) {
    for (int i = 0; i < m_cellSpacings.size(); i++) {
      Architecture::Spacing* ptr = m_cellSpacings[i];
      if (ptr->getFirstEdge() == i1 || ptr->getSecondEdge() == i1) {
        spacing = std::max(spacing, ptr->getSeparation());
      }
    }
    return spacing;
  }

  for (int i = 0; i < m_cellSpacings.size(); i++) {
    Architecture::Spacing* ptr = m_cellSpacings[i];
    if ((ptr->getFirstEdge() == i1 && ptr->getSecondEdge() == i2) ||
        (ptr->getFirstEdge() == i2 && ptr->getSecondEdge() == i1)) {
      spacing = std::max(spacing, ptr->getSeparation());
    }
  }
  return spacing;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::clear_edge_type() {
  for (int i = 0; i < m_edgeTypes.size(); i++) {
    if (m_edgeTypes[i].first != 0) delete[] m_edgeTypes[i].first;
  }
  m_edgeTypes.clear();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Architecture::init_edge_type() {
  clear_edge_type();
  char* newName = new char[strlen("DEFAULT") + 1];
  strcpy(newName, "DEFAULT");
  m_edgeTypes.push_back(std::pair<char*, int>(newName, EDGETYPE_DEFAULT));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Architecture::add_edge_type(const char* name) {
  for (int i = 0; i < m_edgeTypes.size(); i++) {
    std::pair<char*, int>& temp = m_edgeTypes[i];
    if (strcmp(temp.first, name) == 0) {
      // Edge type already exists.
      return temp.second;
    }
  }
  char* newName = new char[strlen(name) + 1];
  strcpy(newName, name);
  int n = m_edgeTypes.size();
  m_edgeTypes.push_back(std::pair<char*, int>(newName, n));
  return n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Row::Row()
    : m_rowLoc(0),
      m_rowHeight(0),
      m_numSites(0),
      m_siteWidth(0),
      m_subRowOrigin(0),
      m_siteOrient(0),
      m_siteSymmetry(0),
      m_siteSpacing(0),
      m_id(0),
      m_powerTop(RowPower_UNK),
      m_powerBot(RowPower_UNK)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Row::~Row() {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Spacing::Spacing(int i1, int i2, double sep)
    : m_i1(i1), m_i2(i2), m_sep(sep) {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Region::Region()
    : m_id(-1)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Architecture::Region::~Region() { m_rects.clear(); }

}  // namespace dpo
