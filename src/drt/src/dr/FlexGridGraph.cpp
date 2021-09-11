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

#include "dr/FlexGridGraph.h"

#include <fstream>
#include <iostream>
#include <map>

#include "dr/FlexDR.h"

using namespace std;
using namespace fr;

void FlexGridGraph::initGrids(
    const map<frCoord, map<frLayerNum, frTrackPattern*>>& xMap,
    const map<frCoord, map<frLayerNum, frTrackPattern*>>& yMap,
    const map<frLayerNum, dbTechLayerDir>& zMap,
    bool followGuide)
{
  // initialize coord vectors
  xCoords_.clear();
  yCoords_.clear();
  zCoords_.clear();
  zHeights_.clear();
  layerRouteDirections_.clear();
  for (auto& [k, v] : xMap) {
    xCoords_.push_back(k);
  }
  for (auto& [k, v] : yMap) {
    yCoords_.push_back(k);
  }
  frCoord zHeight = 0;
  // vector<frCoord> via2viaMinLenTmp(4, 0);
  for (auto& [k, v] : zMap) {
    zCoords_.push_back(k);
    zHeight += getTech()->getLayer(k)->getPitch() * VIACOST;
    zHeights_.push_back(zHeight);
    layerRouteDirections_.push_back(v);
  }
  // initialize all grids
  frMIdx xDim, yDim, zDim;
  getDim(xDim, yDim, zDim);
  nodes_.clear();
  nodes_.resize(xDim * yDim * zDim, Node());
  // new
  prevDirs_.clear();
  srcs_.clear();
  dsts_.clear();
  prevDirs_.resize(xDim * yDim * zDim * 3, 0);
  srcs_.resize(xDim * yDim * zDim, 0);
  dsts_.resize(xDim * yDim * zDim, 0);
  guides_.clear();
  if (followGuide) {
    guides_.resize(xDim * yDim * zDim, 0);
  } else {
    guides_.resize(xDim * yDim * zDim, 1);
  }
}

bool FlexGridGraph::outOfDieVia(frMIdx x,
                                frMIdx y,
                                frMIdx z,
                                const frBox& dieBox)
{
  frViaDef* via = getTech()->getLayer(getLayerNum(z) + 1)->getDefaultViaDef();
  frBox viaBox(via->getLayer1ShapeBox());
  viaBox.merge(via->getLayer2ShapeBox());
  viaBox.shift(xCoords_[x], yCoords_[y]);
  return !dieBox.contains(viaBox);
}

bool FlexGridGraph::isWorkerBorder(frMIdx v, bool isVert)
{
  if (isVert)
    return xCoords_[v] == drWorker_->getRouteBox().left()
           || xCoords_[v] == drWorker_->getRouteBox().right();
  return yCoords_[v] == drWorker_->getRouteBox().bottom()
         || yCoords_[v] == drWorker_->getRouteBox().top();
}
bool FlexGridGraph::hasAlignedUpDefTrack(
    frLayerNum layerNum,
    const map<frLayerNum, frTrackPattern*>& xSubMap,
    const map<frLayerNum, frTrackPattern*>& ySubMap) const
{
  for (frLayerNum lNum = layerNum + 2;
       lNum < (int) getTech()->getLayers().size();
       lNum += 2) {
    auto it = xSubMap.find(lNum);
    if (it != xSubMap.end()) {  // has x track in lNum
      if (it->second)           // has track pattern, i.e., the track is default
        return true;
    }
    it = ySubMap.find(lNum);
    if (it != ySubMap.end()) {  // has y track in lNum
      if (it->second)           // has track pattern, i.e., the track is default
        return true;
    }
  }
  return false;
}

void FlexGridGraph::initEdges(
    const frDesign* design,
    map<frCoord, map<frLayerNum, frTrackPattern*>>& xMap,
    map<frCoord, map<frLayerNum, frTrackPattern*>>& yMap,
    const map<frLayerNum, dbTechLayerDir>& zMap,
    const frBox& bbox,
    bool initDR)
{
  frMIdx xDim, yDim, zDim;
  getDim(xDim, yDim, zDim);
  // initialize grid graph
  frMIdx xIdx = 0, yIdx = 0, zIdx = 0;
  design->getTopBlock()->getDieBox(dieBox_);
  for (const auto& [layerNum, dir] : zMap) {
    frLayerNum nonPrefLayerNum;
    const auto layer = getTech()->getLayer(layerNum);
    if (layerNum + 2 <= getTech()->getTopLayerNum()) {
      nonPrefLayerNum = layerNum + 2;
    } else if (layerNum - 2 >= getTech()->getBottomLayerNum()) {
      nonPrefLayerNum = layerNum - 2;
    } else {
      nonPrefLayerNum = layerNum;
    }
    yIdx = 0;
    for (auto& [yCoord, ySubMap] : yMap) {
      auto yIt = ySubMap.find(layerNum);
      auto yIt2 = ySubMap.find(layerNum + 2);
      auto yIt3 = ySubMap.find(nonPrefLayerNum);
      bool yFound = (yIt != ySubMap.end());
      bool yFound2 = (yIt2 != ySubMap.end());
      bool yFound3 = (yIt3 != ySubMap.end());
      xIdx = 0;
      for (auto& [xCoord, xSubMap] : xMap) {
        auto xIt = xSubMap.find(layerNum);
        auto xIt2 = xSubMap.find(layerNum + 2);
        auto xIt3 = xSubMap.find(nonPrefLayerNum);
        bool xFound = (xIt != xSubMap.end());
        bool xFound2 = (xIt2 != xSubMap.end());
        bool xFound3 = (xIt3 != xSubMap.end());
        // add cost to out-of-die edge
        bool outOfDiePlanar = false;
        // add edge for preferred direction
        if (dir == dbTechLayerDir::HORIZONTAL && yFound) {
          if (layerNum >= BOTTOM_ROUTING_LAYER
              && layerNum <= TOP_ROUTING_LAYER) {
            if (layer->getLef58RightWayOnGridOnlyConstraint() == nullptr
                || yIt->second != nullptr) {
              addEdge(xIdx, yIdx, zIdx, frDirEnum::E, bbox, initDR);
              if (yIt->second == nullptr || outOfDiePlanar
                  || isWorkerBorder(yIdx, false)) {
                setGridCostE(xIdx, yIdx, zIdx);
              }
            }
          }
          // via to upper layer
          if (xFound2) {
            if (!outOfDieVia(xIdx, yIdx, zIdx, dieBox_)) {
              addEdge(xIdx, yIdx, zIdx, frDirEnum::U, bbox, initDR);
              bool condition
                  = (yIt->second == nullptr || xIt2->second == nullptr);
              if (condition) {
                setGridCostU(xIdx, yIdx, zIdx);
              }
            }
          }
        } else if (dir == dbTechLayerDir::VERTICAL && xFound) {
          if (layerNum >= BOTTOM_ROUTING_LAYER
              && layerNum <= TOP_ROUTING_LAYER) {
            if (layer->getLef58RightWayOnGridOnlyConstraint() == nullptr
                || xIt->second != nullptr) {
              addEdge(xIdx, yIdx, zIdx, frDirEnum::N, bbox, initDR);
              if (xIt->second == nullptr || outOfDiePlanar
                  || isWorkerBorder(xIdx, true)) {
                setGridCostN(xIdx, yIdx, zIdx);
              }
            }
          }
          // via to upper layer
          if (yFound2) {
            if (!outOfDieVia(xIdx, yIdx, zIdx, dieBox_)) {
              addEdge(xIdx, yIdx, zIdx, frDirEnum::U, bbox, initDR);
              bool condition
                  = (yIt2->second == nullptr || xIt->second == nullptr);
              if (condition) {
                setGridCostU(xIdx, yIdx, zIdx);
              }
            }
          }
        }
        // get non pref track layer --> use upper layer pref dir track if
        // possible
        if (USENONPREFTRACKS && !layer->isUnidirectional()) {
          // add edge for non-preferred direction
          // vertical non-pref track
          if (dir == dbTechLayerDir::HORIZONTAL && xFound3) {
            if (layerNum >= BOTTOM_ROUTING_LAYER
                && layerNum <= TOP_ROUTING_LAYER) {
              addEdge(xIdx, yIdx, zIdx, frDirEnum::N, bbox, initDR);
              setGridCostN(xIdx, yIdx, zIdx);
            }
            // horizontal non-pref track
          } else if (dir == dbTechLayerDir::VERTICAL && yFound3) {
            if (layerNum >= BOTTOM_ROUTING_LAYER
                && layerNum <= TOP_ROUTING_LAYER) {
              addEdge(xIdx, yIdx, zIdx, frDirEnum::E, bbox, initDR);
              setGridCostE(xIdx, yIdx, zIdx);
            }
          }
        }
        ++xIdx;
      }
      ++yIdx;
    }
    ++zIdx;
  }
  // this creates via edges over each ap until reaching a default track or a
  // layer with normal routing; in this case it creates jogs connections to the
  // neighboring tracks
  for (const Point3D& apPt : drWorker_->getSpecialAccessAPs()) {
    for (int i = 0; i < 2; i++) {  // down and up
      bool up = (bool) i;
      int inc = up ? 1 : -1;
      frMIdx startZ = getMazeZIdx(apPt.z());
      frLayerNum nextLNum = getLayerNum(startZ) + 2 * inc;
      if (!up)
        startZ--;
      frMIdx xIdx = getMazeXIdx(apPt.x());
      frMIdx yIdx = getMazeYIdx(apPt.y());
      // create the edges
      for (int zIdx = startZ; zIdx >= 0 && zIdx < (int) zCoords_.size() - 1;
           zIdx += inc, nextLNum += inc * 2) {
        addEdge(xIdx, yIdx, zIdx, frDirEnum::U, bbox, initDR);
        auto& xSubMap = xMap[apPt.x()];
        auto xTrack = xSubMap.find(nextLNum);
        if (xTrack != xSubMap.end() && xTrack->second != nullptr)
          break;
        auto& ySubMap = yMap[apPt.y()];
        auto yTrack = ySubMap.find(nextLNum);
        if (yTrack != ySubMap.end() && yTrack->second != nullptr)
          break;
        // didnt find default track, then create tracks if possible
        bool restrictedRouting
            = getTech()->getLayer(nextLNum)->isUnidirectional()
              || nextLNum < BOTTOM_ROUTING_LAYER
              || nextLNum > TOP_ROUTING_LAYER;
        if (!restrictedRouting && nextLNum >= VIA_ACCESS_LAYERNUM) {
          dbTechLayerDir prefDir
              = design->getTech()->getLayer(nextLNum)->getDir();
          xMap[apPt.x()][nextLNum] = nullptr;  // to keep coherence
          yMap[apPt.y()][nextLNum] = nullptr;
          frMIdx nextZ = up ? zIdx + 1 : zIdx;
          if (prefDir == dbTechLayerDir::HORIZONTAL) {
            addEdge(xIdx, yIdx, nextZ, frDirEnum::N, bbox, initDR);
            if (yIdx - 1 >= 0)
              addEdge(xIdx, yIdx - 1, nextZ, frDirEnum::N, bbox, initDR);
          } else {
            addEdge(xIdx, yIdx, nextZ, frDirEnum::E, bbox, initDR);
            if (xIdx - 1 >= 0)
              addEdge(xIdx - 1, yIdx, nextZ, frDirEnum::E, bbox, initDR);
          }
          break;
        }
      }
    }
  }
}

// initialization: update grid graph topology, does not assign edge cost
void FlexGridGraph::init(const frDesign* design,
                         const frBox& routeBBox,
                         const frBox& extBBox,
                         map<frCoord, map<frLayerNum, frTrackPattern*>>& xMap,
                         map<frCoord, map<frLayerNum, frTrackPattern*>>& yMap,
                         bool initDR,
                         bool followGuide)
{
  auto* via_data = getDRWorker()->getViaData();
  halfViaEncArea_ = &via_data->halfViaEncArea;
  via2viaMinLen_ = &via_data->via2viaMinLen;
  via2turnMinLen_ = &via_data->via2turnMinLen;
  via2viaMinLenNew_ = &via_data->via2viaMinLenNew;

  // get tracks intersecting with the Maze bbox
  map<frLayerNum, dbTechLayerDir> zMap;
  initTracks(design, xMap, yMap, zMap, extBBox);
  initGrids(xMap, yMap, zMap, followGuide);  // buildGridGraph
  initEdges(
      design, xMap, yMap, zMap, routeBBox, initDR);  // add edges and edgeCost
}

// initialization helpers
// get all tracks intersecting with the Maze bbox, left/bottom are inclusive
void FlexGridGraph::initTracks(
    const frDesign* design,
    map<frCoord, map<frLayerNum, frTrackPattern*>>& xMap,
    map<frCoord, map<frLayerNum, frTrackPattern*>>& yMap,
    map<frLayerNum, dbTechLayerDir>& zMap,
    const frBox& bbox)
{
  for (auto& layer : getTech()->getLayers()) {
    if (layer->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frLayerNum currLayerNum = layer->getLayerNum();
    dbTechLayerDir currPrefRouteDir = layer->getDir();
    for (auto& tp : design->getTopBlock()->getTrackPatterns(currLayerNum)) {
      // allow wrongway if global varialble and design rule allow
      bool flag = (USENONPREFTRACKS && !layer->isUnidirectional())
                      ? (tp->isHorizontal()
                         && currPrefRouteDir == dbTechLayerDir::VERTICAL)
                            || (!tp->isHorizontal()
                                && currPrefRouteDir == dbTechLayerDir::HORIZONTAL)
                      : true;
      if (flag) {
        int trackNum = ((tp->isHorizontal() ? bbox.left() : bbox.bottom())
                        - tp->getStartCoord())
                       / (int) tp->getTrackSpacing();
        if (trackNum < 0) {
          trackNum = 0;
        }
        if (trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
            < (tp->isHorizontal() ? bbox.left() : bbox.bottom())) {
          ++trackNum;
        }
        for (; trackNum < (int) tp->getNumTracks()
               && trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
                      < (tp->isHorizontal() ? bbox.right() : bbox.top());
             ++trackNum) {
          frCoord trackLoc
              = trackNum * tp->getTrackSpacing() + tp->getStartCoord();
          if (tp->isHorizontal()) {
            xMap[trackLoc][currLayerNum] = tp.get();
          } else {
            yMap[trackLoc][currLayerNum] = tp.get();
          }
        }
      }
    }
    zMap[currLayerNum] = currPrefRouteDir;
  }
}

void FlexGridGraph::resetStatus()
{
  resetSrc();
  resetDst();
  resetPrevNodeDir();
}

void FlexGridGraph::resetSrc()
{
  srcs_.assign(srcs_.size(), 0);
}

void FlexGridGraph::resetDst()
{
  dsts_.assign(dsts_.size(), 0);
}

void FlexGridGraph::resetPrevNodeDir()
{
  prevDirs_.assign(prevDirs_.size(), 0);
}

// print the grid graph with edge and vertex for debug purpose
void FlexGridGraph::print() const
{
  ofstream mazeLog(OUT_MAZE_FILE.c_str());
  if (mazeLog.is_open()) {
    // print edges
    frBox gridBBox;
    getBBox(gridBBox);
    mazeLog << "printing Maze grid (" << gridBBox.left() << ", "
            << gridBBox.bottom() << ") -- (" << gridBBox.right() << ", "
            << gridBBox.top() << ")\n";
    frMIdx xDim, yDim, zDim;
    getDim(xDim, yDim, zDim);

    if (xDim == 0 || yDim == 0 || zDim == 0) {
      cout << "Error: dimension == 0\n";
      return;
    } else {
      cout << "extBBox (xDim, yDim, zDim) = (" << xDim << ", " << yDim << ", "
           << zDim << ")\n";
    }

    frPoint p;
    for (frMIdx xIdx = 0; xIdx < xDim; ++xIdx) {
      for (frMIdx yIdx = 0; yIdx < yDim; ++yIdx) {
        for (frMIdx zIdx = 0; zIdx < zDim; ++zIdx) {
          if (hasEdge(xIdx, yIdx, zIdx, frDirEnum::N)) {
            if (yIdx + 1 >= yDim) {
              cout << "Error: no edge (" << xIdx << ", " << yIdx << ", " << zIdx
                   << ", N) " << yDim << endl;
              continue;
            }
            mazeLog << "Edge: " << getPoint(p, xIdx, yIdx).x() << " "
                    << getPoint(p, xIdx, yIdx).y() << " " << zIdx << " "
                    << getPoint(p, xIdx, yIdx + 1).x() << " "
                    << getPoint(p, xIdx, yIdx + 1).y() << " " << zIdx << "\n";
          }
          if (hasEdge(xIdx, yIdx, zIdx, frDirEnum::E)) {
            if (xIdx + 1 >= xDim) {
              cout << "Error: no edge (" << xIdx << ", " << yIdx << ", " << zIdx
                   << ", E) " << xDim << endl;
              continue;
            }
            mazeLog << "Edge: " << getPoint(p, xIdx, yIdx).x() << " "
                    << getPoint(p, xIdx, yIdx).y() << " " << zIdx << " "
                    << getPoint(p, xIdx + 1, yIdx).x() << " "
                    << getPoint(p, xIdx + 1, yIdx).y() << " " << zIdx << "\n";
          }
          if (hasEdge(xIdx, yIdx, zIdx, frDirEnum::U)) {
            if (zIdx + 1 >= zDim) {
              cout << "Error: no edge (" << xIdx << ", " << yIdx << ", " << zIdx
                   << ", U) " << zDim << endl;
              continue;
            }
            mazeLog << "Edge: " << getPoint(p, xIdx, yIdx).x() << " "
                    << getPoint(p, xIdx, yIdx).y() << " " << zIdx << " "
                    << getPoint(p, xIdx, yIdx).x() << " "
                    << getPoint(p, xIdx, yIdx).y() << " " << zIdx + 1 << "\n";
          }
        }
      }
    }
  } else {
    cout << "Error: Fail to open maze log\n";
  }
}
