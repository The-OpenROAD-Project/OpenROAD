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

#include <cmath>

#include "gr/FlexGR.h"
#include "gr/FlexGRGridGraph.h"

using namespace std;
using namespace fr;

bool FlexGRGridGraph::search(vector<FlexMazeIdx>& connComps,
                             grNode* nextPinNode,
                             vector<FlexMazeIdx>& path,
                             FlexMazeIdx& ccMazeIdx1,
                             FlexMazeIdx& ccMazeIdx2,
                             const frPoint& centerPt)
{
  int stepCnt = 0;

  // prep nextPinBox
  frMIdx xDim, yDim, zDim;
  getDim(xDim, yDim, zDim);
  FlexMazeIdx dstMazeIdx1(xDim - 1, yDim - 1, zDim - 1);
  FlexMazeIdx dstMazeIdx2(0, 0, 0);
  FlexMazeIdx mi;

  auto loc = nextPinNode->getLoc();
  auto lNum = nextPinNode->getLayerNum();
  getMazeIdx(loc, lNum, mi);
  // update dstMazeIdx1, dstMazeIdx2
  dstMazeIdx1.set(min(dstMazeIdx1.x(), mi.x()),
                  min(dstMazeIdx1.y(), mi.y()),
                  min(dstMazeIdx1.z(), mi.z()));
  dstMazeIdx2.set(max(dstMazeIdx2.x(), mi.x()),
                  max(dstMazeIdx2.y(), mi.y()),
                  max(dstMazeIdx2.z(), mi.z()));

  wavefront_ = FlexGRWavefront();

  frPoint currPt;
  // push connected components to wavefront
  for (auto& idx : connComps) {
    if (isDst(idx.x(), idx.y(), idx.z())) {
      path.push_back(FlexMazeIdx(idx.x(), idx.y(), idx.z()));
      return true;
    }
    getPoint(idx.x(), idx.y(), currPt);
    frCoord currDist
        = abs(currPt.x() - centerPt.x()) + abs(currPt.y() - centerPt.y());
    FlexGRWavefrontGrid currGrid(
        idx.x(),
        idx.y(),
        idx.z(),
        currDist,
        0,
        getEstCost(idx, dstMazeIdx1, dstMazeIdx2, frDirEnum::UNKNOWN));
    wavefront_.push(currGrid);
  }

  while (!wavefront_.empty()) {
    auto currGrid = wavefront_.top();
    wavefront_.pop();
    if (getPrevAstarNodeDir(currGrid.x(), currGrid.y(), currGrid.z())
        != frDirEnum::UNKNOWN) {
      continue;
    }
    // test
    if (isDst(currGrid.x(), currGrid.y(), currGrid.z())) {
      traceBackPath(currGrid, path, connComps, ccMazeIdx1, ccMazeIdx2);
      return true;
    } else {
      // expand and update wavefront
      expandWavefront(currGrid, dstMazeIdx1, dstMazeIdx2, centerPt);
    }
  }
  return false;
}

frCost FlexGRGridGraph::getEstCost(const FlexMazeIdx& src,
                                   const FlexMazeIdx& dstMazeIdx1,
                                   const FlexMazeIdx& dstMazeIdx2,
                                   const frDirEnum& dir)
{
  // bend cost
  int bendCnt = 0;
  frPoint srcPoint, dstPoint1, dstPoint2;
  getPoint(src.x(), src.y(), srcPoint);
  getPoint(dstMazeIdx1.x(), dstMazeIdx1.y(), dstPoint1);
  getPoint(dstMazeIdx2.x(), dstMazeIdx2.y(), dstPoint2);
  frCoord minCostX
      = max(max(dstPoint1.x() - srcPoint.x(), srcPoint.x() - dstPoint2.x()), 0);
  frCoord minCostY
      = max(max(dstPoint1.y() - srcPoint.y(), srcPoint.y() - dstPoint2.y()), 0);
  frCoord minCostZ = max(max(getZHeight(dstMazeIdx1.z()) - getZHeight(src.z()),
                             getZHeight(src.z()) - getZHeight(dstMazeIdx2.z())),
                         0);

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
  return (minCostX + minCostY + minCostZ + bendCnt);
}

void FlexGRGridGraph::traceBackPath(const FlexGRWavefrontGrid& currGrid,
                                    vector<FlexMazeIdx>& path,
                                    vector<FlexMazeIdx>& root,
                                    FlexMazeIdx& ccMazeIdx1,
                                    FlexMazeIdx& ccMazeIdx2)
{
  frDirEnum prevDir = frDirEnum::UNKNOWN, currDir = frDirEnum::UNKNOWN;
  int currX = currGrid.x(), currY = currGrid.y(), currZ = currGrid.z();
  // pop content in buffer
  auto backTraceBuffer = currGrid.getBackTraceBuffer();
  for (int i = 0; i < GRWAVEFRONTBUFFERSIZE; ++i) {
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
    }
    getPrevGrid(currX, currY, currZ, currDir);
    prevDir = currDir;
  }
  // add final path to src, only add when path exists; no path exists (src =
  // dst)
  if (!path.empty()) {
    path.push_back(FlexMazeIdx(currX, currY, currZ));
  }
  for (auto& mi : path) {
    ccMazeIdx1.set(min(ccMazeIdx1.x(), mi.x()),
                   min(ccMazeIdx1.y(), mi.y()),
                   min(ccMazeIdx1.z(), mi.z()));
    ccMazeIdx2.set(max(ccMazeIdx2.x(), mi.x()),
                   max(ccMazeIdx2.y(), mi.y()),
                   max(ccMazeIdx2.z(), mi.z()));
  }
}

frDirEnum FlexGRGridGraph::getLastDir(
    const std::bitset<GRWAVEFRONTBITSIZE>& buffer)
{
  auto currDirVal = buffer.to_ulong() & 0b111u;
  return static_cast<frDirEnum>(currDirVal);
}

void FlexGRGridGraph::getPrevGrid(frMIdx& gridX,
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

void FlexGRGridGraph::expandWavefront(FlexGRWavefrontGrid& currGrid,
                                      const FlexMazeIdx& dstMazeIdx1,
                                      const FlexMazeIdx& dstMazeIdx2,
                                      const frPoint& centerPt)
{
  // N
  if (isExpandable(currGrid, frDirEnum::N)) {
    expand(currGrid, frDirEnum::N, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
  // E
  if (isExpandable(currGrid, frDirEnum::E)) {
    expand(currGrid, frDirEnum::E, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
  // S
  if (isExpandable(currGrid, frDirEnum::S)) {
    expand(currGrid, frDirEnum::S, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
  // W
  if (isExpandable(currGrid, frDirEnum::W)) {
    expand(currGrid, frDirEnum::W, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
  // U
  if (isExpandable(currGrid, frDirEnum::U)) {
    expand(currGrid, frDirEnum::U, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
  // D
  if (isExpandable(currGrid, frDirEnum::D)) {
    expand(currGrid, frDirEnum::D, dstMazeIdx1, dstMazeIdx2, centerPt);
  }
}

bool FlexGRGridGraph::isExpandable(const FlexGRWavefrontGrid& currGrid,
                                   frDirEnum dir)
{
  frMIdx gridX = currGrid.x();
  frMIdx gridY = currGrid.y();
  frMIdx gridZ = currGrid.z();
  bool hg = hasEdge(gridX, gridY, gridZ, dir);
  reverse(gridX, gridY, gridZ, dir);
  if (!hg || isSrc(gridX, gridY, gridZ)
      || getPrevAstarNodeDir(gridX, gridY, gridZ) != frDirEnum::UNKNOWN
      || currGrid.getLastDir() == dir) {
    return false;
  } else {
    return true;
  }
}

void FlexGRGridGraph::expand(FlexGRWavefrontGrid& currGrid,
                             const frDirEnum& dir,
                             const FlexMazeIdx& dstMazeIdx1,
                             const FlexMazeIdx& dstMazeIdx2,
                             const frPoint& centerPt)
{
  frCost nextEstCost, nextPathCost;
  int gridX = currGrid.x();
  int gridY = currGrid.y();
  int gridZ = currGrid.z();

  getNextGrid(gridX, gridY, gridZ, dir);

  FlexMazeIdx nextIdx(gridX, gridY, gridZ);
  // get cost
  nextEstCost = getEstCost(nextIdx, dstMazeIdx1, dstMazeIdx2, dir);
  nextPathCost = getNextPathCost(currGrid, dir);

  frPoint currPt;
  getPoint(gridX, gridY, currPt);
  frCoord currDist
      = abs(currPt.x() - centerPt.x()) + abs(currPt.y() - centerPt.y());

  FlexGRWavefrontGrid nextWavefrontGrid(gridX,
                                        gridY,
                                        gridZ,
                                        currDist,
                                        nextPathCost,
                                        nextPathCost + nextEstCost,
                                        currGrid.getBackTraceBuffer());
  // update wavefront buffer
  auto tailDir = nextWavefrontGrid.shiftAddBuffer(dir);
  // commit grid prev direction if needed
  auto tailIdx = getTailIdx(nextIdx, nextWavefrontGrid);
  if (tailDir != frDirEnum::UNKNOWN) {
    if (getPrevAstarNodeDir(tailIdx.x(), tailIdx.y(), tailIdx.z())
            == frDirEnum::UNKNOWN
        || getPrevAstarNodeDir(tailIdx.x(), tailIdx.y(), tailIdx.z())
               == tailDir) {
      setPrevAstarNodeDir(tailIdx.x(), tailIdx.y(), tailIdx.z(), tailDir);
      wavefront_.push(nextWavefrontGrid);
    }
  } else {
    // add to wavefront
    wavefront_.push(nextWavefrontGrid);
  }
  return;
}

void FlexGRGridGraph::getNextGrid(frMIdx& gridX,
                                  frMIdx& gridY,
                                  frMIdx& gridZ,
                                  const frDirEnum dir)
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

frCost FlexGRGridGraph::getNextPathCost(const FlexGRWavefrontGrid& currGrid,
                                        const frDirEnum& dir)
{
  frMIdx gridX = currGrid.x();
  frMIdx gridY = currGrid.y();
  frMIdx gridZ = currGrid.z();
  frCost nextPathCost = currGrid.getPathCost();
  // bending cost
  auto currDir = currGrid.getLastDir();
  if (currDir != dir && currDir != frDirEnum::UNKNOWN) {
    // original
    ++nextPathCost;
  }

  // currently no congeston on via direction
  bool congCost = (dir == frDirEnum::U || dir == frDirEnum::D) ? false : true;

  bool blockCost = hasBlock(gridX, gridY, gridZ, dir);
  bool overflowCost = false;

  frMIdx tmpX = gridX;
  frMIdx tmpY = gridY;
  frMIdx tmpZ = gridZ;
  frDirEnum tmpDir = dir;
  correct(tmpX, tmpY, tmpZ, tmpDir);
  unsigned rawDemand = 0;
  unsigned rawSupply = 0;

  if (tmpDir != frDirEnum::U && tmpDir != frDirEnum::D) {
    rawDemand = getRawDemand(tmpX, tmpY, tmpZ, tmpDir);
    rawSupply = getRawSupply(tmpX, tmpY, tmpZ, tmpDir);
  }

  bool histCost = hasHistoryCost(tmpX, tmpY, tmpZ);

  overflowCost = (rawDemand >= rawSupply * grWorker_->getCongThresh());

  nextPathCost
      += getEdgeLength(gridX, gridY, gridZ, dir)
         + (congCost
                ? getCongCost(rawDemand, rawSupply * grWorker_->getCongThresh())
                      * getEdgeLength(gridX, gridY, gridZ, dir)
                : 0)
         + (histCost ? 4
                           * getCongCost(rawDemand,
                                         rawSupply * grWorker_->getCongThresh())
                           * getHistoryCost(gridX, gridY, gridZ)
                           * getEdgeLength(gridX, gridY, gridZ, dir)
                     : 0)
         + (blockCost
                ? BLOCKCOST * getEdgeLength(gridX, gridY, gridZ, dir) * 100
                : 0)
         + (overflowCost ? 128 * getEdgeLength(gridX, gridY, gridZ, dir) : 0);
  return nextPathCost;
}

double FlexGRGridGraph::getCongCost(int demand, int supply)
{
  return (demand * (4 / (1.0 + exp(supply - demand))) / (supply + 1));
}

FlexMazeIdx FlexGRGridGraph::getTailIdx(const FlexMazeIdx& currIdx,
                                        const FlexGRWavefrontGrid& currGrid)
{
  int gridX = currIdx.x();
  int gridY = currIdx.y();
  int gridZ = currIdx.z();
  auto backTraceBuffer = currGrid.getBackTraceBuffer();
  for (int i = 0; i < GRWAVEFRONTBUFFERSIZE; ++i) {
    int currDirVal
        = backTraceBuffer.to_ulong()
          - ((backTraceBuffer.to_ulong() >> DIRBITSIZE) << DIRBITSIZE);
    frDirEnum currDir = static_cast<frDirEnum>(currDirVal);
    backTraceBuffer >>= DIRBITSIZE;
    getPrevGrid(gridX, gridY, gridZ, currDir);
  }
  return FlexMazeIdx(gridX, gridY, gridZ);
}
