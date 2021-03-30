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

#include "gr/FlexGRGridGraph.h"

#include <iostream>

#include "gr/FlexGR.h"

using namespace std;
using namespace fr;

void FlexGRGridGraph::init()
{
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  xgp_ = &(gCellPatterns.at(0));
  ygp_ = &(gCellPatterns.at(1));

  initCoords();
  initGrids();
  initEdges();
}

void FlexGRGridGraph::initCoords()
{
  map<frLayerNum, frPrefRoutingDirEnum> zMap;
  frPoint gcellIdxLL = getGRWorker()->getRouteGCellIdxLL();
  frPoint gcellIdxUR = getGRWorker()->getRouteGCellIdxUR();
  // xCoords
  for (int xIdx = gcellIdxLL.x(); xIdx <= gcellIdxUR.x(); xIdx++) {
    frBox gcellBox;
    getDesign()->getTopBlock()->getGCellBox(frPoint(xIdx, 0), gcellBox);
    xCoords_.push_back((gcellBox.left() + gcellBox.right()) / 2);
  }
  // yCoords
  for (int yIdx = gcellIdxLL.y(); yIdx <= gcellIdxUR.y(); yIdx++) {
    frBox gcellBox;
    getDesign()->getTopBlock()->getGCellBox(frPoint(0, yIdx), gcellBox);
    yCoords_.push_back((gcellBox.bottom() + gcellBox.top()) / 2);
  }
  // z
  if (!is2DRouting_) {
    for (auto& layer : getTech()->getLayers()) {
      if (layer->getType() != frLayerTypeEnum::ROUTING) {
        continue;
      }
      frLayerNum lNum = layer->getLayerNum();
      frPrefRoutingDirEnum prefRouteDir = layer->getDir();
      zMap[lNum] = prefRouteDir;
    }
  } else {
    // 2D routing only has one layer on layerNum == 2
    zMap[2] = frcNonePrefRoutingDir;
  }

  frCoord zHeight = 0;
  for (auto& [k, v] : zMap) {
    zCoords_.push_back(k);
    zHeight += getTech()->getLayer(k)->getPitch() * VIACOST;
    zHeights_.push_back(zHeight);
    zDirs_.push_back((v == frcHorzPrefRoutingDir));
  }
}

void FlexGRGridGraph::initGrids()
{
  frMIdx xDim, yDim, zDim;
  getDim(xDim, yDim, zDim);
  bits_.clear();
  bits_.resize(xDim * yDim * zDim, 0);
  prevDirs_.clear();
  srcs_.clear();
  dsts_.clear();
  prevDirs_.resize(xDim * yDim * zDim * 3, 0);
  srcs_.resize(xDim * yDim * zDim, 0);
  dsts_.resize(xDim * yDim * zDim, 0);
}

void FlexGRGridGraph::initEdges()
{

  for (frMIdx zIdx = 0; zIdx < (int) zCoords_.size(); zIdx++) {
    auto dir = is2DRouting_ ? frcNonePrefRoutingDir
                            : (zDirs_[zIdx] ? frcHorzPrefRoutingDir
                                            : frcVertPrefRoutingDir);
    for (frMIdx xIdx = 0; xIdx < (int) xCoords_.size(); xIdx++) {
      for (frMIdx yIdx = 0; yIdx < (int) yCoords_.size(); yIdx++) {
        // horz
        if ((dir == frcNonePrefRoutingDir || dir == frcHorzPrefRoutingDir)
            && ((xIdx + 1) != (int) xCoords_.size())) {
          bool flag = addEdge(xIdx, yIdx, zIdx, frDirEnum::E);
        }
        // vert
        if ((dir == frcNonePrefRoutingDir || dir == frcVertPrefRoutingDir)
            && ((yIdx + 1) != (int) yCoords_.size())) {
          bool flag = addEdge(xIdx, yIdx, zIdx, frDirEnum::N);
        }
        // via
        if ((zIdx + 1) != (int) zCoords_.size()) {
          bool flag = addEdge(xIdx, yIdx, zIdx, frDirEnum::U);
        }
      }
    }
  }
}

void FlexGRGridGraph::resetStatus()
{
  resetSrc();
  resetDst();
  resetPrevNodeDir();
}

void FlexGRGridGraph::resetSrc()
{
  srcs_.assign(srcs_.size(), 0);
}

void FlexGRGridGraph::resetDst()
{
  dsts_.assign(dsts_.size(), 0);
}

void FlexGRGridGraph::resetPrevNodeDir()
{
  prevDirs_.assign(prevDirs_.size(), 0);
}
