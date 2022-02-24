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

///////////////////////////////////////////////////////////////////////////////
// Description:
//
// A simple legalizer used to populate data structures prior to detailed
// placement.

//////////////////////////////////////////////////////////////////////////////
// Includes.
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <boost/format.hpp>
#include <deque>
#include <iostream>
#include <set>
#include <vector>
#include "utl/Logger.h"
#include "detailed_manager.h"
#include "detailed_segment.h"
#include "legalize_shift.h"

using utl::DPO;

namespace dpo {

class ShiftLegalizer::Clump
{
 public:
  int m_id = 0;
  double m_weight = 0.0;
  double m_wposn = 0.0;
  // Left edge of clump should be integer, although
  // the computation can be double.
  int m_posn = 0;
  std::vector<Node*> m_nodes;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ShiftLegalizer::ShiftLegalizer(ShiftLegalizerParams& params)
    : m_params(params), m_mgr(0), m_arch(0), m_network(0), m_rt(0) {}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ShiftLegalizer::~ShiftLegalizer() {}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool ShiftLegalizer::legalize(DetailedMgr& mgr) {
  // The intention of this legalizer is to simply snap cells into
  // their closest segments and then do a "clumping" to remove
  // any overlap.  The "real intention" is to simply take an
  // existing legal placement and populate the data structures
  // used for detailed placement.
  //
  // When the snapping and shifting occurs, there really should
  // be no displacement at all.

  m_mgr = &mgr;

  m_arch = m_mgr->getArchitecture();
  m_network = m_mgr->getNetwork();
  m_rt = m_mgr->getRoutingParams();

  // Categorize the cells and create the different segments.
  mgr.collectFixedCells();
  mgr.collectSingleHeightCells();
  mgr.collectMultiHeightCells();
  mgr.collectWideCells();  // XXX: This requires segments!
  mgr.findBlockages(false); // Exclude routing blockages.
  mgr.findSegments();

  std::vector<std::pair<double, double> > origPos;
  origPos.resize(m_network->getNumNodes() );
  for (int i = 0; i < m_network->getNumNodes() ; i++) {
    Node* ndi = m_network->getNode(i) ;
    origPos[ndi->getId()] = std::make_pair(ndi->getLeft(), ndi->getBottom());
  }

  bool retval = true;
  bool isDisp = false;

  // Note: In both the "snap" and the "shift", if the placement
  // is already legal, there should be no displacement.  I
  // should issue a warning if there is any sort of displacement.
  // However, the real goal or check is whether there are any
  // problems with alignment, etc.

  std::vector<Node*> cells;  // All movable cells.
  // Snap.
  if (mgr.m_singleHeightCells.size() != 0) {
    mgr.assignCellsToSegments(mgr.m_singleHeightCells);

    cells.insert(cells.end(), mgr.m_singleHeightCells.begin(),
                 mgr.m_singleHeightCells.end());
  }
  for (size_t i = 2; i < mgr.m_multiHeightCells.size(); i++) {
    if (mgr.m_multiHeightCells[i].size() != 0) {
      mgr.assignCellsToSegments(mgr.m_multiHeightCells[i]);

      cells.insert(cells.end(), mgr.m_multiHeightCells[i].begin(),
                   mgr.m_multiHeightCells[i].end());
    }
  }

  // Check for displacement after the snap.  Shouldn't be any.
  for (size_t i = 0; i < cells.size(); i++) {
    Node* ndi = cells[i];

    double dx = std::fabs(ndi->getLeft() - origPos[ndi->getId()].first);
    double dy = std::fabs(ndi->getBottom() - origPos[ndi->getId()].second);
    if (dx > 1.0e-3 || dy > 1.0e-3) {
      isDisp = true;
    }
  }

  // Topological order - required for a shift.
  size_t size = m_network->getNumNodes() ;
  m_incoming.resize(size);
  m_outgoing.resize(size);
  for (size_t i = 0; i < mgr.m_segments.size(); i++) {
    int segId = mgr.m_segments[i]->getSegId();
    for (size_t j = 1; j < mgr.m_cellsInSeg[segId].size(); j++) {
      Node* prev = mgr.m_cellsInSeg[segId][j - 1];
      Node* curr = mgr.m_cellsInSeg[segId][j];

      m_incoming[curr->getId()].push_back(prev->getId());
      m_outgoing[prev->getId()].push_back(curr->getId());
    }
  }
  std::vector<bool> visit(size, false);
  std::vector<int> count(size, 0);
  std::vector<Node*> order;
  order.reserve(size);
  for (size_t i = 0; i < cells.size(); i++) {
    Node* ndi = cells[i];
    count[ndi->getId()] = (int)m_incoming[ndi->getId()].size();
    if (count[ndi->getId()] == 0) {
      visit[ndi->getId()] = true;
      order.push_back(ndi);
    }
  }
  for (size_t i = 0; i < order.size(); i++) {
    Node* ndi = order[i];
    for (size_t j = 0; j < m_outgoing[ndi->getId()].size(); j++) {
      Node* ndj = m_network->getNode(m_outgoing[ndi->getId()][j]);

      --count[ndj->getId()];
      if (count[ndj->getId()] == 0) {
        visit[ndj->getId()] = true;
        order.push_back(ndj);
      }
    }
  }
  if (order.size() != cells.size()) {
    // This is a fatal error!
    m_mgr->internalError("Cells incorrectly ordered during shifting");
  }

  // Shift.
  shift(cells);

  // Check for displacement after the shift.  Shouldn't be any.
  for (size_t i = 0; i < cells.size(); i++) {
    Node* ndi = cells[i];

    double dx = std::fabs(ndi->getLeft() - origPos[ndi->getId()].first);
    double dy = std::fabs(ndi->getBottom() - origPos[ndi->getId()].second);
    if (dx > 1.0e-3 || dy > 1.0e-3) {
      isDisp = true;
    }
  }

  if (isDisp) {
    m_mgr->getLogger()->warn(DPO, 200, "Unexpected displacement during legalization.");

    retval = false;
  }

  // Check.  If any of out internal checks fail, print
  // some sort of warning.
  int err1 = mgr.checkRegionAssignment();
  int err2 = mgr.checkRowAlignment();
  int err3 = mgr.checkSiteAlignment();
  int err4 = mgr.checkOverlapInSegments();
  int err5 = mgr.checkEdgeSpacingInSegments();

  // Good place to issue some sort of warning.
  if (err1 != 0 || err2 != 0 || err3 != 0 || err4 != 0 || err5 != 0) {
    m_mgr->getLogger()->warn(DPO, 201, "Placement check failure during legalization.");

    retval = false;
  }

  return retval;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
double ShiftLegalizer::shift(std::vector<Node*>& cells) {
  // Do some setup and then shift cells to reduce movement from
  // original positions.  If no overlap, this should do nothing.
  //
  // Note: I don't even try to correct for site alignment.  I'll
  // print a warning, but will otherwise continue.

  int nnodes = m_network->getNumNodes();
  int nsegs = m_mgr->getNumSegments();

  std::vector<Node*>::iterator it;


  // We need to add dummy cells to the left and the
  // right of every segment.
  m_dummiesLeft.resize(nsegs);
  for (int i = 0; i < nsegs; i++) {
    DetailedSeg* segPtr = m_mgr->m_segments[i];

    int rowId = segPtr->getRowId();

    Node* ndi = new Node();

    ndi->setId(nnodes + i);
    ndi->setLeft(segPtr->getMinX());
    ndi->setBottom(m_arch->getRow(rowId)->getBottom());
    ndi->setWidth(0.0);
    ndi->setHeight(m_arch->getRow(rowId)->getHeight());

    m_dummiesLeft[i] = ndi;
  }

  m_dummiesRight.resize(nsegs);
  for (int i = 0; i < nsegs; i++) {
    DetailedSeg* segPtr = m_mgr->m_segments[i];

    int rowId = segPtr->getRowId();

    Node* ndi = new Node();

    ndi->setId(nnodes + nsegs + i);
    ndi->setLeft(segPtr->getMaxX());
    ndi->setBottom(m_arch->getRow(rowId)->getBottom());
    ndi->setWidth(0.0);
    ndi->setHeight(m_arch->getRow(rowId)->getHeight());

    m_dummiesRight[i] = ndi;
  }

  // Jam all the left dummies nodes into segments.
  for (int i = 0; i < nsegs; i++) {
    m_mgr->m_cellsInSeg[i].insert(m_mgr->m_cellsInSeg[i].begin(),
                                  m_dummiesLeft[i]);
  }

  // Jam all the right dummies nodes into segments.
  for (int i = 0; i < nsegs; i++) {
    m_mgr->m_cellsInSeg[i].push_back(m_dummiesRight[i]);
  }

  // Need to create the "graph" prior to clumping.
  m_outgoing.resize(nnodes + 2 * nsegs);
  m_incoming.resize(nnodes + 2 * nsegs);
  m_ptr.resize(nnodes + 2 * nsegs);
  m_offset.resize(nnodes + 2 * nsegs);

  for (size_t i = 0; i < m_incoming.size(); i++) {
    m_incoming[i].clear();
    m_outgoing[i].clear();
  }
  for (size_t i = 0; i < m_mgr->m_segments.size(); i++) {
    int segId = m_mgr->m_segments[i]->getSegId();
    for (size_t j = 1; j < m_mgr->m_cellsInSeg[segId].size(); j++) {
      Node* prev = m_mgr->m_cellsInSeg[segId][j - 1];
      Node* curr = m_mgr->m_cellsInSeg[segId][j];

      m_incoming[curr->getId()].push_back(prev->getId());
      m_outgoing[prev->getId()].push_back(curr->getId());
    }
  }

  // Clump everything; This does the X- and Y-direction.  Note
  // that the vector passed to the clumping is only the
  // movable cells; the clumping knows about the dummy cells
  // on the left and the right.
  double retval = clump(cells);

  bool isError = false;
  // Remove all the dummies from the segments.
  for (int i = 0; i < nsegs; i++) {
    // Should _at least_ be the left and right dummies.
    if (m_mgr->m_cellsInSeg[i].size() < 2 ||
        m_mgr->m_cellsInSeg[i].front() != m_dummiesLeft[i] ||
        m_mgr->m_cellsInSeg[i].back() != m_dummiesRight[i]) {
      isError = true;
    }

    if (m_mgr->m_cellsInSeg[i].back() == m_dummiesRight[i]) {
      m_mgr->m_cellsInSeg[i].pop_back();
    }
    if (m_mgr->m_cellsInSeg[i].front() == m_dummiesLeft[i]) {
      m_mgr->m_cellsInSeg[i].erase(m_mgr->m_cellsInSeg[i].begin());
    }
  }

  for (size_t i = 0; i < m_dummiesRight.size(); i++) {
    delete m_dummiesRight[i];
  }
  for (size_t i = 0; i < m_dummiesLeft.size(); i++) {
    delete m_dummiesLeft[i];
  }

  if (isError) {
    m_mgr->internalError("Shifting failed during legalization");
  }

  return retval;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
double ShiftLegalizer::clump(std::vector<Node*>& order) {
  // Clumps provided cells.
  std::fill(m_offset.begin(), m_offset.end(), 0);
  std::fill(m_ptr.begin(), m_ptr.end(), (Clump*)0);

  size_t n = m_dummiesLeft.size() + order.size() + m_dummiesRight.size();

  m_clumps.resize(n);

  int clumpId = 0;

  // Left side of segments.
  for (size_t i = 0; i < m_dummiesLeft.size(); i++) {
    Node* ndi = m_dummiesLeft[i];

    Clump* r = &(m_clumps[clumpId]);

    m_offset[ndi->getId()] = 0;
    m_ptr[ndi->getId()] = r;

    double wt = 1.0e8;

    r->m_id = clumpId;
    r->m_nodes.erase(r->m_nodes.begin(), r->m_nodes.end());
    r->m_nodes.push_back(ndi);
    //r->m_width = ndi->getWidth();
    r->m_wposn = wt * ndi->getLeft();
    r->m_weight = wt;  // Massive weight for segment start.
    //r->m_posn = r->m_wposn / r->m_weight;
    r->m_posn = ndi->getLeft();

    ++clumpId;
  }

  for (size_t i = 0; i < order.size(); i++) {
    Node* ndi = order[i];

    Clump* r = &(m_clumps[clumpId]);

    m_offset[ndi->getId()] = 0;
    m_ptr[ndi->getId()] = r;

    double wt = 1.0;

    r->m_id = (int)i;
    r->m_nodes.erase(r->m_nodes.begin(), r->m_nodes.end());
    r->m_nodes.push_back(ndi);
    //r->m_width = ndi->getWidth();
    r->m_wposn = wt * ndi->getLeft();
    r->m_weight = wt;
    //r->m_posn = r->m_wposn / r->m_weight;
    r->m_posn = ndi->getLeft();

    // Always ensure the left edge is within the segments
    // in which the cell is assigned.
    for (size_t j = 0; j < m_mgr->m_reverseCellToSegs[ndi->getId()].size();
         j++) {
      DetailedSeg* segPtr = m_mgr->m_reverseCellToSegs[ndi->getId()][j];
      int xmin = segPtr->getMinX();
      int xmax = segPtr->getMaxX();
      // Left edge always within segment.
      r->m_posn = std::min(std::max(r->m_posn, xmin), xmax - ndi->getWidth());
    }

    ++clumpId;
  }

  // Right side of segments.
  for (size_t i = 0; i < m_dummiesRight.size(); i++) {
    Node* ndi = m_dummiesRight[i];

    Clump* r = &(m_clumps[clumpId]);

    m_offset[ndi->getId()] = 0;
    m_ptr[ndi->getId()] = r;

    double wt = 1.0e8;

    r->m_id = clumpId;
    r->m_nodes.erase(r->m_nodes.begin(), r->m_nodes.end());
    r->m_nodes.push_back(ndi);
    //r->m_width = ndi->getWidth();
    r->m_wposn = wt * ndi->getLeft();
    r->m_weight = wt;  // Massive weight for segment end.
    //r->m_posn = r->m_wposn / r->m_weight;
    r->m_posn = ndi->getLeft();

    ++clumpId;
  }

  // Perform the clumping.
  for (size_t i = 0; i < n; i++) {
    merge(&(m_clumps[i]));
  }

  // Replace cells.
  double retval = 0.;
  for (size_t i = 0; i < order.size(); i++) {
    Node* ndi = order[i];

    int rowId = m_mgr->m_reverseCellToSegs[ndi->getId()][0]->getRowId();
    for (size_t r = 1; r < m_mgr->m_reverseCellToSegs[ndi->getId()].size();
         r++) {
      rowId = std::min(rowId,
                       m_mgr->m_reverseCellToSegs[ndi->getId()][r]->getRowId());
    }

    Clump* r = m_ptr[ndi->getId()];

    // Left edge.
    int oldX = ndi->getLeft();
    int newX = r->m_posn + m_offset[ndi->getId()];

    ndi->setLeft(newX);

    // Bottom edge.
    int oldY = ndi->getBottom();
    int newY = m_arch->getRow(rowId)->getBottom();

    ndi->setBottom(newY);

    int dX = oldX - newX;
    int dY = oldY - newY;
    retval += (dX * dX + dY * dY);  // Quadratic or something else?
  }

  return retval;
}
void ShiftLegalizer::merge(Clump* r) {
  // Find most violated constraint and merge clumps if required.

  int dist = 0;
  Clump* l = 0;
  while (violated(r, l, dist) == true) {
    // Merge clump r into clump l which, in turn, could result in more merges.

    // Move blocks from r to l and update offsets, etc.
    for (size_t i = 0; i < r->m_nodes.size(); i++) {
      Node* ndi = r->m_nodes[i];
      m_offset[ndi->getId()] += dist;
      m_ptr[ndi->getId()] = l;
    }
    l->m_nodes.insert(l->m_nodes.end(), r->m_nodes.begin(), r->m_nodes.end());

    // Remove blocks from clump r.
    r->m_nodes.clear();

    // Update position of clump l.
    l->m_wposn += r->m_wposn - dist * r->m_weight;
    l->m_weight += r->m_weight;
    // Rounding down should always be fine since we merge to the left.
    l->m_posn = (int)std::floor(l->m_wposn / l->m_weight);

    // Since clump l changed position, we need to make it the new right clump
    // and see if there are more merges to the left.
    r = l;
  }
}
bool ShiftLegalizer::violated(Clump* r, Clump*& l, int& dist) {
  // We need to figure out if the right clump needs to be merged
  // into the left clump.  This will be needed if there would
  // be overlap among any cell in the right clump and any cell
  // in the left clump.  Look for the worst case.

  int nnodes = m_network->getNumNodes() ;
  int nsegs = m_mgr->getNumSegments();

  l = nullptr;
  int worst_diff = std::numeric_limits<int>::max();
  dist = std::numeric_limits<int>::max();

  for (size_t i = 0; i < r->m_nodes.size(); i++) {
    Node* ndr = r->m_nodes[i];

    // Look at each cell that must be left of current cell.
    for (size_t j = 0; j < m_incoming[ndr->getId()].size(); j++) {
      int id = m_incoming[ndr->getId()][j];

      // Could be that the node is _not_ a network node; it
      // might be a left or right dummy node.
      Node* ndl = 0;
      if (id < nnodes) {
        ndl = m_network->getNode(id);
      } else if (id < nnodes + nsegs) {
        ndl = m_dummiesLeft[id - nnodes];
      } else {
        ndl = m_dummiesRight[id - nnodes - nsegs];
      }

      Clump* t = m_ptr[ndl->getId()];
      if (t == r) {
        // Same clump.
        continue;
      }
      // Get left edge of both cells.
      int pdst = r->m_posn + m_offset[ndr->getId()];
      int psrc = t->m_posn + m_offset[ndl->getId()];
      int gap = ndl->getWidth();
      int diff = pdst - (psrc + gap);
      if (diff < 0 && diff < worst_diff) {
        // Leaving clump r at its current position would result
        // in overlap with clump t.  So, we would need to merge
        // clump r with clump t.
        worst_diff = diff;
        l = t;
        dist = m_offset[ndl->getId()] + gap - m_offset[ndr->getId()];
      }
    }
  }

  return (l != 0) ? true : false;
}

}  // namespace dpo
