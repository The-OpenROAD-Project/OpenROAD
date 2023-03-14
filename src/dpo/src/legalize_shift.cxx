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
#include "legalize_shift.h"

#include "architecture.h"
#include "detailed_manager.h"
#include "detailed_segment.h"
#include "utl/Logger.h"

using utl::DPO;

namespace dpo {

struct ShiftLegalizer::Clump
{
  int id_ = 0;
  double weight_ = 0.0;
  double wposn_ = 0.0;
  // Left edge of clump should be integer, although
  // the computation can be double.
  int posn_ = 0;
  std::vector<Node*> nodes_;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ShiftLegalizer::ShiftLegalizer() = default;
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ShiftLegalizer::~ShiftLegalizer() = default;
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool ShiftLegalizer::legalize(DetailedMgr& mgr)
{
  // The intention of this legalizer is to simply snap cells into
  // their closest segments and then do a "clumping" to remove
  // any overlap.  The "real intention" is to simply take an
  // existing legal placement and populate the data structures
  // used for detailed placement.
  //
  // When the snapping and shifting occurs, there really should
  // be no displacement at all.

  mgr_ = &mgr;

  arch_ = mgr_->getArchitecture();
  network_ = mgr_->getNetwork();
  rt_ = mgr_->getRoutingParams();

  // Categorize the cells and create the different segments.
  mgr.collectFixedCells();
  mgr.collectSingleHeightCells();
  mgr.collectMultiHeightCells();
  mgr.collectWideCells();    // XXX: This requires segments!
  mgr.findBlockages(false);  // Exclude routing blockages.
  mgr.findSegments();

  std::vector<std::pair<double, double>> origPos;
  origPos.resize(network_->getNumNodes());
  for (int i = 0; i < network_->getNumNodes(); i++) {
    const Node* ndi = network_->getNode(i);
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
  if (!mgr.getSingleHeightCells().empty()) {
    mgr.assignCellsToSegments(mgr.getSingleHeightCells());

    cells.insert(cells.end(),
                 mgr.getSingleHeightCells().begin(),
                 mgr.getSingleHeightCells().end());
  }
  for (size_t i = 2; i < mgr.getNumMultiHeights(); i++) {
    if (!mgr.getMultiHeightCells(i).empty()) {
      mgr.assignCellsToSegments(mgr.getMultiHeightCells(i));

      cells.insert(cells.end(),
                   mgr.getMultiHeightCells(i).begin(),
                   mgr.getMultiHeightCells(i).end());
    }
  }

  // Check for displacement after the snap.  Shouldn't be any.
  for (const Node* ndi : cells) {
    const double dx = std::fabs(ndi->getLeft() - origPos[ndi->getId()].first);
    const double dy
        = std::fabs(ndi->getBottom() - origPos[ndi->getId()].second);
    if (dx > 1.0e-3 || dy > 1.0e-3) {
      isDisp = true;
    }
  }

  // Topological order - required for a shift.
  const size_t size = network_->getNumNodes();
  incoming_.resize(size);
  outgoing_.resize(size);
  for (size_t i = 0; i < mgr.getNumSegments(); i++) {
    const int segId = mgr.getSegment(i)->getSegId();
    for (size_t j = 1; j < mgr.getNumCellsInSeg(segId); j++) {
      const Node* prev = mgr.getCellsInSeg(segId)[j - 1];
      const Node* curr = mgr.getCellsInSeg(segId)[j];

      incoming_[curr->getId()].push_back(prev->getId());
      outgoing_[prev->getId()].push_back(curr->getId());
    }
  }
  std::vector<bool> visit(size, false);
  std::vector<int> count(size, 0);
  std::vector<const Node*> order;
  order.reserve(size);
  for (const Node* ndi : cells) {
    count[ndi->getId()] = (int) incoming_[ndi->getId()].size();
    if (count[ndi->getId()] == 0) {
      visit[ndi->getId()] = true;
      order.push_back(ndi);
    }
  }
  for (size_t i = 0; i < order.size(); i++) {
    const Node* ndi = order[i];
    for (auto& outgoing : outgoing_[ndi->getId()]) {
      const Node* ndj = network_->getNode(outgoing);

      --count[ndj->getId()];
      if (count[ndj->getId()] == 0) {
        visit[ndj->getId()] = true;
        order.push_back(ndj);
      }
    }
  }
  if (order.size() != cells.size()) {
    // This is a fatal error!
    mgr_->internalError("Cells incorrectly ordered during shifting");
  }

  // Shift.
  shift(cells);

  // Check for displacement after the shift.  Shouldn't be any.
  for (const Node* ndi : cells) {
    const double dx = std::fabs(ndi->getLeft() - origPos[ndi->getId()].first);
    const double dy
        = std::fabs(ndi->getBottom() - origPos[ndi->getId()].second);
    if (dx > 1.0e-3 || dy > 1.0e-3) {
      isDisp = true;
    }
  }

  if (isDisp) {
    mgr_->getLogger()->warn(
        DPO, 200, "Unexpected displacement during legalization.");

    retval = false;
  }

  // Check.  If any of out internal checks fail, print
  // some sort of warning.
  const int err1 = mgr.checkRegionAssignment();
  const int err2 = mgr.checkRowAlignment();
  const int err3 = mgr.checkSiteAlignment();
  const int err4 = mgr.checkOverlapInSegments();
  const int err5 = mgr.checkEdgeSpacingInSegments();

  // Good place to issue some sort of warning.
  if (err1 != 0 || err2 != 0 || err3 != 0 || err4 != 0 || err5 != 0) {
    mgr_->getLogger()->warn(
        DPO, 201, "Placement check failure during legalization.");

    retval = false;
  }

  return retval;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
double ShiftLegalizer::shift(std::vector<Node*>& cells)
{
  // Do some setup and then shift cells to reduce movement from
  // original positions.  If no overlap, this should do nothing.
  //
  // Note: I don't even try to correct for site alignment.  I'll
  // print a warning, but will otherwise continue.

  const int nnodes = network_->getNumNodes();
  const int nsegs = mgr_->getNumSegments();

  // We need to add dummy cells to the left and the
  // right of every segment.
  dummiesLeft_.resize(nsegs);
  for (int i = 0; i < nsegs; i++) {
    const DetailedSeg* segPtr = mgr_->getSegment(i);

    const int rowId = segPtr->getRowId();

    Node* ndi = new Node();

    ndi->setId(nnodes + i);
    ndi->setLeft(segPtr->getMinX());
    ndi->setBottom(arch_->getRow(rowId)->getBottom());
    ndi->setWidth(0.0);
    ndi->setHeight(arch_->getRow(rowId)->getHeight());

    dummiesLeft_[i] = ndi;
  }

  dummiesRight_.resize(nsegs);
  for (int i = 0; i < nsegs; i++) {
    const DetailedSeg* segPtr = mgr_->getSegment(i);

    const int rowId = segPtr->getRowId();

    Node* ndi = new Node();

    ndi->setId(nnodes + nsegs + i);
    ndi->setLeft(segPtr->getMaxX());
    ndi->setBottom(arch_->getRow(rowId)->getBottom());
    ndi->setWidth(0.0);
    ndi->setHeight(arch_->getRow(rowId)->getHeight());

    dummiesRight_[i] = ndi;
  }

  // Jam all the left dummies nodes into segments.
  for (int i = 0; i < nsegs; i++) {
    mgr_->addToFrontCellsInSeg(i, dummiesLeft_[i]);
  }

  // Jam all the right dummies nodes into segments.
  for (int i = 0; i < nsegs; i++) {
    mgr_->addToBackCellsInSeg(i, dummiesRight_[i]);
  }

  // Need to create the "graph" prior to clumping.
  outgoing_.resize(nnodes + 2 * nsegs);
  incoming_.resize(nnodes + 2 * nsegs);
  ptr_.resize(nnodes + 2 * nsegs);
  offset_.resize(nnodes + 2 * nsegs);

  for (size_t i = 0; i < incoming_.size(); i++) {
    incoming_[i].clear();
    outgoing_[i].clear();
  }
  for (size_t i = 0; i < mgr_->getNumSegments(); i++) {
    const int segId = mgr_->getSegment(i)->getSegId();
    for (size_t j = 1; j < mgr_->getNumCellsInSeg(segId); j++) {
      const Node* prev = mgr_->getCellsInSeg(segId)[j - 1];
      const Node* curr = mgr_->getCellsInSeg(segId)[j];

      incoming_[curr->getId()].push_back(prev->getId());
      outgoing_[prev->getId()].push_back(curr->getId());
    }
  }

  // Clump everything; This does the X- and Y-direction.  Note
  // that the vector passed to the clumping is only the
  // movable cells; the clumping knows about the dummy cells
  // on the left and the right.
  const double retval = clump(cells);

  bool isError = false;
  // Remove all the dummies from the segments.
  for (int i = 0; i < nsegs; i++) {
    // Should _at least_ be the left and right dummies.
    if (mgr_->getNumCellsInSeg(i) < 2
        || mgr_->getCellsInSeg(i).front() != dummiesLeft_[i]
        || mgr_->getCellsInSeg(i).back() != dummiesRight_[i]) {
      isError = true;
    }

    if (mgr_->getCellsInSeg(i).back() == dummiesRight_[i]) {
      mgr_->popBackCellsInSeg(i);
    }
    if (mgr_->getCellsInSeg(i).front() == dummiesLeft_[i]) {
      mgr_->popFrontCellsInSeg(i);
    }
  }

  for (auto dummiesRight : dummiesRight_) {
    delete dummiesRight;
  }
  for (auto dummiesLeft : dummiesLeft_) {
    delete dummiesLeft;
  }

  if (isError) {
    mgr_->internalError("Shifting failed during legalization");
  }

  return retval;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
double ShiftLegalizer::clump(std::vector<Node*>& order)
{
  // Clumps provided cells.
  std::fill(offset_.begin(), offset_.end(), 0);
  std::fill(ptr_.begin(), ptr_.end(), (Clump*) nullptr);

  const size_t n = dummiesLeft_.size() + order.size() + dummiesRight_.size();

  clumps_.resize(n);

  int clumpId = 0;

  // Left side of segments.
  for (Node* ndi : dummiesLeft_) {
    Clump* r = &(clumps_[clumpId]);

    offset_[ndi->getId()] = 0;
    ptr_[ndi->getId()] = r;

    const double wt = 1.0e8;

    r->id_ = clumpId;
    r->nodes_.clear();
    r->nodes_.push_back(ndi);
    r->wposn_ = wt * ndi->getLeft();
    r->weight_ = wt;  // Massive weight for segment start.
    r->posn_ = ndi->getLeft();

    ++clumpId;
  }

  for (size_t i = 0; i < order.size(); i++) {
    Node* ndi = order[i];

    Clump* r = &(clumps_[clumpId]);

    offset_[ndi->getId()] = 0;
    ptr_[ndi->getId()] = r;

    const double wt = 1.0;

    r->id_ = (int) i;
    r->nodes_.clear();
    r->nodes_.push_back(ndi);
    r->wposn_ = wt * ndi->getLeft();
    r->weight_ = wt;
    r->posn_ = ndi->getLeft();

    // Always ensure the left edge is within the segments
    // in which the cell is assigned.
    for (size_t j = 0; j < mgr_->getNumReverseCellToSegs(ndi->getId()); j++) {
      DetailedSeg* segPtr = mgr_->getReverseCellToSegs(ndi->getId())[j];
      int xmin = segPtr->getMinX();
      int xmax = segPtr->getMaxX();
      // Left edge always within segment.
      r->posn_ = std::min(std::max(r->posn_, xmin), xmax - ndi->getWidth());
    }

    ++clumpId;
  }

  // Right side of segments.
  for (Node* ndi : dummiesRight_) {
    Clump* r = &(clumps_[clumpId]);

    offset_[ndi->getId()] = 0;
    ptr_[ndi->getId()] = r;

    const double wt = 1.0e8;

    r->id_ = clumpId;
    r->nodes_.clear();
    r->nodes_.push_back(ndi);
    r->wposn_ = wt * ndi->getLeft();
    r->weight_ = wt;  // Massive weight for segment end.
    r->posn_ = ndi->getLeft();

    ++clumpId;
  }

  // Perform the clumping.
  for (size_t i = 0; i < n; i++) {
    merge(&(clumps_[i]));
  }

  // Replace cells.
  double retval = 0.;
  for (Node* ndi : order) {
    int rowId = mgr_->getReverseCellToSegs(ndi->getId())[0]->getRowId();
    for (size_t r = 1; r < mgr_->getNumReverseCellToSegs(ndi->getId()); r++) {
      rowId = std::min(rowId,
                       mgr_->getReverseCellToSegs(ndi->getId())[r]->getRowId());
    }

    const Clump* r = ptr_[ndi->getId()];

    // Left edge.
    const int oldX = ndi->getLeft();
    const int newX = r->posn_ + offset_[ndi->getId()];

    ndi->setLeft(newX);

    // Bottom edge.
    const int oldY = ndi->getBottom();
    const int newY = arch_->getRow(rowId)->getBottom();

    ndi->setBottom(newY);

    const int dX = oldX - newX;
    const int dY = oldY - newY;
    retval += (dX * dX + dY * dY);  // Quadratic or something else?
  }

  return retval;
}
void ShiftLegalizer::merge(Clump* r)
{
  // Find most violated constraint and merge clumps if required.

  int dist = 0;
  Clump* l = nullptr;
  while (violated(r, l, dist) == true) {
    // Merge clump r into clump l which, in turn, could result in more merges.

    // Move blocks from r to l and update offsets, etc.
    for (const Node* ndi : r->nodes_) {
      offset_[ndi->getId()] += dist;
      ptr_[ndi->getId()] = l;
    }
    l->nodes_.insert(l->nodes_.end(), r->nodes_.begin(), r->nodes_.end());

    // Remove blocks from clump r.
    r->nodes_.clear();

    // Update position of clump l.
    l->wposn_ += r->wposn_ - dist * r->weight_;
    l->weight_ += r->weight_;
    // Rounding down should always be fine since we merge to the left.
    l->posn_ = (int) std::floor(l->wposn_ / l->weight_);

    // Since clump l changed position, we need to make it the new right clump
    // and see if there are more merges to the left.
    r = l;
  }
}

bool ShiftLegalizer::violated(Clump* r, Clump*& l, int& dist)
{
  // We need to figure out if the right clump needs to be merged
  // into the left clump.  This will be needed if there would
  // be overlap among any cell in the right clump and any cell
  // in the left clump.  Look for the worst case.

  const int nnodes = network_->getNumNodes();
  const int nsegs = mgr_->getNumSegments();

  l = nullptr;
  int worst_diff = std::numeric_limits<int>::max();
  dist = std::numeric_limits<int>::max();

  for (size_t i = 0; i < r->nodes_.size(); i++) {
    const Node* ndr = r->nodes_[i];

    // Look at each cell that must be left of current cell.
    for (size_t j = 0; j < incoming_[ndr->getId()].size(); j++) {
      const int id = incoming_[ndr->getId()][j];

      // Could be that the node is _not_ a network node; it
      // might be a left or right dummy node.
      Node* ndl = nullptr;
      if (id < nnodes) {
        ndl = network_->getNode(id);
      } else if (id < nnodes + nsegs) {
        ndl = dummiesLeft_[id - nnodes];
      } else {
        ndl = dummiesRight_[id - nnodes - nsegs];
      }

      Clump* t = ptr_[ndl->getId()];
      if (t == r) {
        // Same clump.
        continue;
      }
      // Get left edge of both cells.
      const int pdst = r->posn_ + offset_[ndr->getId()];
      const int psrc = t->posn_ + offset_[ndl->getId()];
      const int gap = ndl->getWidth();
      const int diff = pdst - (psrc + gap);
      if (diff < 0 && diff < worst_diff) {
        // Leaving clump r at its current position would result
        // in overlap with clump t.  So, we would need to merge
        // clump r with clump t.
        worst_diff = diff;
        l = t;
        dist = offset_[ndl->getId()] + gap - offset_[ndr->getId()];
      }
    }
  }

  return l != nullptr;
}

}  // namespace dpo
