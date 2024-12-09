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

#include <limits>

#include "dr/FlexDR.h"
#include "dr/FlexDR_graphics.h"
#include "dr/FlexGridGraph.h"

namespace drt {
const int debugMazeIter = std::numeric_limits<int>::max();

void FlexGridGraph::printExpansion(const FlexWavefrontGrid& currGrid,
                                   const std::string& keyword)
{
  auto dir = currGrid.getLastDir();
  auto gridX = currGrid.x();
  auto gridY = currGrid.y();
  auto gridZ = currGrid.z();
  dir = (frDirEnum) (OPPOSITEDIR - (int) dir);
  bool gridCost = hasGridCost(gridX, gridY, gridZ, dir);
  bool drcCost = hasRouteShapeCostAdj(gridX, gridY, gridZ, dir, false);
  bool markerCost = hasMarkerCostAdj(gridX, gridY, gridZ, dir);
  bool shapeCost = hasFixedShapeCostAdj(gridX, gridY, gridZ, dir, false);
  bool blockCost = isBlocked(gridX, gridY, gridZ, dir);
  bool guideCost = hasGuide(gridX, gridY, gridZ, dir);
  frCoord edgeLength = getEdgeLength(gridX, gridY, gridZ, dir);
  Point pt;
  getPoint(pt, currGrid.x(), currGrid.y());
  dump_file_ << fmt::format(
      "{} {} pt {} cost {} pathCost {} lastDir {} estCost {} gridX {} gridY "
      "{} ",
      keyword,
      currGrid.z(),
      pt,
      currGrid.getCost(),
      currGrid.getPathCost(),
      currGrid.getLastDir(),
      currGrid.getCost() - currGrid.getPathCost(),
      gridX,
      gridY);
  dump_file_ << fmt::format(
      "gridCost {} drcCost {} markerCost {} shapeCost {} blockCost {} "
      "guideCost {} edgeLength {} ",
      gridCost,
      drcCost,
      markerCost,
      shapeCost,
      blockCost,
      guideCost,
      edgeLength);
  dump_file_ << fmt::format(
      "id {} parent_id {}\n", currGrid.getId(), currGrid.getParentId());
  dump_file_.flush();
}
void FlexGridGraph::expand(FlexWavefrontGrid& currGrid,
                           const frDirEnum& dir,
                           const FlexMazeIdx& dstMazeIdx1,
                           const FlexMazeIdx& dstMazeIdx2,
                           const Point& centerPt,
                           ExpandDir expandDir,
                           bool route_with_jumpers)
{
  frCost nextEstCost, nextPathCost;
  int gridX = currGrid.x();
  int gridY = currGrid.y();
  int gridZ = currGrid.z();

  getNextGrid(gridX, gridY, gridZ, dir);

  FlexMazeIdx nextIdx(gridX, gridY, gridZ);
  // get cost
  nextEstCost
      = getEstCost(FlexMazeIdx(currGrid.x(), currGrid.y(), currGrid.z()),
                   dstMazeIdx1,
                   dstMazeIdx2,
                   dir);
  nextPathCost = getNextPathCost(currGrid, dir, route_with_jumpers);

  Point currPt;
  getPoint(currPt, gridX, gridY);
  frCoord currDist = Point::manhattanDistance(currPt, centerPt);

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

  FlexWavefrontGrid nextWavefrontGrid(gridX,
                                      gridY,
                                      gridZ,
                                      nextVLengthX,
                                      nextVLengthY,
                                      nextIsPrevViaUp,
                                      nextTLength,
                                      currDist,
                                      nextPathCost,
                                      nextPathCost + nextEstCost,
                                      currGrid.getBackTraceBuffer(),
                                      currGrid.getBackTraceCosts());
  if (dir == frDirEnum::U || dir == frDirEnum::D) {
    nextWavefrontGrid.resetLength();
    if (dir == frDirEnum::U) {
      nextWavefrontGrid.setPrevViaUp(false);
    } else {
      nextWavefrontGrid.setPrevViaUp(true);
    }
  }
  if (currGrid.getSrcTaperBox()
      && currGrid.getSrcTaperBox()->contains(nextWavefrontGrid.x(),
                                             nextWavefrontGrid.y(),
                                             nextWavefrontGrid.z())) {
    nextWavefrontGrid.setSrcTaperBox(currGrid.getSrcTaperBox());
  }
  // update wavefront buffer
  auto [tailDir, tailCost]
      = nextWavefrontGrid.shiftAddBuffer(dir, currGrid.getPathCost());
  // non-buffer enablement is faster for ripup all
  // commit grid prev direction if needed
  auto tailIdx = getTailIdx(nextIdx, nextWavefrontGrid);
  auto& wavefront = (expandDir == ExpandDir::FORWARD) ? wavefrontForward_
                                                      : wavefrontBackward_;
  if (tailDir != frDirEnum::UNKNOWN) {
    if (getPrevAstarNodeDir(tailIdx) == frDirEnum::UNKNOWN
        || getPrevAstarNodeDir(tailIdx) == tailDir) {
      setPrevAstarNodeDir(
          tailIdx.x(), tailIdx.y(), tailIdx.z(), expandDir, tailDir);
      setPathCost(tailIdx.x(), tailIdx.y(), tailIdx.z(), tailCost);
      if (debug_) {
        nextWavefrontGrid.setId(curr_id_++);
        nextWavefrontGrid.setParentId(currGrid.getId());
        printExpansion(nextWavefrontGrid, "Pushing");
      }
      wavefront.push(nextWavefrontGrid);
    }
  } else {
    // add to wavefront
    if (debug_) {
      nextWavefrontGrid.setId(curr_id_++);
      nextWavefrontGrid.setParentId(currGrid.getId());
      printExpansion(nextWavefrontGrid, "Pushing");
    }
    wavefront.push(nextWavefrontGrid);
  }
  if (drWorker_->getDRIter() >= debugMazeIter) {
    std::cout << "Creating " << nextWavefrontGrid.x() << " "
              << nextWavefrontGrid.y() << " " << nextWavefrontGrid.z()
              << " coords: " << xCoords_[nextWavefrontGrid.x()] << " "
              << yCoords_[nextWavefrontGrid.y()] << " cost "
              << nextWavefrontGrid.getCost() << " g "
              << nextWavefrontGrid.getPathCost() << "\n";
  }
}

void FlexGridGraph::expandWavefront(FlexWavefrontGrid& currGrid,
                                    const FlexMazeIdx& dstMazeIdx1,
                                    const FlexMazeIdx& dstMazeIdx2,
                                    const Point& centerPt,
                                    ExpandDir expandDir,
                                    bool route_with_jumpers)
{
  for (const auto dir : frDirEnumAll) {
    if (isExpandable(currGrid, dir, expandDir)) {
      expand(currGrid,
             dir,
             dstMazeIdx1,
             dstMazeIdx2,
             centerPt,
             expandDir,
             route_with_jumpers);
    }
  }
}

frCost FlexGridGraph::getEstCost(const FlexMazeIdx& src,
                                 const FlexMazeIdx& dstMazeIdx1,
                                 const FlexMazeIdx& dstMazeIdx2,
                                 const frDirEnum& dir) const
{
  int gridX = src.x();
  int gridY = src.y();
  int gridZ = src.z();
  auto edgeLength = getEdgeLength(gridX, gridY, gridZ, dir);
  getNextGrid(gridX, gridY, gridZ, dir);
  // bend cost
  int bendCnt = 0;
  int forbiddenPenalty = 0;
  Point srcPoint, dstPoint1, dstPoint2;
  getPoint(srcPoint, gridX, gridY);
  getPoint(dstPoint1, dstMazeIdx1.x(), dstMazeIdx1.y());
  getPoint(dstPoint2, dstMazeIdx2.x(), dstMazeIdx2.y());
  frCoord minCostX = std::max(std::max(dstPoint1.x() - srcPoint.x(),
                                       srcPoint.x() - dstPoint2.x()),
                              0)
                     * 1;
  frCoord minCostY = std::max(std::max(dstPoint1.y() - srcPoint.y(),
                                       srcPoint.y() - dstPoint2.y()),
                              0)
                     * 1;
  frCoord minCostZ
      = std::max(std::max(getZHeight(dstMazeIdx1.z()) - getZHeight(gridZ),
                          getZHeight(gridZ) - getZHeight(dstMazeIdx2.z())),
                 0)
        * 1;

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

  // If we are on the destination layer we will have to wrong way jog or
  // via up/down or down/up so add the cheapest of those to the estimate
  if (src.z() == dstMazeIdx1.z() && dstMazeIdx1.z() == dstMazeIdx2.z()) {
  }

  Point nextPoint;
  getPoint(nextPoint, gridX, gridY);
  // avoid propagating to location that will cause forbidden via spacing to
  // boundary pin
  bool isForbidden = false;
  if (dstMazeIdx1.z() == dstMazeIdx2.z() && gridZ == dstMazeIdx1.z()) {
    auto layerNum = (gridZ + 1) * 2;
    auto layer = getTech()->getLayer(layerNum);
    if (!router_cfg_->USENONPREFTRACKS || layer->isUnidirectional()) {
      bool isH = (layer->getDir() == dbTechLayerDir::HORIZONTAL);
      if (isH && dstMazeIdx1.y() == dstMazeIdx2.y()) {
        auto gap = abs(nextPoint.y() - dstPoint1.y());
        if (gap
            && (layerNum - 2 < router_cfg_->BOTTOM_ROUTING_LAYER
                || getTech()->isVia2ViaForbiddenLen(
                    gridZ - 1, false, false, false, gap, ndr_))
            && (layerNum + 2 > getTech()->getTopLayerNum()
                || getTech()->isVia2ViaForbiddenLen(
                    gridZ + 1, true, true, false, gap, ndr_))) {
          isForbidden = true;
        }
      } else if (!isH && dstMazeIdx1.x() == dstMazeIdx2.x()) {
        auto gap = abs(nextPoint.x() - dstPoint1.x());
        if (gap
            && (layerNum - 2 < router_cfg_->BOTTOM_ROUTING_LAYER
                || getTech()->isVia2ViaForbiddenLen(
                    gridZ - 1, false, false, true, gap, ndr_))
            && (layerNum + 2 > getTech()->getTopLayerNum()
                || getTech()->isVia2ViaForbiddenLen(
                    gridZ + 1, true, true, true, gap, ndr_))) {
          isForbidden = true;
        }
      }
    }
  }
  if (isForbidden) {
    if (drWorker_->getDRIter() >= 3) {
      forbiddenPenalty = 2 * ggMarkerCost_ * edgeLength;
    } else {
      forbiddenPenalty = 2 * ggDRCCost_ * edgeLength;
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
    case frDirEnum::UNKNOWN:
      break;
  }
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
    case frDirEnum::UNKNOWN:
      break;
  }
}

frCost FlexGridGraph::getNextPathCost(const FlexWavefrontGrid& currGrid,
                                      const frDirEnum& dir,
                                      bool route_with_jumpers) const
{
  frMIdx gridX = currGrid.x();
  frMIdx gridY = currGrid.y();
  frMIdx gridZ = currGrid.z();
  frCost nextPathCost = currGrid.getPathCost();
  frCoord edgeLength = getEdgeLength(gridX, gridY, gridZ, dir);
  // bending cost
  auto currDir = currGrid.getLastDir();
  auto lNum = getLayerNum(currGrid.z());
  auto layer = getTech()->getLayer(lNum);

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
    if (currVLengthX == 0 && currVLengthY > 0) {
      isForbiddenVia2Via
          = getTech()->isVia2ViaForbiddenLen(gridZ,
                                             !(currGrid.isPrevViaUp()),
                                             !isCurrViaUp,
                                             false,
                                             currVLengthY,
                                             ndr_);
      // check only x
    } else if (currVLengthX > 0 && currVLengthY == 0) {
      isForbiddenVia2Via
          = getTech()->isVia2ViaForbiddenLen(gridZ,
                                             !(currGrid.isPrevViaUp()),
                                             !isCurrViaUp,
                                             true,
                                             currVLengthX,
                                             ndr_);
      // check both x and y
    } else {
      if (getTech()->isVia2ViaPRL(gridZ,
                                  !(currGrid.isPrevViaUp()),
                                  !isCurrViaUp,
                                  false,
                                  currVLengthY)
          || getTech()->isVia2ViaPRL(gridZ,
                                     !(currGrid.isPrevViaUp()),
                                     !isCurrViaUp,
                                     true,
                                     currVLengthX)) {
        isForbiddenVia2Via
            = getTech()->isVia2ViaForbiddenLen(gridZ,
                                               !(currGrid.isPrevViaUp()),
                                               !isCurrViaUp,
                                               false,
                                               currVLengthY,
                                               ndr_)
              || getTech()->isVia2ViaForbiddenLen(gridZ,
                                                  !(currGrid.isPrevViaUp()),
                                                  !isCurrViaUp,
                                                  true,
                                                  currVLengthX,
                                                  ndr_);
      } else {
        isForbiddenVia2Via
            = getTech()->isVia2ViaForbiddenLen(gridZ,
                                               !(currGrid.isPrevViaUp()),
                                               !isCurrViaUp,
                                               false,
                                               currVLengthY,
                                               ndr_)
              && getTech()->isVia2ViaForbiddenLen(gridZ,
                                                  !(currGrid.isPrevViaUp()),
                                                  !isCurrViaUp,
                                                  true,
                                                  currVLengthX,
                                                  ndr_);
      }
    }

    if (isForbiddenVia2Via) {
      if (drWorker_) {
        if (drWorker_->getDRIter() >= debugMazeIter) {
          std::cout << "isForbiddenVia2Via\n";
        }
        if (drWorker_->getDRIter() >= 3) {
          nextPathCost += 2 * ggMarkerCost_ * edgeLength;
        } else {
          nextPathCost += 2 * ggDRCCost_ * edgeLength;
        }
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
      if (drWorker_) {
        if (drWorker_->getDRIter() >= debugMazeIter) {
          std::cout << "isForbiddenTLen\n";
        }
        if (drWorker_->getDRIter() >= 3) {
          nextPathCost += 2 * ggDRCCost_ * edgeLength;
        } else {
          nextPathCost += 2 * ggMarkerCost_ * edgeLength;
        }
      }
    }
  }
  nextPathCost += getCosts(gridX,
                           gridY,
                           gridZ,
                           dir,
                           layer,
                           useNDRCosts(currGrid),
                           route_with_jumpers);

  return nextPathCost;
}

frCost FlexGridGraph::getCosts(frMIdx gridX,
                               frMIdx gridY,
                               frMIdx gridZ,
                               frDirEnum dir,
                               frLayer* layer,
                               bool considerNDR,
                               bool route_with_jumpers) const
{
  bool gridCost = hasGridCost(gridX, gridY, gridZ, dir);
  bool drcCost = hasRouteShapeCostAdj(gridX, gridY, gridZ, dir, considerNDR);
  bool markerCost = hasMarkerCostAdj(gridX, gridY, gridZ, dir);
  bool shapeCost = hasFixedShapeCostAdj(gridX, gridY, gridZ, dir, considerNDR);
  bool blockCost = isBlocked(gridX, gridY, gridZ, dir);
  bool guideCost = hasGuide(gridX, gridY, gridZ, dir);
  frCoord edgeLength = getEdgeLength(gridX, gridY, gridZ, dir);

  // increase cost when a net has jumper
  frUInt4 jumper_cost = route_with_jumpers ? 10 : 1;

  // temporarily disable guideCost
  return getEdgeLength(gridX, gridY, gridZ, dir)
         + (gridCost ? router_cfg_->GRIDCOST * edgeLength : 0)
         + (drcCost ? ggDRCCost_ * edgeLength : 0)
         + (markerCost ? ggMarkerCost_ * edgeLength : 0)
         + (shapeCost ? ggFixedShapeCost_ * edgeLength : 0)
         + (blockCost ? router_cfg_->BLOCKCOST * layer->getMinWidth() * 20 : 0)
         + (!guideCost ? (router_cfg_->GUIDECOST * jumper_cost) * edgeLength
                       : 0);
}

bool FlexGridGraph::useNDRCosts(const FlexWavefrontGrid& p) const
{
  if (ndr_) {
    if (p.getSrcTaperBox()
        && p.getSrcTaperBox()->contains(p.x(), p.y(), p.z())) {
      return false;
    }
    if (dstTaperBox && dstTaperBox->contains(p.x(), p.y(), p.z())) {
      return false;
    }
    return true;
  }
  return false;
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
  if (it == tracks.end()) {
    it = std::prev(it);
  }
  return it - tracks.begin();
}

FlexMazeIdx FlexGridGraph::getTailIdx(const FlexMazeIdx& currIdx,
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

bool FlexGridGraph::isExpandable(const FlexWavefrontGrid& currGrid,
                                 frDirEnum dir,
                                 ExpandDir expandDir) const
{
  frMIdx gridX = currGrid.x();
  frMIdx gridY = currGrid.y();
  frMIdx gridZ = currGrid.z();
  bool hg = hasEdge(gridX, gridY, gridZ, dir);
  reverse(gridX, gridY, gridZ, dir);
  if (!hg || isSrc(gridX, gridY, gridZ, expandDir)
      || isClosed(gridX, gridY, gridZ, expandDir)
      ||  // comment out for non-buffer enablement
      currGrid.getLastDir() == dir) {
    return false;
  }
  if (ndr_) {
    frCoord halfWidth
        = (frCoord) getTech()->getLayer(getLayerNum(currGrid.z()))->getWidth()
          / 2;
    if (ndr_->getWidth(currGrid.z()) > 2 * halfWidth
        && !isSrc(currGrid.x(), currGrid.y(), currGrid.z(), expandDir)) {
      halfWidth = ndr_->getWidth(currGrid.z()) / 2;
      // if the expansion goes parallel to a die border and the wire goes out of
      // the die box, forbid expansion
      if (dir == frDirEnum::N || dir == frDirEnum::S) {
        if (xCoords_[currGrid.x()] - halfWidth < dieBox_.xMin()
            || xCoords_[currGrid.x()] + halfWidth > dieBox_.xMax()) {
          return false;
        }
      } else if (dir == frDirEnum::E || dir == frDirEnum::W) {
        if (yCoords_[currGrid.y()] - halfWidth < dieBox_.yMin()
            || yCoords_[currGrid.y()] + halfWidth > dieBox_.yMax()) {
          return false;
        }
      }
    }
  }

  return true;
}

void FlexGridGraph::traceBackPath(const FlexWavefrontGrid& currGrid,
                                  std::vector<FlexMazeIdx>& path,
                                  std::vector<FlexMazeIdx>& root,
                                  FlexMazeIdx& ccMazeIdx1,
                                  FlexMazeIdx& ccMazeIdx2,
                                  ExpandDir expandDir) const
{
  frDirEnum prevDir = frDirEnum::UNKNOWN, currDir = frDirEnum::UNKNOWN;
  int currX = currGrid.x(), currY = currGrid.y(), currZ = currGrid.z();
  // pop content in buffer
  auto backTraceBuffer = currGrid.getBackTraceBuffer();
  for (int i = 0; i < WAVEFRONTBUFFERSIZE; ++i) {
    // current grid is src
    if (isSrc(currX, currY, currZ, expandDir)) {
      break;
    }
    // get last direction
    currDir = getLastDir(backTraceBuffer);
    backTraceBuffer >>= DIRBITSIZE;
    if (currDir == frDirEnum::UNKNOWN) {
      std::cout << "Warning: unexpected direction in tracBackPath\n";
      break;
    }
    root.emplace_back(currX, currY, currZ);
    // push point to path
    if (currDir != prevDir) {
      path.emplace_back(currX, currY, currZ);
    }
    getPrevGrid(currX, currY, currZ, currDir);
    prevDir = currDir;
  }
  // trace back according to grid prev dir
  while (!isSrc(currX, currY, currZ, expandDir)) {
    // get last direction
    currDir = getPrevAstarNodeDir({currX, currY, currZ});
    root.emplace_back(currX, currY, currZ);
    if (currDir == frDirEnum::UNKNOWN) {
      std::cout << "Warning: unexpected direction in tracBackPath\n";
      break;
    }
    if (currDir != prevDir) {
      path.emplace_back(currX, currY, currZ);
    }
    getPrevGrid(currX, currY, currZ, currDir);
    prevDir = currDir;
  }
  // add final path to src, only add when path exists; no path exists (src =
  // dst)
  if (!path.empty()) {
    path.emplace_back(currX, currY, currZ);
  }
  for (auto& mi : path) {
    ccMazeIdx1.set(std::min(ccMazeIdx1.x(), mi.x()),
                   std::min(ccMazeIdx1.y(), mi.y()),
                   std::min(ccMazeIdx1.z(), mi.z()));
    ccMazeIdx2.set(std::max(ccMazeIdx2.x(), mi.x()),
                   std::max(ccMazeIdx2.y(), mi.y()),
                   std::max(ccMazeIdx2.z(), mi.z()));
  }
}

void FlexGridGraph::traceBackPath(const FlexMazeIdx& currGrid,
                                  std::vector<FlexMazeIdx>& path,
                                  std::vector<FlexMazeIdx>& root,
                                  FlexMazeIdx& ccMazeIdx1,
                                  FlexMazeIdx& ccMazeIdx2,
                                  ExpandDir expandDir) const
{
  frDirEnum prevDir = frDirEnum::UNKNOWN, currDir = frDirEnum::UNKNOWN;
  int currX = currGrid.x(), currY = currGrid.y(), currZ = currGrid.z();
  // trace back according to grid prev dir
  while (!isSrc(currX, currY, currZ, expandDir)) {
    // get last direction
    currDir = getPrevAstarNodeDir({currX, currY, currZ});
    root.emplace_back(currX, currY, currZ);
    if (currDir == frDirEnum::UNKNOWN) {
      std::cout << "Warning: unexpected direction in tracBackPath\n";
      break;
    }
    if (currDir != prevDir) {
      path.emplace_back(currX, currY, currZ);
    }
    getPrevGrid(currX, currY, currZ, currDir);
    prevDir = currDir;
  }
  // add final path to src, only add when path exists; no path exists (src =
  // dst)
  if (!path.empty()) {
    path.emplace_back(currX, currY, currZ);
  }
  for (auto& mi : path) {
    ccMazeIdx1.set(std::min(ccMazeIdx1.x(), mi.x()),
                   std::min(ccMazeIdx1.y(), mi.y()),
                   std::min(ccMazeIdx1.z(), mi.z()));
    ccMazeIdx2.set(std::max(ccMazeIdx2.x(), mi.x()),
                   std::max(ccMazeIdx2.y(), mi.y()),
                   std::max(ccMazeIdx2.z(), mi.z()));
  }
}

void FlexGridGraph::traceBackPath(std::vector<FlexMazeIdx>& connComps,
                                  std::vector<FlexMazeIdx>& path,
                                  FlexMazeIdx& ccMazeIdx1,
                                  FlexMazeIdx& ccMazeIdx2,
                                  ExpandDir expandDir)
{
  if (meetingPoint_->wavefrontDir == expandDir) {
    traceBackPath(meetingPoint_->wavefront,
                  path,
                  connComps,
                  ccMazeIdx1,
                  ccMazeIdx2,
                  meetingPoint_->wavefrontDir);
  } else {
    traceBackPath({meetingPoint_->wavefront.x(),
                   meetingPoint_->wavefront.y(),
                   meetingPoint_->wavefront.z()},
                  path,
                  connComps,
                  ccMazeIdx1,
                  ccMazeIdx2,
                  expandDir);
  }
}

bool FlexGridGraph::search(std::vector<FlexMazeIdx>& connComps,
                           drPin* nextPin,
                           std::vector<FlexMazeIdx>& path,
                           FlexMazeIdx& ccMazeIdx1,
                           FlexMazeIdx& ccMazeIdx2,
                           const Point& centerPtForward,
                           std::map<FlexMazeIdx, frBox3D*>& mazeIdx2TaperBox,
                           bool route_with_jumpers)
{
  if (debug_) {
    dump_file_.open("expansions.dump");
  }
  curr_id_ = 1;
  if (drWorker_->getDRIter() >= debugMazeIter) {
    std::cout << "INIT search: target pin " << nextPin->getName()
              << "\nsource points:\n";
    for (auto& idx : connComps) {
      std::cout << idx.x() << " " << idx.y() << " " << idx.z()
                << " coords: " << xCoords_[idx.x()] << " " << yCoords_[idx.y()]
                << "\n";
    }
  }
  // prep nextPinBox
  frMIdx xDim, yDim, zDim;
  getDim(xDim, yDim, zDim);
  FlexMazeIdx dstMazeForwardIdx1(xDim - 1, yDim - 1, zDim - 1);
  FlexMazeIdx dstMazeForwardIdx2(0, 0, 0);
  for (auto& ap : nextPin->getAccessPatterns()) {
    FlexMazeIdx mi = ap->getMazeIdx();
    dstMazeForwardIdx1.set(std::min(dstMazeForwardIdx1.x(), mi.x()),
                           std::min(dstMazeForwardIdx1.y(), mi.y()),
                           std::min(dstMazeForwardIdx1.z(), mi.z()));
    dstMazeForwardIdx2.set(std::max(dstMazeForwardIdx2.x(), mi.x()),
                           std::max(dstMazeForwardIdx2.y(), mi.y()),
                           std::max(dstMazeForwardIdx2.z(), mi.z()));
  }

  wavefrontForward_.cleanup();
  wavefrontBackward_.cleanup();
  // init wavefront for forward search
  Point currPt;
  for (auto& idx : connComps) {
    if (isDst(idx.x(), idx.y(), idx.z(), ExpandDir::FORWARD)) {
      path.emplace_back(idx.x(), idx.y(), idx.z());
      return true;
    }
    getPoint(currPt, idx.x(), idx.y());
    frCoord currDist = Point::manhattanDistance(currPt, centerPtForward);
    FlexWavefrontGrid currGrid(
        idx.x(),
        idx.y(),
        idx.z(),
        std::numeric_limits<frCoord>::max(),
        std::numeric_limits<frCoord>::max(),
        true,
        std::numeric_limits<frCoord>::max(),
        currDist,
        0,
        getEstCost(
            idx, dstMazeForwardIdx1, dstMazeForwardIdx2, frDirEnum::UNKNOWN));
    if (ndr_ && router_cfg_->AUTO_TAPER_NDR_NETS) {
      auto it = mazeIdx2TaperBox.find(idx);
      if (it != mazeIdx2TaperBox.end()) {
        currGrid.setSrcTaperBox(it->second);
      }
    }
    if (debug_) {
      currGrid.setId(curr_id_++);
      printExpansion(currGrid, "Pushing");
    }
    wavefrontForward_.push(currGrid);
  }
  FlexMazeIdx dstMazeBackwardIdx;
  dstMazeBackwardIdx = {wavefrontForward_.top().x(),
                        wavefrontForward_.top().y(),
                        wavefrontForward_.top().z()};
  Point centerPtBackward;
  getPoint(centerPtBackward, dstMazeBackwardIdx.x(), dstMazeBackwardIdx.y());
  // init wavefront for backward search
  for (auto& ap : nextPin->getAccessPatterns()) {
    auto idx = ap->getMazeIdx();
    getPoint(currPt, idx.x(), idx.y());
    frCoord currDist = Point::manhattanDistance(currPt, centerPtBackward);
    FlexWavefrontGrid currGrid(
        idx.x(),
        idx.y(),
        idx.z(),
        std::numeric_limits<frCoord>::max(),
        std::numeric_limits<frCoord>::max(),
        true,
        std::numeric_limits<frCoord>::max(),
        currDist,
        0,
        getEstCost(
            idx, dstMazeBackwardIdx, dstMazeBackwardIdx, frDirEnum::UNKNOWN));
    if (ndr_ && router_cfg_->AUTO_TAPER_NDR_NETS) {
      auto it = mazeIdx2TaperBox.find(idx);
      if (it != mazeIdx2TaperBox.end()) {
        currGrid.setSrcTaperBox(it->second);
      }
    }
    if (debug_) {
      currGrid.setId(curr_id_++);
      printExpansion(currGrid, "Pushing");
    }
    wavefrontBackward_.push(currGrid);
  }

  meetingPoint_ = {};
  ExpandDir expandDir = ExpandDir::BACKWARD;
  while (!wavefrontForward_.empty() || !wavefrontBackward_.empty()) {
    if (meetingPoint_) {
      frCost potentialCost = 0;
      if (!wavefrontForward_.empty()) {
        potentialCost += wavefrontForward_.top().getPathCost();
      }
      if (!wavefrontBackward_.empty()) {
        potentialCost += wavefrontBackward_.top().getPathCost();
      }
      if (meetingPoint_->wavefront.getPathCost() + meetingPoint_->mazeCost
          < potentialCost) {
        break;
      }
    }
    ExpandDir oppositeExpandDir = expandDir;
    FlexMazeIdx dstMazeIdx1, dstMazeIdx2;
    Point centerPt;
    FlexWavefront* wavefront;
    if (expandDir == ExpandDir::FORWARD) {
      expandDir = ExpandDir::BACKWARD;
      dstMazeIdx1 = dstMazeIdx2 = dstMazeBackwardIdx;
      centerPt = centerPtBackward;
      wavefront = &wavefrontBackward_;
    } else {
      expandDir = ExpandDir::FORWARD;
      dstMazeIdx1 = dstMazeForwardIdx1;
      dstMazeIdx2 = dstMazeForwardIdx2;
      centerPt = centerPtForward;
      wavefront = &wavefrontForward_;
    }
    if (wavefront->empty()) {
      continue;
    }
    auto currGrid = wavefront->top();
    if (debug_) {
      printExpansion(currGrid, "Popping");
    }
    wavefront->pop();
    if (isClosed(currGrid.x(), currGrid.y(), currGrid.z(), expandDir)) {
    } else if (isClosed(currGrid.x(),
                        currGrid.y(),
                        currGrid.z(),
                        oppositeExpandDir)) {
      frCost backwardCost
          = getPathCost(currGrid.x(), currGrid.y(), currGrid.z());
      if (!meetingPoint_
          || meetingPoint_->wavefront.getPathCost() + meetingPoint_->mazeCost
                 > currGrid.getPathCost() + backwardCost) {
        meetingPoint_ = MeetingPoint{currGrid, expandDir, backwardCost};
      }
    } else {
      if (graphics_) {
        graphics_->searchNode(this, currGrid);
      }
      if (drWorker_->getDRIter() >= debugMazeIter) {
        std::cout << "Expanding " << currGrid.x() << " " << currGrid.y() << " "
                  << currGrid.z() << " coords: " << xCoords_[currGrid.x()]
                  << " " << yCoords_[currGrid.y()] << " cost "
                  << currGrid.getCost() << " g " << currGrid.getPathCost()
                  << "\n";
      }
      if (isDst(currGrid.x(), currGrid.y(), currGrid.z(), expandDir)) {
        traceBackPath(
            currGrid, path, connComps, ccMazeIdx1, ccMazeIdx2, expandDir);
        if (expandDir == ExpandDir::BACKWARD) {
          std::reverse(path.begin(), path.end());
        }
        return true;
      }
      // expand and update wavefront
      expandWavefront(currGrid,
                      dstMazeIdx1,
                      dstMazeIdx2,
                      centerPt,
                      expandDir,
                      route_with_jumpers);
    }
  }

  if (!meetingPoint_) {
    return false;
  }

  // traceback forward and backward paths
  traceBackPath(connComps, path, ccMazeIdx1, ccMazeIdx2, ExpandDir::BACKWARD);
  if (!path.empty()) {
    std::reverse(path.begin(), path.end());
    path.pop_back();
  }
  traceBackPath(connComps, path, ccMazeIdx1, ccMazeIdx2, ExpandDir::FORWARD);
  return true;
}

}  // namespace drt
