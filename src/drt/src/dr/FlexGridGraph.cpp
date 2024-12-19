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

namespace drt {
void FlexGridGraph::addAccessPointLocation(frLayerNum layer_num,
                                           frCoord x_coord,
                                           frCoord y_coord)
{
  ap_locs_[layer_num].insert(Point(x_coord, y_coord));
}

bool FlexGridGraph::isAccessPointLocation(frLayerNum layer_num,
                                          frCoord x_coord,
                                          frCoord y_coord) const
{
  if (ap_locs_.size() <= layer_num) {
    return false;
  }
  const auto& layer_maze_locs = ap_locs_[layer_num];
  return layer_maze_locs.find(Point(x_coord, y_coord)) != layer_maze_locs.end();
}
void FlexGridGraph::initGrids(
    const std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& xMap,
    const std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& yMap,
    const std::map<frLayerNum, dbTechLayerDir>& zMap,
    bool followGuide)
{
  // initialize coord vectors
  xCoords_.clear();
  yCoords_.clear();
  zCoords_.clear();
  zHeights_.clear();
  layerRouteDirections_.clear();
  xCoords_.reserve(xMap.size());
  for (auto& [k, v] : xMap) {
    xCoords_.push_back(k);
  }
  yCoords_.reserve(yMap.size());
  for (auto& [k, v] : yMap) {
    yCoords_.push_back(k);
  }
  frCoord zHeight = 0;
  // std::vector<frCoord> via2viaMinLenTmp(4, 0);
  zCoords_.reserve(zMap.size());
  for (auto& [k, v] : zMap) {
    zCoords_.push_back(k);
    zHeight += getTech()->getLayer(k)->getPitch() * router_cfg_->VIACOST;
    zHeights_.push_back(zHeight);
    layerRouteDirections_.push_back(v);
  }
  // initialize all grids
  frMIdx xDim, yDim, zDim;
  getDim(xDim, yDim, zDim);
  const int capacity = xDim * yDim * zDim;

  nodes_.clear();
  nodes_.resize(capacity, Node());
  // new
  prevDirs_.clear();
  srcs_.clear();
  dsts_.clear();

  prevDirs_.resize(capacity * 3, false);
  srcs_.resize(capacity, false);
  dsts_.resize(capacity, false);
  guides_.clear();
  if (followGuide) {
    guides_.resize(capacity, false);
  } else {
    guides_.resize(capacity, true);
  }
}

bool FlexGridGraph::outOfDieVia(frMIdx x,
                                frMIdx y,
                                frMIdx z,
                                const Rect& dieBox)
{
  frLayerNum lNum = getLayerNum(z) + 1;
  if (lNum > getTech()->getTopLayerNum()) {
    return false;
  }
  const frViaDef* via = getTech()->getLayer(lNum)->getDefaultViaDef();
  if (!via) {
    return true;
  }
  Rect viaBox(via->getLayer1ShapeBox());
  viaBox.merge(via->getLayer2ShapeBox());
  viaBox.moveDelta(xCoords_[x], yCoords_[y]);
  return !dieBox.contains(viaBox);
}
bool FlexGridGraph::hasOutOfDieViol(frMIdx x, frMIdx y, frMIdx z)
{
  const frLayerNum lNum = getLayerNum(z);
  if (!getTech()->getLayer(lNum)->isUnidirectional()) {
    return false;
  }
  Rect testBoxUp;
  if (lNum + 1 <= getTech()->getTopLayerNum()) {
    const frViaDef* via = getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    if (via) {
      testBoxUp = via->getLayer1ShapeBox();
      testBoxUp.merge(via->getLayer2ShapeBox());
      testBoxUp.moveDelta(xCoords_[x], yCoords_[y]);
    } else {
      // artificial value to indicate no via in test below
      dieBox_.bloat(1, testBoxUp);
    }
  }
  Rect testBoxDown;
  if (lNum - 1 >= getTech()->getBottomLayerNum()) {
    const frViaDef* via = getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    if (via) {
      testBoxDown = via->getLayer1ShapeBox();
      testBoxDown.merge(via->getLayer2ShapeBox());
      testBoxDown.moveDelta(xCoords_[x], yCoords_[y]);
    } else {
      // artificial value to indicate no via in test below
      dieBox_.bloat(1, testBoxDown);
    }
  }
  if (getTech()->getLayer(lNum)->isVertical()) {
    return (testBoxUp.xMax() > dieBox_.xMax()
            || testBoxUp.xMin() < dieBox_.xMin())
           && (testBoxDown.xMax() > dieBox_.xMax()
               || testBoxDown.xMin() < dieBox_.xMin());
  }
  // layer is horizontal
  return (testBoxUp.yMax() > dieBox_.yMax()
          || testBoxUp.yMin() < dieBox_.yMin())
         && (testBoxDown.yMax() > dieBox_.yMax()
             || testBoxDown.yMin() < dieBox_.yMin());
}

bool FlexGridGraph::isWorkerBorder(frMIdx v, bool isVert)
{
  if (isVert) {
    return xCoords_[v] == drWorker_->getRouteBox().xMin()
           || xCoords_[v] == drWorker_->getRouteBox().xMax();
  }
  return yCoords_[v] == drWorker_->getRouteBox().yMin()
         || yCoords_[v] == drWorker_->getRouteBox().yMax();
}
bool FlexGridGraph::hasAlignedUpDefTrack(
    frLayerNum layerNum,
    const std::map<frLayerNum, frTrackPattern*>& xSubMap,
    const std::map<frLayerNum, frTrackPattern*>& ySubMap) const
{
  for (frLayerNum lNum = layerNum + 2;
       lNum < (int) getTech()->getLayers().size();
       lNum += 2) {
    auto it = xSubMap.find(lNum);
    if (it != xSubMap.end()) {  // has x track in lNum
      if (it->second) {         // has track pattern, i.e., the track is default
        return true;
      }
    }
    it = ySubMap.find(lNum);
    if (it != ySubMap.end()) {  // has y track in lNum
      if (it->second) {         // has track pattern, i.e., the track is default
        return true;
      }
    }
  }
  return false;
}

void FlexGridGraph::initEdges(
    const frDesign* design,
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& xMap,
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& yMap,
    const std::map<frLayerNum, dbTechLayerDir>& zMap,
    const Rect& bbox,
    bool initDR)
{
  frMIdx xDim, yDim, zDim;
  getDim(xDim, yDim, zDim);
  // initialize grid graph
  frMIdx xIdx = 0, yIdx = 0, zIdx = 0;
  dieBox_ = design->getTopBlock()->getDieBox();
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
        bool isOutOfDieVia = outOfDieVia(xIdx, yIdx, zIdx, dieBox_);
        // add edge for preferred direction
        if (dir == dbTechLayerDir::HORIZONTAL && yFound) {
          if (layerNum >= router_cfg_->BOTTOM_ROUTING_LAYER
              && layerNum <= router_cfg_->TOP_ROUTING_LAYER) {
            if ((!isOutOfDieVia || !hasOutOfDieViol(xIdx, yIdx, zIdx))
                && (layer->getLef58RightWayOnGridOnlyConstraint() == nullptr
                    || yIt->second != nullptr)) {
              addEdge(xIdx, yIdx, zIdx, frDirEnum::E, bbox, initDR);
              if (yIt->second == nullptr || isWorkerBorder(yIdx, false)) {
                setGridCostE(xIdx, yIdx, zIdx);
              }
            }
          }
          // via to upper layer
          if (xFound2 && !isOutOfDieVia) {
            const bool is_on_grid
                = yIt->second != nullptr && xIt2->second != nullptr;
            const bool allow_off_grid
                = layer->getLef58RightWayOnGridOnlyConstraint() == nullptr
                  || isAccessPointLocation(layerNum, xCoord, yCoord);
            if (is_on_grid || allow_off_grid) {
              addEdge(xIdx, yIdx, zIdx, frDirEnum::U, bbox, initDR);
              if (!is_on_grid) {
                setGridCostU(xIdx, yIdx, zIdx);
              }
            }
          }
        } else if (dir == dbTechLayerDir::VERTICAL && xFound) {
          if (layerNum >= router_cfg_->BOTTOM_ROUTING_LAYER
              && layerNum <= router_cfg_->TOP_ROUTING_LAYER) {
            if ((!isOutOfDieVia || !hasOutOfDieViol(xIdx, yIdx, zIdx))
                && (layer->getLef58RightWayOnGridOnlyConstraint() == nullptr
                    || xIt->second != nullptr)) {
              addEdge(xIdx, yIdx, zIdx, frDirEnum::N, bbox, initDR);
              if (xIt->second == nullptr || isWorkerBorder(xIdx, true)) {
                setGridCostN(xIdx, yIdx, zIdx);
              }
            }
          }
          // via to upper layer
          if (yFound2 && !isOutOfDieVia) {
            const bool is_on_grid
                = xIt->second != nullptr && yIt2->second != nullptr;
            const bool allow_off_grid
                = layer->getLef58RightWayOnGridOnlyConstraint() == nullptr
                  || isAccessPointLocation(layerNum, xCoord, yCoord);

            if (is_on_grid || allow_off_grid) {
              addEdge(xIdx, yIdx, zIdx, frDirEnum::U, bbox, initDR);
              if (!is_on_grid) {
                setGridCostU(xIdx, yIdx, zIdx);
              }
            }
          }
        }
        // get non pref track layer --> use upper layer pref dir track if
        // possible
        if (router_cfg_->USENONPREFTRACKS && !layer->isUnidirectional()) {
          // add edge for non-preferred direction
          // vertical non-pref track
          if (dir == dbTechLayerDir::HORIZONTAL && xFound3) {
            if (layerNum >= router_cfg_->BOTTOM_ROUTING_LAYER
                && layerNum <= router_cfg_->TOP_ROUTING_LAYER) {
              addEdge(xIdx, yIdx, zIdx, frDirEnum::N, bbox, initDR);
              setGridCostN(xIdx, yIdx, zIdx);
            }
            // horizontal non-pref track
          } else if (dir == dbTechLayerDir::VERTICAL && yFound3) {
            if (layerNum >= router_cfg_->BOTTOM_ROUTING_LAYER
                && layerNum <= router_cfg_->TOP_ROUTING_LAYER) {
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
      if (!up) {
        startZ--;
      }
      frMIdx xIdx = getMazeXIdx(apPt.x());
      frMIdx yIdx = getMazeYIdx(apPt.y());
      // create the edges
      for (int zIdx = startZ; zIdx >= 0 && zIdx < (int) zCoords_.size() - 1;
           zIdx += inc, nextLNum += inc * 2) {
        addEdge(xIdx, yIdx, zIdx, frDirEnum::U, bbox, initDR);
        frLayer* nextLayer = getTech()->getLayer(nextLNum);
        const bool restrictedRouting
            = nextLayer->isUnidirectional()
              || nextLNum < router_cfg_->BOTTOM_ROUTING_LAYER
              || nextLNum > router_cfg_->TOP_ROUTING_LAYER;
        if (!restrictedRouting || nextLayer->isVertical()) {
          auto& xSubMap = xMap[apPt.x()];
          auto xTrack = xSubMap.find(nextLNum);
          if (xTrack != xSubMap.end() && xTrack->second != nullptr) {
            break;
          }
        }
        if (!restrictedRouting || nextLayer->isHorizontal()) {
          auto& ySubMap = yMap[apPt.y()];
          auto yTrack = ySubMap.find(nextLNum);
          if (yTrack != ySubMap.end() && yTrack->second != nullptr) {
            break;
          }
        }
        // didn't find default track, then create tracks if possible
        if (!restrictedRouting
            && nextLNum >= router_cfg_->VIA_ACCESS_LAYERNUM) {
          dbTechLayerDir prefDir
              = design->getTech()->getLayer(nextLNum)->getDir();
          xMap[apPt.x()][nextLNum] = nullptr;  // to keep coherence
          yMap[apPt.y()][nextLNum] = nullptr;
          frMIdx nextZ = up ? zIdx + 1 : zIdx;
          // This is a value to make sure the edges we are adding will
          // reach a track on the layer of interest.  It is simpler to
          // be conservative than trying to figure out how many edges
          // to add to hit it precisely.  I intend to obviate the need
          // for this whole approach next.  Note that addEdge checks
          // for bounds so I don't.
          const int max_offset = 20;
          if (prefDir == dbTechLayerDir::HORIZONTAL) {
            for (int offset = 0; offset < max_offset; ++offset) {
              addEdge(xIdx, yIdx + offset, nextZ, frDirEnum::N, bbox, initDR);
              addEdge(xIdx, yIdx - offset, nextZ, frDirEnum::S, bbox, initDR);
            }
          } else {
            for (int offset = 0; offset < max_offset; ++offset) {
              addEdge(xIdx + offset, yIdx, nextZ, frDirEnum::E, bbox, initDR);
              addEdge(xIdx - offset, yIdx, nextZ, frDirEnum::W, bbox, initDR);
            }
          }
          break;
        }
      }
    }
  }
}

// initialization: update grid graph topology, does not assign edge cost
void FlexGridGraph::init(
    const frDesign* design,
    const Rect& routeBBox,
    const Rect& extBBox,
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& xMap,
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& yMap,
    bool initDR,
    bool followGuide)
{
  auto* via_data = getDRWorker()->getViaData();
  halfViaEncArea_ = &via_data->halfViaEncArea;

  // get tracks intersecting with the Maze bbox
  std::map<frLayerNum, dbTechLayerDir> zMap;
  initTracks(design, xMap, yMap, zMap, extBBox);
  initGrids(xMap, yMap, zMap, followGuide);  // buildGridGraph
  initEdges(
      design, xMap, yMap, zMap, routeBBox, initDR);  // add edges and edgeCost
  ap_locs_.clear();
}

// initialization helpers
// get all tracks intersecting with the Maze bbox, left/bottom are inclusive
void FlexGridGraph::initTracks(
    const frDesign* design,
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>&
        horLoc2TrackPatterns,
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>&
        vertLoc2TrackPatterns,
    std::map<frLayerNum, dbTechLayerDir>& layerNum2PreRouteDir,
    const Rect& bbox)
{
  for (auto& layer : getTech()->getLayers()) {
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    frLayerNum currLayerNum = layer->getLayerNum();
    dbTechLayerDir currPrefRouteDir = layer->getDir();
    for (auto& tp : design->getTopBlock()->getTrackPatterns(currLayerNum)) {
      // allow wrongway if global variable and design rule allow
      bool flag
          = (router_cfg_->USENONPREFTRACKS && !layer->isUnidirectional())
                ? (tp->isHorizontal()
                   && currPrefRouteDir == dbTechLayerDir::VERTICAL)
                      || (!tp->isHorizontal()
                          && currPrefRouteDir == dbTechLayerDir::HORIZONTAL)
                : true;
      if (flag) {
        int trackNum = ((tp->isHorizontal() ? bbox.xMin() : bbox.yMin())
                        - tp->getStartCoord())
                       / (int) tp->getTrackSpacing();
        if (trackNum < 0) {
          trackNum = 0;
        }
        if (trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
            < (tp->isHorizontal() ? bbox.xMin() : bbox.yMin())) {
          ++trackNum;
        }
        for (; trackNum < (int) tp->getNumTracks()
               && trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
                      < (tp->isHorizontal() ? bbox.xMax() : bbox.yMax());
             ++trackNum) {
          frCoord trackLoc
              = trackNum * tp->getTrackSpacing() + tp->getStartCoord();
          if (tp->isHorizontal()) {
            horLoc2TrackPatterns[trackLoc][currLayerNum] = tp.get();
          } else {
            vertLoc2TrackPatterns[trackLoc][currLayerNum] = tp.get();
          }
        }
      }
    }
    layerNum2PreRouteDir[currLayerNum] = currPrefRouteDir;
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
  srcs_.assign(srcs_.size(), false);
}

void FlexGridGraph::resetDst()
{
  dsts_.assign(dsts_.size(), false);
}

void FlexGridGraph::resetPrevNodeDir()
{
  prevDirs_.assign(prevDirs_.size(), false);
}

// print the grid graph with edge and vertex for debug purpose
void FlexGridGraph::print() const
{
  std::ofstream mazeLog(router_cfg_->OUT_MAZE_FILE.c_str());
  if (mazeLog.is_open()) {
    // print edges
    Rect gridBBox;
    getBBox(gridBBox);
    mazeLog << "printing Maze grid (" << gridBBox.xMin() << ", "
            << gridBBox.yMin() << ") -- (" << gridBBox.xMax() << ", "
            << gridBBox.yMax() << ")\n";
    frMIdx xDim, yDim, zDim;
    getDim(xDim, yDim, zDim);

    if (xDim == 0 || yDim == 0 || zDim == 0) {
      std::cout << "Error: dimension == 0\n";
      return;
    }
    std::cout << "extBBox (xDim, yDim, zDim) = (" << xDim << ", " << yDim
              << ", " << zDim << ")\n";

    Point p;
    for (frMIdx xIdx = 0; xIdx < xDim; ++xIdx) {
      for (frMIdx yIdx = 0; yIdx < yDim; ++yIdx) {
        for (frMIdx zIdx = 0; zIdx < zDim; ++zIdx) {
          if (hasEdge(xIdx, yIdx, zIdx, frDirEnum::N)) {
            if (yIdx + 1 >= yDim) {
              std::cout << "Error: no edge (" << xIdx << ", " << yIdx << ", "
                        << zIdx << ", N) " << yDim << std::endl;
              continue;
            }
            mazeLog << "Edge: " << getPoint(p, xIdx, yIdx).x() << " "
                    << getPoint(p, xIdx, yIdx).y() << " " << zIdx << " "
                    << getPoint(p, xIdx, yIdx + 1).x() << " "
                    << getPoint(p, xIdx, yIdx + 1).y() << " " << zIdx << "\n";
          }
          if (hasEdge(xIdx, yIdx, zIdx, frDirEnum::E)) {
            if (xIdx + 1 >= xDim) {
              std::cout << "Error: no edge (" << xIdx << ", " << yIdx << ", "
                        << zIdx << ", E) " << xDim << std::endl;
              continue;
            }
            mazeLog << "Edge: " << getPoint(p, xIdx, yIdx).x() << " "
                    << getPoint(p, xIdx, yIdx).y() << " " << zIdx << " "
                    << getPoint(p, xIdx + 1, yIdx).x() << " "
                    << getPoint(p, xIdx + 1, yIdx).y() << " " << zIdx << "\n";
          }
          if (hasEdge(xIdx, yIdx, zIdx, frDirEnum::U)) {
            if (zIdx + 1 >= zDim) {
              std::cout << "Error: no edge (" << xIdx << ", " << yIdx << ", "
                        << zIdx << ", U) " << zDim << std::endl;
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
    std::cout << "Error: Fail to open maze log\n";
  }
}

}  // namespace drt
