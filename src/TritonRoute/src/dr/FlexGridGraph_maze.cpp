/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dr/FlexDR.h"
#include "dr/FlexDR_graphics.h"
#include "dr/FlexGridGraph.h"

using namespace std;
using namespace fr;

void FlexGridGraph::expand(FlexWavefrontGrid& currGrid,
                           const frDirEnum& dir,
                           const FlexMazeIdx& dstMazeIdx1,
                           const FlexMazeIdx& dstMazeIdx2,
                           const frPoint& centerPt)
{
  bool enableOutput = false;
  // bool enableOutput = true;
  frCost nextEstCost, nextPathCost;
  int gridX = currGrid.x();
  int gridY = currGrid.y();
  int gridZ = currGrid.z();

  getNextGrid(gridX, gridY, gridZ, dir);

  FlexMazeIdx nextIdx(gridX, gridY, gridZ);
  // get cost
  nextEstCost = getEstCost(nextIdx, dstMazeIdx1, dstMazeIdx2, dir);
  nextPathCost = getNextPathCost(currGrid, dir);
  if (enableOutput) {
    std::cout << "  expanding from (" << currGrid.x() << ", " << currGrid.y()
              << ", " << currGrid.z()
              << ") [pathCost / totalCost = " << currGrid.getPathCost() << " / "
              << currGrid.getCost() << "] to "
              << "(" << gridX << ", " << gridY << ", " << gridZ
              << ") [pathCost / totalCost = " << nextPathCost << " / "
              << nextPathCost + nextEstCost << "]\n";
  }
  auto lNum = getLayerNum(currGrid.z());
  auto pathWidth = getDesign()->getTech()->getLayer(lNum)->getWidth();
  frPoint currPt;
  getPoint(currPt, gridX, gridY);
  frCoord currDist
      = abs(currPt.x() - centerPt.x()) + abs(currPt.y() - centerPt.y());

  // vlength calculation
  frCoord currVLengthX = 0;
  frCoord currVLengthY = 0;
  currGrid.getVLength(currVLengthX, currVLengthY);
  auto nextVLengthX = currVLengthX;
  auto nextVLengthY = currVLengthY;
  bool nextIsPrevViaUp = currGrid.isPrevViaUp();
  if (dir == frDirEnum::U || dir == frDirEnum::D) {
    nextVLengthX = 0;
    nextVLengthY = 0;
    nextIsPrevViaUp
        = (dir == frDirEnum::D);  // up via if current path goes down
  } else {
    if (currVLengthX != std::numeric_limits<frCoord>::max()
        && currVLengthY != std::numeric_limits<frCoord>::max()) {
      if (dir == frDirEnum::W || dir == frDirEnum::E) {
        nextVLengthX
            += getEdgeLength(currGrid.x(), currGrid.y(), currGrid.z(), dir);
      } else {
        nextVLengthY
            += getEdgeLength(currGrid.x(), currGrid.y(), currGrid.z(), dir);
      }
    }
  }

  // tlength calculation
  auto currTLength = currGrid.getTLength();
  auto nextTLength = currTLength;
  // if there was a turn, then add tlength
  if (currTLength != std::numeric_limits<frCoord>::max()) {
    nextTLength += getEdgeLength(currGrid.x(), currGrid.y(), currGrid.z(), dir);
  }
  // if current is a turn, then reset tlength
  if (currGrid.getLastDir() != frDirEnum::UNKNOWN
      && currGrid.getLastDir() != dir) {
    nextTLength = getEdgeLength(currGrid.x(), currGrid.y(), currGrid.z(), dir);
  }
  // if current is a via, then reset tlength
  if (dir == frDirEnum::U || dir == frDirEnum::D) {
    nextTLength = std::numeric_limits<frCoord>::max();
  }

  FlexWavefrontGrid nextWavefrontGrid(
      gridX,
      gridY,
      gridZ,
      currGrid.getLayerPathArea()
          + getEdgeLength(currGrid.x(), currGrid.y(), currGrid.z(), dir)
                * pathWidth,
      nextVLengthX,
      nextVLengthY,
      nextIsPrevViaUp,
      nextTLength,
      currDist,
      nextPathCost,
      nextPathCost + nextEstCost,
      currGrid.getBackTraceBuffer());
  if (dir == frDirEnum::U || dir == frDirEnum::D) {
    nextWavefrontGrid.resetLayerPathArea();
    nextWavefrontGrid.resetLength();
    if (dir == frDirEnum::U) {
      nextWavefrontGrid.setPrevViaUp(false);
    } else {
      nextWavefrontGrid.setPrevViaUp(true);
    }
    nextWavefrontGrid.addLayerPathArea(
        (dir == frDirEnum::U) ? getHalfViaEncArea(currGrid.z(), false)
                              : getHalfViaEncArea(gridZ, true));
  }
  if (currGrid.getSrcTaperBox()
      && currGrid.getSrcTaperBox()->contains(
          nextWavefrontGrid.x(), nextWavefrontGrid.y(), nextWavefrontGrid.z()))
    nextWavefrontGrid.setSrcTaperBox(currGrid.getSrcTaperBox());
  // update wavefront buffer
  auto tailDir = nextWavefrontGrid.shiftAddBuffer(dir);
  // non-buffer enablement is faster for ripup all
  // commit grid prev direction if needed
  auto tailIdx = getTailIdx(nextIdx, nextWavefrontGrid);
  if (tailDir != frDirEnum::UNKNOWN) {
    if (getPrevAstarNodeDir(tailIdx.x(), tailIdx.y(), tailIdx.z())
            == frDirEnum::UNKNOWN
        || getPrevAstarNodeDir(tailIdx.x(), tailIdx.y(), tailIdx.z())
               == tailDir) {
      setPrevAstarNodeDir(tailIdx.x(), tailIdx.y(), tailIdx.z(), tailDir);
      wavefront_.push(nextWavefrontGrid);
      if (enableOutput) {
        std::cout << "    commit (" << tailIdx.x() << ", " << tailIdx.y()
                  << ", " << tailIdx.z()
                  << ") prev accessing dir = " << (int) tailDir << "\n";
      }
    }
  } else {
    // add to wavefront
    wavefront_.push(nextWavefrontGrid);
  }

  return;
}

void FlexGridGraph::expandWavefront(FlexWavefrontGrid& currGrid,
                                    const FlexMazeIdx& dstMazeIdx1,
                                    const FlexMazeIdx& dstMazeIdx2,
                                    const frPoint& centerPt)
{
  bool enableOutput = false;
  // bool enableOutput = true;
  if (enableOutput) {
    cout << "start expand from (" << currGrid.x() << ", " << currGrid.y()
         << ", " << currGrid.z() << ")\n";
  }
  // if (currGrid.y() == 19 && currGrid.z() == 0) {
  //   cout <<"is expandable (" <<currGrid.x() <<", " <<currGrid.y() <<", "
  //   <<currGrid.z() <<") NESWUD "
  //        <<isExpandable(currGrid, frDirEnum::N)
  //        <<isExpandable(currGrid, frDirEnum::E)
  //        <<isExpandable(currGrid, frDirEnum::S)
  //        <<isExpandable(currGrid, frDirEnum::W)
  //        <<isExpandable(currGrid, frDirEnum::U)
  //        <<isExpandable(currGrid, frDirEnum::D)
  //        <<endl;
  //   cout <<"has edge "
  //        <<gridGraph.hasEdge(currGrid.x(), currGrid.y(), currGrid.z(),
  //        frDirEnum::N)
  //        <<gridGraph.hasEdge(currGrid.x(), currGrid.y(), currGrid.z(),
  //        frDirEnum::E)
  //        <<gridGraph.hasEdge(currGrid.x(), currGrid.y(), currGrid.z(),
  //        frDirEnum::S)
  //        <<gridGraph.hasEdge(currGrid.x(), currGrid.y(), currGrid.z(),
  //        frDirEnum::W)
  //        <<gridGraph.hasEdge(currGrid.x(), currGrid.y(), currGrid.z(),
  //        frDirEnum::U)
  //        <<gridGraph.hasEdge(currGrid.x(), currGrid.y(), currGrid.z(),
  //        frDirEnum::D)
  //        <<endl;
  //   int gridX = currGrid.x();
  //   int gridY = currGrid.y();
  //   int gridZ = currGrid.z();
  //   if (!gridGraph.hasEdge(gridX, gridY, gridZ, frDirEnum::E)) {
  //     ;
  //   } else {
  //     getNextGrid(gridX, gridY, gridZ, frDirEnum::E);
  //     if (gridGraph.isBlocked(gridX, gridY, gridZ)) {
  //       cout <<"blocked" <<endl;
  //     } else if (gridGraph.isSrc(gridX, gridY, gridZ)) {
  //       cout <<"src" <<endl;
  //     } else if (gridGraph.getPrevAstarNodeDir(gridX, gridY, gridZ) !=
  //     frDirEnum::UNKNOWN) {
  //       cout <<"visited" <<endl;
  //     } else {
  //       ;
  //     }
  //   }
  // }
  // auto tmpGrid = currWavefrontGrid;
  //// commit grid prev direction if needed
  // auto tailIdx = getTailIdx(nextIdx, nextWavefrontGrid);
  // if (tailDir != frDirEnum::UNKNOWN) {
  //   if (getPrevAstarNodeDir(tailIdx.x(), tailIdx.y(), tailIdx.z()) ==
  //   frDirEnum::UNKNOWN) {
  //     ;
  //   }
  // }

  // N
  if (isExpandable(currGrid, frDirEnum::N)) {
    expand(currGrid, frDirEnum::N, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
  // else {
  //   std::cout <<"no N" <<endl;
  // }
  // E
  if (isExpandable(currGrid, frDirEnum::E)) {
    expand(currGrid, frDirEnum::E, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
  // else {
  //   std::cout <<"no E" <<endl;
  // }
  // S
  if (isExpandable(currGrid, frDirEnum::S)) {
    expand(currGrid, frDirEnum::S, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
  // else {
  //   std::cout <<"no S" <<endl;
  // }
  // W
  if (isExpandable(currGrid, frDirEnum::W)) {
    expand(currGrid, frDirEnum::W, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
  // else {
  //   std::cout <<"no W" <<endl;
  // }
  // U
  if (isExpandable(currGrid, frDirEnum::U)) {
    expand(currGrid, frDirEnum::U, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
  // else {
  //   std::cout <<"no U" <<endl;
  // }
  // D
  if (isExpandable(currGrid, frDirEnum::D)) {
    expand(currGrid, frDirEnum::D, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
  // else {
  //   std::cout <<"no D" <<endl;
  // }
}

frCost FlexGridGraph::getEstCost(const FlexMazeIdx& src,
                                 const FlexMazeIdx& dstMazeIdx1,
                                 const FlexMazeIdx& dstMazeIdx2,
                                 const frDirEnum& dir) const
{
  // bool enableOutput = true;
  bool enableOutput = false;
  if (enableOutput) {
    cout << "est from (" << src.x() << ", " << src.y() << ", " << src.z()
         << ") "
         << "to (" << dstMazeIdx1.x() << ", " << dstMazeIdx1.y() << ", "
         << dstMazeIdx1.z() << ") (" << dstMazeIdx2.x() << ", "
         << dstMazeIdx2.y() << ", " << dstMazeIdx2.z() << ")";
  }
  // bend cost
  int bendCnt = 0;
  int forbiddenPenalty = 0;
  frPoint srcPoint, dstPoint1, dstPoint2;
  getPoint(srcPoint, src.x(), src.y());
  getPoint(dstPoint1, dstMazeIdx1.x(), dstMazeIdx1.y());
  getPoint(dstPoint2, dstMazeIdx2.x(), dstMazeIdx2.y());
  frCoord minCostX
      = max(max(dstPoint1.x() - srcPoint.x(), srcPoint.x() - dstPoint2.x()), 0)
        * 1;
  frCoord minCostY
      = max(max(dstPoint1.y() - srcPoint.y(), srcPoint.y() - dstPoint2.y()), 0)
        * 1;
  frCoord minCostZ = max(max(getZHeight(dstMazeIdx1.z()) - getZHeight(src.z()),
                             getZHeight(src.z()) - getZHeight(dstMazeIdx2.z())),
                         0)
                     * 1;
  if (enableOutput) {
    cout << " x/y/z min cost = (" << minCostX << ", " << minCostY << ", "
         << minCostZ << ") " << endl;
  }

  bendCnt += (minCostX && dir != frDirEnum::UNKNOWN && dir != frDirEnum::E
              && dir != frDirEnum::W)
                 ? 1
                 : 0;
  bendCnt += (minCostY && dir != frDirEnum::UNKNOWN && dir != frDirEnum::S
              && dir != frDirEnum::N)
                 ? 1
                 : 0;
  bendCnt += (minCostZ && dir != frDirEnum::UNKNOWN && dir != frDirEnum::U
              && dir != frDirEnum::D)
                 ? 1
                 : 0;
  if (enableOutput) {
    cout << "  est cost = " << minCostX + minCostY + minCostZ + bendCnt << endl;
  }

  // If we are on the destination layer we will have to wrong way jog or
  // via up/down or down/up so add the cheapest of those to the estimate
  if (src.z() == dstMazeIdx1.z() && dstMazeIdx1.z() == dstMazeIdx2.z()) {
  }

  int gridX = src.x();
  int gridY = src.y();
  int gridZ = src.z();
  getNextGrid(gridX, gridY, gridZ, dir);
  frPoint nextPoint;
  getPoint(nextPoint, gridX, gridY);
  // avoid propagating to location that will cause fobidden via spacing to
  // boundary pin
  if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    if (drWorker_ && drWorker_->getDRIter() >= 30
        && drWorker_->getRipupMode() == 0) {
      if (dstMazeIdx1 == dstMazeIdx2 && gridZ == dstMazeIdx1.z()) {
        auto layerNum = (gridZ + 1) * 2;
        auto layer = getDesign()->getTech()->getLayer(layerNum);
        bool isH
            = (layer->getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
        if (isH) {
          auto gap = abs(nextPoint.y() - dstPoint1.y());
          if (gap
              && (getDesign()->getTech()->isVia2ViaForbiddenLen(
                      gridZ, false, false, false, gap, ndr_, false)
                  || layerNum - 2 < BOTTOM_ROUTING_LAYER)
              && (getDesign()->getTech()->isVia2ViaForbiddenLen(
                      gridZ, true, true, false, gap, ndr_, false)
                  || layerNum + 2 > getDesign()->getTech()->getTopLayerNum())) {
            forbiddenPenalty = layer->getPitch() * ggDRCCost_ * 20;
          }
        } else {
          auto gap = abs(nextPoint.x() - dstPoint1.x());
          if (gap
              && (getDesign()->getTech()->isVia2ViaForbiddenLen(
                      gridZ, false, false, true, gap, ndr_, false)
                  || layerNum - 2 < BOTTOM_ROUTING_LAYER)
              && (getDesign()->getTech()->isVia2ViaForbiddenLen(
                      gridZ, true, true, true, gap, ndr_, false)
                  || layerNum + 2 > getDesign()->getTech()->getTopLayerNum())) {
            forbiddenPenalty = layer->getPitch() * ggDRCCost_ * 20;
          }
        }
      }
    }
  }

  return (minCostX + minCostY + minCostZ + bendCnt + forbiddenPenalty);
}

frDirEnum FlexGridGraph::getLastDir(
    const std::bitset<WAVEFRONTBITSIZE>& buffer) const
{
  auto currDirVal = buffer.to_ulong() & 0b111u;
  return static_cast<frDirEnum>(currDirVal);
}

void FlexGridGraph::getNextGrid(frMIdx& gridX,
                                frMIdx& gridY,
                                frMIdx& gridZ,
                                const frDirEnum dir) const
{
  switch (dir) {
    case frDirEnum::E:
      ++gridX;
      break;
    case frDirEnum::S:
      --gridY;
      break;
    case frDirEnum::W:
      --gridX;
      break;
    case frDirEnum::N:
      ++gridY;
      break;
    case frDirEnum::U:
      ++gridZ;
      break;
    case frDirEnum::D:
      --gridZ;
      break;
    default:;
  }
  return;
}

void FlexGridGraph::getPrevGrid(frMIdx& gridX,
                                frMIdx& gridY,
                                frMIdx& gridZ,
                                const frDirEnum dir) const
{
  switch (dir) {
    case frDirEnum::E:
      --gridX;
      break;
    case frDirEnum::S:
      ++gridY;
      break;
    case frDirEnum::W:
      ++gridX;
      break;
    case frDirEnum::N:
      --gridY;
      break;
    case frDirEnum::U:
      --gridZ;
      break;
    case frDirEnum::D:
      ++gridZ;
      break;
    default:;
  }
  return;
}

/*inline*/ frCost FlexGridGraph::getNextPathCost(
    const FlexWavefrontGrid& currGrid,
    const frDirEnum& dir) const
{
  // bool enableOutput = true;
  bool enableOutput = false;
  frMIdx gridX = currGrid.x();
  frMIdx gridY = currGrid.y();
  frMIdx gridZ = currGrid.z();
  frCost nextPathCost = currGrid.getPathCost();
  frCoord edgeLength = getEdgeLength(gridX, gridY, gridZ, dir);
  // bending cost
  auto currDir = currGrid.getLastDir();
  auto lNum = getLayerNum(currGrid.z());
  auto layer = getDesign()->getTech()->getLayer(lNum);

  if (currDir != dir && currDir != frDirEnum::UNKNOWN) {
    // original
    ++nextPathCost;
  }

  // via2viaForbiddenLen enablement
  if (dir == frDirEnum::U || dir == frDirEnum::D) {
    frCoord currVLengthX = 0;
    frCoord currVLengthY = 0;
    currGrid.getVLength(currVLengthX, currVLengthY);
    bool isCurrViaUp = (dir == frDirEnum::U);
    bool isForbiddenVia2Via = false;
    // check only y
    if (currVLengthX == 0 && currVLengthY > 0
        && getTech()->isVia2ViaForbiddenLen(gridZ,
                                            !(currGrid.isPrevViaUp()),
                                            !isCurrViaUp,
                                            false,
                                            currVLengthY,
                                            ndr_,
                                            false)) {
      isForbiddenVia2Via = true;
      // check only x
    } else if (currVLengthX > 0 && currVLengthY == 0
               && getTech()->isVia2ViaForbiddenLen(gridZ,
                                                   !(currGrid.isPrevViaUp()),
                                                   !isCurrViaUp,
                                                   true,
                                                   currVLengthX,
                                                   ndr_,
                                                   false)) {
      isForbiddenVia2Via = true;
      // check both x and y
    } else if (currVLengthX > 0 && currVLengthY > 0
               && (getTech()->isVia2ViaForbiddenLen(gridZ,
                                                    !(currGrid.isPrevViaUp()),
                                                    !isCurrViaUp,
                                                    false,
                                                    currVLengthY,
                                                    ndr_)
                   && getTech()->isVia2ViaForbiddenLen(
                       gridZ,
                       !(currGrid.isPrevViaUp()),
                       !isCurrViaUp,
                       true,
                       currVLengthX,
                       ndr_))) {
      isForbiddenVia2Via = true;
    }

    if (isForbiddenVia2Via) {
      if (drWorker_ && drWorker_->getDRIter() >= 3) {
        nextPathCost += ggMarkerCost_ * edgeLength;
      } else {
        nextPathCost += ggDRCCost_ * edgeLength;
      }
    }
  }

  // via2turn forbidden len enablement
  frCoord tLength = std::numeric_limits<frCoord>::max();
  frCoord tLengthDummy = 0;
  bool isTLengthViaUp = false;
  bool isForbiddenTLen = false;
  if (currDir != frDirEnum::UNKNOWN && currDir != dir) {
    // next dir is a via
    if (dir == frDirEnum::U || dir == frDirEnum::D) {
      isTLengthViaUp = (dir == frDirEnum::U);
      // if there was a turn before
      if (currDir == frDirEnum::W || currDir == frDirEnum::E) {
        tLength = currGrid.getTLength();
        if (getTech()->isViaForbiddenTurnLen(
                gridZ, !isTLengthViaUp, true, tLength, ndr_)) {
          isForbiddenTLen = true;
        }
      } else if (currDir == frDirEnum::S || currDir == frDirEnum::N) {
        tLength = currGrid.getTLength();
        if (getTech()->isViaForbiddenTurnLen(
                gridZ, !isTLengthViaUp, false, tLength, ndr_)) {
          isForbiddenTLen = true;
        }
      }
      // curr is a planar turn
    } else {
      isTLengthViaUp = currGrid.isPrevViaUp();
      if (currDir == frDirEnum::W || currDir == frDirEnum::E) {
        currGrid.getVLength(tLength, tLengthDummy);
        if (getTech()->isViaForbiddenTurnLen(
                gridZ, !isTLengthViaUp, true, tLength, ndr_)) {
          isForbiddenTLen = true;
        }
      } else if (currDir == frDirEnum::S || currDir == frDirEnum::N) {
        currGrid.getVLength(tLengthDummy, tLength);
        if (getTech()->isViaForbiddenTurnLen(
                gridZ, !isTLengthViaUp, false, tLength, ndr_)) {
          isForbiddenTLen = true;
        }
      }
    }
    if (isForbiddenTLen) {
      if (drWorker_ && drWorker_->getDRIter() >= 3) {
        nextPathCost += ggDRCCost_ * edgeLength;
      } else {
        nextPathCost += ggMarkerCost_ * edgeLength;
      }
    }
  }

  if (useNDRCosts(currGrid))
    nextPathCost += getCostsNDR(gridX, gridY, gridZ, dir, currDir, layer);
  else {
    nextPathCost += getCosts(gridX, gridY, gridZ, dir, layer);
    if (enableOutput) {
      cout << "edge grid/shape/drc/marker/blk/length = "
           << hasGridCost(gridX, gridY, gridZ, dir) << "/"
           << hasShapeCost(gridX, gridY, gridZ, dir) << "/"
           << hasDRCCost(gridX, gridY, gridZ, dir) << "/"
           << hasMarkerCost(gridX, gridY, gridZ, dir) << "/"
           << isBlocked(gridX, gridY, gridZ, dir) << "/"
           << getEdgeLength(gridX, gridY, gridZ, dir) << endl;
    }
  }

  return nextPathCost;
}

frCoord FlexGridGraph::getCostsNDR(frMIdx gridX,
                                   frMIdx gridY,
                                   frMIdx gridZ,
                                   frDirEnum dir,
                                   frDirEnum prevDir,
                                   frLayer* layer) const
{
  if ((dir == frDirEnum::U || dir == frDirEnum::D) /*&& (dir != prevDir || ndr_->getPrefVia(dir == frDirEnum::U ? gridZ : gridZ-1))*/)
    return getViaCostsNDR(gridX, gridY, gridZ, dir, prevDir, layer);
  frCoord el = getEdgeLength(gridX, gridY, gridZ, dir);
  frCoord cost = el;
  cost += (hasGridCost(gridX, gridY, gridZ, dir) ? GRIDCOST * el : 0);
  cost += (!hasGuide(gridX, gridY, gridZ, dir) ? GUIDECOST * el : 0);
  frMIdx startX, startY, endX, endY;
  frCoord r, x1, x2, y1, y2;
  frCoord sp, wext;
  frCoord layerWidth = max((int) layer->getMinWidth(), ndr_->getWidth(gridZ));
  sp = max(ndr_->getSpacing(gridZ),
           getMinSpacingValue(layer, layerWidth, layer->getMinWidth(), 0));
  wext = max(ndr_->getWireExtension(gridZ), (int) layer->getMinWidth() / 2)
         - layer->getMinWidth() / 2;

  // get iteration bounds
  r = layerWidth / 2 + sp + layer->getMinWidth() / 2 - 1;
  if (dir == frDirEnum::N || dir == frDirEnum::S) {
    startX = getLowerBoundIndex(xCoords_, x1 = (xCoords_[gridX] - r));
    endX = getUpperBoundIndex(xCoords_, x2 = (xCoords_[gridX] + r));
    startY = endY = gridY;
    y1 = y2 = yCoords_[startY];
    if (prevDir == frDirEnum::UNKNOWN || prevDir != dir) {
      if (dir == frDirEnum::N)
        startY
            = getLowerBoundIndex(yCoords_, y1 = (yCoords_[gridY] - r - wext));
      else
        endY = getUpperBoundIndex(yCoords_, y2 = (yCoords_[gridY] + r + wext));
    }
    if (prevDir != frDirEnum::UNKNOWN) {
      getNextGrid(gridX, gridY, gridZ, dir);
      if (isDst(gridX, gridY, gridZ)) {
        if (dir == frDirEnum::N)
          endY
              = getUpperBoundIndex(yCoords_, y2 = (yCoords_[gridY] + r + wext));
        else
          startY
              = getLowerBoundIndex(yCoords_, y1 = (yCoords_[gridY] - r - wext));
      }
      getPrevGrid(gridX, gridY, gridZ, dir);
    }
  } else {
    startY = getLowerBoundIndex(yCoords_, y1 = (yCoords_[gridY] - r));
    endY = getUpperBoundIndex(yCoords_, y2 = (yCoords_[gridY] + r));
    startX = endX = gridX;
    x1 = x2 = xCoords_[startX];
    if (prevDir == frDirEnum::UNKNOWN || prevDir != dir) {
      if (dir == frDirEnum::E)
        startX
            = getLowerBoundIndex(xCoords_, x1 = (xCoords_[gridX] - r - wext));
      else
        endX = getUpperBoundIndex(xCoords_, x2 = (xCoords_[gridX] + r + wext));
    }
    if (prevDir != frDirEnum::UNKNOWN) {
      getNextGrid(gridX, gridY, gridZ, dir);
      if (isDst(gridX, gridY, gridZ)) {
        if (dir == frDirEnum::E)
          endX
              = getUpperBoundIndex(xCoords_, x2 = (xCoords_[gridX] + r + wext));
        else
          startX
              = getLowerBoundIndex(xCoords_, x1 = (xCoords_[gridX] - r - wext));
      }
      getPrevGrid(gridX, gridY, gridZ, dir);
    }
  }
  if (xCoords_[startX] < x1)
    startX++;
  if (xCoords_[endX] > x2)
    endX--;
  if (yCoords_[startY] < y1)
    startY++;
  if (yCoords_[endY] > y2)
    endY--;
  // get costs
  for (frMIdx x = startX; x <= endX; x++) {
    for (frMIdx y = startY; y <= endY; y++) {
      cost += (hasShapeCost(x, y, gridZ, dir) ? SHAPECOST * el : 0);
      cost += (hasDRCCost(x, y, gridZ, dir) ? ggDRCCost_ * el : 0);
      cost += (hasMarkerCost(x, y, gridZ, dir) ? ggMarkerCost_ * el : 0);
      cost += (isBlocked(x, y, gridZ, dir)
                   ? BLOCKCOST * layer->getMinWidth() * 20
                   : 0);
    }
  }
  return cost;
}

frCoord FlexGridGraph::getViaCostsNDR(frMIdx gridX,
                                      frMIdx gridY,
                                      frMIdx gridZ,
                                      frDirEnum dir,
                                      frDirEnum prevDir,
                                      frLayer* layer) const
{
  if (ndr_->getPrefVia(dir == frDirEnum::U ? gridZ : gridZ - 1) == nullptr
      /*&& ndr_->getSpacing(dir == frDirEnum::U ? gridZ : gridZ-1) == 0*/)
    return getCosts(gridX, gridY, gridZ, dir, layer);
  frMIdx startX, startY, endX, endY;
  frCoord x1, x2, y1, y2;
  frCoord layerWidth = max((int) layer->getMinWidth(), ndr_->getWidth(gridZ));
  frCoord r,
      sp = max(ndr_->getSpacing(gridZ),
               getMinSpacingValue(layer, layerWidth, layer->getMinWidth(), 0));

  // get iteration bounds
  r = layerWidth / 2 + sp + layer->getMinWidth() / 2 - 1;
  frCoord el = getEdgeLength(gridX, gridY, gridZ, dir);
  frCoord cost = el;

  startX = getLowerBoundIndex(xCoords_, x1 = (xCoords_[gridX] - r));
  endX = getUpperBoundIndex(xCoords_, x2 = (xCoords_[gridX] + r));
  startY = getLowerBoundIndex(yCoords_, y1 = (yCoords_[gridY] - r));
  endY = getUpperBoundIndex(yCoords_, y2 = (yCoords_[gridY] + r));
  cost += (hasShapeCost(gridX, gridY, gridZ, dir) ? SHAPECOST * el : 0);
  cost += (hasDRCCost(gridX, gridY, gridZ, dir) ? ggDRCCost_ * el : 0);
  cost += (hasMarkerCost(gridX, gridY, gridZ, dir) ? ggMarkerCost_ * el : 0);
  cost += (isBlocked(gridX, gridY, gridZ, dir)
               ? BLOCKCOST * layer->getMinWidth() * 20
               : 0);
  dir = frDirEnum::UNKNOWN;

  if (xCoords_[startX] < x1)
    startX++;
  if (xCoords_[endX] > x2)
    endX--;
  if (yCoords_[startY] < y1)
    startY++;
  if (yCoords_[endY] > y2)
    endY--;
  switch (prevDir) {
    case frDirEnum::N:
      endY = gridY - 1;
      break;
    case frDirEnum::S:
      startY = gridY + 1;
      break;
    case frDirEnum::E:
      startX = gridX + 1;
      break;
    case frDirEnum::W:
      endX = gridX - 1;
      break;
    default:
      break;
  }
  // get costs
  for (frMIdx x = startX; x <= endX; x++) {
    for (frMIdx y = startY; y <= endY; y++) {
      cost += (hasShapeCost(x, y, gridZ, dir) ? SHAPECOST * el : 0);
      cost += (hasDRCCost(x, y, gridZ, dir) ? ggDRCCost_ * el : 0);
      cost += (hasMarkerCost(x, y, gridZ, dir) ? ggMarkerCost_ * el : 0);
    }
  }
  return cost;
}

frCost FlexGridGraph::getCosts(frMIdx gridX,
                               frMIdx gridY,
                               frMIdx gridZ,
                               frDirEnum dir,
                               frLayer* layer) const
{
  bool gridCost = hasGridCost(gridX, gridY, gridZ, dir);
  bool drcCost = hasDRCCost(gridX, gridY, gridZ, dir);
  bool markerCost = hasMarkerCost(gridX, gridY, gridZ, dir);
  bool shapeCost = hasShapeCost(gridX, gridY, gridZ, dir);
  bool blockCost = isBlocked(gridX, gridY, gridZ, dir);
  bool guideCost = hasGuide(gridX, gridY, gridZ, dir);
  frCoord edgeLength = getEdgeLength(gridX, gridY, gridZ, dir);

  // temporarily disable guideCost
  return getEdgeLength(gridX, gridY, gridZ, dir)
         + (gridCost ? GRIDCOST * edgeLength : 0)
         + (drcCost ? ggDRCCost_ * edgeLength : 0)
         + (markerCost ? ggMarkerCost_ * edgeLength : 0)
         + (shapeCost ? SHAPECOST * edgeLength : 0)
         + (blockCost ? BLOCKCOST * layer->getMinWidth() * 20 : 0)
         + (!guideCost ? GUIDECOST * edgeLength : 0);
}

bool FlexGridGraph::useNDRCosts(const FlexWavefrontGrid& p) const
{
  if (ndr_) {
    if (p.getSrcTaperBox() && p.getSrcTaperBox()->contains(p.x(), p.y(), p.z()))
      return false;
    if (dstTaperBox && dstTaperBox->contains(p.x(), p.y(), p.z()))
      return false;
    return true;
  }
  return false;
}
frCoord FlexGridGraph::getMinSpacingValue(frLayer* layer,
                                          frCoord width1,
                                          frCoord width2,
                                          frCoord prl) const
{
  auto con = layer->getMinSpacing();
  if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint)
    return static_cast<frSpacingConstraint*>(con)->getMinSpacing();

  if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint)
    return static_cast<frSpacingTablePrlConstraint*>(con)->find(width1, prl);

  if (con->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint)
    return static_cast<frSpacingTableTwConstraint*>(con)->find(
        width1, width2, prl);
  drWorker_->getLogger()->error(
      utl::ToolId::DRT, 0, "ERROR FlexGridGraph::getMinSpacingValue");
}

frMIdx FlexGridGraph::getLowerBoundIndex(const frVector<frCoord>& tracks,
                                         frCoord v) const
{
  return std::lower_bound(tracks.begin(), tracks.end(), v) - tracks.begin();
}

frMIdx FlexGridGraph::getUpperBoundIndex(const frVector<frCoord>& tracks,
                                         frCoord v) const
{
  auto it = std::upper_bound(tracks.begin(), tracks.end(), v);
  if (it == tracks.end())
    it = std::prev(it);
  return it - tracks.begin();
}

/*inline*/ FlexMazeIdx FlexGridGraph::getTailIdx(
    const FlexMazeIdx& currIdx,
    const FlexWavefrontGrid& currGrid) const
{
  int gridX = currIdx.x();
  int gridY = currIdx.y();
  int gridZ = currIdx.z();
  auto backTraceBuffer = currGrid.getBackTraceBuffer();
  for (int i = 0; i < WAVEFRONTBUFFERSIZE; ++i) {
    int currDirVal
        = backTraceBuffer.to_ulong()
          - ((backTraceBuffer.to_ulong() >> DIRBITSIZE) << DIRBITSIZE);
    frDirEnum currDir = static_cast<frDirEnum>(currDirVal);
    backTraceBuffer >>= DIRBITSIZE;
    getPrevGrid(gridX, gridY, gridZ, currDir);
  }
  return FlexMazeIdx(gridX, gridY, gridZ);
}

/*inline*/ bool FlexGridGraph::isExpandable(const FlexWavefrontGrid& currGrid,
                                            frDirEnum dir) const
{
  // bool enableOutput = true;
  bool enableOutput = false;
  frMIdx gridX = currGrid.x();
  frMIdx gridY = currGrid.y();
  frMIdx gridZ = currGrid.z();
  bool hg = hasEdge(gridX, gridY, gridZ, dir);
  if (enableOutput) {
    if (!hasEdge(gridX, gridY, gridZ, dir)) {
      cout << "no edge@(" << gridX << ", " << gridY << ", " << gridZ << ") "
           << (int) dir << endl;
    }
    if (hasEdge(gridX, gridY, gridZ, dir)
        && !hasGuide(gridX, gridY, gridZ, dir)) {
      cout << "no guide@(" << gridX << ", " << gridY << ", " << gridZ << ") "
           << (int) dir << endl;
    }
  }
  reverse(gridX, gridY, gridZ, dir);
  if (!hg || isSrc(gridX, gridY, gridZ)
      || (getPrevAstarNodeDir(gridX, gridY, gridZ) != frDirEnum::UNKNOWN)
      ||  // comment out for non-buffer enablement
      currGrid.getLastDir() == dir) {
    return false;
  } else {
    return true;
  }
}

void FlexGridGraph::traceBackPath(const FlexWavefrontGrid& currGrid,
                                  vector<FlexMazeIdx>& path,
                                  vector<FlexMazeIdx>& root,
                                  FlexMazeIdx& ccMazeIdx1,
                                  FlexMazeIdx& ccMazeIdx2) const
{
  // bool enableOutput = true;
  bool enableOutput = false;
  if (enableOutput) {
    cout << "    start traceBackPath...\n";
  }
  frDirEnum prevDir = frDirEnum::UNKNOWN, currDir = frDirEnum::UNKNOWN;
  int currX = currGrid.x(), currY = currGrid.y(), currZ = currGrid.z();
  // pop content in buffer
  auto backTraceBuffer = currGrid.getBackTraceBuffer();
  for (int i = 0; i < WAVEFRONTBUFFERSIZE; ++i) {
    // current grid is src
    if (isSrc(currX, currY, currZ)) {
      break;
    }
    // get last direction
    currDir = getLastDir(backTraceBuffer);
    backTraceBuffer >>= DIRBITSIZE;
    if (currDir == frDirEnum::UNKNOWN) {
      cout << "Warning: unexpected direction in tracBackPath\n";
      break;
    }
    root.push_back(FlexMazeIdx(currX, currY, currZ));
    // push point to path
    if (currDir != prevDir) {
      path.push_back(FlexMazeIdx(currX, currY, currZ));
      if (enableOutput) {
        cout << " -- (" << currX << ", " << currY << ", " << currZ << ")";
      }
    }
    getPrevGrid(currX, currY, currZ, currDir);
    prevDir = currDir;
  }
  // trace back according to grid prev dir
  while (isSrc(currX, currY, currZ) == false) {
    // get last direction
    currDir = getPrevAstarNodeDir(currX, currY, currZ);
    root.push_back(FlexMazeIdx(currX, currY, currZ));
    if (currDir == frDirEnum::UNKNOWN) {
      cout << "Warning: unexpected direction in tracBackPath\n";
      break;
    }
    if (currDir != prevDir) {
      path.push_back(FlexMazeIdx(currX, currY, currZ));
      if (enableOutput) {
        cout << " -- (" << currX << ", " << currY << ", " << currZ << ")";
      }
    }
    getPrevGrid(currX, currY, currZ, currDir);
    prevDir = currDir;
  }
  // add final path to src, only add when path exists; no path exists (src =
  // dst)
  if (!path.empty()) {
    path.push_back(FlexMazeIdx(currX, currY, currZ));
    if (enableOutput) {
      cout << " -- (" << currX << ", " << currY << ", " << currZ << ")";
    }
  }
  for (auto& mi : path) {
    ccMazeIdx1.set(min(ccMazeIdx1.x(), mi.x()),
                   min(ccMazeIdx1.y(), mi.y()),
                   min(ccMazeIdx1.z(), mi.z()));
    ccMazeIdx2.set(max(ccMazeIdx2.x(), mi.x()),
                   max(ccMazeIdx2.y(), mi.y()),
                   max(ccMazeIdx2.z(), mi.z()));
  }
  if (enableOutput) {
    cout << endl;
  }
}

bool FlexGridGraph::search(vector<FlexMazeIdx>& connComps,
                           drPin* nextPin,
                           vector<FlexMazeIdx>& path,
                           FlexMazeIdx& ccMazeIdx1,
                           FlexMazeIdx& ccMazeIdx2,
                           const frPoint& centerPt,
                           map<FlexMazeIdx, frBox3D*>& mazeIdx2TaperBox)
{
  // bool enableOutput = true;
  bool enableOutput = false;
  int stepCnt = 0;

  // prep nextPinBox
  frMIdx xDim, yDim, zDim;
  getDim(xDim, yDim, zDim);
  FlexMazeIdx dstMazeIdx1(xDim - 1, yDim - 1, zDim - 1);
  FlexMazeIdx dstMazeIdx2(0, 0, 0);
  FlexMazeIdx mi;
  for (auto& ap : nextPin->getAccessPatterns()) {
    ap->getMazeIdx(mi);
    dstMazeIdx1.set(min(dstMazeIdx1.x(), mi.x()),
                    min(dstMazeIdx1.y(), mi.y()),
                    min(dstMazeIdx1.z(), mi.z()));
    dstMazeIdx2.set(max(dstMazeIdx2.x(), mi.x()),
                    max(dstMazeIdx2.y(), mi.y()),
                    max(dstMazeIdx2.z(), mi.z()));
  }

  wavefront_.cleanup();
  // init wavefront
  frPoint currPt;
  for (auto& idx : connComps) {
    if (isDst(idx.x(), idx.y(), idx.z())) {
      if (enableOutput) {
        cout << "message: astarSearch dst covered (" << idx.x() << ", "
             << idx.y() << ", " << idx.z() << ")" << endl;
      }
      path.push_back(FlexMazeIdx(idx.x(), idx.y(), idx.z()));
      return true;
    }
    // get min area min length
    auto lNum = getLayerNum(idx.z());
    auto minAreaConstraint
        = getDesign()->getTech()->getLayer(lNum)->getAreaConstraint();
    frCoord fakeArea = minAreaConstraint ? minAreaConstraint->getMinArea() : 0;
    getPoint(currPt, idx.x(), idx.y());
    frCoord currDist
        = abs(currPt.x() - centerPt.x()) + abs(currPt.y() - centerPt.y());
    FlexWavefrontGrid currGrid(
        idx.x(),
        idx.y(),
        idx.z(),
        fakeArea,
        std::numeric_limits<frCoord>::max(),
        std::numeric_limits<frCoord>::max(),
        true,
        std::numeric_limits<frCoord>::max(),
        currDist,
        0,
        getEstCost(idx, dstMazeIdx1, dstMazeIdx2, frDirEnum::UNKNOWN));
    if (ndr_ && AUTO_TAPER_NDR_NETS) {
      auto it = mazeIdx2TaperBox.find(idx);
      if (it != mazeIdx2TaperBox.end())
        currGrid.setSrcTaperBox(it->second);
    }
    wavefront_.push(currGrid);
    if (enableOutput) {
      cout << "src add to wavefront (" << idx.x() << ", " << idx.y() << ", "
           << idx.z() << ")" << endl;
    }
  }
  while (!wavefront_.empty()) {
    auto currGrid = wavefront_.top();
    wavefront_.pop();
    if (getPrevAstarNodeDir(currGrid.x(), currGrid.y(), currGrid.z())
        != frDirEnum::UNKNOWN) {
      continue;
    }
    if (graphics_) {
      graphics_->searchNode(this, currGrid);
    }

    if (enableOutput) {
      ++stepCnt;
    }
    // if (stepCnt % 100000 == 0) {
    //   std::cout << "wavefront size = " << wavefront.size() << " at step = "
    //   << stepCnt << "\n";
    // }
    if (isDst(currGrid.x(), currGrid.y(), currGrid.z())) {
      traceBackPath(currGrid, path, connComps, ccMazeIdx1, ccMazeIdx2);
      if (enableOutput) {
        cout << "path found. stepCnt = " << stepCnt << "\n";
      }
      return true;
    } else {
      // expand and update wavefront
      expandWavefront(currGrid, dstMazeIdx1, dstMazeIdx2, centerPt);
    }
  }
  return false;
}
