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

// Description:
// Primarily for maintaining the segments.

#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <memory>
#include <vector>

#include "network.h"
#include "rectangle.h"
#include "utility.h"

namespace utl {
class Logger;
}

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class Architecture;
class DetailedSeg;
class Network;
class RoutingParams;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class DetailedMgr
{
 public:
  DetailedMgr(Architecture* arch, Network* network, RoutingParams* rt);
  virtual ~DetailedMgr();

  void cleanup();

  Architecture* getArchitecture() const { return arch_; }
  Network* getNetwork() const { return network_; }
  RoutingParams* getRoutingParams() const { return rt_; }

  void setLogger(utl::Logger* logger) { logger_ = logger; }
  utl::Logger* getLogger() const { return logger_; }

  void setSeed(int s);

  void setMaxDisplacement(int x, int y);
  void setDisallowOneSiteGaps(bool disallowOneSiteGaps);
  void getMaxDisplacement(int& x, int& y) const
  {
    x = maxDispX_;
    y = maxDispY_;
  }
  int getMaxDisplacementX() const { return maxDispX_; }
  int getMaxDisplacementY() const { return maxDispY_; }
  bool getDisallowOneSiteGaps() const { return disallowOneSiteGaps_; }
  double measureMaximumDisplacement(double& maxX,
                                    double& maxY,
                                    int& violatedX,
                                    int& violatedY);

  void internalError(const std::string& msg);

  void setupObstaclesForDrc();

  void findBlockages(bool includeRouteBlockages = true);
  void findRegionIntervals(
      int regId,
      std::vector<std::vector<std::pair<double, double>>>& intervals);

  void findSegments();
  DetailedSeg* findClosestSegment(const Node* nd);
  void findClosestSpanOfSegmentsDfs(
      const Node* ndi,
      DetailedSeg* segPtr,
      double xmin,
      double xmax,
      int bot,
      int top,
      std::vector<DetailedSeg*>& stack,
      std::vector<std::vector<DetailedSeg*>>& candidates);
  bool findClosestSpanOfSegments(Node* nd, std::vector<DetailedSeg*>& segments);
  bool isInsideABlockage(const Node* nd, double position);
  void assignCellsToSegments(const std::vector<Node*>& nodesToConsider);
  int checkOverlapInSegments();
  int checkEdgeSpacingInSegments();
  int checkSiteAlignment();
  int checkRowAlignment();
  int checkRegionAssignment();
  void getOneSiteGapViolationsPerSegment(
      std::vector<std::vector<int>>& violating_cells,
      bool fix_violations);
  bool fixOneSiteGapViolations(Node* cell,
                               int one_site_gap,
                               int newX,
                               int segment,
                               Node* violatingNode);
  void removeCellFromSegment(const Node* nd, int seg);
  void addCellToSegment(Node* nd, int seg);
  double getCellSpacing(const Node* ndl,
                        const Node* ndr,
                        bool checkPinsOnCells);

  void collectSingleHeightCells();
  void collectMultiHeightCells();
  void collectFixedCells();
  void collectWideCells();

  void restoreOriginalPositions();
  void recordOriginalPositions();

  void resortSegments();
  void resortSegment(DetailedSeg* segPtr);
  void removeAllCellsFromSegments();

  int getNumSegments() const { return static_cast<int>(segments_.size()); }
  DetailedSeg* getSegment(int s) const { return segments_[s]; }
  int getNumSingleHeightRows() const { return numSingleHeightRows_; }
  int getSingleRowHeight() const { return singleRowHeight_; }

  int getNumCellsInSeg(int segId) const { return cellsInSeg_[segId].size(); }
  const std::vector<Node*>& getCellsInSeg(int segId) const
  {
    return cellsInSeg_[segId];
  }
  void addToFrontCellsInSeg(int segId, Node* node)
  {
    cellsInSeg_[segId].insert(cellsInSeg_[segId].begin(), node);
  }
  void addToBackCellsInSeg(int segId, Node* node)
  {
    cellsInSeg_[segId].push_back(node);
  }
  void popFrontCellsInSeg(int segId)
  {
    cellsInSeg_[segId].erase(cellsInSeg_[segId].begin());
  }
  void popBackCellsInSeg(int segId) { cellsInSeg_[segId].pop_back(); }
  void sortCellsInSeg(int segId)
  {
    std::sort(cellsInSeg_[segId].begin(),
              cellsInSeg_[segId].end(),
              DetailedMgr::compareNodesX());
  }
  void sortCellsInSeg(int segId, int start, int end)
  {
    std::sort(cellsInSeg_[segId].begin() + start,
              cellsInSeg_[segId].begin() + end,
              DetailedMgr::compareNodesX());
  }

  int getNumSegsInRow(int rowId) const { return segsInRow_[rowId].size(); }
  const std::vector<DetailedSeg*>& getSegsInRow(int rowId) const
  {
    return segsInRow_[rowId];
  }

  int getNumReverseCellToSegs(int nodeId) const
  {
    return reverseCellToSegs_[nodeId].size();
  }
  const std::vector<DetailedSeg*>& getReverseCellToSegs(int nodeId) const
  {
    return reverseCellToSegs_[nodeId];
  }

  const std::vector<Node*>& getSingleHeightCells() const
  {
    return singleHeightCells_;
  }

  int getNumMultiHeights() const { return multiHeightCells_.size(); }
  const std::vector<Node*>& getMultiHeightCells(int height) const
  {
    return multiHeightCells_[height];
  }
  const std::vector<Node*>& getWideCells() const { return wideCells_; }

  void shuffle(std::vector<Node*>& nodes);
  int getRandom(int limit) const { return (*rng_)() % limit; }

  const std::vector<int>& getCurLeft() const { return curLeft_; }
  const std::vector<int>& getCurBottom() const { return curBottom_; }
  const std::vector<unsigned>& getCurOri() const { return curOri_; }
  const std::vector<int>& getNewLeft() const { return newLeft_; }
  const std::vector<int>& getNewBottom() const { return newBottom_; }
  const std::vector<unsigned>& getNewOri() const { return newOri_; }
  const std::vector<Node*>& getMovedNodes() const { return movedNodes_; }
  int getNMoved() const { return nMoved_; }

  void getSpaceAroundCell(int seg,
                          int ix,
                          double& space,
                          double& larger,
                          int limit = 3);
  void getSpaceAroundCell(int seg,
                          int ix,
                          double& space_left,
                          double& space_right,
                          double& large_left,
                          double& large_right,
                          int limit = 3);

  void removeSegmentOverlapSingle(int regId = -1);
  void removeSegmentOverlapSingleInner(std::vector<Node*>& nodes,
                                       int l,
                                       int r,
                                       int rowId);

  double getTargetUt() const { return targetUt_; }
  void setTargetUt(double ut) { targetUt_ = ut; }

  // Routines for generating moves and swaps.
  bool tryMove(Node* ndi, int xi, int yi, int si, int xj, int yj, int sj);
  bool trySwap(Node* ndi, int xi, int yi, int si, int xj, int yj, int sj);

  // For accepting or rejecting moves and swaps.
  void acceptMove();
  void rejectMove();

  // For help aligning cells to sites.
  bool alignPos(const Node* ndi, int& xi, int xl, int xr);
  int getMoveLimit() { return moveLimit_; }
  void setMoveLimit(unsigned int newMoveLimit) { moveLimit_ = newMoveLimit; }

  struct compareNodesX
  {
    // Needs cell centers.
    bool operator()(Node* p, Node* q) const
    {
      return p->getLeft() + 0.5 * p->getWidth()
             < q->getLeft() + 0.5 * q->getWidth();
    }
    bool operator()(Node*& s, double i) const
    {
      return s->getLeft() + 0.5 * s->getWidth() < i;
    }
    bool operator()(double i, Node*& s) const
    {
      return i < s->getLeft() + 0.5 * s->getWidth();
    }
  };

 private:
  struct compareBlockages
  {
    bool operator()(std::pair<double, double> i1,
                    std::pair<double, double> i2) const
    {
      if (i1.first == i2.first) {
        return i1.second < i2.second;
      }
      return i1.first < i2.first;
    }
  };

  struct compareNodesL
  {
    bool operator()(Node* p, Node* q) const
    {
      return p->getLeft() < q->getLeft();
    }
  };

  // Different routines for trying moves and swaps.
  bool tryMove1(Node* ndi, int xi, int yi, int si, int xj, int yj, int sj);
  bool tryMove2(Node* ndi, int xi, int yi, int si, int xj, int yj, int sj);
  bool tryMove3(Node* ndi, int xi, int yi, int si, int xj, int yj, int sj);

  bool trySwap1(Node* ndi, int xi, int yi, int si, int xj, int yj, int sj);

  // Helper routines for making moves and swaps.
  bool shift(std::vector<Node*>& cells,
             std::vector<int>& targetLeft,
             std::vector<int>& posLeft,
             int leftLimit,
             int rightLimit,
             int segId,
             int rowId);
  bool shiftRightHelper(Node* ndi, int xj, int sj, Node* ndr);
  bool shiftLeftHelper(Node* ndi, int xj, int sj, Node* ndl);
  void getSpaceToLeftAndRight(int seg, int ix, double& left, double& right);

  // For composing list of cells for moves or swaps.
  void clearMoveList();
  bool addToMoveList(Node* ndi,
                     int curLeft,
                     int curBottom,
                     int curSeg,
                     int newLeft,
                     int newBottom,
                     int newSeg);
  bool addToMoveList(Node* ndi,
                     int curLeft,
                     int curBottom,
                     const std::vector<int>& curSegs,
                     int newLeft,
                     int newBottom,
                     const std::vector<int>& newSegs);

  // Standard stuff.
  Architecture* arch_;
  Network* network_;
  RoutingParams* rt_;

  // For output.
  utl::Logger* logger_ = nullptr;

  // Info about rows.
  int numSingleHeightRows_;
  int singleRowHeight_;

  // Generic place for utilization.
  double targetUt_;

  // Target displacement limits.
  int maxDispX_;
  int maxDispY_;
  bool disallowOneSiteGaps_;
  std::vector<Node*> fixedCells_;  // Fixed; filler, macros, temporary, etc.

  // Blockages and segments.
  std::vector<std::vector<std::pair<double, double>>> blockages_;
  std::vector<std::vector<Node*>> cellsInSeg_;
  std::vector<std::vector<DetailedSeg*>> segsInRow_;
  std::vector<DetailedSeg*> segments_;
  // size == #nodes
  std::vector<std::vector<DetailedSeg*>> reverseCellToSegs_;

  // For short and pin access stuff...
  std::vector<std::vector<std::vector<Rectangle>>> obstacles_;

  // Random number generator.
  std::unique_ptr<Placer_RNG> rng_;

  // Info about cells:
  // Single height cells.
  std::vector<Node*> singleHeightCells_;
  // Multi height cells by height.
  std::vector<std::vector<Node*>> multiHeightCells_;
  // Wide movable cells.  Can be single or multi height.
  std::vector<Node*> wideCells_;

  // Original cell positions.
  std::vector<int> origBottom_;
  std::vector<int> origLeft_;

  std::vector<Rectangle> boxes_;

  // For generating a move list... (size = moveLimit_)
  std::vector<int> curLeft_;
  std::vector<int> curBottom_;
  std::vector<int> newLeft_;
  std::vector<int> newBottom_;
  std::vector<unsigned> curOri_;
  std::vector<unsigned> newOri_;
  std::vector<std::vector<int>> curSeg_;
  std::vector<std::vector<int>> newSeg_;
  std::vector<Node*> movedNodes_;
  int nMoved_;
  int moveLimit_;
};

}  // namespace dpo
