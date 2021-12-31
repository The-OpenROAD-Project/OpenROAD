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

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include "detailed_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <cmath>
#include <iostream>
#include <set>
#include <stack>
#include <utility>
#include "detailed_orient.h"
#include "detailed_segment.h"
#include "plotgnu.h"
#include "utility.h"
#include "utl/Logger.h"

using utl::DPO;

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedMgr::DetailedMgr(Architecture* arch, Network* network,
                         RoutingParams* rt)
    : m_arch(arch), m_network(network), m_rt(rt), m_logger(0), m_rng(0) {
  m_singleRowHeight = m_arch->getRow(0)->getHeight();
  m_numSingleHeightRows = m_arch->getRows().size();

  m_rng = new Placer_RNG;
  m_rng->seed(static_cast<unsigned>(10));

  // Utilization...
  m_targetUt = 1.0;
  m_targetMaxMovement = 0.0;

  // For generating a move list...
  m_moveLimit = 10;
  m_nMoved = 0;
  m_curX.resize(m_moveLimit);
  m_curY.resize(m_moveLimit);
  m_newX.resize(m_moveLimit);
  m_newY.resize(m_moveLimit);
  m_curOri.resize(m_moveLimit);
  m_newOri.resize(m_moveLimit);
  m_curSeg.resize(m_moveLimit);
  m_newSeg.resize(m_moveLimit);
  m_movedNodes.resize(m_moveLimit);
  for (size_t i = 0; i < m_moveLimit; i++) {
    m_curSeg[i] = std::vector<int>();
    m_newSeg[i] = std::vector<int>();
  }

  // The purpose of this reverse map is to be able to remove the cell from
  // all segments that it has been placed into.  It only works (i.e., is
  // only up-to-date) if you use the proper routines to add and remove cells
  // to and from segments.
  m_reverseCellToSegs.resize(m_network->getNumNodes() );
  for (size_t i = 0; i < m_reverseCellToSegs.size(); i++) {
    m_reverseCellToSegs[i] = std::vector<DetailedSeg*>();
  }

  recordOriginalPositions();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedMgr::~DetailedMgr() {
  m_blockages.clear();

  m_segsInRow.clear();

  for (int i = 0; i < m_segments.size(); i++) {
    delete m_segments[i];
  }
  m_segments.clear();

  if (m_rng != 0) {
    delete m_rng;
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::setSeed(int seed) {
  m_logger->info(DPO, 401, "Setting random seed to {:d}.", seed);
  m_rng->seed(static_cast<unsigned>(seed));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::internalError( std::string msg )
{
    m_logger->error(DPO, 400, "Detailed improvement internal error: {:s}.", msg.c_str());
    exit(-1);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::findBlockages(bool includeRouteBlockages) {
  // Blockages come from filler, from fixed nodes (possibly with shapes) and
  // from larger macros which are now considered fixed...

  m_blockages.clear();

  // Determine the single height segments and blockages.
  m_blockages.resize(m_numSingleHeightRows);
  for (int i = 0; i < m_blockages.size(); i++) {
    m_blockages[i] = std::vector<std::pair<double, double> >();
  }

  for (int i = 0; i < m_fixedCells.size(); i++) {
    Node* nd = m_fixedCells[i];

    double xmin = std::max(m_arch->getMinX(), nd->getX() - 0.5 * nd->getWidth());
    double xmax = std::min(m_arch->getMaxX(), nd->getX() + 0.5 * nd->getWidth());
    double ymin = std::max(m_arch->getMinY(), nd->getY() - 0.5 * nd->getHeight());
    double ymax = std::min(m_arch->getMaxY(), nd->getY() + 0.5 * nd->getHeight());

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
    xmin -= m_arch->getCellSpacing(0, nd);
    xmax += m_arch->getCellSpacing(nd, 0);

    for (int r = 0; r < m_numSingleHeightRows; r++) {
      double lb = m_arch->getMinY() + r * m_singleRowHeight;
      double ub = lb + m_singleRowHeight;

      if (!(ymax - 1.0e-3 <= lb || ymin + 1.0e-3 >= ub)) {
        m_blockages[r].push_back(std::pair<double, double>(xmin, xmax));
      }
    }
  }

  if (includeRouteBlockages) {
    if (m_rt != 0) {
      // Turn M1 and M2 routing blockages into placement blockages.  The idea
      // here is to be quite conservative and prevent the possibility of pin
      // access problems.  We *ONLY* consider routing obstacles to be placement
      // obstacles if they overlap with an *ENTIRE* site.

      for (int layer = 0; layer <= 1 && layer < m_rt->m_num_layers; layer++) {
        std::vector<Rectangle>& rects = m_rt->m_layerBlockages[layer];
        for (int b = 0; b < rects.size(); b++) {
          double xmin = rects[b].xmin();
          double xmax = rects[b].xmax();
          double ymin = rects[b].ymin();
          double ymax = rects[b].ymax();

          for (int r = 0; r < m_numSingleHeightRows; r++) {
            double lb = m_arch->getMinY() + r * m_singleRowHeight;
            double ub = lb + m_singleRowHeight;

            if (ymax >= ub && ymin <= lb) {
              // Blockage overlaps with the entire row span in the Y-dir...
              // Sites are possibly completely covered!

              double originX = m_arch->getRow(r)->getLeft();
              double siteSpacing = m_arch->getRow(r)->getSiteSpacing();

              int i0 = (int)std::floor((xmin - originX) / siteSpacing);
              int i1 = (int)std::floor((xmax - originX) / siteSpacing);
              if (originX + i1 * siteSpacing != xmax) ++i1;

              if (i1 > i0) {
                m_blockages[r].push_back(std::pair<double, double>(
                    originX + i0 * siteSpacing, originX + i1 * siteSpacing));
              }
            }
          }
        }
      }
    }
  }

  // Sort blockages and merge.
  for (int r = 0; r < m_numSingleHeightRows; r++) {
    if (m_blockages[r].size() == 0) {
      continue;
    }

    std::sort(m_blockages[r].begin(), m_blockages[r].end(), compareBlockages());

    std::stack<std::pair<double, double> > s;
    s.push(m_blockages[r][0]);
    for (int i = 1; i < m_blockages[r].size(); i++) {
      std::pair<double, double> top = s.top();  // copy.
      if (top.second < m_blockages[r][i].first) {
        s.push(m_blockages[r][i]);  // new interval.
      } else {
        if (top.second < m_blockages[r][i].second) {
          top.second = m_blockages[r][i].second;  // extend interval.
        }
        s.pop();      // remove old.
        s.push(top);  // expanded interval.
      }
    }

    m_blockages[r].erase(m_blockages[r].begin(), m_blockages[r].end());
    while (!s.empty()) {
      std::pair<double, double> temp = s.top();  // copy.
      m_blockages[r].push_back(temp);
      s.pop();
    }

    // Intervals need to be sorted, but they are currently in reverse order. Can
    // either resort or reverse.
    std::sort(m_blockages[r].begin(), m_blockages[r].end(),
              compareBlockages());  // Sort to get them left to right.
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::findSegments() {
  // Create the segments into which movable cells are placed.  I do make
  // segment ends line up with sites and that segments don't extend off
  // the chip.

  for (int i = 0; i < m_segments.size(); i++) {
    delete m_segments[i];
  }
  m_segments.erase(m_segments.begin(), m_segments.end());

  int numSegments = 0;
  double x1, x2;
  m_segsInRow.resize(m_numSingleHeightRows);
  for (int r = 0; r < m_numSingleHeightRows; r++) {
    double lx = m_arch->getRow(r)->getLeft();
    double rx = m_arch->getRow(r)->getRight();

    m_segsInRow[r] = std::vector<DetailedSeg*>();

    int n = m_blockages[r].size();
    if (n == 0) {
      // Entire row free.

      x1 = std::max(m_arch->getMinX(), lx);
      x2 = std::min(m_arch->getMaxX(), rx);

      if (x2 > x1) {
        DetailedSeg* segment = new DetailedSeg();
        segment->setSegId(numSegments);
        segment->setRowId(r);
        segment->setMinX(m_arch->getMinX());
        segment->setMaxX(m_arch->getMaxX());

        m_segsInRow[r].push_back(segment);
        m_segments.push_back(segment);

        ++numSegments;
      }
    } else {
      // Divide row.
      if (m_blockages[r][0].first > std::max(m_arch->getMinX(), lx)) {
        x1 = std::max(m_arch->getMinX(), lx);
        x2 = std::min(std::min(m_arch->getMaxX(), rx), m_blockages[r][0].first);

        if (x2 > x1) {
          DetailedSeg* segment = new DetailedSeg();
          segment->setSegId(numSegments);
          segment->setRowId(r);
          segment->setMinX(x1);
          segment->setMaxX(x2);

          m_segsInRow[r].push_back(segment);
          m_segments.push_back(segment);

          ++numSegments;
        }
      }
      for (int i = 1; i < n; i++) {
        if (m_blockages[r][i].first > m_blockages[r][i - 1].second) {
          x1 = std::max(std::max(m_arch->getMinX(), lx),
                        m_blockages[r][i - 1].second);
          x2 = std::min(std::min(m_arch->getMaxX(), rx),
                        m_blockages[r][i - 0].first);

          if (x2 > x1) {
            DetailedSeg* segment = new DetailedSeg();
            segment->setSegId(numSegments);
            segment->setRowId(r);
            segment->setMinX(x1);
            segment->setMaxX(x2);

            m_segsInRow[r].push_back(segment);
            m_segments.push_back(segment);

            ++numSegments;
          }
        }
      }
      if (m_blockages[r][n - 1].second < std::min(m_arch->getMaxX(), rx)) {
        x1 = std::min(std::min(m_arch->getMaxX(), rx),
                      std::max(std::max(m_arch->getMinX(), lx),
                               m_blockages[r][n - 1].second));
        x2 = std::min(m_arch->getMaxX(), rx);

        if (x2 > x1) {
          DetailedSeg* segment = new DetailedSeg();
          segment->setSegId(numSegments);
          segment->setRowId(r);
          segment->setMinX(x1);
          segment->setMaxX(x2);

          m_segsInRow[r].push_back(segment);
          m_segments.push_back(segment);

          ++numSegments;
        }
      }
    }
  }

  // Here, we need to slice up the segments to account for regions.
  std::vector<std::vector<std::pair<double, double> > > intervals;
  for (size_t reg = 1; reg < m_arch->getRegions().size(); reg++) {
    Architecture::Region* regPtr = m_arch->getRegion(reg);

    findRegionIntervals(regPtr->getId(), intervals);

    int split = 0;

    // Now we need to "cutup" the existing segments.  How best to
    // do this???????????????????????????????????????????????????
    // Perhaps I can use the same ideas as for blockages...
    for (size_t r = 0; r < m_numSingleHeightRows; r++) {
      int n = intervals[r].size();
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
        double il = intervals[r][i].first;
        double ir = intervals[r][i].second;
        for (size_t s = 0; s < m_segsInRow[r].size(); s++) {
          DetailedSeg* segPtr = m_segsInRow[r][s];

          double sl = segPtr->getMinX();
          double sr = segPtr->getMaxX();

          // Check for no overlap.
          if (ir <= sl) continue;
          if (il >= sr) continue;

          // Case 1:
          if (il <= sl && ir >= sr) {
            segPtr->setRegId(reg);
          }
          // Case 2:
          else if (il > sl && ir >= sr) {
            ++split;

            segPtr->setMaxX(il);

            DetailedSeg* newPtr = new DetailedSeg();
            newPtr->setSegId(numSegments);
            newPtr->setRowId(r);
            newPtr->setRegId(reg);
            newPtr->setMinX(il);
            newPtr->setMaxX(sr);

            m_segsInRow[r].push_back(newPtr);
            m_segments.push_back(newPtr);

            ++numSegments;
          }
          // Case 3:
          else if (ir < sr && il <= sl) {
            ++split;

            segPtr->setMinX(ir);

            DetailedSeg* newPtr = new DetailedSeg();
            newPtr->setSegId(numSegments);
            newPtr->setRowId(r);
            newPtr->setRegId(reg);
            newPtr->setMinX(sl);
            newPtr->setMaxX(ir);

            m_segsInRow[r].push_back(newPtr);
            m_segments.push_back(newPtr);

            ++numSegments;
          }
          // Case 4:
          else if (il > sl && ir < sr) {
            ++split;
            ++split;

            segPtr->setMaxX(il);

            DetailedSeg* newPtr = new DetailedSeg();
            newPtr->setSegId(numSegments);
            newPtr->setRowId(r);
            newPtr->setRegId(reg);
            newPtr->setMinX(il);
            newPtr->setMaxX(ir);

            m_segsInRow[r].push_back(newPtr);
            m_segments.push_back(newPtr);

            ++numSegments;

            newPtr = new DetailedSeg();
            newPtr->setSegId(numSegments);
            newPtr->setRowId(r);
            newPtr->setRegId(segPtr->getRegId());
            newPtr->setMinX(ir);
            newPtr->setMaxX(sr);

            m_segsInRow[r].push_back(newPtr);
            m_segments.push_back(newPtr);

            ++numSegments;
          } else {
            internalError("Unexpected problem while constructing segments");
          }
        }
      }
    }
  }

  // Make sure segment boundaries line up with sites.
  for (int s = 0; s < m_segments.size(); s++) {
    int rowId = m_segments[s]->getRowId();

    double originX = m_arch->getRow(rowId)->getLeft();
    double siteSpacing = m_arch->getRow(rowId)->getSiteSpacing();

    int ix;

    ix = (int)((m_segments[s]->getMinX() - originX) / siteSpacing);
    if (originX + ix * siteSpacing < m_segments[s]->getMinX()) ++ix;

    if (originX + ix * siteSpacing != m_segments[s]->getMinX())
      m_segments[s]->setMinX(originX + ix * siteSpacing);

    ix = (int)((m_segments[s]->getMaxX() - originX) / siteSpacing);
    if (originX + ix * siteSpacing != m_segments[s]->getMaxX())
      m_segments[s]->setMaxX(originX + ix * siteSpacing);
  }

  // Create the structure for cells in segments.
  m_cellsInSeg.clear();
  m_cellsInSeg.resize(m_segments.size());
  for (size_t i = 0; i < m_cellsInSeg.size(); i++) {
    m_cellsInSeg[i] = std::vector<Node*>();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedSeg* DetailedMgr::findClosestSegment(Node* nd) {
  // Find the closest segment for the node.  First, we consider those segments
  // which lie in the row closest to the cell. Then, we consider segments in
  // other rows which are both above and below the closest row.  Note: We skip
  // segments that are not large enough to hold the current node - assigning a
  // node to a segment which is not wide enough is most certainly going to cause
  // problems later (we will need to re-locate the node), so try to prevent this
  // node.  Further, these sorts of problems typically happen with wide cells
  // which are difficult (a hassle) to move around since they are so large...

  int row = (nd->getY() - m_arch->getMinY()) / m_singleRowHeight;

  double hori;
  double vert;
  double dist1 = std::numeric_limits<double>::max();
  double dist2 = std::numeric_limits<double>::max();
  DetailedSeg* best1 = 0;  // closest segment...
  // closest segment which is wide enough to accomodate the cell...
  DetailedSeg* best2 = 0; 

  // Segments in the current row...
  for (int k = 0; k < m_segsInRow[row].size(); k++) {
    DetailedSeg* curr = m_segsInRow[row][k];

    // Updated for regions.
    if (nd->getRegionId() != curr->getRegId()) {
      continue;
    }

    double x1 = curr->getMinX() + 0.5 * nd->getWidth();
    double x2 = curr->getMaxX() - 0.5 * nd->getWidth();
    double xx = std::max(x1, std::min(x2, nd->getX()));

    hori = std::max(0.0, std::fabs(xx - nd->getX()));
    vert = 0.0;

    bool closer1 = (hori + vert < dist1) ? true : false;
    bool closer2 = (hori + vert < dist2) ? true : false;
    bool fits =
        (nd->getWidth() <= (curr->getMaxX() - curr->getMinX())) ? true : false;

    // Keep track of the closest segment.
    if (best1 == 0 || (best1 != 0 && closer1)) {
      best1 = curr;
      dist1 = hori + vert;
    }
    // Keep track of the closest segment which is wide enough to accomodate the
    // cell.
    if (fits && (best2 == 0 || (best2 != 0 && closer2))) {
      best2 = curr;
      dist2 = hori + vert;
    }
  }

  // Consider rows above and below the current row.
  for (int offset = 1; offset <= m_numSingleHeightRows; offset++) {
    int below = row - offset;
    vert = offset * m_singleRowHeight;

    if (below >= 0) {
      // Consider the row if we could improve on either of the best segments we
      // are recording.
      if ((vert <= dist1 || vert <= dist2)) {
        for (int k = 0; k < m_segsInRow[below].size(); k++) {
          DetailedSeg* curr = m_segsInRow[below][k];

          // Updated for regions.
          if (nd->getRegionId() != curr->getRegId()) {
            continue;
          }

          double x1 = curr->getMinX() + 0.5 * nd->getWidth();
          double x2 = curr->getMaxX() - 0.5 * nd->getWidth();
          double xx = std::max(x1, std::min(x2, nd->getX()));

          hori = std::max(0.0, std::fabs(xx - nd->getX()));

          bool closer1 = (hori + vert < dist1) ? true : false;
          bool closer2 = (hori + vert < dist2) ? true : false;
          bool fits =
              (nd->getWidth() <= (curr->getMaxX() - curr->getMinX())) ? true : false;

          // Keep track of the closest segment.
          if (best1 == 0 || (best1 != 0 && closer1)) {
            best1 = curr;
            dist1 = hori + vert;
          }
          // Keep track of the closest segment which is wide enough to
          // accomodate the cell.
          if (fits && (best2 == 0 || (best2 != 0 && closer2))) {
            best2 = curr;
            dist2 = hori + vert;
          }
        }
      }
    }

    int above = row + offset;
    vert = offset * m_singleRowHeight;

    if (above <= m_numSingleHeightRows - 1) {
      // Consider the row if we could improve on either of the best segments we
      // are recording.
      if ((vert <= dist1 || vert <= dist2)) {
        for (int k = 0; k < m_segsInRow[above].size(); k++) {
          DetailedSeg* curr = m_segsInRow[above][k];

          // Updated for regions.
          if (nd->getRegionId() != curr->getRegId()) {
            continue;
          }

          double x1 = curr->getMinX() + 0.5 * nd->getWidth();
          double x2 = curr->getMaxX() - 0.5 * nd->getWidth();
          double xx = std::max(x1, std::min(x2, nd->getX()));

          hori = std::max(0.0, std::fabs(xx - nd->getX()));

          bool closer1 = (hori + vert < dist1) ? true : false;
          bool closer2 = (hori + vert < dist2) ? true : false;
          bool fits =
              (nd->getWidth() <= (curr->getMaxX() - curr->getMinX())) ? true : false;

          // Keep track of the closest segment.
          if (best1 == 0 || (best1 != 0 && closer1)) {
            best1 = curr;
            dist1 = hori + vert;
          }
          // Keep track of the closest segment which is wide enough to
          // accomodate the cell.
          if (fits && (best2 == 0 || (best2 != 0 && closer2))) {
            best2 = curr;
            dist2 = hori + vert;
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
    Node* ndi, DetailedSeg* segPtr, double xmin, double xmax, int bot, int top,
    std::vector<DetailedSeg*>& stack,
    std::vector<std::vector<DetailedSeg*> >& candidates

) {
  stack.push_back(segPtr);
  int rowId = segPtr->getRowId();

  if (rowId < top) {
    ++rowId;
    for (size_t s = 0; s < m_segsInRow[rowId].size(); s++) {
      segPtr = m_segsInRow[rowId][s];
      double overlap =
          std::min(xmax, segPtr->getMaxX()) - std::max(xmin, segPtr->getMinX());

      if (overlap >= 1.0e-3) {
        // Must find the reduced X-interval.
        double xl = std::max(xmin, segPtr->getMinX());
        double xr = std::min(xmax, segPtr->getMaxX());
        findClosestSpanOfSegmentsDfs(ndi, segPtr, xl, xr, bot, top, stack,
                                     candidates);
      }
    }
  } else {
    // Reaching this point should imply that we have a consecutive set of
    // segments which is potentially valid for placing the cell.
    int spanned = top - bot + 1;
    if (stack.size() != spanned) {
      internalError( "Multi-height cell spans an incorrect number of segments" );
    }
    candidates.push_back(stack);
  }
  stack.pop_back();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::findClosestSpanOfSegments(
    Node* nd, std::vector<DetailedSeg*>& segments) {
  // Intended for multi-height cells...  Finds the number of rows the cell
  // spans and then attempts to find a vector of segments (in different
  // rows) into which the cell can be assigned.

  int spanned = (int)((nd->getHeight() / m_singleRowHeight) + 0.5);
  if(spanned <= 1) {
    return false;
  }

  double disp1 = std::numeric_limits<double>::max();
  double disp2 = std::numeric_limits<double>::max();

  std::vector<std::vector<DetailedSeg*> > candidates;
  std::vector<DetailedSeg*> stack;

  std::vector<DetailedSeg*> best1;  // closest.
  std::vector<DetailedSeg*> best2;  // closest that fits.

  // The efficiency of this is not good.  The information about overlapping
  // segments for multi-height cells could easily be precomputed for efficiency.
  bool flip = false;
  for (size_t r = 0; r < m_arch->getRows().size(); r++) {
    // XXX: NEW! Check power compatibility of this cell with the row.  A
    // call to this routine will check both the bottom and the top rows
    // for power compatibility.
    if (!m_arch->power_compatible(nd, m_arch->getRow(r), flip)) {
      continue;
    }

    // Scan the segments in this row and look for segments in the required
    // number of rows above and below that result in non-zero interval.
    int b = r;
    int t = r + spanned - 1;
    if (t >= m_arch->getRows().size()) {
      continue;
    }

    for (size_t sb = 0; sb < m_segsInRow[b].size(); sb++) {
      DetailedSeg* segPtr = m_segsInRow[b][sb];

      candidates.clear();
      stack.clear();

      findClosestSpanOfSegmentsDfs(nd, segPtr, segPtr->getMinX(), segPtr->getMaxX(),
                                   b, t, stack, candidates);
      if (candidates.size() == 0) {
        continue;
      }

      // Evaluate the candidate segments.  Determine the distance of the bottom
      // of the node to the bottom of the first segment.  Determine the overlap
      // in the interval in the X-direction and determine the required distance.

      for (size_t i = 0; i < candidates.size(); i++) {
        // NEW: All of the segments must have the same region ID and that region
        // ID must be the same as the region ID of the cell.  If not, then we
        // are going to violate a fence region constraint.
        bool regionsOkay = true;
        for (size_t j = 0; j < candidates[i].size(); j++) {
          DetailedSeg* segPtr = candidates[i][j];
          if (segPtr->getRegId() != nd->getRegionId()) {
            regionsOkay = false;
          }
        }

        // XXX: Should region constraints be hard or soft?  If hard, there is
        // more change for failure!
        if (!regionsOkay) {
          continue;
        }

        DetailedSeg* segPtr = candidates[i][0];
        double ymin = m_arch->getRow(segPtr->getRowId())->getBottom();
        double xmin = segPtr->getMinX();
        double xmax = segPtr->getMaxX();
        for (size_t j = 0; j < candidates[i].size(); j++) {
          segPtr = candidates[i][j];
          xmin = std::max(xmin, segPtr->getMinX());
          xmax = std::min(xmax, segPtr->getMaxX());
        }

        double dy = std::fabs(nd->getY() - 0.5 * nd->getHeight() - ymin);
        double ww = std::min(nd->getWidth(), xmax - xmin);
        double xx =
            std::max(xmin + 0.5 * ww, std::min(xmax - 0.5 * ww, nd->getX()));
        double dx = std::fabs(nd->getX() - xx);

        if (best1.size() == 0 || (dx + dy < disp1)) {
          if (1) {
            best1 = candidates[i];
            disp1 = dx + dy;
          }
        }
        if (best2.size() == 0 || (dx + dy < disp2)) {
          if (nd->getWidth() <= xmax - xmin + 1.0e-3) {
            best2 = candidates[i];
            disp2 = dx + dy;
          }
        }
      }
    }
  }

  segments.erase(segments.begin(), segments.end());
  if (best2.size() != 0) {
    segments = best2;
    return true;
  }
  if (best1.size() != 0) {
    segments = best1;
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::assignCellsToSegments(std::vector<Node*>& nodesToConsider) {
  // For the provided list of cells which are assumed movable, assign those
  // cells to segments.
  //
  // XXX: Multi height cells are assigned to multiple rows!  In other words,
  // a cell can exist in multiple rows.
  //
  // Hmmm...  What sorts of checks should be done when assigning a multi-height
  // cell to multiple rows?  This presumes that there are segments which are
  // on top of each other...  It is possible not this routine that needs to
  // be fixed, but rather the routine which finds the segments...

  // Assign cells to segments.
  int nAssigned = 0;
  double movementX = 0.;
  double movementY = 0.;
  for (int i = 0; i < nodesToConsider.size(); i++) {
    Node* nd = nodesToConsider[i];

    int nRowsSpanned = (int)((nd->getHeight() / m_singleRowHeight) + 0.5);

    if (nRowsSpanned == 1) {
      // Single height.
      DetailedSeg* segPtr = findClosestSegment(nd);
      if (segPtr == 0) {
        internalError("Unable to assign single height cell to segment" );
      }

      int rowId = segPtr->getRowId();
      int segId = segPtr->getSegId();

      // Add to segment.
      addCellToSegment(nd, segId);
      ++nAssigned;

      // Move the cell's position into the segment.  XXX: Do I even need to do
      // this?????????????????????????????????????????????????????????????????
      double x1 = segPtr->getMinX() + 0.5 * nd->getWidth();
      double x2 = segPtr->getMaxX() - 0.5 * nd->getWidth();
      double xx = std::max(x1, std::min(x2, nd->getX()));
      double yy = m_arch->getRow(rowId)->getBottom() + 0.5 * nd->getHeight();

      movementX += std::fabs(nd->getX() - xx);
      movementY += std::fabs(nd->getY() - yy);

      nd->setX(xx);
      nd->setY(yy);
    } else {
      // Multi height.
      std::vector<DetailedSeg*> segments;
      if (!findClosestSpanOfSegments(nd, segments)) {
        internalError("Unable to assign multi-height cell to segment" );
      } else {
        if (segments.size() != nRowsSpanned) {
          internalError("Unable to assign multi-height cell to segment" );
        }
        // NB: adding a cell to a segment does _not_ change its position.
        DetailedSeg* segPtr = segments[0];
        double xmin = segPtr->getMinX();
        double xmax = segPtr->getMaxX();
        for (size_t s = 0; s < segments.size(); s++) {
          segPtr = segments[s];
          xmin = std::max(xmin, segPtr->getMinX());
          xmax = std::min(xmax, segPtr->getMaxX());
          addCellToSegment(nd, segments[s]->getSegId());
        }
        ++nAssigned;

        // Move the cell's position into the segment.  XXX: Do I even need to do
        // this?????????????????????????????????????????????????????????????????
        segPtr = segments[0];
        int rowId = segPtr->getRowId();

        double x1 = xmin + 0.5 * nd->getWidth();
        double x2 = xmax - 0.5 * nd->getWidth();
        double xx = std::max(x1, std::min(x2, nd->getX()));
        double yy = m_arch->getRow(rowId)->getBottom() + 0.5 * nd->getHeight();

        movementX += std::fabs(nd->getX() - xx);
        movementY += std::fabs(nd->getY() - yy);

        nd->setX(xx);
        nd->setY(yy);
      }
    }
  }
  m_logger->info(DPO, 310,
                 "Assigned {:d} cells into segments.  Movement in X-direction "
                 "is {:f}, movement in Y-direction is {:f}.",
                 nAssigned, movementX, movementY);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeCellFromSegment(Node* nd, int seg) {
  // Removing a node from a segment means a few things...  It means: 1) removing
  // it from the cell list for the segment; 2) removing its width from the
  // segment utilization; 3) updating the required gaps between cells in the
  // segment.

  std::vector<Node*>::iterator it =
      std::find(m_cellsInSeg[seg].begin(), m_cellsInSeg[seg].end(), nd);
  if (m_cellsInSeg[seg].end() == it) {
    // Should not happen.
    internalError("Cell not found in expected segment" );
  }
  int ix = it - m_cellsInSeg[seg].begin();
  int n = m_cellsInSeg[seg].size() - 1;

  // Remove this segment from the reverse map.
  std::vector<DetailedSeg*>::iterator its =
      std::find(m_reverseCellToSegs[nd->getId()].begin(),
                m_reverseCellToSegs[nd->getId()].end(), m_segments[seg]);
  if (m_reverseCellToSegs[nd->getId()].end() == its) {
    // Should not happen.
    internalError("Cannot find segment for cell" );
  }
  m_reverseCellToSegs[nd->getId()].erase(its);

  // Determine gaps with the cell in the row and with the cell not in the row.
  Node* prev_cell = (ix == 0) ? 0 : m_cellsInSeg[seg][ix - 1];
  Node* next_cell = (ix == n) ? 0 : m_cellsInSeg[seg][ix + 1];

  double curr_gap = 0.;
  if (prev_cell) {
    curr_gap += m_arch->getCellSpacing(prev_cell, nd);
  }
  if (next_cell) {
    curr_gap += m_arch->getCellSpacing(nd, next_cell);
  }
  double next_gap = 0.;
  if (prev_cell && next_cell) {
    next_gap += m_arch->getCellSpacing(prev_cell, next_cell);
  }

  m_cellsInSeg[seg].erase(it);                // Removes the cell...
  m_segments[seg]->m_util -= nd->getWidth();  // Removes the utilization...
  m_segments[seg]->m_gapu -= curr_gap;        // Updates the required gaps...
  m_segments[seg]->m_gapu += next_gap;        // Updates the required gaps...
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeCellFromSegmentTest(Node* nd, int seg, double& util,
                                            double& gapu) {
  // Current utilization and gap requirements.  Then, determine the change...
  util = m_segments[seg]->m_util;
  gapu = m_segments[seg]->m_gapu;

  std::vector<Node*>::iterator it =
      std::find(m_cellsInSeg[seg].begin(), m_cellsInSeg[seg].end(), nd);
  if (m_cellsInSeg[seg].end() == it) {
    internalError("Cell not found in expected segment" );
  }
  int ix = it - m_cellsInSeg[seg].begin();
  int n = m_cellsInSeg[seg].size() - 1;

  // Determine gaps with the cell in the row and with the cell not in the row.
  Node* prev_cell = (ix == 0) ? 0 : m_cellsInSeg[seg][ix - 1];
  Node* next_cell = (ix == n) ? 0 : m_cellsInSeg[seg][ix + 1];

  double curr_gap = 0.;
  if (prev_cell) {
    curr_gap += m_arch->getCellSpacing(prev_cell, nd);
  }
  if (next_cell) {
    curr_gap += m_arch->getCellSpacing(nd, next_cell);
  }
  double next_gap = 0.;
  if (prev_cell && next_cell) {
    next_gap += m_arch->getCellSpacing(prev_cell, next_cell);
  }

  util -= nd->getWidth();  // Removes the utilization...
  gapu -= curr_gap;        // Updates the required gaps...
  gapu += next_gap;        // Updates the required gaps...
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::addCellToSegment(Node* nd, int seg) {
  // Adding a node to a segment means a few things...  It means:
  // 1) adding it to the SORTED cell list for the segment;
  // 2) adding its width to the segment utilization;
  // 3) adding the required gaps between cells in the segment.

  // Need to figure out where the cell goes in the sorted list...
  std::vector<Node*>::iterator it =
      std::lower_bound(m_cellsInSeg[seg].begin(), m_cellsInSeg[seg].end(),
                       nd->getX(), compareNodesX());
  if (it == m_cellsInSeg[seg].end()) {
    // Cell is at the end of the segment.

    Node* prev_cell =
        (m_cellsInSeg[seg].size() == 0) ? 0 : m_cellsInSeg[seg].back();
    double curr_gap = 0.;
    double next_gap = 0.;
    if (prev_cell) {
      next_gap += m_arch->getCellSpacing(prev_cell, nd);
    }

    m_cellsInSeg[seg].push_back(nd);            // Add the cell...
    m_segments[seg]->m_util += nd->getWidth();  // Adds the utilization...
    m_segments[seg]->m_gapu -= curr_gap;        // Updates the required gaps...
    m_segments[seg]->m_gapu += next_gap;        // Updates the required gaps...
  } else {
    int ix = it - m_cellsInSeg[seg].begin();
    Node* next_cell = m_cellsInSeg[seg][ix];
    Node* prev_cell = (ix == 0) ? 0 : m_cellsInSeg[seg][ix - 1];

    double curr_gap = 0.;
    if (prev_cell && next_cell) {
      curr_gap += m_arch->getCellSpacing(prev_cell, next_cell);
    }
    double next_gap = 0.;
    if (prev_cell) {
      next_gap += m_arch->getCellSpacing(prev_cell, nd);
    }
    if (next_cell) {
      next_gap += m_arch->getCellSpacing(nd, next_cell);
    }

    m_cellsInSeg[seg].insert(it, nd);           // Adds the cell...
    m_segments[seg]->m_util += nd->getWidth();  // Adds the utilization...
    m_segments[seg]->m_gapu -= curr_gap;        // Updates the required gaps...
    m_segments[seg]->m_gapu += next_gap;        // Updates the required gaps...
  }

  std::vector<DetailedSeg*>::iterator its =
      std::find(m_reverseCellToSegs[nd->getId()].begin(),
                m_reverseCellToSegs[nd->getId()].end(), m_segments[seg]);
  if (m_reverseCellToSegs[nd->getId()].end() != its) {
    internalError("Segment already present in cell to segment map");
  }
  int spanned = (int)(nd->getHeight() / m_singleRowHeight + 0.5);
  if (m_reverseCellToSegs[nd->getId()].size() >= spanned) {
    internalError("Cell to segment map incorrectly sized");
  }
  m_reverseCellToSegs[nd->getId()].push_back(m_segments[seg]);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::addCellToSegmentTest(Node* nd, int seg, double x,
                                       double& util, double& gapu) {
  // Determine the utilization and required gaps if node 'nd' is added to
  // segment 'seg' at position 'x'.  XXX: This routine will should *BOT* be used
  // to add a node to a segment in which it is already placed!  XXX: The code
  // does not make sense for a swap.

  // Current utilization and gap requirements.  Then, determine the change...
  util = m_segments[seg]->m_util;
  gapu = m_segments[seg]->m_gapu;

  // Need to figure out where the cell goes in the sorted list...
  std::vector<Node*>::iterator it =
      std::lower_bound(m_cellsInSeg[seg].begin(), m_cellsInSeg[seg].end(),
                       x /* tentative location. */, compareNodesX());

  if (it == m_cellsInSeg[seg].end()) {
    // No node in segment with position that is larger than the current node.
    // The current node goes at the end...

    Node* prev_cell =
        (m_cellsInSeg[seg].size() == 0) ? 0 : m_cellsInSeg[seg].back();
    double curr_gap = 0.;
    double next_gap = 0.;
    if (prev_cell) {
      next_gap += m_arch->getCellSpacing(prev_cell, nd);
    }

    util += nd->getWidth();  // Adds the utilization...
    gapu -= curr_gap;        // Updates the required gaps...
    gapu += next_gap;        // Updates the required gaps...
  } else {
    int ix = it - m_cellsInSeg[seg].begin();
    Node* next_cell = m_cellsInSeg[seg][ix];
    Node* prev_cell = (ix == 0) ? 0 : m_cellsInSeg[seg][ix - 1];

    double curr_gap = 0.;
    if (prev_cell && next_cell) {
      curr_gap += m_arch->getCellSpacing(prev_cell, next_cell);
    }
    double next_gap = 0.;
    if (prev_cell) {
      next_gap += m_arch->getCellSpacing(prev_cell, nd);
    }
    if (next_cell) {
      next_gap += m_arch->getCellSpacing(nd, next_cell);
    }

    util += nd->getWidth();  // Adds the utilization...
    gapu -= curr_gap;        // Updates the required gaps...
    gapu += next_gap;        // Updates the required gaps...
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::recordOriginalDimensions() {
  m_origW.resize(m_network->getNumNodes() );
  m_origH.resize(m_network->getNumNodes() );
  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i);
    m_origW[nd->getId()] = nd->getWidth();
    m_origH[nd->getId()] = nd->getHeight();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::restoreOriginalDimensions() {
  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i);
    nd->setWidth(m_origW[nd->getId()]);
    nd->setHeight(m_origH[nd->getId()]);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::recordOriginalPositions() {
  m_origX.resize(m_network->getNumNodes() );
  m_origY.resize(m_network->getNumNodes() );
  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i);
    m_origX[nd->getId()] = nd->getX();
    m_origY[nd->getId()] = nd->getY();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::restoreOriginalPositions() {
  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i);
    nd->setX(m_origX[nd->getId()]);
    nd->setY(m_origY[nd->getId()]);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::recordBestPositions() {
  m_bestX.resize(m_network->getNumNodes() );
  m_bestY.resize(m_network->getNumNodes() );
  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i);
    m_bestX[nd->getId()] = nd->getX();
    m_bestY[nd->getId()] = nd->getY();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::restoreBestPositions() {
  // This also required redoing the segments.
  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i);
    nd->setX(m_bestX[nd->getId()]);
    nd->setY(m_bestY[nd->getId()]);
  }
  assignCellsToSegments(m_singleHeightCells);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedMgr::measureMaximumDisplacement(bool& violated) {
  violated = false;

  // double limit = GET_PARAM_FLOAT( PLACER_MAX_DISPLACEMENT );  // XXX: Check
  // the real limit, not with the tolerance.
  double limit = 1.0e8;

  double max_disp = 0.;
  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i);
    if (nd->isTerminal() || nd->isTerminalNI() || nd->isFixed()) {
      continue;
    }

    double diffX = nd->getX() - m_origX[nd->getId()];
    double diffY = nd->getY() - m_origY[nd->getId()];

    max_disp = std::max(max_disp, std::fabs(diffX) + std::fabs(diffY));
  }

  violated = (max_disp > limit) ? true : false;

  return max_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::isNodeAlignedToRow(Node* nd) {
  // Check to see if the node is aligned to a row.

  // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the
  // bottom of the cell instead of the center of the cell.  Need to assign a
  // cell to multiple segments.

  int numRows = m_arch->getRows().size();

  double yb = nd->getY() - 0.5 * nd->getHeight();
  double yt = nd->getY() + 0.5 * nd->getHeight();

  int rb = (int)((yb - m_arch->getMinY()) / m_singleRowHeight);
  int rt = (int)((yt - m_arch->getMinY()) / m_singleRowHeight);
  rb = std::min(numRows - 1, std::max(0, rb));
  rt = std::min(numRows, std::max(0, rt));
  if (rt == rb) ++rt;

  double bot_r = m_arch->getMinY() + rb * m_singleRowHeight;
  double top_r = m_arch->getMinY() + rt * m_singleRowHeight;

  if (!(std::fabs(yb - bot_r) < 1.0e-3 && std::fabs(yt - top_r) < 1.0e-3)) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::setupObstaclesForDrc() {
  // Setup rectangular obstacles for short and pin access checks.  Do only as
  // rectangles per row and per layer.  I had used rtrees, but it wasn't working
  // any better.
  double xmin, xmax, ymin, ymax;

  m_obstacles.resize(m_arch->getRows().size());

  for (int row_id = 0; row_id < m_arch->getRows().size(); row_id++) {
    m_obstacles[row_id].resize(m_rt->m_num_layers);

    double originX = m_arch->getRow(row_id)->getLeft();
    double siteSpacing = m_arch->getRow(row_id)->getSiteSpacing();
    int numSites = m_arch->getRow(row_id)->getNumSites();

    // Blockages relevant to this row...
    for (int layer_id = 0; layer_id < m_rt->m_num_layers; layer_id++) {
      m_obstacles[row_id][layer_id].clear();

      std::vector<Rectangle>& rects = m_rt->m_layerBlockages[layer_id];
      for (int b = 0; b < rects.size(); b++) {
        // Extract obstacles which interfere with this row only.
        xmin = originX;
        xmax = originX + numSites * siteSpacing;
        ymin = m_arch->getRow(row_id)->getBottom();
        ymax = m_arch->getRow(row_id)->getTop();

        if (rects[b].xmax() <= xmin) continue;
        if (rects[b].xmin() >= xmax) continue;
        if (rects[b].ymax() <= ymin) continue;
        if (rects[b].ymin() >= ymax) continue;

        m_obstacles[row_id][layer_id].push_back(rects[b]);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkSegments(double& worst) {
  // Compute the segment utilization and gap requirements.  Compute the
  // number of violations (return value) and the worst violation.  This
  // also updates the utilizations of the segments.

  int count = 0;

  worst = 0.;
  for (int i = 0; i < m_segments.size(); i++) {
    DetailedSeg* segment = m_segments[i];

    double width = segment->getMaxX() - segment->getMinX();
    int segId = segment->getSegId();

    std::vector<Node*>& nodes = m_cellsInSeg[segId];
    std::stable_sort(nodes.begin(), nodes.end(), compareNodesX());

    double util = 0.;
    for (int k = 0; k < nodes.size(); k++) {
      util += nodes[k]->getWidth();
    }

    double gapu = 0.;
    for (int k = 1; k < nodes.size(); k++) {
      gapu += m_arch->getCellSpacing(nodes[k - 1], nodes[k - 0]);
    }

    segment->m_util = util;
    segment->m_gapu = gapu;

    if (util + gapu > std::max(0.0, width)) {
      ++count;
    }
    worst = std::max(worst, ((util + gapu) - width));
  }

  return count;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectSingleHeightCells() {
  // Routine to collect only the movable single height cells.
  //
  // XXX: This code also shifts cells to ensure that they are within the
  // placement area.  It also lines the cell up with its bottom row by
  // assuming rows are stacked continuously one on top of the other which
  // may or may not be a correct assumption.
  // Do I need to do any of this really?????????????????????????????????

  m_singleHeightCells.erase(m_singleHeightCells.begin(),
                            m_singleHeightCells.end());
  m_singleRowHeight = m_arch->getRow(0)->getHeight();
  m_numSingleHeightRows = m_arch->getRows().size();

  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i);
  
    if (nd->isTerminal() || nd->isTerminalNI() || nd->isFixed()) {
      continue;
    }
    if (m_arch->isMultiHeightCell(nd)) {
      continue;
    }

    m_singleHeightCells.push_back(nd);
  }
  m_logger->info(DPO, 318, "Collected {:d} single height cells.", m_singleHeightCells.size());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectMultiHeightCells() {
  // Routine to collect only the movable multi height cells.
  //
  // XXX: This code also shifts cells to ensure that they are within the
  // placement area.  It also lines the cell up with its bottom row by
  // assuming rows are stacked continuously one on top of the other which
  // may or may not be a correct assumption.
  // Do I need to do any of this really?????????????????????????????????

  m_multiHeightCells.erase(m_multiHeightCells.begin(),
                           m_multiHeightCells.end());
  // Just in case...  Make the matrix for holding multi-height cells at
  // least large enough to hold single height cells (although we don't
  // even bothering storing such cells in this matrix).
  m_multiHeightCells.resize(2);
  for (size_t i = 0; i < m_multiHeightCells.size(); i++) {
    m_multiHeightCells[i] = std::vector<Node*>();
  }
  m_singleRowHeight = m_arch->getRow(0)->getHeight();
  m_numSingleHeightRows = m_arch->getRows().size();

  int m_numMultiHeightCells = 0;
  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i);

    if (nd->isTerminal() || nd->isTerminalNI() || nd->isFixed() || m_arch->isSingleHeightCell(nd)) {
      continue;
    }

    int nRowsSpanned = m_arch->getCellHeightInRows(nd);

    if (nRowsSpanned >= m_multiHeightCells.size()) {
      m_multiHeightCells.resize(nRowsSpanned + 1, std::vector<Node*>());
    }
    m_multiHeightCells[nRowsSpanned].push_back(nd);
    ++m_numMultiHeightCells;
  }
  for (size_t i = 0; i < m_multiHeightCells.size(); i++) {
    if (m_multiHeightCells[i].size() == 0) {
      continue;
    }
    m_logger->info(DPO, 319, "Collected {:d} multi-height cells spanning {:d} rows.", 
        m_multiHeightCells[i].size(), i);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::moveMultiHeightCellsToFixed() {
  // Adds multi height cells to the list of fixed cells.  This is a hack
  // if we are only placing single height cells; we really should not
  // need to do this...

  // Be safe; recompute the fixed cells.
  collectFixedCells();

  for (size_t i = 0; i < m_multiHeightCells.size(); i++) {
    m_fixedCells.insert(m_fixedCells.end(), m_multiHeightCells[i].begin(),
                        m_multiHeightCells[i].end());
    // XXX: Should we mark the multi height cells are temporarily fixed???
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectFixedCells() {
  // Fixed cells are used only to create blockages which, in turn, are used to
  // create obstacles.  Obstacles are then used to create the segments into
  // which cells can be placed.
  //
  // AAK: 01-dec-2021.  I noticed an error with respect to bookshelf format
  // and the handling of TERMINAL_NI cells.  One can place movable cells on
  // top of these sorts of terminals.  Therefore, they should NOT be considered
  // as fixed, at least with respect to creating blockages.

  m_fixedMacros.erase(m_fixedMacros.begin(), m_fixedMacros.end());
  m_fixedCells.erase(m_fixedCells.begin(), m_fixedCells.end());

  // Insert filler.
  for (size_t i = 0; i < m_network->getNumFillerNodes(); i++) {
    // Filler does not count as fixed macro...
    Node* nd = m_network->getFillerNode(i);
    m_fixedCells.push_back(nd);
  }
  // Insert fixed items, shapes AND macrocells.
  for (size_t i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i);

    if (!nd->isFixed() || nd->isTerminalNI()) {
      continue;
    }

    // Fixed or macrocell (with or without shapes).
    if (m_network->getNumShapes(nd) == 0) {
      m_fixedMacros.push_back(nd);
      m_fixedCells.push_back(nd);
    } else {
      // Shape.
      for (int j = 0; j < m_network->getNumShapes(nd); j++) {
        Node* shape = m_network->getShape(nd,j);
        m_fixedMacros.push_back(shape);
        m_fixedCells.push_back(shape);
      }
    }
  }

  m_logger->info(DPO, 320, "Collected {:d} fixed cells (excluded terminal_NI).", m_fixedCells.size());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::collectWideCells() {
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

  m_wideCells.erase(m_wideCells.begin(), m_wideCells.end());
  for (int s = 0; s < m_segments.size(); s++) {
    DetailedSeg* curr = m_segments[s];

    std::vector<Node*>& nodes = m_cellsInSeg[s];
    for (int k = 0; k < nodes.size(); k++) {
      Node* ndi = nodes[k];
      if (ndi->getWidth() > curr->getMaxX() - curr->getMinX()) {
        m_wideCells.push_back(ndi);
      }
    }
  }
  m_logger->info(DPO, 321, "Collected {:d} wide cells.", m_wideCells.size());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeOverlapMinimumShift() {
  // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the
  // bottom of the cell instead of the center of the cell.  Need to assign a
  // cell to multiple segments.

  // double disp_limit = 0.98 * GET_PARAM_FLOAT( PLACER_MAX_DISPLACEMENT );
  double disp_limit = 1.0e8;

  // Simple heuristic to remove overlap amoung cells in all segments.  If we
  // cannot satisfy gap requirements, we will ignore the gaps.  If the cells
  // are just too wide, we will end up with overlap.
  //
  // We will print some warnings about different sorts of failures.

  int nWidthFails = 0;
  int nDispFails = 0;
  int nGapFails = 0;

  std::vector<double> llx;
  std::vector<double> tmp;
  std::vector<double> wid;

  std::vector<double> tarr;
  std::vector<double> treq;
  double x;
  for (int s = 0; s < m_segments.size(); s++) {
    DetailedSeg* segment = m_segments[s];

    std::vector<Node*>& nodes = m_cellsInSeg[segment->getSegId()];
    if (nodes.size() == 0) {
      continue;
    }

    std::sort(nodes.begin(), nodes.end(), compareNodesX());

    double util = 0.;
    for (int j = 0; j < nodes.size(); j++) {
      util += nodes[j]->getWidth();
    }

    double gapu = 0.;
    for (int j = 1; j < nodes.size(); j++) {
      gapu += m_arch->getCellSpacing(nodes[j - 1], nodes[j - 0]);
    }

    segment->m_util = util;
    segment->m_gapu = gapu;

    int rowId = segment->getRowId();

    double originX = m_arch->getRow(rowId)->getLeft();
    double siteSpacing = m_arch->getRow(rowId)->getSiteSpacing();

    llx.resize(nodes.size());
    tmp.resize(nodes.size());
    wid.resize(nodes.size());

    double space = segment->getMaxX() - segment->getMinX();

    for (int i = 0; i < nodes.size(); i++) {
      Node* nd = nodes[i];
      wid[i] = nd->getWidth();
    }
    // XXX: Adjust cell widths to satisfy gap constraints.  But, we prefer to
    // violate gap constraints rather than having cell overlap.  So, if an
    // increase in width will violate the available space, then simply skip the
    // width increment.
    bool failedToSatisfyGaps = false;
    for (int i = 1; i < nodes.size(); i++) {
      Node* ndl = nodes[i - 1];
      Node* ndr = nodes[i - 0];

      double gap = m_arch->getCellSpacing(ndl, ndr);
      // 'util' is the total amount of cell width (including any gaps included
      // so far...).  'gap' is the next amount of width to be included.  Skip
      // the gap if we are going to violate the space limit.
      if (gap != 0.0) {
        if (util + gap <= space) {
          wid[i - 1] += gap;
          util += gap;
        } else {
          failedToSatisfyGaps = true;
        }
      }
    }

    if (failedToSatisfyGaps) {
      ++nGapFails;
    }
    double cell_width = 0.;
    for (int i = 0; i < nodes.size(); i++) {
      cell_width += wid[i];
    }

    if (cell_width > space) {
      // Scale... now things should fit, but will overlap...
      double scale = space / cell_width;
      for (int i = 0; i < nodes.size(); i++) {
        wid[i] *= scale;
      }

      ++nWidthFails;
    }
    for (int i = 0; i < nodes.size(); i++) {
      Node* nd = nodes[i];
      llx[i] = ((int)(((nd->getX() - 0.5 * nd->getWidth()) - originX) /
                      siteSpacing)) *
                   siteSpacing +
               originX;
    }

    // Determine the leftmost and rightmost location for the edge of the cell
    // while trying to satisfy the displacement limits.  If we discover that we
    // cannot satisfy the limits, then ignore the limits and shift as little as
    // possible to remove the overlap. If we cannot satisfy the limits, then
    // it's a problem we must figure out how to address later.  Note that we
    // might have already violated the displacement limit in the Y-direction in
    // which case then "game is already over".

    tarr.resize(nodes.size());
    treq.resize(nodes.size());

    x = segment->getMinX();
    for (int i = 0; i < nodes.size(); i++) {
      Node* ndi = nodes[i];

      double limit =
          std::max(0.0, disp_limit - std::fabs(ndi->getY() - getOrigY(ndi)));
      double pos = (getOrigX(ndi) - 0.5 * ndi->getWidth()) - limit;
      pos = std::max(pos, segment->getMinX());

      long int ix = (long int)((pos - originX) / siteSpacing);
      if (originX + ix * siteSpacing < pos) ++ix;
      if (originX + ix * siteSpacing != pos) pos = originX + ix * siteSpacing;

      tarr[i] = std::max(x, pos);
      x = tarr[i] + wid[i];
    }
    x = segment->getMaxX();
    for (int i = nodes.size() - 1; i >= 0; i--) {
      Node* ndi = nodes[i];

      double limit =
          std::max(0.0, disp_limit - std::fabs(ndi->getY() - getOrigY(ndi)));
      double pos = (getOrigX(ndi) + 0.5 * ndi->getWidth()) + limit;
      pos = std::min(pos, x);

      long int ix = (long int)((pos - originX) / siteSpacing);

      if (originX + ix * siteSpacing != pos) pos = originX + ix * siteSpacing;

      treq[i] = std::min(x, pos);
      x = treq[i] - wid[i];
    }

    // For each node, if treq[i]-tarr[i] >= wid[i], then the cell will fit.  If
    // not, we cannot satisfy the displacement limit, so just shift for minimum
    // movement.
    bool okay_disp = true;
    for (int i = 0; i < nodes.size(); i++) {
      if (treq[i] - tarr[i] < wid[i]) {
        okay_disp = false;
      }
    }

    if (okay_disp) {
      // Do the shifting while observing the limits...
      x = segment->getMaxX();
      for (int i = nodes.size() - 1; i >= 0; i--) {
        // Node cannot go beyond (x-wid[i]), prefers to be at llx[i], but cannot
        // be below tarr[i].
        x = std::min(x, treq[i]);
        llx[i] = std::max(tarr[i], std::min(x - wid[i], llx[i]));
        x = llx[i];
      }
    } else {
      // Do the shifting to minimize movement from current location...

      // The leftmost position for each cell.
      x = segment->getMinX();
      for (int i = 0; i < nodes.size(); i++) {
        tmp[i] = x;
        x += wid[i];
      }
      //  The rightmost position for each cell.
      x = segment->getMaxX();
      for (int i = nodes.size() - 1; i >= 0; i--) {
        // Node cannot be beyond (x-wid[i]), prefers to be at llx[i], but cannot
        // be below tmp[i].
        llx[i] = std::max(tmp[i], std::min(x - wid[i], llx[i]));
        x = llx[i];
      }
    }

    for (int i = 0; i < nodes.size(); i++) {
      Node* ndi = nodes[i];

      ndi->setX(llx[i] + 0.5 * ndi->getWidth());

      double dx = ndi->getX() - getOrigX(ndi);
      double dy = ndi->getY() - getOrigY(ndi);
      double limit = std::fabs(dx) + std::fabs(dy);
      if (limit > disp_limit) {
        ++nDispFails;
      }
    }
  }

  return;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::cleanup() {
  // Various cleanups.
  for (int i = 0; i < m_wideCells.size(); i++) {
    Node* ndi = m_wideCells[i];
    ndi->setFixed(NodeFixed_NOT_FIXED);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkOverlapInSegments() {
  // Scan each segment and check for overlap between adjacent cells.
  // Also check for cells off the left and right edge of the segments.
  //
  // I use to check for other things too; e.g., segment utilization
  // being too large with and without gaps.  I've removed that
  // since it isn't really required.  Further, with multi-height
  // cells, we might have forced deadspace.
  //
  // I do not take into account padding when I check for overlap as
  // I consider that a different sort of issue.

  std::vector<Node*> temp;
  temp.reserve(m_network->getNumNodes() );

  int err_n = 0;
  // The following is for some printing if we need help finding a bug.
  // I don't want to print all the potential errors since that could
  // be too overwhelming.
  for (int s = 0; s < m_segments.size(); s++) {
    double xmin = m_segments[s]->getMinX();
    double xmax = m_segments[s]->getMaxX();

    // To be safe, gather cells in each segment and re-sort them.
    temp.erase(temp.begin(), temp.end());
    for (int j = 0; j < m_cellsInSeg[s].size(); j++) {
      Node* ndj = m_cellsInSeg[s][j];
      temp.push_back(ndj);
    }
    std::sort(temp.begin(), temp.end(), compareNodesX());

    for (int j = 1; j < temp.size(); j++) {
      Node* ndi = temp[j - 1];
      Node* ndj = temp[j];

      double ri = ndi->getX() + 0.5 * ndi->getWidth();
      double lj = ndj->getX() - 0.5 * ndj->getWidth();

      if (ri >= lj + 1.0e-3) {
        err_n++;
      }
    }
    for (int j = 0; j < temp.size(); j++) {
      Node* ndi = temp[j];

      double li = ndi->getX() - 0.5 * ndi->getWidth();
      double ri = ndi->getX() + 0.5 * ndi->getWidth();

      if (li <= xmin - 1.0e-3 || ri >= xmax + 1.0e-3) {
        err_n++;
      }
    }
  }

  m_logger->info(DPO, 311, "Found {:d} overlaps between adjacent cells.",
                 err_n);
  return err_n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkEdgeSpacingInSegments() {
  // Check for spacing violations according to the spacing table.  Note
  // that there might not be a spacing table in which case we will
  // return no errors.  I should also check for padding errors although
  // we might not have any paddings either! :).

  std::vector<Node*> temp;
  temp.reserve(m_network->getNumNodes() );

  double dummyPadding = 0.;
  double rightPadding = 0.;
  double leftPadding = 0.;

  int err_n = 0;
  int err_p = 0;
  for (int s = 0; s < m_segments.size(); s++) {

    // To be safe, gather cells in each segment and re-sort them.
    temp.erase(temp.begin(), temp.end());
    for (int j = 0; j < m_cellsInSeg[s].size(); j++) {
      Node* ndj = m_cellsInSeg[s][j];
      temp.push_back(ndj);
    }
    std::sort(temp.begin(), temp.end(), compareNodesL());

    for (int j = 1; j < temp.size(); j++) {
      Node* ndl = temp[j - 1];
      Node* ndr = temp[j];

      double llx_l = ndl->getX() - 0.5 * ndl->getWidth();
      double rlx_l = llx_l + ndl->getWidth();

      double llx_r = ndr->getX() - 0.5 * ndr->getWidth();
      //double rlx_r = llx_r + ndr->getWidth();

      double gap = llx_r - rlx_l;

      double spacing = m_arch->getCellSpacingUsingTable(ndl->getRightEdgeType(),
                                                        ndr->getLeftEdgeType());

      m_arch->getCellPadding(ndl, dummyPadding, rightPadding);
      m_arch->getCellPadding(ndr, leftPadding, dummyPadding);
      double padding = leftPadding + rightPadding;

      if (!(gap >= spacing - 1.0e-3)) {
        ++err_n;
      }
      if (!(gap >= padding - 1.0e-3)) {
        ++err_p;
      }
    }
  }

  m_logger->info(
      DPO, 312,
      "Found {:d} edge spacing violations and {:d} padding violations.", err_n,
      err_p);

  return err_n + err_p;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkRegionAssignment() {
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
  temp.reserve(m_network->getNumNodes() );

  int err_n = 0;
  for (int s = 0; s < m_segments.size(); s++) {

    // To be safe, gather cells in each segment and re-sort them.
    temp.erase(temp.begin(), temp.end());
    for (int j = 0; j < m_cellsInSeg[s].size(); j++) {
      Node* ndj = m_cellsInSeg[s][j];
      temp.push_back(ndj);
    }
    std::sort(temp.begin(), temp.end(), compareNodesL());

    for (int j = 0; j < temp.size(); j++) {
      Node* ndi = temp[j];
      if (ndi->getRegionId() != m_segments[s]->getRegId()) {
        ++err_n;
      }
    }
  }

  m_logger->info(DPO, 313, "Found {:d} cells in wrong regions.", err_n);

  return err_n;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkSiteAlignment() {
  // Ensure that the left edge of each cell is aligned with a site.  We only
  // consider cells that are within segments.
  int err_n = 0;

  double singleRowHeight = getSingleRowHeight();
  int nCellsInSegments = 0;
  int nCellsNotInSegments = 0;
  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i); 

    if (nd->isTerminal() || nd->isTerminalNI() || nd->isFixed()) {
      continue;
    }

    double xl = nd->getX() - 0.5 * nd->getWidth();
    double yb = nd->getY() - 0.5 * nd->getHeight();

    // Determine the spanned rows. XXX: Is this strictly correct?  It
    // assumes rows are continuous and that the bottom row lines up
    // with the bottom of the architecture.
    int rb = (int)((yb - m_arch->getMinY()) / singleRowHeight);
    int spanned = (int)((nd->getHeight() / singleRowHeight) + 0.5);
    int rt = rb + spanned - 1;

    if (m_reverseCellToSegs[nd->getId()].size() == 0) {
      ++nCellsNotInSegments;
      continue;
    } else if (m_reverseCellToSegs[nd->getId()].size() != spanned) {
      internalError( "Reverse cell map incorrectly sized." );
    }
    ++nCellsInSegments;

    if (rb < 0 || rt >= m_arch->getRows().size()) {
      // Either off the top of the bottom of the chip, so this is not
      // exactly an alignment problem, but still a problem so count it.
      ++err_n;
    }
    rb = std::max(rb, 0);
    rt = std::min(rt, (int)m_arch->getRows().size() - 1);

    for (int r = rb; r <= rt; r++) {
      double siteSpacing = m_arch->getRow(r)->getSiteSpacing();
      double originX = m_arch->getRow(r)->getLeft();

      // XXX: Should I check the site to the left and right to avoid rounding
      // errors???
      int sid = (int)(((xl - originX) / siteSpacing) + 0.5);
      double xt = originX + sid * siteSpacing;
      if (std::fabs(xl - xt) > 1.0e-3) {
        ++err_n;
      }
    }
  }
  m_logger->info(DPO, 314, "Found {:d} site alignment problems.", err_n);
  return err_n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int DetailedMgr::checkRowAlignment(int max_err_n) {
  // Ensure that the bottom of each cell is aligned with a row.
  int err_n = 0;

  double singleRowHeight = getSingleRowHeight();
  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* nd = m_network->getNode(i);

    if (nd->isTerminal() || nd->isTerminalNI() || nd->isFixed()) {
      continue;
    }
    double yb = nd->getY() - 0.5 * nd->getHeight();
    double yt = nd->getY() + 0.5 * nd->getHeight();

    // Determine the spanned rows. XXX: Is this strictly correct?  It
    // assumes rows are continuous and that the bottom row lines up
    // with the bottom of the architecture.
    int rb = (int)((yb - m_arch->getMinY()) / singleRowHeight);
    int spanned = (int)((nd->getHeight() / singleRowHeight) + 0.5);
    int rt = rb + spanned - 1;

    if (rb < 0 || rt >= m_arch->getRows().size()) {
      // Either off the top of the bottom of the chip, so this is not
      // exactly an alignment problem, but still a problem so count it.
      ++err_n;
      continue;
    }
    double y1 = m_arch->getMinY() + rb * singleRowHeight;
    double y2 = m_arch->getMinY() + rt * singleRowHeight + singleRowHeight;

    if (std::fabs(yb - y1) > 1.0e-3 || std::fabs(yt - y2) > 1.0e-3) {
      ++err_n;
    }
  }
  m_logger->info(DPO, 315, "Found {:d} row alignment problems.", err_n);
  return err_n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedMgr::getCellSpacing(Node* ndl, Node* ndr,
                                   bool checkPinsOnCells) {
  // Compute any required spacing between cells.  This could be from an edge
  // type rule, or due to adjacent pins on the cells.  Checking pins on cells is
  // more time consuming.

  if (ndl == 0 || ndr == 0) {
    return 0.0;
  }
  double spacing1 = m_arch->getCellSpacing(ndl, ndl);
  if (!checkPinsOnCells) {
    return spacing1;
  }
  double spacing2 = 0.0;
  {
    Pin* pinl = 0;
    Pin* pinr = 0;

    // Right-most pin on the left cell.
    for (int i = 0; i < ndl->getPins().size(); i++) {
      Pin* pin = ndl->getPins()[i];
      if (pinl == 0 || pin->getOffsetX() > pinl->getOffsetX()) {
        pinl = pin;
      }
    }

    // Left-most pin on the right cell.
    for (int i = 0; i < ndr->getPins().size(); i++) {
      Pin* pin = ndr->getPins()[i];
      if (pinr == 0 || pin->getOffsetX() < pinr->getOffsetX()) {
        pinr = pin;
      }
    }
    // If pins on the same layer, do something.
    if (pinl != 0 && pinr != 0 && pinl->getPinLayer() == pinr->getPinLayer()) {
      // Determine the spacing requirements between these two pins.   Then,
      // translate this into a spacing requirement between the two cells.  XXX:
      // Since it is implicit that the cells are in the same row, we can
      // determine the widest pin and the parallel run length without knowing
      // the actual location of the cells...  At least I think so...

      double xmin1 = pinl->getOffsetX() - 0.5 * pinl->getPinWidth();
      double xmax1 = pinl->getOffsetX() + 0.5 * pinl->getPinWidth();
      double ymin1 = pinl->getOffsetY() - 0.5 * pinl->getPinHeight();
      double ymax1 = pinl->getOffsetY() + 0.5 * pinl->getPinHeight();

      double xmin2 = pinr->getOffsetX() - 0.5 * pinr->getPinWidth();
      double xmax2 = pinr->getOffsetX() + 0.5 * pinr->getPinWidth();
      double ymin2 = pinr->getOffsetY() - 0.5 * pinr->getPinHeight();
      double ymax2 = pinr->getOffsetY() + 0.5 * pinr->getPinHeight();

      double ww = std::max(std::min(ymax1 - ymin1, xmax1 - xmin1),
                           std::min(ymax2 - ymin2, xmax2 - xmin2));
      double py =
          std::max(0.0, std::min(ymax1, ymax2) - std::max(ymin1, ymin2));

      spacing2 = m_rt->get_spacing(pinl->getPinLayer(), ww, py);
      double gapl = (+0.5 * ndl->getWidth()) - xmax1;
      double gapr = xmin2 - (-0.5 * ndr->getWidth());
      spacing2 = std::max(0.0, spacing2 - gapl - gapr);

      if (spacing2 > spacing1) {
        // The spacing requirement due to the routing layer is larger than the
        // spacing requirement due to the edge constraint.  Interesting.
        ;
      }
    }
  }
  return std::max(spacing1, spacing2);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::getSpaceAroundCell(int seg, int ix, double& space,
                                     double& larger, int limit) {
  // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the
  // bottom of the cell instead of the center of the cell.  Need to assign a
  // cell to multiple segments.

  Node* ndi = m_cellsInSeg[seg][ix];
  Node* ndj = 0;
  Node* ndk = 0;

  int n = m_cellsInSeg[seg].size();
  double xmin = m_segments[seg]->getMinX();
  double xmax = m_segments[seg]->getMaxX();

  // Space to the immediate left and right of the cell.
  double space_left = 0;
  if (ix == 0) {
    space_left += (ndi->getX() - 0.5 * ndi->getWidth()) - xmin;
  } else {
    --ix;
    ndj = m_cellsInSeg[seg][ix];

    space_left += (ndi->getX() - 0.5 * ndi->getWidth()) -
                  (ndj->getX() + 0.5 * ndj->getWidth());
    ++ix;
  }

  double space_right = 0;
  if (ix == n - 1) {
    space_right += xmax - (ndi->getX() + 0.5 * ndi->getWidth());
  } else {
    ++ix;
    ndj = m_cellsInSeg[seg][ix];
    space_right += (ndj->getX() - 0.5 * ndj->getWidth()) -
                   (ndi->getX() + 0.5 * ndi->getWidth());
  }
  space = space_left + space_right;

  // Space three cells 'limit' cells to the left and 'limit' cells to the right.
  if (ix < limit) {
    ndj = m_cellsInSeg[seg][0];
    larger = ndj->getX() - 0.5 * ndj->getWidth() - xmin;
  } else {
    larger = 0;
  }
  for (int j = std::max(0, ix - limit); j <= std::min(n - 1, ix + limit); j++) {
    ndj = m_cellsInSeg[seg][j];
    if (j < n - 1) {
      ndk = m_cellsInSeg[seg][j + 1];
      larger += (ndk->getX() - 0.5 * ndk->getWidth()) -
                (ndj->getX() + 0.5 * ndj->getWidth());
    } else {
      larger += xmax - (ndj->getX() + 0.5 * ndj->getWidth());
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::getSpaceAroundCell(int seg, int ix, double& space_left,
                                     double& space_right, double& large_left,
                                     double& large_right, int limit) {
  // XXX: UPDATE TO ACCOMMODATE MULTI-HEIGHT CELLS.  Likely requires using the
  // bottom of the cell instead of the center of the cell.  Need to assign a
  // cell to multiple segments.

  Node* ndi = m_cellsInSeg[seg][ix];
  Node* ndj = 0;
  Node* ndk = 0;

  int n = m_cellsInSeg[seg].size();
  double xmin = m_segments[seg]->getMinX();
  double xmax = m_segments[seg]->getMaxX();

  // Space to the immediate left and right of the cell.
  space_left = 0;
  if (ix == 0) {
    space_left += (ndi->getX() - 0.5 * ndi->getWidth()) - xmin;
  } else {
    --ix;
    ndj = m_cellsInSeg[seg][ix];
    space_left += (ndi->getX() - 0.5 * ndi->getWidth()) -
                  (ndj->getX() + 0.5 * ndj->getWidth());
    ++ix;
  }

  space_right = 0;
  if (ix == n - 1) {
    space_right += xmax - (ndi->getX() + 0.5 * ndi->getWidth());
  } else {
    ++ix;
    ndj = m_cellsInSeg[seg][ix];
    space_right += (ndj->getX() - 0.5 * ndj->getWidth()) -
                   (ndi->getX() + 0.5 * ndi->getWidth());
  }
  // Space three cells 'limit' cells to the left and 'limit' cells to the right.
  large_left = 0;
  if (ix < limit) {
    ndj = m_cellsInSeg[seg][0];
    large_left = ndj->getX() - 0.5 * ndj->getWidth() - xmin;
  }
  for (int j = std::max(0, ix - limit); j < ix; j++) {
    ndj = m_cellsInSeg[seg][j];
    ndk = m_cellsInSeg[seg][j + 1];
    large_left += (ndk->getX() - 0.5 * ndk->getWidth()) -
                  (ndj->getX() + 0.5 * ndj->getWidth());
  }
  large_right = 0;
  for (int j = ix; j <= std::min(n - 1, ix + limit); j++) {
    ndj = m_cellsInSeg[seg][j];
    if (j < n - 1) {
      ndk = m_cellsInSeg[seg][j + 1];
      large_right += (ndk->getX() - 0.5 * ndk->getWidth()) -
                     (ndj->getX() + 0.5 * ndj->getWidth());
    } else {
      large_right += xmax - (ndj->getX() + 0.5 * ndj->getWidth());
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::findRegionIntervals(
    int regId,
    std::vector<std::vector<std::pair<double, double> > >& intervals) {
  // Find intervals within each row that are spanned by the specified region.
  // We ignore the default region 0, since it is "everywhere".

  if (regId < 1 || regId >= m_arch->getRegions().size()) {
    std::cout << "Error." << std::endl;
    exit(-1);
  }

  // Initialize.
  intervals.clear();
  intervals.resize(m_numSingleHeightRows);
  for (int i = 0; i < intervals.size(); i++) {
    intervals[i] = std::vector<std::pair<double, double> >();
  }

  Architecture::Region* regPtr = m_arch->getRegion(regId);
  if (regPtr->getId() != regId) {
    std::cout << "Error." << std::endl;
    exit(-1);
  }

  // Look at the rectangles within the region.
  for (const Rectangle& rect : regPtr->getRects()) {
    double xmin = rect.xmin();
    double xmax = rect.xmax();
    double ymin = rect.ymin();
    double ymax = rect.ymax();

    for (int r = 0; r < m_numSingleHeightRows; r++) {
      double lb = m_arch->getMinY() + r * m_singleRowHeight;
      double ub = lb + m_singleRowHeight;

      if (ymax >= ub && ymin <= lb) {
        // Blockage overlaps with the entire row span in the Y-dir... Sites
        // are possibly completely covered!

        double originX = m_arch->getRow(r)->getLeft();
        double siteSpacing = m_arch->getRow(r)->getSiteSpacing();

        int i0 = (int)std::floor((xmin - originX) / siteSpacing);
        int i1 = (int)std::floor((xmax - originX) / siteSpacing);
        if (originX + i1 * siteSpacing != xmax) ++i1;

        if (i1 > i0) {
          intervals[r].push_back(std::pair<double, double>(
              originX + i0 * siteSpacing, originX + i1 * siteSpacing));
        }
      }
    }
  }

  // Sort intervals and merge.  We merge, since the region might have been
  // defined with rectangles that touch (so it is "wrong" to create an
  // artificial boundary).
  for (int r = 0; r < m_numSingleHeightRows; r++) {
    if (intervals[r].size() == 0) {
      continue;
    }

    // Sort to get intervals left to right.
    std::sort(intervals[r].begin(), intervals[r].end(), compareBlockages());

    std::stack<std::pair<double, double> > s;
    s.push(intervals[r][0]);
    for (int i = 1; i < intervals[r].size(); i++) {
      std::pair<double, double> top = s.top();  // copy.
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

    intervals[r].erase(intervals[r].begin(), intervals[r].end());
    while (!s.empty()) {
      std::pair<double, double> temp = s.top();  // copy.
      intervals[r].push_back(temp);
      s.pop();
    }

    // Sort to get them left to right.
    std::sort(intervals[r].begin(), intervals[r].end(), compareBlockages());
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeSegmentOverlapSingle(int regId) {
  // Loops over the segments.  Finds intervals of single height cells and
  // attempts to do a min shift to remove overlap.

  for (size_t s = 0; s < this->m_segments.size(); s++) {
    DetailedSeg* segPtr = this->m_segments[s];

    if (!(segPtr->getRegId() == regId || regId == -1)) {
      continue;
    }

    int segId = segPtr->getSegId();
    int rowId = segPtr->getRowId();

    double rowy = m_arch->getRow(rowId)->getCenterY();
    double left = segPtr->getMinX();
    double rite = segPtr->getMaxX();

    std::vector<Node*> nodes;
    for (size_t n = 0; n < this->m_cellsInSeg[segId].size(); n++) {
      Node* ndi = this->m_cellsInSeg[segId][n];

      int spanned = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);
      if (spanned == 1) {
        ndi->setY(rowy);
      }

      if (spanned == 1) {
        nodes.push_back(ndi);
      } else {
        // Multi-height.
        if (nodes.size() == 0) {
          left = ndi->getX() + 0.5 * ndi->getWidth();
        } else {
          rite = ndi->getX() - 0.5 * ndi->getWidth();
          // solve.
          removeSegmentOverlapSingleInner(nodes, left, rite, rowId);
          // prepare for next.
          nodes.erase(nodes.begin(), nodes.end());
          left = ndi->getX() + 0.5 * ndi->getWidth();
          rite = segPtr->getMaxX();
        }
      }
    }
    if (nodes.size() != 0) {
      // solve.
      removeSegmentOverlapSingleInner(nodes, left, rite, rowId);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeSegmentOverlapSingleInner(std::vector<Node*>& nodes_in,
                                                  double xmin, double xmax,
                                                  int rowId) {
  // Quickly remove overlap between a range of cells in a segment.  Try to
  // satisfy gaps and try to align with sites.

  std::vector<Node*> nodes = nodes_in;
  std::stable_sort(nodes.begin(), nodes.end(), compareNodesX());

  std::vector<double> llx;
  std::vector<double> tmp;
  std::vector<double> wid;

  llx.resize(nodes.size());
  tmp.resize(nodes.size());
  wid.resize(nodes.size());

  double x;
  double originX = m_arch->getRow(rowId)->getLeft();
  double siteSpacing = m_arch->getRow(rowId)->getSiteSpacing();
  int ix;

  double space = xmax - xmin;
  double util = 0.;
  double gapu = 0.;

  for (int k = 0; k < nodes.size(); k++) {
    util += nodes[k]->getWidth();
  }
  for (int k = 1; k < nodes.size(); k++) {
    gapu += m_arch->getCellSpacing(nodes[k - 1], nodes[k - 0]);
  }

  // Get width for each cell.
  for (int i = 0; i < nodes.size(); i++) {
    Node* nd = nodes[i];
    wid[i] = nd->getWidth();
  }

  // Try to get site alignment.  Adjust the left and right into which
  // we are placing cells.  If we don't need to shrink cells, then I
  // think this should work.
  {
    double tot = 0;
    for (int i = 0; i < nodes.size(); i++) {
      tot += wid[i];
    }
    if (space > tot) {
      // Try to fix the left boundary.
      ix = (int)(((xmin)-originX) / siteSpacing);
      if (originX + ix * siteSpacing < xmin) ++ix;
      x = originX + ix * siteSpacing;
      if (xmax - x >= tot + 1.0e-3 && std::fabs(xmin - x) > 1.0e-3) {
        xmin = x;
      }
      space = xmax - xmin;
    }
    if (space > tot) {
      // Try to fix the right boundary.
      ix = (int)(((xmax)-originX) / siteSpacing);
      if (originX + ix * siteSpacing > xmax) --ix;
      x = originX + ix * siteSpacing;
      if (x - xmin >= tot + 1.0e-3 && std::fabs(xmax - x) > 1.0e-3) {
        xmax = x;
      }
      space = xmax - xmin;
    }
  }

  // Try to include the necessary gap as long as we don't exceed the
  // available space.
  for (int i = 1; i < nodes.size(); i++) {
    Node* ndl = nodes[i - 1];
    Node* ndr = nodes[i - 0];

    double gap = m_arch->getCellSpacing(ndl, ndr);
    if (gap != 0.0) {
      if (util + gap <= space) {
        wid[i - 1] += gap;
        util += gap;
      }
    }
  }

  double cell_width = 0.;
  for (int i = 0; i < nodes.size(); i++) {
    cell_width += wid[i];
  }
  if (cell_width > space) {
    // Scale... now things should fit, but will overlap...
    double scale = space / cell_width;
    for (int i = 0; i < nodes.size(); i++) {
      wid[i] *= scale;
    }
  }

  // The position for the left edge of each cell; this position will be
  // site aligned.
  for (int i = 0; i < nodes.size(); i++) {
    Node* nd = nodes[i];
    ix = (int)(((nd->getX() - 0.5 * nd->getWidth()) - originX) / siteSpacing);
    llx[i] = originX + ix * siteSpacing;
  }
  // The leftmost position for the left edge of each cell.  Should also
  // be site aligned unless we had to shrink cell widths to make things
  // fit (which means overlap anyway).
  // ix = (int)(((xmin) - originX)/siteSpacing);
  // if( originX + ix*siteSpacing < xmin )
  //    ++ix;
  // x = originX + ix * siteSpacing;
  x = xmin;
  for (int i = 0; i < nodes.size(); i++) {
    tmp[i] = x;
    x += wid[i];
  }

  // The rightmost position for the left edge of each cell.  Should also
  // be site aligned unless we had to shrink cell widths to make things
  // fit (which means overlap anyway).
  // ix = (int)(((xmax) - originX)/siteSpacing);
  // if( originX + ix*siteSpacing > xmax )
  //    --ix;
  // x = originX + ix * siteSpacing;
  x = xmax;
  for (int i = nodes.size() - 1; i >= 0; i--) {
    llx[i] = std::max(tmp[i], std::min(x - wid[i], llx[i]));
    x = llx[i];  // Update rightmost position.
  }
  for (int i = 0; i < nodes.size(); i++) {
    nodes[i]->setX(llx[i] + 0.5 * nodes[i]->getWidth());
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::resortSegments() {
  // Resort the nodes in the segments.  This might be required if we did
  // something to move cells around and broke the ordering.
  for (size_t i = 0; i < m_segments.size(); i++) {
    DetailedSeg* segPtr = m_segments[i];
    resortSegment(segPtr);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::resortSegment(DetailedSeg* segPtr) {
  int segId = segPtr->getSegId();
  std::stable_sort(m_cellsInSeg[segId].begin(), m_cellsInSeg[segId].end(),
                   compareNodesX());
  segPtr->m_util = 0.;
  for (size_t n = 0; n < m_cellsInSeg[segId].size(); n++) {
    Node* ndi = m_cellsInSeg[segId][n];
    segPtr->m_util += ndi->getWidth();
  }
  segPtr->m_gapu = 0.;
  for (size_t n = 1; n < m_cellsInSeg[segId].size(); n++) {
    Node* ndl = m_cellsInSeg[segId][n - 1];
    Node* ndr = m_cellsInSeg[segId][n];
    segPtr->m_gapu += m_arch->getCellSpacing(ndl, ndr);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::removeAllCellsFromSegments() {
  // This routine removes _ALL_ cells from all segments.  It clears all
  // reverse maps and so forth.  Basically, it leaves things as if the
  // segments have all been created, but nothing has been inserted.
  for (size_t i = 0; i < m_segments.size(); i++) {
    DetailedSeg* segPtr = m_segments[i];
    int segId = segPtr->getSegId();
    m_cellsInSeg[segId].erase(m_cellsInSeg[segId].begin(),
                              m_cellsInSeg[segId].end());
    segPtr->m_util = 0.;
    segPtr->m_gapu = 0.;
  }
  for (size_t i = 0; i < m_reverseCellToSegs.size(); i++) {
    m_reverseCellToSegs[i].erase(m_reverseCellToSegs[i].begin(),
                                 m_reverseCellToSegs[i].end());
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct compare_node_segment_x {
  bool operator()(Node*& s, double i) const { return s->getX() < i; }
  bool operator()(double i, Node*& s) const { return i < s->getX(); }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::alignPos(Node* ndi, double& xi, double xl, double xr) {
  // Given a cell with a target, determine a close site aligned position
  // such that the cell falls entirely within [xl,xr].

  double originX = m_arch->getRow(0)->getLeft();
  double siteSpacing = m_arch->getRow(0)->getSiteSpacing();

  double xp;
  double w = ndi->getWidth();
  int ix;

  // Work with left edge.
  xr -= w;  // [xl,xr] is now range for left edge of cell.

  // Left edge of cell within [xl,xr] closest to target.
  xp = std::max(xl, std::min(xr, xi - 0.5 * w));

  ix = (int)((xp - originX) / siteSpacing + 0.5);
  xp = originX + ix * siteSpacing;  // Left edge aligned.

  if (xp <= xl - 1.0e-3) {
    xp += siteSpacing;
  } else if (xp >= xr + 1.0e-3) {
    xp -= siteSpacing;
  }

  if (xp <= xl - 1.0e-3 || xp >= xr + 1.0e-3) {
    return false;
  }

  // Set new target.
  xi = xp + 0.5 * w;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::shift(std::vector<Node*>& cells, std::vector<double>& tarX,
                        std::vector<double>& posX, double left, double right,
                        int segId, int rowId) {
  // Given a lis of _ordered_ cells with targets, position the cells
  // within the provided boundaries while observing spacing.
  //
  // XXX: Need to pre-allocate a maximum size for the problem to
  // avoid constant reallocation.

  double originX = m_arch->getRow(rowId)->getLeft();
  double siteSpacing = m_arch->getRow(rowId)->getSiteSpacing();
  double siteWidth = m_arch->getRow(rowId)->getSiteWidth();

  // Number of cells.
  int ncells = cells.size();

  // Sites within the provided range.
  int i0 = (int)((left - originX) / siteSpacing);
  if (originX + i0 * siteSpacing < left) {
    ++i0;
  }
  int i1 = (int)((right - originX) / siteSpacing);
  if (originX + i1 * siteSpacing + siteWidth >= right + 1.0e-3) {
    --i1;
  }
  int nsites = i1 - i0 + 1;
  int ii, jj;

  // Get cell widths while accounting for spacing/padding.  We
  // ignore spacing/padding at the ends (should adjust left
  // and right edges prior to calling).  Convert spacing into
  // number of sites.
  // Change cell widths to be in terms of number of sites.
  std::vector<int> swid;
  swid.resize(ncells);
  std::fill(swid.begin(), swid.end(), 0);
  int rsites = 0;
  for (int i = 0; i < ncells; i++) {
    Node* ndi = cells[i];
    double width = ndi->getWidth();
    if (i != ncells-1) {
      width += m_arch->getCellSpacing(ndi, cells[i+1]);
    }
    swid[i] = (int)std::ceil(width / siteSpacing);  // or siteWidth???
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
  std::vector<std::vector<std::pair<int, int> > > prev;
  std::vector<std::vector<double> > tcost;
  std::vector<std::vector<double> > cost;
  tcost.resize(nsites + 1);
  prev.resize(nsites + 1);
  cost.resize(nsites + 1);
  for (size_t i = 0; i <= nsites; i++) {
    tcost[i].resize(ncells + 1);
    prev[i].resize(ncells + 1);
    cost[i].resize(ncells + 1);

    std::fill(tcost[i].begin(), tcost[i].end(),
              std::numeric_limits<double>::max());
    std::fill(prev[i].begin(), prev[i].end(), std::make_pair(-1, -1));
    std::fill(cost[i].begin(), cost[i].end(), 0.0);
  }

  // Fill in costs of cells to sites.
  for (int j = 1; j <= ncells; j++) {
    Node* ndi = cells[j - 1];

    // Skip invalid sites.
    for (int i = 1; i <= nsites; i++) {
      // Cell will cover real sites from [site_id,site_id+width-1].

      int site_id = i0 + i - 1;
      if (site_id < site_l[j - 1] || site_id > site_r[j - 1]) {
        continue;
      }

      // Figure out cell position if cell aligned to current site.
      int x = originX + site_id * siteSpacing + 0.5 * ndi->getWidth();
      cost[i][j] = std::fabs(x - tarX[j - 1]);
    }
  }

  // Fill in total costs.
  tcost[0][0] = 0.;
  for (int j = 1; j <= ncells; j++) {
    // Width info; for indexing.
    int prev_wid = (j - 1 == 0) ? 1 : swid[j - 2];
    int curr_wid = swid[j - 1];

    for (int i = 1; i <= nsites; i++) {
      // Current site is site_id and covers [site_id,site_id+width-1].
      int site_id = i0 + i - 1;

      // Cost if site skipped.
      ii = i - 1;
      jj = j;
      {
        double c = tcost[ii][jj];
        if (c < tcost[i][j]) {
          tcost[i][j] = c;
          prev[i][j] = std::make_pair(ii, jj);
        }
      }

      // Cost if site used; avoid if invalid (too far left or right).
      ii = i - prev_wid;
      jj = j - 1;
      if (!(ii < 0 || site_id + curr_wid - 1 > i1)) {
        double c = tcost[ii][jj] + cost[i][j];
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
    std::pair<int, int> curr = std::make_pair(nsites, ncells);
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
    std::pair<int, int> curr = std::make_pair(nsites, ncells);
    while (curr.first != -1 && curr.second != -1) {
      if (curr.first == 0 && curr.second == 0) {
        break;
      }
      int curr_i = curr.first;   // Site.
      int curr_j = curr.second;  // Cell.

      if (curr_j != prev[curr_i][curr_j].second) {
        // We've placed the cell at the site.
        Node* ndi = cells[curr_j - 1];

        int ix = i0 + curr_i - 1;
        posX[curr_j - 1] = originX + ix * siteSpacing + 0.5 * ndi->getWidth();
      }

      curr = prev[curr_i][curr_j];
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::tryMove1(Node* ndi, double xi, double yi, int si, double xj,
                           double yj, int sj) {
  // Try to move a single height cell from its current position in a new
  // segment.

  if (sj == -1) {
    // Huh?
    return false;
  }
  int rj = m_segments[sj]->getRowId();

  double row_y = m_arch->getRow(rj)->getCenterY();
  if (std::fabs(yj - row_y) >= 1.0e-3) {
    yj = row_y;
  }

  m_nMoved = 0;

  if (sj == si) {
    // Same segment.
    return false;
  }
  if (ndi->getRegionId() != m_segments[sj]->getRegId()) {
    // Region compatibility.
    return false;
  }

  int spanned = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);
  if (spanned != 1) {
    // Multi-height cell.
    return false;
  }

  double util, gapu, width, change;
  double x1, x2;
  int ix, n, site_id;
  double originX = m_arch->getRow(rj)->getLeft();
  double siteSpacing = m_arch->getRow(rj)->getSiteSpacing();
  std::vector<Node*>::iterator it;

  // Find the cells to the left and to the right of the target location.
  Node* ndr = 0;
  Node* ndl = 0;
  if (m_cellsInSeg[sj].size() != 0) {
    it = std::lower_bound(m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), xj,
                          compare_node_segment_x());

    if (it == m_cellsInSeg[sj].end()) {
      // Nothing to the right of the target position.  But, there must be
      // something to the left since we know the segment is not empty.
      ndl = m_cellsInSeg[sj].back();
    } else {
      ndr = *it;
      if (it != m_cellsInSeg[sj].begin()) {
        --it;
        ndl = *it;
      }
    }
  }

  // Different cases depending on the presences of cells on the left and the
  // right.
  if (ndl == 0 && ndr == 0) {
    // No left or right cell implies an empty segment.

    if (m_cellsInSeg[sj].size() != 0) {
      return false;
    }

    // Still need to check cell spacing in case of padding.
    DetailedSeg* seg_j = m_segments[sj];
    width = seg_j->getMaxX() - seg_j->getMinX();
    util = seg_j->m_util;
    gapu = seg_j->m_gapu;

    change = ndi->getWidth();
    change += m_arch->getCellSpacing(0, ndi);
    change += m_arch->getCellSpacing(ndi, 0);

    if (util + gapu + change > width) {
      return false;
    }

    x1 = m_segments[sj]->getMinX() + m_arch->getCellSpacing(0, ndi);
    x2 = m_segments[sj]->getMaxX() - m_arch->getCellSpacing(ndi, 0);
    if (!alignPos(ndi, xj, x1, x2)) {
      return false;
    }

    if (m_nMoved >= m_moveLimit) {
      return false;
    }
    m_movedNodes[m_nMoved] = ndi;
    m_curX[m_nMoved] = ndi->getX();
    m_curY[m_nMoved] = ndi->getY();
    m_curSeg[m_nMoved].clear();
    m_curSeg[m_nMoved].push_back(si);
    m_newX[m_nMoved] = xj;
    m_newY[m_nMoved] = yj;
    m_newSeg[m_nMoved].clear();
    m_newSeg[m_nMoved].push_back(sj);
    ++m_nMoved;

    return true;
  } else if (ndl != 0 && ndr == 0) {
    // End of segment...

    DetailedSeg* seg_j = m_segments[sj];
    width = seg_j->getMaxX() - seg_j->getMinX();
    util = seg_j->m_util;
    gapu = seg_j->m_gapu;

    // Determine required space and see if segment is violated.
    change = ndi->getWidth();
    change += m_arch->getCellSpacing(ndl, ndi);
    change += m_arch->getCellSpacing(ndi, 0);

    if (util + gapu + change > width) {
      return false;
    }

    x1 = (ndl->getX() + 0.5 * ndl->getWidth()) +
         m_arch->getCellSpacing(ndl, ndi);
    x2 = m_segments[sj]->getMaxX() - m_arch->getCellSpacing(ndi, 0);
    if (!alignPos(ndi, xj, x1, x2)) {
      return false;
    }

    if (m_nMoved >= m_moveLimit) {
      return false;
    }
    m_movedNodes[m_nMoved] = ndi;
    m_curX[m_nMoved] = ndi->getX();
    m_curY[m_nMoved] = ndi->getY();
    m_curSeg[m_nMoved].clear();
    m_curSeg[m_nMoved].push_back(si);
    m_newX[m_nMoved] = xj;
    m_newY[m_nMoved] = yj;
    m_newSeg[m_nMoved].clear();
    m_newSeg[m_nMoved].push_back(sj);
    ++m_nMoved;

    // Shift cells left if necessary.
    it = std::find(m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), ndl);
    if (m_cellsInSeg[sj].end() == it) {
      // Error.
      return false;
    }
    ix = it - m_cellsInSeg[sj].begin();
    n = 0;
    while (ix >= n && (ndl->getX() + 0.5 * ndl->getWidth() +
                           m_arch->getCellSpacing(ndl, ndi) >
                       xj - 0.5 * ndi->getWidth())) {
      // Cell ndl to the left of ndi overlaps due to placement of ndi.
      // Therefore, we need to shift ndl.  If we cannot, then just fail.
      spanned = (int)(ndl->getHeight() / m_singleRowHeight + 0.5);
      if (spanned != 1) {
        return false;
      }

      xj -= 0.5 * ndi->getWidth();
      xj -= m_arch->getCellSpacing(ndl, ndi);
      xj -= 0.5 * ndl->getWidth();

      // Site alignment.
      site_id = (int)std::floor(((xj - 0.5 * ndl->getWidth()) - originX) /
                                siteSpacing);
      if (xj > originX + site_id * siteSpacing + 0.5 * ndl->getWidth()) {
        xj = originX + site_id * siteSpacing + 0.5 * ndl->getWidth();
      }

      if (m_nMoved >= m_moveLimit) {
        return false;
      }
      m_movedNodes[m_nMoved] = ndl;
      m_curX[m_nMoved] = ndl->getX();
      m_curY[m_nMoved] = ndl->getY();
      m_curSeg[m_nMoved].clear();
      m_curSeg[m_nMoved].push_back(sj);
      m_newX[m_nMoved] = xj;
      m_newY[m_nMoved] = ndl->getY();
      m_newSeg[m_nMoved].clear();
      m_newSeg[m_nMoved].push_back(sj);
      ++m_nMoved;

      // Fail if we shift off the end of a segment.
      if (xj - 0.5 * ndl->getWidth() - m_arch->getCellSpacing(0, ndl) <
          m_segments[sj]->getMinX()) {
        return false;
      }
      if (ix == n) {
        // We shifted down to the last cell... Everything must be okay!
        break;
      }

      ndi = ndl;
      ndl = m_cellsInSeg[sj][--ix];
    }

    return true;
  } else if (ndl == 0 && ndr != 0) {
    // Start of segment...

    DetailedSeg* seg_j = m_segments[sj];
    width = seg_j->getMaxX() - seg_j->getMinX();
    util = seg_j->m_util;
    gapu = seg_j->m_gapu;

    // Determine required space and see if segment is violated.
    change = 0.0;
    change += ndi->getWidth();
    change += m_arch->getCellSpacing(0, ndi);
    change += m_arch->getCellSpacing(ndi, ndr);

    if (util + gapu + change > width) {
      return false;
    }

    x1 = m_segments[sj]->getMinX() + m_arch->getCellSpacing(0, ndi);;
    x2 = (ndr->getX() - 0.5 * ndr->getWidth()) -
         m_arch->getCellSpacing(ndi, ndr);
    if (!alignPos(ndi, xj, x1, x2)) {
      return false;
    }

    if (m_nMoved >= m_moveLimit) {
      return false;
    }

    m_movedNodes[m_nMoved] = ndi;
    m_curX[m_nMoved] = ndi->getX();
    m_curY[m_nMoved] = ndi->getY();
    m_curSeg[m_nMoved].clear();
    m_curSeg[m_nMoved].push_back(si);
    m_newX[m_nMoved] = xj;
    m_newY[m_nMoved] = yj;
    m_newSeg[m_nMoved].clear();
    m_newSeg[m_nMoved].push_back(sj);
    ++m_nMoved;

    // Shift cells right if necessary...
    it = std::find(m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), ndr);
    if (m_cellsInSeg[sj].end() == it) {
      // Error.
      return false;
    }
    ix = it - m_cellsInSeg[sj].begin();
    n = m_cellsInSeg[sj].size() - 1;
    while (ix <= n && ndr->getX() - 0.5 * ndr->getWidth() -
                              m_arch->getCellSpacing(ndi, ndr) <
                          xj + 0.5 * ndi->getWidth()) {
      // Cell ndr to the right of ndi overlaps due to placement of ndi.
      // Therefore, we need to shift ndr.  If we cannot, then just fail.
      spanned = (int)(ndr->getHeight() / m_singleRowHeight + 0.5);
      if (spanned != 1) {
        return false;
      }

      xj += 0.5 * ndi->getWidth();
      xj += m_arch->getCellSpacing(ndi, ndr);
      xj += 0.5 * ndr->getWidth();

      // Site alignment.
      site_id = (int)std::ceil(((xj - 0.5 * ndr->getWidth()) - originX) /
                               siteSpacing);
      if (xj < originX + site_id * siteSpacing + 0.5 * ndr->getWidth()) {
        xj = originX + site_id * siteSpacing + 0.5 * ndr->getWidth();
      }

      if (m_nMoved >= m_moveLimit) {
        return false;
      }
      m_movedNodes[m_nMoved] = ndr;
      m_curX[m_nMoved] = ndr->getX();
      m_curY[m_nMoved] = ndr->getY();
      m_curSeg[m_nMoved].clear();
      m_curSeg[m_nMoved].push_back(sj);
      m_newX[m_nMoved] = xj;
      m_newY[m_nMoved] = ndr->getY();
      m_newSeg[m_nMoved].clear();
      m_newSeg[m_nMoved].push_back(sj);
      ++m_nMoved;

      // Fail if we shift off end of segment.
      if (xj + 0.5 * ndr->getWidth() + m_arch->getCellSpacing(ndr, 0) >
          m_segments[sj]->getMaxX()) {
        return false;
      }

      if (ix == n) {
        // We shifted down to the last cell... Everything must be okay!
        break;
      }

      ndi = ndr;
      ndr = m_cellsInSeg[sj][++ix];
    }

    return true;
  } else if (ndl != 0 && ndr != 0) {
    // Cell to both left and right of target position.

    // In middle of segment...

    DetailedSeg* seg_j = m_segments[sj];
    width = seg_j->getMaxX() - seg_j->getMinX();
    util = seg_j->m_util;
    gapu = seg_j->m_gapu;

    // Determine required space and see if segment is violated.
    change = 0.0;
    change += ndi->getWidth();
    change += m_arch->getCellSpacing(ndl, ndi);
    change += m_arch->getCellSpacing(ndi, ndr);
    change -= m_arch->getCellSpacing(ndl, ndr);

    if (util + gapu + change > width) {
      return false;
    }

    x1 = (ndl->getX() + 0.5 * ndl->getWidth()) +
         m_arch->getCellSpacing(ndl, ndi);
    x2 = (ndr->getX() - 0.5 * ndr->getWidth()) -
         m_arch->getCellSpacing(ndi, ndr);
    if (!alignPos(ndi, xj, x1, x2)) {
      return false;
    }

    if (m_nMoved >= m_moveLimit) {
      return false;
    }
    m_movedNodes[m_nMoved] = ndi;
    m_curX[m_nMoved] = ndi->getX();
    m_curY[m_nMoved] = ndi->getY();
    m_curSeg[m_nMoved].clear();
    m_curSeg[m_nMoved].push_back(si);
    m_newX[m_nMoved] = xj;
    m_newY[m_nMoved] = yj;
    m_newSeg[m_nMoved].clear();
    m_newSeg[m_nMoved].push_back(sj);
    ++m_nMoved;

    // Shift cells right if necessary...
    it = std::find(m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), ndr);
    if (m_cellsInSeg[sj].end() == it) {
      // Error.
      return false;
    }
    ix = it - m_cellsInSeg[sj].begin();
    n = m_cellsInSeg[sj].size() - 1;
    while (ix <= n && (ndr->getX() - 0.5 * ndr->getWidth() -
                           m_arch->getCellSpacing(ndi, ndr) <
                       xj + 0.5 * ndi->getWidth())) {
      // If here, it means that the cell on the right overlaps with the new
      // location for the cell we are trying to move.  ==> THE RIGHT CELL NEEDS
      // TO BE SHIFTED. We cannot handle multi-height cells right now, so if
      // this is a multi-height cell, we return falure!
      spanned = (int)(ndr->getHeight() / m_singleRowHeight + 0.5);
      if (spanned != 1) {
        return false;
      }

      xj += 0.5 * ndi->getWidth();
      xj += m_arch->getCellSpacing(ndi, ndr);
      xj += 0.5 * ndr->getWidth();

      // Site alignment.
      site_id = (int)std::ceil(((xj - 0.5 * ndr->getWidth()) - originX) /
                               siteSpacing);
      if (xj < originX + site_id * siteSpacing + 0.5 * ndr->getWidth()) {
        xj = originX + site_id * siteSpacing + 0.5 * ndr->getWidth();
      }

      if (m_nMoved >= m_moveLimit) {
        return false;
      }
      m_movedNodes[m_nMoved] = ndr;
      m_curX[m_nMoved] = ndr->getX();
      m_curY[m_nMoved] = ndr->getY();
      m_curSeg[m_nMoved].clear();
      m_curSeg[m_nMoved].push_back(sj);
      m_newX[m_nMoved] = xj;
      m_newY[m_nMoved] = ndr->getY();
      m_newSeg[m_nMoved].clear();
      m_newSeg[m_nMoved].push_back(sj);
      ++m_nMoved;

      // Fail if we go off the end of the segment.
      if (xj + 0.5 * ndr->getWidth() + m_arch->getCellSpacing(ndr, 0) >
          m_segments[sj]->getMaxX()) {
        return false;
      }

      if (ix == n) {
        // We shifted down to the last cell... Everything must be okay!
        break;
      }

      ndi = ndr;
      ndr = m_cellsInSeg[sj][++ix];
    }

    // Shift cells left if necessary...
    ndi = m_movedNodes[0];  // Reset to the original cell.
    xj = m_newX[0];
    it = std::find(m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), ndl);
    if (m_cellsInSeg[sj].end() == it) {
      std::cout << "Error." << std::endl;
      exit(-1);
    }
    ix = it - m_cellsInSeg[sj].begin();
    n = 0;
    while (ix >= n && (ndl->getX() + 0.5 * ndl->getWidth() +
                           m_arch->getCellSpacing(ndl, ndi) >
                       xj - 0.5 * ndi->getWidth())) {
      // If here, it means that the cell on the left overlaps with the new
      // location for the cell we are trying to move.  ==> THE LEFT CELL NEEDS
      // TO BE SHIFTED. We cannot handle multi-height cells right now, so if
      // this is a multi-height cell, we return falure!
      spanned = (int)(ndl->getHeight() / m_singleRowHeight + 0.5);
      if (spanned != 1) {
        return false;
      }

      xj -= 0.5 * ndi->getWidth();
      xj -= m_arch->getCellSpacing(ndl, ndi);
      xj -= 0.5 * ndl->getWidth();

      // Site alignment.
      site_id = (int)std::floor(((xj - 0.5 * ndl->getWidth()) - originX) /
                                siteSpacing);
      if (xj > originX + site_id * siteSpacing + 0.5 * ndl->getWidth()) {
        xj = originX + site_id * siteSpacing + 0.5 * ndl->getWidth();
      }

      if (m_nMoved >= m_moveLimit) {
        return false;
      }
      m_movedNodes[m_nMoved] = ndl;
      m_curX[m_nMoved] = ndl->getX();
      m_curY[m_nMoved] = ndl->getY();
      m_curSeg[m_nMoved].clear();
      m_curSeg[m_nMoved].push_back(sj);
      m_newX[m_nMoved] = xj;
      m_newY[m_nMoved] = ndl->getY();
      m_newSeg[m_nMoved].clear();
      m_newSeg[m_nMoved].push_back(sj);
      ++m_nMoved;

      // Fail if we go off the end of the segment.
      if (xj - 0.5 * ndl->getWidth() - m_arch->getCellSpacing(0, ndl) <
          m_segments[sj]->getMinX()) {
        return false;
      }
      if (ix == n) {
        // We shifted down to the last cell... Everything must be okay!
        break;
      }

      ndi = ndl;
      ndl = m_cellsInSeg[sj][--ix];
    }

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::tryMove2(Node* ndi, double xi, double yi, int si, double xj,
                           double yj, int sj) {
  // Very simple move within the same segment.

  // Nothing to move.
  m_nMoved = 0;

  if (sj != si) {
    return false;
  }

  int rj = m_segments[sj]->getRowId();

  int nn = m_cellsInSeg[si].size() - 1;

  double row_y = m_arch->getRow(rj)->getCenterY();
  if (std::fabs(yj - row_y) >= 1.0e-3) {
    yj = row_y;
  }

  std::vector<Node*>::iterator it_i;
  std::vector<Node*>::iterator it_j;
  int ix_j = -1;

  double x1, x2;
  double space_left_j, space_right_j, large_left_j, large_right_j;

  // Find closest cell to the right of the target location.
  Node* ndj = 0;
  if (m_cellsInSeg[sj].size() != 0) {
    it_j = std::lower_bound(m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(),
                            xj, compare_node_segment_x());

    if (it_j == m_cellsInSeg[sj].end()) {
      // No node in segment with position greater than xj.  So, pick the last
      // node.
      ndj = m_cellsInSeg[sj].back();
      ix_j = m_cellsInSeg[sj].size() - 1;
    } else {
      ndj = *it_j;
      ix_j = it_j - m_cellsInSeg[sj].begin();
    }
  }
  // Not finding a cell is weird, since we should at least find the cell
  // we are attempting to move.
  if (ix_j == -1 || ndj == 0) {
    return false;
  }

  // Note that it is fine if ndj is the same as ndi; we are just trying
  // to move to a new position adjacent to some block.

  // Space on each side of ndj.
  getSpaceAroundCell(sj, ix_j, space_left_j, space_right_j, large_left_j,
                     large_right_j);

  it_i = std::find(m_cellsInSeg[si].begin(), m_cellsInSeg[si].end(), ndi);

  if (ndi->getWidth() <= space_left_j) {
    // Might fit on the left, depending on spacing.
    Node* prev = (ix_j == 0) ? 0 : m_cellsInSeg[sj][ix_j - 1];
    x1 = ndj->getX() - 0.5 * ndj->getWidth() - space_left_j +
         m_arch->getCellSpacing(prev, ndi);
    x2 = ndj->getX() - 0.5 * ndj->getWidth() - m_arch->getCellSpacing(ndi, ndj);

    if (!alignPos(ndi, xj, x1, x2)) {
      return false;
    }

    if (m_nMoved >= m_moveLimit) {
      return false;
    }

    m_movedNodes[m_nMoved] = ndi;
    m_curX[m_nMoved] = ndi->getX();
    m_curY[m_nMoved] = ndi->getY();
    m_curSeg[m_nMoved].clear();
    m_curSeg[m_nMoved].push_back(si);
    m_newX[m_nMoved] = xj;
    m_newY[m_nMoved] = yj;
    m_newSeg[m_nMoved].clear();
    m_newSeg[m_nMoved].push_back(sj);
    ++m_nMoved;
    return true;
  }
  if (ndi->getWidth() <= space_right_j) {
    // Might fit on the right, depending on spacing.
    Node* next = (ix_j == nn) ? 0 : m_cellsInSeg[sj][ix_j + 1];
    x1 = ndj->getX() + 0.5 * ndj->getWidth() + m_arch->getCellSpacing(ndj, ndi);
    x2 = ndj->getX() + 0.5 * ndj->getWidth() + space_right_j -
         m_arch->getCellSpacing(ndi, next);
    if (!alignPos(ndi, xj, x1, x2)) {
      return false;
    }

    if (m_nMoved >= m_moveLimit) {
      return false;
    }
    m_movedNodes[m_nMoved] = ndi;
    m_curX[m_nMoved] = ndi->getX();
    m_curY[m_nMoved] = ndi->getY();
    m_curSeg[m_nMoved].clear();
    m_curSeg[m_nMoved].push_back(si);
    m_newX[m_nMoved] = xj;
    m_newY[m_nMoved] = yj;
    m_newSeg[m_nMoved].clear();
    m_newSeg[m_nMoved].push_back(sj);
    ++m_nMoved;

    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::tryMove3(Node* ndi, double xi, double yi, int si, double xj,
                           double yj, int sj) {
  m_nMoved = 0;
  const int move_limit = 10;

  // Code to try and move a multi-height cell to another location.  Simple
  // in that it only looks for gaps.

  double singleRowHeight = m_arch->getRow(0)->getHeight();

  std::vector<Node*>::iterator it_j;
  double xmin, xmax;

  // Ensure multi-height, although I think this code should work for single
  // height cells too.
  int spanned = (int)(ndi->getHeight() / singleRowHeight + 0.5);
  if (spanned <= 1 || spanned != m_reverseCellToSegs[ndi->getId()].size()) {
    return false;
  }

  // Need to turn the target location into a span of rows.  I'm assuming
  // rows are layered on after the other...

  int rb = (((yj - 0.5 * ndi->getHeight()) - m_arch->getRow(0)->getBottom()) /
            singleRowHeight);
  while (rb + spanned >= m_arch->getRows().size()) {
    --rb;
  }
  int rt = rb + spanned - 1;  // Cell would occupy rows [rb,rt].

  // XXX: Need to check voltages and possibly adjust up or down a bit to
  // satisfy these requirements.  Or, we could just check those requirements
  // and return false as a failed attempt.
  bool flip = false;
  if (!m_arch->power_compatible(ndi, m_arch->getRow(rb), flip)) {
    return false;
  }
  yj = m_arch->getRow(rb)->getBottom() + 0.5 * ndi->getHeight();

  // Next find the segments based on the targeted x location.  We might be
  // outside of our region or there could be a blockage.  So, we need a flag.
  std::vector<int> segs;
  for (int r = rb; r <= rt; r++) {
    bool gotSeg = false;
    for (int s = 0; s < m_segsInRow[r].size() && !gotSeg; s++) {
      DetailedSeg* segPtr = m_segsInRow[r][s];
      if (segPtr->getRegId() == ndi->getRegionId()) {
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
  xmin = -std::numeric_limits<double>::max();
  xmax = std::numeric_limits<double>::max();
  for (size_t s = 0; s < segs.size(); s++) {
    DetailedSeg* segPtr = m_segments[segs[s]];

    int segId = m_segments[segs[s]]->getSegId();
    Node* left = nullptr;
    Node* rite = nullptr;

    if (m_cellsInSeg[segId].size() != 0) {
      it_j = std::lower_bound(m_cellsInSeg[segId].begin(),
                              m_cellsInSeg[segId].end(), xj,
                              compare_node_segment_x());
      if (it_j == m_cellsInSeg[segId].end()) {
        // Nothing to the right; the last cell in the row will be on the left.
        left = m_cellsInSeg[segId].back();

        // If the cell on the left turns out to be the current cell, then we
        // can assume this cell is not there and look to the left "one cell
        // more".
        if (left == ndi) {
          if (it_j != m_cellsInSeg[segId].begin()) {
            --it_j;
            left = *it_j;
            ++it_j;
          } else {
            left = nullptr;
          }
        }
      } else {
        rite = *it_j;
        if (it_j != m_cellsInSeg[segId].begin()) {
          --it_j;
          left = *it_j;
          if (left == ndi) {
            if (it_j != m_cellsInSeg[segId].begin()) {
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

    double lx = (left == nullptr) ? segPtr->getMinX()
                                  : (left->getX() + 0.5 * left->getWidth());
    double rx = (rite == nullptr) ? segPtr->getMaxX()
                                  : (rite->getX() - 0.5 * rite->getWidth());
    if (left != nullptr) lx += m_arch->getCellSpacing(left, ndi);
    if (rite != nullptr) rx -= m_arch->getCellSpacing(ndi, rite);

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

    if (m_nMoved >= move_limit) {
      return false;
    }
    m_movedNodes[m_nMoved] = ndi;
    m_curX[m_nMoved] = ndi->getX();
    m_curY[m_nMoved] = ndi->getY();
    m_curSeg[m_nMoved].clear();
    for (size_t i = 0; i < m_reverseCellToSegs[ndi->getId()].size(); i++) {
      m_curSeg[m_nMoved].push_back(
          m_reverseCellToSegs[ndi->getId()][i]->getSegId());
    }
    m_newX[m_nMoved] = xj;
    m_newY[m_nMoved] = yj;
    m_newSeg[m_nMoved].clear();
    m_newSeg[m_nMoved].insert(m_newSeg[m_nMoved].end(), segs.begin(),
                              segs.end());
    ++m_nMoved;

    return true;
  } 
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedMgr::trySwap1(Node* ndi, double xi, double yi, int si, double xj,
                           double yj, int sj) {
  // Tries to swap two single height cells.  Swapped cell is one near
  // the target location.  Does not do any shifting so only ndi and
  // ndj are involved.

  const int move_limit = 10;
  m_nMoved = 0;

  std::vector<Node*>::iterator it_i;
  std::vector<Node*>::iterator it_j;
  int ix_i = -1;
  int ix_j = -1;
  Node* prev;
  Node* next;

  Node* ndj = 0;
  if (m_cellsInSeg[sj].size() != 0) {
    it_j = std::lower_bound(m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(),
                            xj, compare_node_segment_x());
    if (it_j == m_cellsInSeg[sj].end()) {
      ndj = m_cellsInSeg[sj].back();
    } else {
      ndj = *it_j;
    }
  }
  if (ndj == ndi) {
    ndj = 0;
  }

  if (ndi == 0 || ndj == 0) {
    return false;
  }

  int spanned_i = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);
  int spanned_j = (int)(ndj->getHeight() / m_singleRowHeight + 0.5);
  if (spanned_i != 1 || spanned_j != 1) {
    return false;
  }

  // Find space around both of the cells.
  double space_left_i, space_right_i, large_left_i, large_right_i;
  double space_left_j, space_right_j, large_left_j, large_right_j;

  it_i = std::find(m_cellsInSeg[si].begin(), m_cellsInSeg[si].end(), ndi);
  ix_i = it_i - m_cellsInSeg[si].begin();

  it_j = std::find(m_cellsInSeg[sj].begin(), m_cellsInSeg[sj].end(), ndj);
  ix_j = it_j - m_cellsInSeg[sj].begin();

  getSpaceAroundCell(sj, ix_j, space_left_j, space_right_j, large_left_j,
                     large_right_j);
  getSpaceAroundCell(si, ix_i, space_left_i, space_right_i, large_left_i,
                     large_right_i);

  double x1, x2, x3, x4;

  int ni = m_cellsInSeg[si].size() - 1;
  int nj = m_cellsInSeg[sj].size() - 1;

  bool adjacent = false;
  if ((si == sj) && (ix_i + 1 == ix_j || ix_j + 1 == ix_i)) {
    adjacent = true;
  }

  if (!adjacent) {
    // Cells are not adjacent.  Therefore we only need to look at the
    // size of the holes created and see if each cell can fit into
    // the gap created by removal of the other cell.
    double hole_i = ndi->getWidth() + space_left_i + space_right_i;
    double hole_j = ndj->getWidth() + space_left_j + space_right_j;

    // Need to take into account edge spacing.
    prev = (ix_i == 0) ? 0 : m_cellsInSeg[si][ix_i - 1];
    next = (ix_i == ni) ? 0 : m_cellsInSeg[si][ix_i + 1];
    double need_j = ndj->getWidth();
    need_j += m_arch->getCellSpacing(prev, ndj);
    need_j += m_arch->getCellSpacing(ndj, next);

    prev = (ix_j == 0) ? 0 : m_cellsInSeg[sj][ix_j - 1];
    next = (ix_j == nj) ? 0 : m_cellsInSeg[sj][ix_j + 1];
    double need_i = ndi->getWidth();
    need_i += m_arch->getCellSpacing(prev, ndi);
    need_i += m_arch->getCellSpacing(ndi, next);

    if (!(need_j <= hole_i + 1.0e-3 && need_i <= hole_j + 1.0e-3)) {
      return false;
    }

    // Need to refine and align positions.
    prev = (ix_j == 0) ? 0 : m_cellsInSeg[sj][ix_j - 1];
    next = (ix_j == nj) ? 0 : m_cellsInSeg[sj][ix_j + 1];

    x1 = (ndj->getX() - 0.5 * ndj->getWidth()) - space_left_j +
         m_arch->getCellSpacing(prev, ndi);
    x2 = (ndj->getX() + 0.5 * ndj->getWidth()) + space_right_j -
         m_arch->getCellSpacing(ndi, next);
    if (!alignPos(ndi, xj, x1, x2)) {
      return false;
    }

    prev = (ix_i == 0) ? 0 : m_cellsInSeg[si][ix_i - 1];
    next = (ix_i == ni) ? 0 : m_cellsInSeg[si][ix_i + 1];

    x3 = (ndi->getX() - 0.5 * ndi->getWidth()) - space_left_i +
         m_arch->getCellSpacing(prev, ndj);
    x4 = (ndi->getX() + 0.5 * ndi->getWidth()) + space_right_i -
         m_arch->getCellSpacing(ndj, next);
    if (!alignPos(ndj, xi, x3, x4)) {
      return false;
    }

    if (m_nMoved >= move_limit) {
      return false;
    }
    m_movedNodes[m_nMoved] = ndi;
    m_curX[m_nMoved] = ndi->getX();
    m_curY[m_nMoved] = ndi->getY();
    m_curSeg[m_nMoved].clear();
    m_curSeg[m_nMoved].push_back(si);
    m_newX[m_nMoved] = xj;
    m_newY[m_nMoved] = ndj->getY();
    m_newSeg[m_nMoved].clear();
    m_newSeg[m_nMoved].push_back(sj);
    ++m_nMoved;

    if (m_nMoved >= move_limit) {
      return false;
    }
    m_movedNodes[m_nMoved] = ndj;
    m_curX[m_nMoved] = ndj->getX();
    m_curY[m_nMoved] = ndj->getY();
    m_curSeg[m_nMoved].clear();
    m_curSeg[m_nMoved].push_back(sj);
    m_newX[m_nMoved] = xi;
    m_newY[m_nMoved] = ndi->getY();
    m_newSeg[m_nMoved].clear();
    m_newSeg[m_nMoved].push_back(si);
    ++m_nMoved;

    return true;
  } else {
    // Same row and adjacent.
    if (ix_i + 1 == ix_j) {
      // cell i is left of cell j.
      double hole = ndi->getWidth() + ndj->getWidth();
      hole += space_right_j;
      hole += space_left_i;
      double gap = (ndj->getX() - 0.5 * ndj->getWidth()) -
                   (ndi->getX() + 0.5 * ndi->getWidth());
      hole += gap;

      Node* prev = (ix_i == 0) ? nullptr : m_cellsInSeg[si][ix_i - 1];
      Node* next = (ix_j == nj) ? nullptr : m_cellsInSeg[sj][ix_j + 1];
      double need = ndi->getWidth() + ndj->getWidth();
      need += m_arch->getCellSpacing(prev, ndj);
      need += m_arch->getCellSpacing(ndi, next);
      need += m_arch->getCellSpacing(ndj, ndi);

      if (!(need <= hole + 1.0e-3)) {
        return false;
      }

      double left = ndi->getX() - 0.5 * ndi->getWidth() - space_left_i +
                    m_arch->getCellSpacing(prev, ndj);
      double right = ndj->getX() + 0.5 * ndj->getWidth() + space_right_j -
                     m_arch->getCellSpacing(ndi, next);
      std::vector<Node*> cells;
      std::vector<double> tarX;
      std::vector<double> posX;
      cells.push_back(ndj);
      tarX.push_back(xi);
      posX.push_back(0.);
      cells.push_back(ndi);
      tarX.push_back(xj);
      posX.push_back(0.);
      if (shift(cells, tarX, posX, left, right, si, m_segments[si]->getRowId()) ==
          false) {
        return false;
      }
      xi = posX[0];
      xj = posX[1];

    } else if (ix_j + 1 == ix_i) {
      // cell j is left of cell i.
      double hole = ndi->getWidth() + ndj->getWidth();
      hole += space_right_i;
      hole += space_left_j;
      hole += (ndi->getX() - 0.5 * ndi->getWidth()) -
              (ndj->getX() + 0.5 * ndj->getWidth());

      Node* prev = (ix_j == 0) ? nullptr : m_cellsInSeg[sj][ix_j - 1];
      Node* next = (ix_i == ni) ? nullptr : m_cellsInSeg[si][ix_i + 1];
      double need = ndi->getWidth() + ndj->getWidth();
      need += m_arch->getCellSpacing(prev, ndi);
      need += m_arch->getCellSpacing(ndj, next);
      need += m_arch->getCellSpacing(ndi, ndj);

      if (!(need <= hole + 1.0e-3)) {
        return false;
      }

      double left = ndj->getX() - 0.5 * ndj->getWidth() - space_left_j +
                    m_arch->getCellSpacing(prev, ndi);
      double right = ndi->getX() + 0.5 * ndi->getWidth() + space_right_i -
                     m_arch->getCellSpacing(ndj, next);
      std::vector<Node*> cells;
      std::vector<double> tarX;
      std::vector<double> posX;
      cells.push_back(ndi);
      tarX.push_back(xj);
      posX.push_back(0.);
      cells.push_back(ndj);
      tarX.push_back(xi);
      posX.push_back(0.);
      if (shift(cells, tarX, posX, left, right, si, m_segments[si]->getRowId()) ==
          false) {
        return false;
      }
      xj = posX[0];
      xi = posX[1];
    } else {
      // Shouldn't get here.
      return false;
    }

    if (m_nMoved >= move_limit) {
      return false;
    }
    m_movedNodes[m_nMoved] = ndi;
    m_curX[m_nMoved] = ndi->getX();
    m_curY[m_nMoved] = ndi->getY();
    m_curSeg[m_nMoved].clear();
    m_curSeg[m_nMoved].push_back(si);
    m_newX[m_nMoved] = xj;
    m_newY[m_nMoved] = ndj->getY();
    m_newSeg[m_nMoved].clear();
    m_newSeg[m_nMoved].push_back(sj);
    ++m_nMoved;

    if (m_nMoved >= move_limit) {
      return false;
    }
    m_movedNodes[m_nMoved] = ndj;
    m_curX[m_nMoved] = ndj->getX();
    m_curY[m_nMoved] = ndj->getY();
    m_curSeg[m_nMoved].clear();
    m_curSeg[m_nMoved].push_back(sj);
    m_newX[m_nMoved] = xi;
    m_newY[m_nMoved] = ndi->getY();
    m_newSeg[m_nMoved].clear();
    m_newSeg[m_nMoved].push_back(si);
    ++m_nMoved;

    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::acceptMove() {
  // Moves stored list of cells.  XXX: Only single height cells.

  for (int i = 0; i < m_nMoved; i++) {
    Node* ndi = m_movedNodes[i];

    // Remove node from current segment.
    for (size_t s = 0; s < m_curSeg[i].size(); s++) {
      this->removeCellFromSegment(ndi, m_curSeg[i][s]);
    }

    // Update position and orientation.
    ndi->setX(m_newX[i]);
    ndi->setY(m_newY[i]);
    // XXX: Need to do the orientiation.
    ;

    // Insert into new segment.
    for (size_t s = 0; s < m_newSeg[i].size(); s++) {
      this->addCellToSegment(ndi, m_newSeg[i][s]);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::rejectMove() {
  m_nMoved = 0;  // Sufficient, I think.
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedMgr::debugSegments() {
  // Do some debug checks on segments.

  // Confirm the segments are sorted.
  int err1 = 0;
  for (size_t s = 0; s < m_segments.size(); s++) {
    DetailedSeg* segPtr = m_segments[s];
    int segId = segPtr->getSegId();
    for (size_t i = 1; i < m_cellsInSeg[segId].size(); i++) {
      Node* prev = m_cellsInSeg[segId][i - 1];
      Node* curr = m_cellsInSeg[segId][i];
      if (prev->getX() >= curr->getX() + 1.0e-3) {
        ++err1;
      }
    }
  }

  // Confirm that cells are in their mapped segments, as well as the revers.
  int err2 = 0;
  for (size_t i = 0; i < m_network->getNumNodes() ; i++) {
    Node* ndi = m_network->getNode(i);
    for (size_t s = 0; s < m_reverseCellToSegs[ndi->getId()].size(); s++) {
      DetailedSeg* segPtr = m_reverseCellToSegs[ndi->getId()][s];
      int segId = segPtr->getSegId();

      std::vector<Node*>::iterator it = std::find(
          m_cellsInSeg[segId].begin(), m_cellsInSeg[segId].end(), ndi);
      if (m_cellsInSeg[segId].end() == it) {
        ++err2;
      }
    }
  }
  std::vector<std::vector<DetailedSeg*> > reverse;
  reverse.resize(m_network->getNumNodes() );
  int err3 = 0;
  for (size_t s = 0; s < m_segments.size(); s++) {
    DetailedSeg* segPtr = m_segments[s];
    int segId = segPtr->getSegId();
    for (size_t i = 0; i < m_cellsInSeg[segId].size(); i++) {
      Node* ndi = m_cellsInSeg[segId][i];
      reverse[ndi->getId()].push_back(segPtr);
      std::vector<DetailedSeg*>::iterator it =
          std::find(m_reverseCellToSegs[ndi->getId()].begin(),
                    m_reverseCellToSegs[ndi->getId()].end(), segPtr);
      if (m_reverseCellToSegs[ndi->getId()].end() == it) {
        ++err3;
      }
    }
  }
  int err4 = 0;
  for (size_t i = 0; i < m_network->getNumNodes() ; i++) {
    Node* ndi = m_network->getNode(i);
    if (m_reverseCellToSegs[ndi->getId()].size() !=
        reverse[ndi->getId()].size()) {
      ++err4;
      std::stable_sort(m_reverseCellToSegs[ndi->getId()].begin(),
                       m_reverseCellToSegs[ndi->getId()].end());
      std::stable_sort(reverse[ndi->getId()].begin(),
                       reverse[ndi->getId()].end());
      for (size_t s = 0; s < m_reverseCellToSegs[ndi->getId()].size(); s++) {
        if (m_reverseCellToSegs[ndi->getId()][s] != reverse[ndi->getId()][s]) {
          ++err4;
        }
      }
    }
  }

  if(err1 != 0 || err2 != 0 || err3 != 0 || err4 != 0 )
  {
    m_logger->warn(DPO, 316, "Segment check: {:d} unsorted segments, {:d} cells not in mapped segment, {:d} cells not in map, {:d} incorrect cell map size.", err1, err2, err3, err4 );
  }
}

}  // namespace dpo
