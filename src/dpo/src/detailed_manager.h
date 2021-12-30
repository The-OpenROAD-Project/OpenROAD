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
#include <deque>
#include <vector>
#include "architecture.h"
#include "network.h"
#include "rectangle.h"
#include "router.h"
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
class DetailedSeg;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class DetailedMgr {
 public:
  DetailedMgr(Architecture* arch, Network* network, RoutingParams* rt);
  virtual ~DetailedMgr();

  void cleanup();

  Architecture* getArchitecture() const { return m_arch; }
  Network* getNetwork() const { return m_network; }
  RoutingParams* getRoutingParams() const { return m_rt; }

  void setLogger(utl::Logger* logger) { m_logger = logger; }
  utl::Logger* getLogger() const { return m_logger; }

  void setSeed(int seed);

  void internalError( std::string msg );

  void setupObstaclesForDrc();

  void findBlockages(bool includeRouteBlockages = true);
  void findRegionIntervals(
      int regId,
      std::vector<std::vector<std::pair<double, double> > >& intervals);

  void findSegments();
  DetailedSeg* findClosestSegment(Node* nd);
  void findClosestSpanOfSegmentsDfs(
      Node* ndi, DetailedSeg* segPtr, double xmin, double xmax, int bot,
      int top, std::vector<DetailedSeg*>& stack,
      std::vector<std::vector<DetailedSeg*> >& candidates);
  bool findClosestSpanOfSegments(Node* nd, std::vector<DetailedSeg*>& segments);

  void assignCellsToSegments(std::vector<Node*>& nodesToConsider);
  int checkSegments(double& worst);
  int checkOverlapInSegments();
  int checkEdgeSpacingInSegments();
  int checkSiteAlignment();
  int checkRowAlignment(int max_err_n = 0);
  int checkRegionAssignment();

  void removeCellFromSegmentTest(Node* nd, int seg, double& util, double& gapu);
  void addCellToSegmentTest(Node* nd, int seg, double x, double& util,
                            double& gapu);
  void removeCellFromSegment(Node* nd, int seg);
  void addCellToSegment(Node* nd, int seg);
  double getCellSpacing(Node* ndl, Node* ndr, bool checkPinsOnCells);

  void collectSingleHeightCells();
  void collectMultiHeightCells();
  void moveMultiHeightCellsToFixed();
  void collectFixedCells();
  void collectWideCells();

  void restoreOriginalPositions();
  void recordOriginalPositions();
  void restoreOriginalDimensions();
  void recordOriginalDimensions();

  void restoreBestPositions();
  void recordBestPositions();

  void resortSegments();
  void resortSegment(DetailedSeg* segPtr);
  void removeAllCellsFromSegments();

  double getOrigX(Node* nd) const { return m_origX[nd->getId()]; }
  double getOrigY(Node* nd) const { return m_origY[nd->getId()]; }
  double getOrigW(Node* nd) const { return m_origW[nd->getId()]; }
  double getOrigH(Node* nd) const { return m_origH[nd->getId()]; }

  bool isNodeAlignedToRow(Node* nd);

  double measureMaximumDisplacement(bool& violated);
  void removeOverlapMinimumShift();

  size_t getNumSegments() const { return m_segments.size(); }
  DetailedSeg* getSegment(int s) const { return m_segments[s]; }
  int getNumSingleHeightRows() const {
    return m_numSingleHeightRows;
  }
  int getSingleRowHeight() const { return m_singleRowHeight; }

  void getSpaceAroundCell(int seg, int ix, double& space, double& larger,
                          int limit = 3);
  void getSpaceAroundCell(int seg, int ix, double& space_left,
                          double& space_right, double& large_left,
                          double& large_right, int limit = 3);

  bool fixSegments();
  void moveCellsBetweenSegments(int iteration);
  void pushCellsBetweenSegments(int iteration);
  void moveCellsBetweenSegments(DetailedSeg* segment, int leftRightTol,
                                double offsetTol, double scoringTol);

  void removeSegmentOverlapSingle(int regId = -1);
  void removeSegmentOverlapSingleInner(std::vector<Node*>& nodes, double l,
                                       double r, int rowId);

  void debugSegments();

  double getTargetUt() const { return m_targetUt; }
  void setTargetUt(double ut) { m_targetUt = ut; }

  double getMaxMovement() const { return m_targetMaxMovement; }
  void setTargetMaxMovement(double movement) { m_targetMaxMovement = movement; }

  bool alignPos(Node* ndi, double& xi, double xl, double xr);
  bool shift(std::vector<Node*>& cells, std::vector<double>& tarX,
             std::vector<double>& posX, double left, double right, int segId,
             int rowId);

  bool tryMove1(Node* ndi, double xi, double yi, int si, double xj, double yj,
                int sj);
  bool tryMove2(Node* ndi, double xi, double yi, int si, double xj, double yj,
                int sj);
  bool tryMove3(Node* ndi, double xi, double yi, int si, double xj, double yj,
                int sj);

  bool trySwap1(Node* ndi, double xi, double yi, int si, double xj, double yj,
                int sj);

  void acceptMove();
  void rejectMove();

 public:
  struct compareBlockages {
    bool operator()(std::pair<double, double> i1,
                    std::pair<double, double> i2) const {
      if (i1.first == i2.first) {
        return i1.second < i2.second;
      }
      return i1.first < i2.first;
    }
  };

  struct compareNodesX {
    bool operator()(Node* p, Node* q) const {
      return p->getX() < q->getX();
    }
    bool operator()(Node*& s, double i) const { return s->getX() < i; }
    bool operator()(double i, Node*& s) const { return i < s->getX(); }
  };

  struct compareNodesL {
    bool operator()(Node* p, Node* q) const {
      return p->getX() - 0.5 * p->getWidth() < q->getX() - 0.5 * q->getWidth();
    }
  };

 protected:
  // Standard stuff.
  Architecture* m_arch;
  Network* m_network;
  RoutingParams* m_rt;

  // For output.
  utl::Logger* m_logger;

  // Info about rows.
  int m_numSingleHeightRows;
  double m_singleRowHeight;

  // Generic place for utilization.
  double m_targetUt;
  double m_targetMaxMovement;

  std::vector<Node*> m_fixedCells;   // Fixed; filler, macros, temporary, etc.

 public:
  // Blockages and segments.
  std::vector<std::vector<std::pair<double, double> > > m_blockages;
  std::vector<std::vector<Node*> > m_cellsInSeg;
  std::vector<std::vector<DetailedSeg*> > m_segsInRow;
  std::vector<DetailedSeg*> m_segments;
  std::vector<std::vector<DetailedSeg*> > m_reverseCellToSegs;

  // For short and pin access stuff...
  std::vector<std::vector<std::vector<Rectangle> > > m_obstacles;

  // Random number generator.
  Placer_RNG* m_rng;

  // Info about cells.
  std::vector<Node*> m_singleHeightCells;  // Single height cells.
  std::vector<std::vector<Node*> >
      m_multiHeightCells;            // Multi height cells by height.
  std::vector<Node*> m_fixedMacros;  // Fixed; only macros.
  std::vector<Node*>
      m_wideCells;  // Wide movable cells.  Can be single of multi.

  // Info about original cell positions and dimensions.
  std::vector<double> m_origX;
  std::vector<double> m_origY;
  std::vector<double> m_origW;
  std::vector<double> m_origH;

  std::vector<double> m_bestX;
  std::vector<double> m_bestY;

  std::vector<Rectangle> m_boxes;

  // For generating a move list...
  std::vector<double> m_curX;
  std::vector<double> m_curY;
  std::vector<double> m_newX;
  std::vector<double> m_newY;
  std::vector<unsigned> m_curOri;
  std::vector<unsigned> m_newOri;
  std::vector<std::vector<int> > m_curSeg;
  std::vector<std::vector<int> > m_newSeg;
  std::vector<Node*> m_movedNodes;
  int m_nMoved;
  int m_moveLimit;
};

}  // namespace dpo
