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
  void getMaxDisplacement(int& x, int& y) const
  {
    x = maxDispX_;
    y = maxDispY_;
  }
  int getMaxDisplacementX() const { return maxDispX_; }
  int getMaxDisplacementY() const { return maxDispY_; }
  double measureMaximumDisplacement(double& maxX,
                                    double& maxY,
                                    int& violatedX,
                                    int& violatedY);

  void internalError(std::string msg);

  void setupObstaclesForDrc();

  void findBlockages(bool includeRouteBlockages = true);
  void findRegionIntervals(
      int regId,
      std::vector<std::vector<std::pair<double, double>>>& intervals);

  void findSegments();
  DetailedSeg* findClosestSegment(Node* nd);
  void findClosestSpanOfSegmentsDfs(
      Node* ndi,
      DetailedSeg* segPtr,
      double xmin,
      double xmax,
      int bot,
      int top,
      std::vector<DetailedSeg*>& stack,
      std::vector<std::vector<DetailedSeg*>>& candidates);
  bool findClosestSpanOfSegments(Node* nd, std::vector<DetailedSeg*>& segments);

  void assignCellsToSegments(std::vector<Node*>& nodesToConsider);
  int checkOverlapInSegments();
  int checkEdgeSpacingInSegments();
  int checkSiteAlignment();
  int checkRowAlignment();
  int checkRegionAssignment();

  void removeCellFromSegment(Node* nd, int seg);
  void addCellToSegment(Node* nd, int seg);
  double getCellSpacing(Node* ndl, Node* ndr, bool checkPinsOnCells);

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
  bool alignPos(Node* ndi, int& xi, int xl, int xr);

 public:
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

  struct compareNodesL
  {
    bool operator()(Node* p, Node* q) const
    {
      return p->getLeft() < q->getLeft();
    }
  };

 private:
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
  bool shiftLeftHelper(Node* ndi, int xj, int sj, Node* ndr);
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
                     std::vector<int>& curSegs,
                     int newLeft,
                     int newBottom,
                     std::vector<int>& newSegs);

 private:
  // Standard stuff.
  Architecture* arch_;
  Network* network_;
  RoutingParams* rt_;

  // For output.
  utl::Logger* logger_;

  // Info about rows.
  int numSingleHeightRows_;
  int singleRowHeight_;

  // Generic place for utilization.
  double targetUt_;

  // Target displacement limits.
  int maxDispX_;
  int maxDispY_;

  std::vector<Node*> fixedCells_;  // Fixed; filler, macros, temporary, etc.

 public:
  // Blockages and segments.
  std::vector<std::vector<std::pair<double, double>>> blockages_;
  std::vector<std::vector<Node*>> cellsInSeg_;
  std::vector<std::vector<DetailedSeg*>> segsInRow_;
  std::vector<DetailedSeg*> segments_;
  std::vector<std::vector<DetailedSeg*>> reverseCellToSegs_;

  // For short and pin access stuff...
  std::vector<std::vector<std::vector<Rectangle>>> obstacles_;

  // Random number generator.
  Placer_RNG* rng_;

  // Info about cells.
  std::vector<Node*> singleHeightCells_;  // Single height cells.
  std::vector<std::vector<Node*>>
      multiHeightCells_;  // Multi height cells by height.
  std::vector<Node*>
      wideCells_;  // Wide movable cells.  Can be single of multi.

  // Original cell positions.
  std::vector<int> origBottom_;
  std::vector<int> origLeft_;

  std::vector<Rectangle> boxes_;

  // For generating a move list...
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
