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

#ifndef _FLEX_GRID_GRAPH_H
#define _FLEX_GRID_GRAPH_H

#include "frBaseTypes.h"
#include "FlexMazeTypes.h"
#include "frDesign.h"
#include "db/drObj/drPin.h"
#include "dr/FlexWavefront.h"
#include <map>
#include <iostream>
#include <cstring>


namespace fr {
  class FlexDRWorker;
  class FlexDRGraphics;
  class FlexGridGraph {
  public:
    // constructors
    //FlexGridGraph() {}
    FlexGridGraph(frDesign* designIn, FlexDRWorker* workerIn):
                  design_(designIn), drWorker_(workerIn),
                  graphics_(nullptr),
                  xCoords_(), yCoords_(), zCoords_(), zHeights_(),
                  ggDRCCost_(0), ggMarkerCost_(0), halfViaEncArea_(nullptr),
                  via2viaMinLen_(nullptr), via2viaMinLenNew_(nullptr), 
                  via2turnMinLen_(nullptr), ndr_(nullptr), dstTaperBox(nullptr) {}
    // getters
    frTechObject* getTech() const {
      return design_->getTech();
    }
    frDesign* getDesign() const {
      return design_;
    }
    FlexDRWorker* getDRWorker() const {
      return drWorker_;
    }
    // getters
    // unsafe access, no check
    bool isAstarVisited(frMIdx x, frMIdx y, frMIdx z) const {
      return (getPrevAstarNodeDir(x, y, z) == frDirEnum::UNKNOWN);
    }
    // unsafe access, no check
    frDirEnum getPrevAstarNodeDir(frMIdx x, frMIdx y, frMIdx z) const {
      auto baseIdx = 3 * getIdx(x, y, z);
      return (frDirEnum)(((unsigned short)(prevDirs_[baseIdx]    ) << 2) +
                         ((unsigned short)(prevDirs_[baseIdx + 1]) << 1) +
                         ((unsigned short)(prevDirs_[baseIdx + 2]) << 0));
    }
    // unsafe access, no check
    bool isSrc(frMIdx x, frMIdx y, frMIdx z) const {
      return srcs_[getIdx(x, y, z)];
    }
    // unsafe access, no check
    bool isDst(frMIdx x, frMIdx y, frMIdx z) const {
      return dsts_[getIdx(x, y, z)];
    }
    bool isDst(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const {
        getNextGrid(x, y, z, dir);
        bool b = dsts_[getIdx(x, y, z)];
        getPrevGrid(x, y, z, dir);
        return b;
    }
    // unsafe access, no check
    bool isBlocked(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        const Node& node = nodes_[getIdx(x, y, z)];
        switch (dir) {
          case frDirEnum::E:
            return node.isBlockedEast;
          case frDirEnum::N:
            return node.isBlockedNorth;
          case frDirEnum::U:
            return node.isBlockedUp;
          default:
            return false;
        }
      } else {
        return false;
      }
    }
    // unsafe access, no check
    bool isSVia(frMIdx x, frMIdx y, frMIdx z) const {
      return nodes_[getIdx(x, y, z)].hasSpecialVia;
    }
    // unsafe access, no check
    bool hasGridCostE(frMIdx x, frMIdx y, frMIdx z) const {
      return nodes_[getIdx(x, y, z)].hasGridCostEast;
    }
    // unsafe access, no check
    bool hasGridCostN(frMIdx x, frMIdx y, frMIdx z) const {
      return nodes_[getIdx(x, y, z)].hasGridCostNorth;
    }
    // unsafe access, no check
    bool hasGridCostU(frMIdx x, frMIdx y, frMIdx z) const {
      return nodes_[getIdx(x, y, z)].hasGridCostUp;
    }

    void getBBox(frBox &in) const {
      if (xCoords_.size() && yCoords_.size()) {
        in.set(xCoords_.front(), yCoords_.front(), xCoords_.back(), yCoords_.back());
      }
    }
    void getDim(frMIdx &xDim, frMIdx &yDim, frMIdx &zDim) const {
      xDim = xCoords_.size();
      yDim = yCoords_.size();
      zDim = zCoords_.size();
    }
    // unsafe access
    frPoint& getPoint(frPoint &in, frMIdx x, frMIdx y) const {
      in.set(xCoords_[x], yCoords_[y]);
      return in;
    }
    // unsafe access
    frLayerNum getLayerNum(frMIdx z) const {
      return zCoords_[z];
    }
    bool hasMazeXIdx(frCoord in) const {
      return std::binary_search(xCoords_.begin(), xCoords_.end(), in);
    }
    bool hasMazeYIdx(frCoord in) const {
      return std::binary_search(yCoords_.begin(), yCoords_.end(), in);
    }
    bool hasMazeZIdx(frLayerNum in) const {
      return std::binary_search(zCoords_.begin(), zCoords_.end(), in);
    }
    bool hasIdx(const frPoint &p, frLayerNum lNum) const {
      return (hasMazeXIdx(p.x()) && hasMazeYIdx(p.y()) && hasMazeZIdx(lNum));
    }
    bool hasMazeIdx(const frPoint &p, frLayerNum lNum) const {
      return (hasMazeXIdx(p.x()) && hasMazeYIdx(p.y()) && hasMazeZIdx(lNum));
    }
    frMIdx getMazeXIdx(frCoord in) const {
      auto it = std::lower_bound(xCoords_.begin(), xCoords_.end(), in);
      return it - xCoords_.begin();
    }
    frMIdx getMazeYIdx(frCoord in) const {
      auto it = std::lower_bound(yCoords_.begin(), yCoords_.end(), in);
      return it - yCoords_.begin();
    }
    frMIdx getMazeZIdx(frLayerNum in) const {
      auto it = std::lower_bound(zCoords_.begin(), zCoords_.end(), in);
      return it - zCoords_.begin();
    }
    FlexMazeIdx& getMazeIdx(FlexMazeIdx &mIdx, const frPoint &p, frLayerNum layerNum) const {
      mIdx.set(getMazeXIdx(p.x()), getMazeYIdx(p.y()), getMazeZIdx(layerNum));
      return mIdx;
    }
    // unsafe access, z always = 0
    void getIdxBox(FlexMazeIdx &mIdx1, FlexMazeIdx &mIdx2, const frBox &box) const {
      mIdx1.set(std::lower_bound(xCoords_.begin(), xCoords_.end(), box.left()  ) - xCoords_.begin(),
                std::lower_bound(yCoords_.begin(), yCoords_.end(), box.bottom()) - yCoords_.begin(),
                mIdx1.z());
      mIdx2.set(frMIdx(std::upper_bound(xCoords_.begin(), xCoords_.end(), box.right() ) - xCoords_.begin()) - 1,
                frMIdx(std::upper_bound(yCoords_.begin(), yCoords_.end(), box.top()   ) - yCoords_.begin()) - 1,
                mIdx2.z());
    }
    frCoord getZHeight(frMIdx in) const {
      return zHeights_[in];
    }
    bool getZDir(frMIdx in) const {
      return zDirs_[in];
    }
    bool hasEdge(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        const Node& node = nodes_[getIdx(x, y, z)];
        switch (dir) {
          case frDirEnum::E:
            return node.hasEastEdge;
          case frDirEnum::N:
            return node.hasNorthEdge;
          case frDirEnum::U:
            return node.hasUpEdge;
          default:
            return false;
        }
      } else {
        return false;
      }
    }
    bool hasGridCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const {
      bool sol = false;
      correct(x, y, z, dir);
      switch(dir) {
        case frDirEnum::E:
          sol = hasGridCostE(x, y, z);
          break;
        case frDirEnum::N:
          sol = hasGridCostN(x, y, z);
          break;
        default:
          sol = hasGridCostU(x, y, z);
      }
      return sol;
    }
    bool hasShapeCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const {
      frUInt4 sol = 0;
      if (dir != frDirEnum::D && dir != frDirEnum::U) {
        reverse(x, y, z, dir);
        sol = nodes_[getIdx(x, y, z)].shapeCostPlanar;
      } else {
        correctU(x, y, z, dir);
        const Node& node = nodes_[getIdx(x, y, z)];
        sol = isOverrideShapeCost(x, y, z, dir) ? 0 : node.shapeCostVia;
      }
      return (sol);
    }
    bool isOverrideShapeCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const {
      if (dir != frDirEnum::D && dir != frDirEnum::U) {
        return false;
      } else {
        correctU(x, y, z, dir);
        auto idx = getIdx(x, y, z);
        return nodes_[idx].overrideShapeCostVia;
      }
    }
    bool hasDRCCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const {
      frUInt4 sol = 0;
      if (dir != frDirEnum::D && dir != frDirEnum::U) {
        reverse(x, y, z, dir);
        auto idx = getIdx(x, y, z);
        sol = nodes_[idx].drcCostPlanar;
      } else {
        correctU(x, y, z, dir);
        auto idx = getIdx(x, y, z);
        sol = nodes_[idx].drcCostVia;
      }
      return (sol);
    }
    bool hasMarkerCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const {
      frUInt4 sol = 0;
      // old
      if (dir != frDirEnum::D && dir != frDirEnum::U) {
        reverse(x, y, z, dir);
        auto idx = getIdx(x, y, z);
        sol += nodes_[idx].markerCostPlanar;
      } else {
        correctU(x, y, z, dir);
        auto idx = getIdx(x, y, z);
        sol += nodes_[idx].markerCostVia;
      }
      return (sol);
    }
    bool hasAnyPlanarCost(frMIdx x, frMIdx y, frMIdx z) const{
        auto& nd = nodes_[getIdx(x, y, z)];
        return nd.drcCostPlanar || nd.markerCostPlanar || nd.shapeCostPlanar;
    }
    frCoord xCoord(frMIdx x) const{
        return xCoords_[x];
    }
    frCoord yCoord(frMIdx y) const{
        return yCoords_[y];
    }
    // unsafe access
    frCoord getEdgeLength(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const {
      frCoord sol = 0;
      correct(x, y, z, dir);
      //if (isValid(x, y, z, dir)) {
        switch (dir) {
          case frDirEnum::E:
            sol = xCoords_[x+1] - xCoords_[x];
            break;
          case frDirEnum::N:
            sol = yCoords_[y+1] - yCoords_[y];
            break;
          case frDirEnum::U:
            sol = zHeights_[z+1] - zHeights_[z];
            break;
          default:
            ;
        }
      //}
      return sol;
    }
    bool isEdgeInBox(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, const frBox &box, bool initDR) const {
      bool sol = false;
      correct(x, y, z, dir);
      if (isValid(x, y, z, dir)) {
        auto x1 = x;
        auto y1 = y;
        auto z1 = z;
        reverse(x1, y1, z1, dir);
        frPoint pt, pt1;
        getPoint(pt, x, y);
        getPoint(pt1, x1, y1);
        if (box.contains(pt) && box.contains(pt1)) {
          // initDR must not use top and rightmost track
          if (initDR &&
              ((box.right() == pt.x() && box.right() == pt1.x()) ||
               (box.top()   == pt.y() && box.top()   == pt1.y()))) {
            sol = false;
          } else {
            sol = true;
          }
        } else {
          sol = false;
        }
      } else {
        sol = false;
      }
      return sol;
    }
    // setters
    bool addEdge(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, const frBox &box, bool initDR) {
      bool sol = false;
      if (!isEdgeInBox(x, y, z, dir, box, initDR)) {
        sol = false;
      } else {
        //cout <<"orig edge (" <<x <<", " <<y <<", " <<z <<", " <<int(dir) <<")" <<endl;
        correct(x, y, z, dir);
        //cout <<"corr edge (" <<x <<", " <<y <<", " <<z <<", " <<int(dir) <<")" <<endl;
        if (isValid(x, y, z, dir)) {
          Node& node = nodes_[getIdx(x, y, z)];
          switch (dir) {
            case frDirEnum::E:
              node.hasEastEdge = true;
              sol = true;
              break;
            case frDirEnum::N:
              node.hasNorthEdge = true;
              sol = true;
              break;
            case frDirEnum::U:
              node.hasUpEdge = true;
              sol = true;
              break;
            default:
              ;
          }
        } else {
          //cout <<"not valid edge";
        }
      }
      return sol;
    }
    bool removeEdge(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) {
      bool sol = false;
      correct(x, y, z, dir);
      if (isValid(x, y, z, dir)) {
        Node& node = nodes_[getIdx(x, y, z)];
        switch (dir) {
          case frDirEnum::E:
            node.hasEastEdge = false;
            sol = true;
            break;
          case frDirEnum::N:
            node.hasNorthEdge = false;
            sol = true;
            break;
          case frDirEnum::U:
            node.hasUpEdge = false;
            sol = true;
            break;
          default:
            ;
        }
      }
      return sol;
    }
    void setBlocked(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        Node& node = nodes_[getIdx(x, y, z)];
        switch (dir) {
          case frDirEnum::E:
            node.isBlockedEast = true;
            break;
          case frDirEnum::N:
            node.isBlockedNorth = true;
            break;
          case frDirEnum::U:
            node.isBlockedUp = true;
            break;
          default:
            ;
        }
      }
    }
    void resetBlocked(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        Node& node = nodes_[getIdx(x, y, z)];
        switch (dir) {
          case frDirEnum::E:
            node.isBlockedEast = false;
            break;
          case frDirEnum::N:
            node.isBlockedNorth = false;
            break;
          case frDirEnum::U:
            node.isBlockedUp = false;
            break;
          default:
            ;
        }
      }
    }
    void addDRCCostPlanar(frMIdx x, frMIdx y, frMIdx z) {
      auto& node = nodes_[getIdx(x, y, z)];
      node.drcCostPlanar = addToByte(node.drcCostPlanar, 1);
    }
    void addDRCCostVia(frMIdx x, frMIdx y, frMIdx z) {
      auto& node = nodes_[getIdx(x, y, z)];
      node.drcCostVia = addToByte(node.drcCostVia, 1);
    }
    void subDRCCostPlanar(frMIdx x, frMIdx y, frMIdx z) {
      auto& node = nodes_[getIdx(x, y, z)];
      node.drcCostPlanar = subFromByte(node.drcCostPlanar, 1);
    }
    void subDRCCostVia(frMIdx x, frMIdx y, frMIdx z) {
      auto& node = nodes_[getIdx(x, y, z)];
      node.drcCostVia = subFromByte(node.drcCostVia, 1);
    }
    void resetDRCCostPlanar(frMIdx x, frMIdx y, frMIdx z) {
      auto idx = getIdx(x, y, z);
      nodes_[idx].drcCostPlanar = 0;
    }
    void resetDRCCostVia(frMIdx x, frMIdx y, frMIdx z) {
      auto idx = getIdx(x, y, z);
      nodes_[idx].drcCostVia = 0;
    }
    void addMarkerCostPlanar(frMIdx x, frMIdx y, frMIdx z) {
      auto& node = nodes_[getIdx(x, y, z)];
      node.markerCostPlanar = addToByte(node.markerCostPlanar, 10);
    }
    void addMarkerCostVia(frMIdx x, frMIdx y, frMIdx z) {
      auto& node = nodes_[getIdx(x, y, z)];
      node.markerCostVia = addToByte(node.markerCostVia, 10);
    }
    void addMarkerCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        Node& node = nodes_[getIdx(x, y, z)];
        switch (dir) {
          case frDirEnum::E:
          case frDirEnum::N:
            node.markerCostPlanar = addToByte(node.markerCostPlanar, 10);
            break;
          case frDirEnum::U:
            node.markerCostVia = addToByte(node.markerCostVia, 10);
            break;
          default:
            ;
        }
      }
    }
    bool decayMarkerCostPlanar(frMIdx x, frMIdx y, frMIdx z, float d) {
      auto idx = getIdx(x, y, z);
      Node& node = nodes_[idx];
      int currCost = node.markerCostPlanar;
      currCost *= d;
      currCost = std::max(0, currCost);
      node.markerCostPlanar = currCost;
      return (currCost == 0);
    }
    bool decayMarkerCostVia(frMIdx x, frMIdx y, frMIdx z, float d) {
      auto idx = getIdx(x, y, z);
      Node& node = nodes_[idx];
      int currCost = node.markerCostVia;
      currCost *= d;
      currCost = std::max(0, currCost);
      node.markerCostVia = currCost;
      return (currCost == 0);
    }
    bool decayMarkerCostPlanar(frMIdx x, frMIdx y, frMIdx z) {
      auto idx = getIdx(x, y, z);
      Node& node = nodes_[idx];
      int currCost = node.markerCostPlanar;
      currCost--;
      currCost = std::max(0, currCost);
      node.markerCostPlanar = currCost;
      return (currCost == 0);
    }
    bool decayMarkerCostVia(frMIdx x, frMIdx y, frMIdx z) {
      auto idx = getIdx(x, y, z);
      Node& node = nodes_[idx];
      int currCost = node.markerCostVia;
      currCost--;
      currCost = std::max(0, currCost);
      node.markerCostVia = currCost;
      return (currCost == 0);
    }
    bool decayMarkerCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, float d) {
      correct(x, y, z, dir);
      int currCost = 0;
      if (isValid(x, y, z)) {
        Node& node = nodes_[getIdx(x, y, z)];
        switch (dir) {
          case frDirEnum::E:
            currCost = node.markerCostPlanar;
            currCost *= d;
            currCost = std::max(0, currCost);
            node.markerCostPlanar = currCost;
          case frDirEnum::N:
            currCost = node.markerCostPlanar;
            currCost *= d;
            currCost = std::max(0, currCost);
            node.markerCostPlanar = currCost;
          case frDirEnum::U:
            currCost = node.markerCostVia;
            currCost *= d;
            currCost = std::max(0, currCost);
            node.markerCostVia = currCost;
          default:
            ;
        }
      }
      return (currCost == 0);
    }
    void addShapeCostPlanar(frMIdx x, frMIdx y, frMIdx z) {
      if (isValid(x, y, z)) {
        auto& node = nodes_[getIdx(x, y, z)];
        node.shapeCostPlanar = addToByte(node.shapeCostPlanar, 1);
      }
    }
    void addShapeCostVia(frMIdx x, frMIdx y, frMIdx z) {
      if (isValid(x, y, z)) {
        auto& node = nodes_[getIdx(x, y, z)];
        node.shapeCostVia = addToByte(node.shapeCostVia, 1);
      }
    }
    void subShapeCostPlanar(frMIdx x, frMIdx y, frMIdx z) {
      if (isValid(x, y, z)) {
        auto& node = nodes_[getIdx(x, y, z)];
        node.shapeCostPlanar = subFromByte(node.shapeCostPlanar, 1);
      }
    }
    void subShapeCostVia(frMIdx x, frMIdx y, frMIdx z) {
      if (isValid(x, y, z)) {
        auto& node = nodes_[getIdx(x, y, z)];
        node.shapeCostVia = subFromByte(node.shapeCostVia, 1);
      }
    }

    // unsafe access, no idx check
    void setPrevAstarNodeDir(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) {
      auto baseIdx = 3 * getIdx(x, y, z);
      prevDirs_[baseIdx]     = ((unsigned short)dir >> 2) & 1;
      prevDirs_[baseIdx + 1] = ((unsigned short)dir >> 1) & 1;
      prevDirs_[baseIdx + 2] = ((unsigned short)dir     ) & 1;
    }
    // unsafe access, no idx check
    void setSrc(frMIdx x, frMIdx y, frMIdx z) {
      srcs_[getIdx(x, y, z)] = 1;
    }
    void setSrc(const FlexMazeIdx &mi) {
      srcs_[getIdx(mi.x(), mi.y(), mi.z())] = 1;
    }
    // unsafe access, no idx check
    void setDst(frMIdx x, frMIdx y, frMIdx z) {
      dsts_[getIdx(x, y, z)] = 1;
    }
    void setDst(const FlexMazeIdx &mi) {
      dsts_[getIdx(mi.x(), mi.y(), mi.z())] = 1;
    }
    // unsafe access
    void setSVia(frMIdx x, frMIdx y, frMIdx z) {
      nodes_[getIdx(x, y, z)].hasSpecialVia = true;
    }
    void setOverrideShapeCostVia(frMIdx x, frMIdx y, frMIdx z) {
      nodes_[getIdx(x, y, z)].overrideShapeCostVia = true;
    }
    void resetOverrideShapeCostVia(frMIdx x, frMIdx y, frMIdx z) {
      nodes_[getIdx(x, y, z)].overrideShapeCostVia = false;
    }
    void setGridCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        Node& node = nodes_[getIdx(x, y, z)];
        switch (dir) {
          case frDirEnum::E:
            node.hasGridCostEast = true;
            break;
          case frDirEnum::N:
            node.hasGridCostNorth = true;
            break;
          case frDirEnum::U:
            node.hasGridCostUp = true;
            break;
          default:
            ;
        }
      }
    }
    void setGridCostE(frMIdx x, frMIdx y, frMIdx z) {
      nodes_[getIdx(x, y, z)].hasGridCostEast = true;
    }
    void setGridCostN(frMIdx x, frMIdx y, frMIdx z) {
      nodes_[getIdx(x, y, z)].hasGridCostNorth = true;
    }
    void setGridCostU(frMIdx x, frMIdx y, frMIdx z) {
      nodes_[getIdx(x, y, z)].hasGridCostUp = true;
    }
    // unsafe access, no idx check
    void resetSrc(frMIdx x, frMIdx y, frMIdx z) {
      srcs_[getIdx(x, y, z)] = 0;
    }
    void resetSrc(const FlexMazeIdx &mi) {
      srcs_[getIdx(mi.x(), mi.y(), mi.z())] = 0;
    }
    // unsafe access, no idx check
    void resetDst(frMIdx x, frMIdx y, frMIdx z) {
      dsts_[getIdx(x, y, z)] = 0;
    }
    void resetDst(const FlexMazeIdx &mi) {
      dsts_[getIdx(mi.x(), mi.y(), mi.z())] = 0;
    }
    void resetGridCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) {
      correct(x, y, z, dir);
      if (isValid(x, y, z)) {
        Node& node = nodes_[getIdx(x, y, z)];
        switch (dir) {
          case frDirEnum::E:
            node.hasGridCostEast = false;
            break;
          case frDirEnum::N:
            node.hasGridCostNorth = false;
            break;
          case frDirEnum::U:
            node.hasGridCostUp = false;
            break;
          default:
            ;
        }
      }
    }

    bool hasGuide(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const {
      reverse(x, y, z, dir);
      auto idx = getIdx(x, y, z);
      return guides_[idx];
    }
    // must be safe access because idx1 and idx2 may be invalid
    void setGuide(frMIdx x1, frMIdx y1, frMIdx x2, frMIdx y2, frMIdx z) {
      //if (!(isValid(x1, y1, z) && isValid(x2, y2, z))) {
      //  return;
      //}
      if (x2 < x1 || y2 < y1) {
        return;
      }
      if (getZDir(z)) { // H
        for (int i = y1; i <= y2; i++) {
          auto idx1 = getIdx(x1, i, z);
          auto idx2 = getIdx(x2, i, z);
          std::fill(guides_.begin() + idx1, guides_.begin() + idx2 + 1, 1);
          //std::cout <<"fill H from " <<idx1 <<" to " <<idx2 <<" ("
          //          <<x1 <<", " <<i <<", " <<z <<") ("
          //          <<x2 <<", " <<i <<", " <<z <<") "
          //          <<std::endl;
        }
      } else { // V
        for (int i = x1; i <= x2; i++) {
          auto idx1 = getIdx(i, y1, z);
          auto idx2 = getIdx(i, y2, z);
          std::fill(guides_.begin() + idx1, guides_.begin() + idx2 + 1, 1);
          //std::cout <<"fill V from " <<idx1 <<" to " <<idx2 <<" ("
          //          <<i <<", " <<y1 <<", " <<z <<") ("
          //          <<i <<", " <<y2 <<", " <<z <<") "
          //          <<std::endl;
        }
      }
    }
    void resetGuide(frMIdx x1, frMIdx y1, frMIdx x2, frMIdx y2, frMIdx z) {
      //if (!(isValid(x1, y1, z) && isValid(x2, y2, z))) {
      //  return;
      //}
      if (x2 < x1 || y2 < y1) {
        return;
      }
      if (getZDir(z)) { // H
        for (int i = y1; i <= y2; i++) {
          auto idx1 = getIdx(x1, i, z);
          auto idx2 = getIdx(x2, i, z);
          std::fill(guides_.begin() + idx1, guides_.begin() + idx2 + 1, 0);
          //std::cout <<"unfill H from " <<idx1 <<" to " <<idx2 <<" ("
          //          <<x1 <<", " <<i <<", " <<z <<") ("
          //          <<x2 <<", " <<i <<", " <<z <<") "
          //          <<std::endl;
        }
      } else { // V
        for (int i = x1; i <= x2; i++) {
          auto idx1 = getIdx(i, y1, z);
          auto idx2 = getIdx(i, y2, z);
          std::fill(guides_.begin() + idx1, guides_.begin() + idx2 + 1, 0);
          //std::cout <<"unfill V from " <<idx1 <<" to " <<idx2 <<" ("
          //          <<i <<", " <<y1 <<", " <<z <<") ("
          //          <<i <<", " <<y2 <<", " <<z <<") "
          //          <<std::endl;
        }
      }
    }
    void setGraphics(FlexDRGraphics* g) {
      graphics_ = g;
    }

    void setNDR(frNonDefaultRule* ndr){
        ndr_ = ndr;
    }

    void setDstTaperBox(frBox3D* t){
        dstTaperBox = t;
    }

    frCoord getCostsNDR(frMIdx gridX, frMIdx gridY, frMIdx gridZ, frDirEnum dir, frDirEnum prevDir, frLayer* layer) const;
    frCoord getViaCostsNDR(frMIdx gridX, frMIdx gridY, frMIdx gridZ, frDirEnum dir, frDirEnum prevDir, frLayer* layer) const;
    frCoord getMinSpacingValue(frLayer* layer, frCoord width1, frCoord width2, frCoord prl) const;
    frCost getCosts(frMIdx gridX, frMIdx gridY, frMIdx gridZ, frDirEnum dir, frLayer* layer) const;
    bool useNDRCosts(const FlexWavefrontGrid& p) const;
    
    frNonDefaultRule* getNDR() const{
        return ndr_;
    }
    const frBox3D* getDstTaperBox() const{
        return dstTaperBox;
    }
    // functions
    void init(const frBox &routeBBox, const frBox &extBBox,
              std::map<frCoord, std::map<frLayerNum, frTrackPattern* > > &xMap,
              std::map<frCoord, std::map<frLayerNum, frTrackPattern* > > &yMap,
              bool initDR, bool followGuide);
    void print() const;
    void resetStatus();
    void resetPrevNodeDir();
    void resetSrc();
    void resetDst();
    bool search(std::vector<FlexMazeIdx> &connComps, drPin* nextPin, std::vector<FlexMazeIdx> &path,
                FlexMazeIdx &ccMazeIdx1, FlexMazeIdx &ccMazeIdx2, const frPoint &centerPt,
                std::map<FlexMazeIdx, frBox3D*>& mazeIdx2TaperBox);
    void setCost(frUInt4 drcCostIn, frUInt4 markerCostIn) {
      ggDRCCost_    = drcCostIn;
      ggMarkerCost_ = markerCostIn;
    }
    frCoord getHalfViaEncArea(frMIdx z, bool isLayer1) const {
      return (isLayer1 ? (*halfViaEncArea_)[z].first: (*halfViaEncArea_)[z].second);
    }
    bool allowVia2ViaZeroLen(frMIdx z, bool isPrevViaUp, bool isCurrViaUp) const {
      return ((*via2viaMinLen_)[z].second)[((unsigned)isPrevViaUp << 1) + (unsigned)isCurrViaUp];
    }
    frCoord getVia2ViaMinLen(frMIdx z, bool isPrevViaUp, bool isCurrViaUp) const {
      return ((*via2viaMinLen_)[z].first)[((unsigned)isPrevViaUp << 1) + (unsigned)isCurrViaUp];
    }
    frCoord getVia2ViaMinLenNew(frMIdx z, bool isPrevViaUp, bool isCurrViaUp, bool isCurrDirY) const {
      return (*via2viaMinLenNew_)[z][((unsigned)isPrevViaUp << 2) +
                                    ((unsigned)isCurrViaUp << 1) +
                                     (unsigned)isCurrDirY];
    }
    frCoord getVia2TurnMinLen(frMIdx z, bool isPrevViaUp, bool isCurrDirY) const {
      return (*via2turnMinLen_)[z][((unsigned)isPrevViaUp << 1) + (unsigned)isCurrDirY];
    }
    void cleanup() {
      nodes_.clear();
      nodes_.shrink_to_fit();
      srcs_.clear();
      srcs_.shrink_to_fit();
      dsts_.clear();
      dsts_.shrink_to_fit();
      guides_.clear();
      guides_.shrink_to_fit();
      xCoords_.clear();
      xCoords_.shrink_to_fit();
      yCoords_.clear();
      yCoords_.shrink_to_fit();
      zHeights_.clear();
      zHeights_.shrink_to_fit();
      zDirs_.clear();
      yCoords_.shrink_to_fit();
      yCoords_.clear();
      yCoords_.shrink_to_fit();
      wavefront_.cleanup();
      wavefront_.fit();
    }
  protected:
    frDesign*     design_;
    FlexDRWorker* drWorker_;
    FlexDRGraphics*  graphics_; // owned by FlexDR

    struct Node {
      Node() {
        std::memset(this, 0, sizeof(Node));
      }
      // Byte 0
      frUInt4 hasEastEdge : 1;
      frUInt4 hasNorthEdge : 1;
      frUInt4 hasUpEdge : 1;
      frUInt4 isBlockedEast : 1;
      frUInt4 isBlockedNorth : 1;
      frUInt4 isBlockedUp : 1;
      frUInt4 unused1 : 1;
      frUInt4 unused2 : 1;
      // Byte 1
      frUInt4 hasSpecialVia : 1;
      frUInt4 overrideShapeCostVia : 1;
      frUInt4 hasGridCostEast : 1;
      frUInt4 hasGridCostNorth : 1;
      frUInt4 hasGridCostUp : 1;
      frUInt4 unused3 : 1;
      frUInt4 unused4 : 1;
      frUInt4 unused5 : 1;
      // Byte 2
      frUInt4 drcCostPlanar : 8;
      // Byte 3
      frUInt4 drcCostVia : 8;
      // Byte4
      frUInt4 markerCostPlanar : 8;
      // Byte5
      frUInt4 markerCostVia : 8;
      // Byte6
      frUInt4 shapeCostVia : 8;
      // Byte7
      frUInt4 shapeCostPlanar : 8;
    };
    static_assert(sizeof(Node) == 8);
    frVector<Node>                             nodes_;
    std::vector<bool>                          prevDirs_;
    std::vector<bool>                          srcs_;
    std::vector<bool>                          dsts_;
    std::vector<bool>                          guides_;
    frVector<frCoord>                          xCoords_;
    frVector<frCoord>                          yCoords_;
    frVector<frLayerNum>                       zCoords_;
    frVector<frCoord>                          zHeights_; // accumulated Z diff
    std::vector<bool>                          zDirs_; // is horz dir
    frUInt4                                    ggDRCCost_;
    frUInt4                                    ggMarkerCost_;
    // temporary variables
    FlexWavefront                              wavefront_;
    const std::vector<std::pair<frCoord, frCoord> >* halfViaEncArea_; // std::pair<layer1area, layer2area>
    // via2viaMinLen[z][0], last via is down, curr via is down
    // via2viaMinLen[z][1], last via is down, curr via is up
    // via2viaMinLen[z][2], last via is up, curr via is down
    // via2viaMinLen[z][3], last via is up, curr via is up
    const std::vector<std::pair<std::vector<frCoord>, std::vector<bool> > >* via2viaMinLen_;
    const std::vector<std::vector<frCoord> >* via2viaMinLenNew_;
    const std::vector<std::vector<frCoord> >* via2turnMinLen_;
    //ndr related
    frNonDefaultRule*                          ndr_;
    frBox3D*                                   dstTaperBox;   //taper box for the current dest pin in the search

    // internal getters
    frMIdx getIdx(frMIdx xIdx, frMIdx yIdx, frMIdx zIdx) const {
      auto xSize = xCoords_.size();
      auto ySize = yCoords_.size();

      frMIdx zDirModifier =
          (getZDir(zIdx)) ? (xIdx + yIdx * xSize) : (yIdx + xIdx * ySize);
      frMIdx partialCoordinates = zIdx * xSize * ySize;

      return zDirModifier + partialCoordinates;
    }

    frUInt4 addToByte(frUInt4 augend, frUInt4 summand) {
      constexpr frUInt4 limit = (1u << 8) - 1;
      return std::min(augend + summand, limit);
    }

    frUInt4 subFromByte(frUInt4 minuend, frUInt4 subtrahend) {
      return std::max(minuend - subtrahend, 0u);
    }

    // internal utility
    void correct(frMIdx &x, frMIdx &y, frMIdx &z, frDirEnum &dir) const {
      switch (dir) {
        case frDirEnum::W:
          x--;
          dir = frDirEnum::E;
          break;
        case frDirEnum::S:
          y--;
          dir = frDirEnum::N;
          break;
        case frDirEnum::D:
          z--;
          dir = frDirEnum::U;
          break;
        default:
          ;
      }
      return;
    }
    void correctU(frMIdx &x, frMIdx &y, frMIdx &z, frDirEnum &dir) const {
      switch (dir) {
        case frDirEnum::D:
          z--;
          dir = frDirEnum::U;
          break;
        default:
          ;
      }
      return;
    }
    void reverse(frMIdx &x, frMIdx &y, frMIdx &z, frDirEnum &dir) const {
      switch (dir) {
        case frDirEnum::E:
          x++;
          dir = frDirEnum::W;
          break;
        case frDirEnum::S:
          y--;
          dir = frDirEnum::N;
          break;
        case frDirEnum::W:
          x--;
          dir = frDirEnum::E;
          break;
        case frDirEnum::N:
          y++;
          dir = frDirEnum::S;
          break;
        case frDirEnum::U:
          z++;
          dir = frDirEnum::D;
          break;
        case frDirEnum::D:
          z--;
          dir = frDirEnum::U;
          break;
        default:
          ;
      }
      return;
    }
    frMIdx getLowerBoundIndex(const frVector<frCoord>& tracks, frCoord v) const;
    frMIdx getUpperBoundIndex(const frVector<frCoord>& tracks, frCoord v) const;

    void getPrevGrid(frMIdx &gridX, frMIdx &gridY, frMIdx &gridZ, const frDirEnum dir) const;
    void getNextGrid(frMIdx &gridX, frMIdx &gridY, frMIdx &gridZ, const frDirEnum dir) const;
    bool isValid(frMIdx x, frMIdx y, frMIdx z) const {
      if (x < 0 || y < 0 || z < 0 ||
          x >= (frMIdx)xCoords_.size() || y >= (frMIdx)yCoords_.size() || z >= (frMIdx)zCoords_.size()) {
        return false;
      } else {
        return true;
      }
    }
    bool isValid(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const {
      auto sol = isValid(x, y, z);
      reverse(x, y, z, dir);
      return sol && isValid(x, y, z);
    }
    // internal init utility
    void initTracks(std::map<frCoord, std::map<frLayerNum, frTrackPattern* > > &horLoc2TrackPatterns,
                    std::map<frCoord, std::map<frLayerNum, frTrackPattern* > > &vertLoc2TrackPatterns,
                    std::map<frLayerNum, frPrefRoutingDirEnum> &layerNum2PreRouteDir,
                    const frBox &bbox);
    void initGrids(const std::map<frCoord, std::map<frLayerNum, frTrackPattern* > > &xMap,
                   const std::map<frCoord, std::map<frLayerNum, frTrackPattern* > > &yMap,
                   const std::map<frLayerNum, frPrefRoutingDirEnum> &zMap, bool followGuide);
    void initEdges(const std::map<frCoord, std::map<frLayerNum, frTrackPattern* > > &xMap,
                   const std::map<frCoord, std::map<frLayerNum, frTrackPattern* > > &yMap,
                   const std::map<frLayerNum, frPrefRoutingDirEnum> &zMap,
                   const frBox &bbox, bool initDR);
    frCost getEstCost(const FlexMazeIdx &src, const FlexMazeIdx &dstMazeIdx1, const FlexMazeIdx &dstMazeIdx2, const frDirEnum &dir) const;
    frCost getNextPathCost(const FlexWavefrontGrid &currGrid, const frDirEnum &dir) const;
    frDirEnum getLastDir(const std::bitset<WAVEFRONTBITSIZE> &buffer) const;
    void traceBackPath(const FlexWavefrontGrid &currGrid, std::vector<FlexMazeIdx> &path,
                       std::vector<FlexMazeIdx> &root, FlexMazeIdx &ccMazeIdx1, FlexMazeIdx &ccMazeIdx2) const;
    void expandWavefront(FlexWavefrontGrid &currGrid, const FlexMazeIdx &dstMazeIdx1,
                         const FlexMazeIdx &dstMazeIdx2, const frPoint &centerPt);
    bool isExpandable(const FlexWavefrontGrid &currGrid, frDirEnum dir) const;
    //bool isOpposite(const frDirEnum &dir1, const frDirEnum &dir2);
    FlexMazeIdx getTailIdx(const FlexMazeIdx &currIdx, const FlexWavefrontGrid &currGrid) const;
    void expand(FlexWavefrontGrid &currGrid, const frDirEnum &dir, const FlexMazeIdx &dstMazeIdx1, const FlexMazeIdx &dstMazeIdx2,
                const frPoint &centerPt);
  };
}




#endif
