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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "gr/FlexGRGridGraph.h"
#include "gr/FlexGR.h"
#include <iostream>

using namespace std;
using namespace fr;

void FlexGRGridGraph::init() {
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  xgp = &(gCellPatterns.at(0));
  ygp = &(gCellPatterns.at(1));

  initCoords();
  initGrids();
  initEdges();
}

void FlexGRGridGraph::initCoords() {
  map<frLayerNum, frPrefRoutingDirEnum> zMap;
  frPoint gcellIdxLL = getGRWorker()->getRouteGCellIdxLL();
  frPoint gcellIdxUR = getGRWorker()->getRouteGCellIdxUR();
  // xCoords
  for (int xIdx = gcellIdxLL.x(); xIdx <= gcellIdxUR.x(); xIdx++) {
    frBox gcellBox;
    getDesign()->getTopBlock()->getGCellBox(frPoint(xIdx, 0), gcellBox);
    xCoords.push_back((gcellBox.left() + gcellBox.right()) / 2);
  }
  // yCoords
  for (int yIdx = gcellIdxLL.y(); yIdx <= gcellIdxUR.y(); yIdx++) {
    frBox gcellBox;
    getDesign()->getTopBlock()->getGCellBox(frPoint(0, yIdx), gcellBox);
    yCoords.push_back((gcellBox.bottom() + gcellBox.top()) / 2);
  }
  // z
  if (!is2DRouting) {
    for (auto &layer: getTech()->getLayers()) {
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
  for (auto &[k, v]: zMap) {
    zCoords.push_back(k);
    zHeight += getTech()->getLayer(k)->getPitch() * VIACOST;
    zHeights.push_back(zHeight);
    zDirs.push_back((v == frcHorzPrefRoutingDir));
  }
}

void FlexGRGridGraph::initGrids() {
  frMIdx xDim, yDim, zDim;
  getDim(xDim, yDim, zDim);
  bits.clear();
  bits.resize(xDim*yDim*zDim, 0);
  prevDirs.clear();
  srcs.clear();
  dsts.clear();
  prevDirs.resize(xDim*yDim*zDim*3, 0);
  srcs.resize(xDim*yDim*zDim, 0);
  dsts.resize(xDim*yDim*zDim, 0);
}

void FlexGRGridGraph::initEdges() {
  bool enableOutput = false;

  for (frMIdx zIdx = 0; zIdx < (int)zCoords.size(); zIdx++) {
    auto dir = is2DRouting ? frcNonePrefRoutingDir : (zDirs[zIdx] ? frcHorzPrefRoutingDir : frcVertPrefRoutingDir);
    for (frMIdx xIdx = 0; xIdx < (int)xCoords.size(); xIdx++) {
      for (frMIdx yIdx = 0; yIdx < (int)yCoords.size(); yIdx++) {
        // horz
        if (enableOutput) {
          if (hasEdge(xIdx, yIdx, zIdx, frDirEnum::E)) {
            cout <<"Error: (" <<xIdx <<", " <<yIdx <<", " <<zIdx <<", E) already set" <<endl;
          }
        }
        if ((dir == frcNonePrefRoutingDir || dir == frcHorzPrefRoutingDir) &&
            ((xIdx + 1) != (int)xCoords.size())) {
          bool flag = addEdge(xIdx, yIdx, zIdx, frDirEnum::E);
          if (enableOutput) {
            if (!flag) {
              cout << "Error: (" <<xIdx <<", " <<yIdx <<", " <<zIdx <<", E) addEdge failed" <<endl;
            }
          }
        }
        // vert
        if (enableOutput) {
          if (hasEdge(xIdx, yIdx, zIdx, frDirEnum::N)) {
            cout <<"Error: (" <<xIdx <<", " <<yIdx <<", " <<zIdx <<", N) already set" <<endl;
          }
        }
        if ((dir == frcNonePrefRoutingDir || dir == frcVertPrefRoutingDir) &&
            ((yIdx + 1) != (int)yCoords.size())) {
          bool flag = addEdge(xIdx, yIdx, zIdx, frDirEnum::N);
          if (enableOutput) {
            if (!flag) {
              cout << "Error: (" <<xIdx <<", " <<yIdx <<", " <<zIdx <<", N) addEdge failed" <<endl;
            }
          }
        }
        // via
        if (enableOutput) {
          if (hasEdge(xIdx, yIdx, zIdx, frDirEnum::U)) {
            cout <<"Error: (" <<xIdx <<", " <<yIdx <<", " <<zIdx <<", U) already set" <<endl;
          }
        }
        if ((zIdx + 1) != (int)zCoords.size()) {
          bool flag = addEdge(xIdx, yIdx, zIdx, frDirEnum::U);
          if (enableOutput) {
            if (!flag) {
              cout << "Error: (" <<xIdx <<", " <<yIdx <<", " <<zIdx <<", U) addEdge failed" <<endl;
            }
          }
        }
      }
    }
  }
}

void FlexGRGridGraph::resetStatus() {
  resetSrc();
  resetDst();
  resetPrevNodeDir();
}

void FlexGRGridGraph::resetSrc() {
  srcs.assign(srcs.size(), 0);
}

void FlexGRGridGraph::resetDst() {
  dsts.assign(dsts.size(), 0);
}

void FlexGRGridGraph::resetPrevNodeDir() {
  prevDirs.assign(prevDirs.size(), 0);
}
