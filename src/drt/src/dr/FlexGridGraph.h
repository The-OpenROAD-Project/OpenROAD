// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/container/flat_map.hpp"
#include "boost/container/flat_set.hpp"
#include "db/drObj/drPin.h"
#include "db/infra/frBox.h"
#include "db/obj/frTrackPattern.h"
#include "db/tech/frConstraint.h"
#include "db/tech/frLayer.h"
#include "db/tech/frTechObject.h"
#include "dr/FlexMazeTypes.h"
#include "dr/FlexWavefront.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "global.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace drt {

using frLayerCoordTrackPatternMap = boost::container::
    flat_map<frLayerNum, boost::container::flat_map<frCoord, frTrackPattern*>>;
using frLayerDirMap
    = boost::container::flat_map<frLayerNum, odb::dbTechLayerDir>;

class FlexDRWorker;
class AbstractDRGraphics;
class FlexGridGraph
{
 public:
  // constructors
  FlexGridGraph(frTechObject* techIn,
                utl::Logger* loggerIn,
                FlexDRWorker* workerIn,
                RouterConfiguration* router_cfg)
      : tech_(techIn),
        logger_(loggerIn),
        drWorker_(workerIn),
        router_cfg_(router_cfg),
        ap_locs_(techIn->getTopLayerNum() + 1)
  {
  }
  // getters
  frTechObject* getTech() const { return tech_; }
  FlexDRWorker* getDRWorker() const { return drWorker_; }

  // unsafe access, no check
  bool isBlocked(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
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
  bool isSVia(frMIdx x, frMIdx y, frMIdx z) const
  {
    return nodes_[getIdx(x, y, z)].hasSpecialVia;
  }
  bool isSVia(frMIdx idx) const { return nodes_[idx].hasSpecialVia; }
  // unsafe access, no check
  bool hasGridCostE(frMIdx x, frMIdx y, frMIdx z) const
  {
    return nodes_[getIdx(x, y, z)].hasGridCostEast;
  }
  // unsafe access, no check
  bool hasGridCostN(frMIdx x, frMIdx y, frMIdx z) const
  {
    return nodes_[getIdx(x, y, z)].hasGridCostNorth;
  }
  // unsafe access, no check
  bool hasGridCostU(frMIdx x, frMIdx y, frMIdx z) const
  {
    return nodes_[getIdx(x, y, z)].hasGridCostUp;
  }

  void getBBox(odb::Rect& in) const
  {
    if (!xCoords_.empty() && !yCoords_.empty()) {
      in.init(
          xCoords_.front(), yCoords_.front(), xCoords_.back(), yCoords_.back());
    }
  }
  void getDim(frMIdx& xDim, frMIdx& yDim, frMIdx& zDim) const
  {
    xDim = xCoords_.size();
    yDim = yCoords_.size();
    zDim = zCoords_.size();
  }
  // unsafe access
  odb::Point& getPoint(odb::Point& in, frMIdx x, frMIdx y) const
  {
    in = {xCoords_[x], yCoords_[y]};
    return in;
  }
  // unsafe access
  frLayerNum getLayerNum(frMIdx z) const { return zCoords_[z]; }

  // unsafe access
  frLayerNum getMinLayerNum() const { return zCoords_.front(); }
  frLayerNum getMaxLayerNum() const { return zCoords_.back(); }

  bool hasMazeXIdx(frCoord in) const
  {
    return std::ranges::binary_search(xCoords_, in);
  }
  bool hasMazeYIdx(frCoord in) const
  {
    return std::ranges::binary_search(yCoords_, in);
  }
  bool hasMazeZIdx(frLayerNum in) const
  {
    return std::ranges::binary_search(zCoords_, in);
  }
  bool hasIdx(const odb::Point& p, frLayerNum lNum) const
  {
    return (hasMazeXIdx(p.x()) && hasMazeYIdx(p.y()) && hasMazeZIdx(lNum));
  }
  bool hasMazeIdx(const odb::Point& p, frLayerNum lNum) const
  {
    return (hasMazeXIdx(p.x()) && hasMazeYIdx(p.y()) && hasMazeZIdx(lNum));
  }
  frMIdx getMazeXIdx(frCoord in) const
  {
    auto it = std::ranges::lower_bound(xCoords_, in);
    return it - xCoords_.begin();
  }
  frMIdx getMazeYIdx(frCoord in) const
  {
    auto it = std::ranges::lower_bound(yCoords_, in);
    return it - yCoords_.begin();
  }
  frMIdx getMazeZIdx(frLayerNum in) const
  {
    auto it = std::ranges::lower_bound(zCoords_, in);
    return it - zCoords_.begin();
  }
  FlexMazeIdx& getMazeIdx(FlexMazeIdx& mIdx,
                          const odb::Point& p,
                          frLayerNum layerNum) const
  {
    mIdx.set(getMazeXIdx(p.x()), getMazeYIdx(p.y()), getMazeZIdx(layerNum));
    return mIdx;
  }

  enum getIdxBox_EnclosureType
  {
    uncertain,  // output box may enclose or be enclosed by box (uncertain
                // behavior). (output box == imaginary box (in frCoords) created
                // by mIdx1 and mIdx2)
    enclose,    // ensures output box encloses box
    isEnclosed  // ensures output box is enclosed by box
  };

  void getIdxBox(FlexMazeIdx& mIdx1,
                 FlexMazeIdx& mIdx2,
                 const odb::Rect& box,
                 getIdxBox_EnclosureType enclosureOption = uncertain) const
  {
    mIdx1.set(std::ranges::lower_bound(xCoords_, box.xMin()) - xCoords_.begin(),
              std::ranges::lower_bound(yCoords_, box.yMin()) - yCoords_.begin(),
              mIdx1.z());
    if (enclosureOption == 1) {
      if (xCoords_[mIdx1.x()] > box.xMin()) {
        mIdx1.setX(std::max(0, mIdx1.x() - 1));
      }
      if (yCoords_[mIdx1.y()] > box.yMin()) {
        mIdx1.setY(std::max(0, mIdx1.y() - 1));
      }
    }
    const int ux
        = std::ranges::upper_bound(xCoords_, box.xMax()) - xCoords_.begin();
    const int uy
        = std::ranges::upper_bound(yCoords_, box.yMax()) - yCoords_.begin();
    mIdx2.set(
        frMIdx(std::max(0, ux - 1)), frMIdx(std::max(0, uy - 1)), mIdx2.z());
    if (enclosureOption == 2) {
      if (xCoords_[mIdx2.x()] > box.xMax()) {
        mIdx2.setX(std::max(0, mIdx2.x() - 1));
      }
      if (yCoords_[mIdx2.y()] > box.yMax()) {
        mIdx2.setY(std::max(0, mIdx2.y() - 1));
      }
    }
  }
  frCoord getZHeight(frMIdx in) const { return zHeights_[in]; }
  odb::dbTechLayerDir getZDir(frMIdx in) const
  {
    return layerRouteDirections_[in];
  }
  int getLayerCount() { return zCoords_.size(); }
  bool hasEdge(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
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
  bool hasGridCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    bool sol = false;
    correct(x, y, z, dir);
    switch (dir) {
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
  // gets fixed shape cost in the adjacent node following dir
  frUInt4 getFixedShapeCostAdj(frMIdx x,
                               frMIdx y,
                               frMIdx z,
                               frDirEnum dir,
                               bool consider_ndr) const
  {
    frUInt4 sol = 0;
    if (dir != frDirEnum::D && dir != frDirEnum::U) {
      reverse(x, y, z, dir);
      const Node& node = nodes_[getIdx(x, y, z)];
      if (dir == frDirEnum::W || dir == frDirEnum::E) {
        if (consider_ndr) {
          sol = std::max(node.fixedShapeCostPlanarHorz,
                         node.fixedShapeCostPlanarHorzNDR);
        } else {
          sol = node.fixedShapeCostPlanarHorz;
        }
      } else {
        if (consider_ndr) {
          sol = std::max(node.fixedShapeCostPlanarVert,
                         node.fixedShapeCostPlanarVertNDR);
        } else {
          sol = node.fixedShapeCostPlanarVert;
        }
      }
    } else {
      correctU(x, y, z, dir);
      const Node& node = nodes_[getIdx(x, y, z)];
      if (isOverrideShapeCost(x, y, z, dir)) {
        sol = 0;
      } else {
        if (consider_ndr) {
          sol = std::max(node.fixedShapeCostVia, node.fixedShapeCostViaNDR);
        } else {
          sol = node.fixedShapeCostVia;
        }
      }
    }
    return (sol);
  }
  bool hasFixedShapeCostAdj(frMIdx x,
                            frMIdx y,
                            frMIdx z,
                            frDirEnum dir,
                            bool consider_ndr = false) const
  {
    return getFixedShapeCostAdj(x, y, z, dir, consider_ndr);
  }
  bool isOverrideShapeCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    if (dir != frDirEnum::D && dir != frDirEnum::U) {
      return false;
    }
    correctU(x, y, z, dir);
    auto idx = getIdx(x, y, z);
    return nodes_[idx].overrideShapeCostVia;
  }
  // gets route shape cost in the adjacent node following dir
  frUInt4 getRouteShapeCostAdj(frMIdx x,
                               frMIdx y,
                               frMIdx z,
                               frDirEnum dir,
                               bool consider_ndr) const
  {
    frUInt4 sol = 0;
    if (dir != frDirEnum::D && dir != frDirEnum::U) {
      reverse(x, y, z, dir);
      auto idx = getIdx(x, y, z);
      if (consider_ndr) {
        sol = std::max(nodes_[idx].routeShapeCostPlanar,
                       nodes_[idx].routeShapeCostPlanarNDR);
      } else {
        sol = nodes_[idx].routeShapeCostPlanar;
      }
    } else {
      correctU(x, y, z, dir);
      auto idx = getIdx(x, y, z);
      if (consider_ndr) {
        sol = std::max(nodes_[idx].routeShapeCostVia,
                       nodes_[idx].routeShapeCostViaNDR);
      } else {
        sol = nodes_[idx].routeShapeCostVia;
      }
    }
    return (sol);
  }
  bool hasRouteShapeCostAdj(frMIdx x,
                            frMIdx y,
                            frMIdx z,
                            frDirEnum dir,
                            bool consider_ndr = false) const
  {
    return getRouteShapeCostAdj(x, y, z, dir, consider_ndr);
  }
  // gets marker cost in the adjacent node following dir
  frUInt4 getMarkerCostAdj(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
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
  bool hasMarkerCostAdj(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    return getMarkerCostAdj(x, y, z, dir);
  }
  frCoord xCoord(frMIdx x) const { return xCoords_[x]; }
  frCoord yCoord(frMIdx y) const { return yCoords_[y]; }
  // unsafe access
  frCoord getEdgeLength(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    frCoord sol = 0;
    correct(x, y, z, dir);
    // if (isValid(x, y, z, dir)) {
    switch (dir) {
      case frDirEnum::E:
        sol = xCoords_[x + 1] - xCoords_[x];
        break;
      case frDirEnum::N:
        sol = yCoords_[y + 1] - yCoords_[y];
        break;
      case frDirEnum::U:
        sol = zHeights_[z + 1] - zHeights_[z];
        break;
      default:;
    }
    //}
    return sol;
  }
  bool isEdgeInBox(frMIdx x,
                   frMIdx y,
                   frMIdx z,
                   frDirEnum dir,
                   const odb::Rect& box,
                   bool initDR) const
  {
    bool sol = false;
    correct(x, y, z, dir);
    if (isValid(x, y, z, dir)) {
      auto x1 = x;
      auto y1 = y;
      auto z1 = z;
      reverse(x1, y1, z1, dir);
      odb::Point pt, pt1;
      getPoint(pt, x, y);
      getPoint(pt1, x1, y1);
      sol = box.intersects(pt) && box.intersects(pt1);
    } else {
      sol = false;
    }
    return sol;
  }
  // setters
  void setLogger(utl::Logger* loggerIn) { logger_ = loggerIn; }
  bool addEdge(frMIdx x,
               frMIdx y,
               frMIdx z,
               frDirEnum dir,
               const odb::Rect& box,
               bool initDR)
  {
    bool sol = false;
    if (!isEdgeInBox(x, y, z, dir, box, initDR)) {
      sol = false;
    } else {
      correct(x, y, z, dir);
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
          default:;
        }
      } else {
        // std::cout <<"not valid edge";
      }
    }
    return sol;
  }
  bool removeEdge(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
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
        default:;
      }
    }
    return sol;
  }
  void setBlocked(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
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
        default:;
      }
    }
  }
  void resetBlocked(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
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
        default:;
      }
    }
  }
  void addRouteShapeCostPlanar(frMIdx idx, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.routeShapeCostPlanarNDR = addToByte(node.routeShapeCostPlanarNDR, 1);
    } else {
      node.routeShapeCostPlanar = addToByte(node.routeShapeCostPlanar, 1);
    }
  }
  void addRouteShapeCostVia(frMIdx idx, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.routeShapeCostViaNDR = addToByte(node.routeShapeCostViaNDR, 1);
    } else {
      node.routeShapeCostVia = addToByte(node.routeShapeCostVia, 1);
    }
  }
  void subRouteShapeCostPlanar(frMIdx idx, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.routeShapeCostPlanarNDR
          = subFromByte(node.routeShapeCostPlanarNDR, 1);
    } else {
      node.routeShapeCostPlanar = subFromByte(node.routeShapeCostPlanar, 1);
    }
  }
  void subRouteShapeCostVia(frMIdx idx, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.routeShapeCostViaNDR = subFromByte(node.routeShapeCostViaNDR, 1);
    } else {
      node.routeShapeCostVia = subFromByte(node.routeShapeCostVia, 1);
    }
  }
  void resetRouteShapeCostVia(frMIdx idx, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.routeShapeCostViaNDR = 0;
    } else {
      node.routeShapeCostVia = 0;
    }
  }
  void addMarkerCostPlanar(frMIdx x, frMIdx y, frMIdx z)
  {
    auto& node = nodes_[getIdx(x, y, z)];
    node.markerCostPlanar = addToByte(node.markerCostPlanar, 10);
  }
  void addMarkerCostVia(frMIdx x, frMIdx y, frMIdx z)
  {
    auto& node = nodes_[getIdx(x, y, z)];
    node.markerCostVia = addToByte(node.markerCostVia, 10);
  }
  void addMarkerCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
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
        default:;
      }
    }
  }
  bool decayMarkerCostPlanar(frMIdx x, frMIdx y, frMIdx z, float d)
  {
    auto idx = getIdx(x, y, z);
    Node& node = nodes_[idx];
    int currCost = node.markerCostPlanar;
    currCost *= d;
    currCost = std::max(0, currCost);
    node.markerCostPlanar = currCost;
    return (currCost == 0);
  }
  bool decayMarkerCostVia(frMIdx x, frMIdx y, frMIdx z, float d)
  {
    auto idx = getIdx(x, y, z);
    Node& node = nodes_[idx];
    int currCost = node.markerCostVia;
    currCost *= d;
    currCost = std::max(0, currCost);
    node.markerCostVia = currCost;
    return (currCost == 0);
  }
  bool decayMarkerCostPlanar(frMIdx x, frMIdx y, frMIdx z)
  {
    auto idx = getIdx(x, y, z);
    Node& node = nodes_[idx];
    int currCost = node.markerCostPlanar;
    currCost--;
    currCost = std::max(0, currCost);
    node.markerCostPlanar = currCost;
    return (currCost == 0);
  }
  bool decayMarkerCostVia(frMIdx x, frMIdx y, frMIdx z)
  {
    auto idx = getIdx(x, y, z);
    Node& node = nodes_[idx];
    int currCost = node.markerCostVia;
    currCost--;
    currCost = std::max(0, currCost);
    node.markerCostVia = currCost;
    return (currCost == 0);
  }
  bool decayMarkerCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir, float d)
  {
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
          break;
        case frDirEnum::N:
          currCost = node.markerCostPlanar;
          currCost *= d;
          currCost = std::max(0, currCost);
          node.markerCostPlanar = currCost;
          break;
        case frDirEnum::U:
          currCost = node.markerCostVia;
          currCost *= d;
          currCost = std::max(0, currCost);
          node.markerCostVia = currCost;
          break;
        default:;
      }
    }
    return (currCost == 0);
  }
  void addFixedShapeCostPlanar(frMIdx idx, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.fixedShapeCostPlanarHorzNDR
          = addToByte(node.fixedShapeCostPlanarHorzNDR, 1);
      node.fixedShapeCostPlanarVertNDR
          = addToByte(node.fixedShapeCostPlanarVertNDR, 1);
    } else {
      node.fixedShapeCostPlanarHorz
          = addToByte(node.fixedShapeCostPlanarHorz, 1);
      node.fixedShapeCostPlanarVert
          = addToByte(node.fixedShapeCostPlanarVert, 1);
    }
  }
  void setFixedShapeCostPlanarVert(frMIdx idx, frUInt4 c, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.fixedShapeCostPlanarVertNDR = c;
    } else {
      node.fixedShapeCostPlanarVert = c;
    }
  }
  void setFixedShapeCostPlanarHorz(frMIdx idx, frUInt4 c, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.fixedShapeCostPlanarHorzNDR = c;
    } else {
      node.fixedShapeCostPlanarHorz = c;
    }
  }
  void addFixedShapeCostVia(frMIdx idx, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.fixedShapeCostViaNDR = addToByte(node.fixedShapeCostViaNDR, 1);
    } else {
      node.fixedShapeCostVia = addToByte(node.fixedShapeCostVia, 1);
    }
  }
  void setFixedShapeCostVia(frMIdx idx, frUInt4 c, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.fixedShapeCostViaNDR = c;
    } else {
      node.fixedShapeCostVia = c;
    }
  }
  void subFixedShapeCostPlanar(frMIdx idx, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.fixedShapeCostPlanarHorzNDR
          = subFromByte(node.fixedShapeCostPlanarHorzNDR, 1);
      node.fixedShapeCostPlanarVertNDR
          = subFromByte(node.fixedShapeCostPlanarVertNDR, 1);
    } else {
      node.fixedShapeCostPlanarHorz
          = subFromByte(node.fixedShapeCostPlanarHorz, 1);
      node.fixedShapeCostPlanarVert
          = subFromByte(node.fixedShapeCostPlanarVert, 1);
    }
  }
  void subFixedShapeCostVia(frMIdx idx, bool ndr = false)
  {
    auto& node = nodes_[idx];
    if (ndr) {
      node.fixedShapeCostViaNDR = subFromByte(node.fixedShapeCostViaNDR, 1);
    } else {
      node.fixedShapeCostVia = subFromByte(node.fixedShapeCostVia, 1);
    }
  }

  // unsafe access, no idx check
  void setSrc(frMIdx x, frMIdx y, frMIdx z) { srcs_[getIdx(x, y, z)] = true; }
  void setSrc(const FlexMazeIdx& mi)
  {
    srcs_[getIdx(mi.x(), mi.y(), mi.z())] = true;
  }
  // unsafe access, no idx check
  void setDst(frMIdx x, frMIdx y, frMIdx z) { dsts_[getIdx(x, y, z)] = true; }
  void setDst(const FlexMazeIdx& mi)
  {
    dsts_[getIdx(mi.x(), mi.y(), mi.z())] = true;
  }
  // unsafe access
  void setSVia(frMIdx x, frMIdx y, frMIdx z)
  {
    nodes_[getIdx(x, y, z)].hasSpecialVia = true;
  }
  void setOverrideShapeCostVia(frMIdx x, frMIdx y, frMIdx z)
  {
    nodes_[getIdx(x, y, z)].overrideShapeCostVia = true;
  }
  void resetOverrideShapeCostVia(frMIdx x, frMIdx y, frMIdx z)
  {
    nodes_[getIdx(x, y, z)].overrideShapeCostVia = false;
  }
  void setGridCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
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
        default:;
      }
    }
  }
  void setGridCostE(frMIdx x, frMIdx y, frMIdx z)
  {
    nodes_[getIdx(x, y, z)].hasGridCostEast = true;
  }
  void setGridCostN(frMIdx x, frMIdx y, frMIdx z)
  {
    nodes_[getIdx(x, y, z)].hasGridCostNorth = true;
  }
  void setGridCostU(frMIdx x, frMIdx y, frMIdx z)
  {
    nodes_[getIdx(x, y, z)].hasGridCostUp = true;
  }
  // unsafe access, no idx check
  void resetSrc(frMIdx x, frMIdx y, frMIdx z)
  {
    srcs_[getIdx(x, y, z)] = false;
  }
  void resetSrc(const FlexMazeIdx& mi)
  {
    srcs_[getIdx(mi.x(), mi.y(), mi.z())] = false;
  }
  // unsafe access, no idx check
  void resetDst(frMIdx x, frMIdx y, frMIdx z)
  {
    dsts_[getIdx(x, y, z)] = false;
  }
  void resetDst(const FlexMazeIdx& mi)
  {
    dsts_[getIdx(mi.x(), mi.y(), mi.z())] = false;
  }
  void resetGridCost(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
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
        default:;
      }
    }
  }

  bool hasGuide(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    reverse(x, y, z, dir);
    auto idx = getIdx(x, y, z);
    return guides_[idx];
  }
  // must be safe access because idx1 and idx2 may be invalid
  void setGuide(frMIdx x1, frMIdx y1, frMIdx x2, frMIdx y2, frMIdx z)
  {
    if (x2 < x1 || y2 < y1) {
      return;
    }
    switch (getZDir(z).getValue()) {
      case odb::dbTechLayerDir::HORIZONTAL:
        for (int i = y1; i <= y2; i++) {
          auto idx1 = getIdx(x1, i, z);
          auto idx2 = getIdx(x2, i, z);
          std::fill(guides_.begin() + idx1, guides_.begin() + idx2 + 1, 1);
        }
        break;
      case odb::dbTechLayerDir::VERTICAL:
        for (int i = x1; i <= x2; i++) {
          auto idx1 = getIdx(i, y1, z);
          auto idx2 = getIdx(i, y2, z);
          std::fill(guides_.begin() + idx1, guides_.begin() + idx2 + 1, 1);
        }
        break;
      case odb::dbTechLayerDir::NONE:
        std::cout << "Error: Invalid preferred direction on layer " << z << ".";
        break;
    }
  }
  void resetGuide(frMIdx x1, frMIdx y1, frMIdx x2, frMIdx y2, frMIdx z)
  {
    if (x2 < x1 || y2 < y1) {
      return;
    }
    switch (getZDir(z).getValue()) {
      case odb::dbTechLayerDir::HORIZONTAL:
        for (int i = y1; i <= y2; i++) {
          auto idx1 = getIdx(x1, i, z);
          auto idx2 = getIdx(x2, i, z);
          std::fill(guides_.begin() + idx1, guides_.begin() + idx2 + 1, 0);
        }
        break;
      case odb::dbTechLayerDir::VERTICAL:
        for (int i = x1; i <= x2; i++) {
          auto idx1 = getIdx(i, y1, z);
          auto idx2 = getIdx(i, y2, z);
          std::fill(guides_.begin() + idx1, guides_.begin() + idx2 + 1, 0);
        }
        break;
      case odb::dbTechLayerDir::NONE:
        std::cout << "Error: Invalid preferred direction on layer " << z << ".";
        break;
    }
  }
  void setGraphics(AbstractDRGraphics* g) { graphics_ = g; }

  void setNDR(frNonDefaultRule* ndr) { ndr_ = ndr; }

  void setDstTaperBox(frBox3D* t) { dstTaperBox_ = t; }

  frCost getCosts(frMIdx gridX,
                  frMIdx gridY,
                  frMIdx gridZ,
                  frDirEnum dir,
                  frLayer* layer,
                  bool considerNDR,
                  bool route_with_jumpers) const;
  bool useNDRCosts(const FlexWavefrontGrid& p) const;

  frNonDefaultRule* getNDR() const { return ndr_; }
  const frBox3D* getDstTaperBox() const { return dstTaperBox_; }
  // functions
  void init(const frDesign* design,
            const odb::Rect& routeBBox,
            const odb::Rect& extBBox,
            frLayerCoordTrackPatternMap& xMap,
            frLayerCoordTrackPatternMap& yMap,
            bool initDR,
            bool followGuide);
  void print() const;
  void resetStatus();
  void resetPrevNodeDir();
  void resetSrc();
  void resetDst();
  bool search(std::vector<FlexMazeIdx>& connComps,
              drPin* nextPin,
              std::vector<FlexMazeIdx>& path,
              FlexMazeIdx& ccMazeIdx1,
              FlexMazeIdx& ccMazeIdx2,
              const odb::Point& centerPt,
              std::map<FlexMazeIdx, frBox3D*>& mazeIdx2TaperBox,
              bool route_with_jumpers);
  void setCost(frUInt4 drcCostIn,
               frUInt4 markerCostIn,
               frUInt4 FixedShapeCostIn)
  {
    ggDRCCost_ = drcCostIn;
    ggMarkerCost_ = markerCostIn;
    ggFixedShapeCost_ = FixedShapeCostIn;
  }
  frCoord getHalfViaEncArea(frMIdx z, bool isLayer1) const
  {
    return (isLayer1 ? (*halfViaEncArea_)[z].first
                     : (*halfViaEncArea_)[z].second);
  }
  int nTracksX() { return xCoords_.size(); }
  int nTracksY() { return yCoords_.size(); }
  void cleanup()
  {
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
    layerRouteDirections_.clear();
    yCoords_.shrink_to_fit();
    yCoords_.clear();
    yCoords_.shrink_to_fit();
    wavefront_.cleanup();
    wavefront_.fit();
  }

  void printNode(frMIdx x, frMIdx y, frMIdx z)
  {
    Node& n = nodes_[getIdx(x, y, z)];
    std::cout << "\nNode ( " << x << " " << y << " " << z << " ) (idx) / "
              << " ( " << xCoords_[x] << " " << yCoords_[y] << " ) (coords)\n";
    std::cout << "hasEastEdge " << n.hasEastEdge << "\n";
    std::cout << "hasNorthEdge " << n.hasNorthEdge << "\n";
    std::cout << "hasUpEdge " << n.hasUpEdge << "\n";
    std::cout << "isBlockedEast " << n.isBlockedEast << "\n";
    std::cout << "isBlockedNorth " << n.isBlockedNorth << "\n";
    std::cout << "isBlockedUp " << n.isBlockedUp << "\n";
    std::cout << "hasSpecialVia " << n.hasSpecialVia << "\n";
    std::cout << "overrideShapeCostVia " << n.overrideShapeCostVia << "\n";
    std::cout << "hasGridCostEast " << n.hasGridCostEast << "\n";
    std::cout << "hasGridCostNorth " << n.hasGridCostNorth << "\n";
    std::cout << "hasGridCostUp " << n.hasGridCostUp << "\n";
    std::cout << "routeShapeCostPlanar " << n.routeShapeCostPlanar << "\n";
    std::cout << "routeShapeCostVia " << n.routeShapeCostVia << "\n";
    std::cout << "markerCostPlanar " << n.markerCostPlanar << "\n";
    std::cout << "markerCostVia " << n.markerCostVia << "\n";
    std::cout << "fixedShapeCostVia " << n.fixedShapeCostVia << "\n";
    std::cout << "fixedShapeCostPlanarHorz " << n.fixedShapeCostPlanarHorz
              << "\n";
    std::cout << "fixedShapeCostPlanarVert " << n.fixedShapeCostPlanarVert
              << "\n";
  }

  void addAccessPointLocation(frLayerNum layer_num,
                              frCoord x_coord,
                              frCoord y_coord);

  bool isAccessPointLocation(frLayerNum layer_num,
                             frCoord x_coord,
                             frCoord y_coord) const;

 private:
  frTechObject* tech_ = nullptr;
  utl::Logger* logger_ = nullptr;
  FlexDRWorker* drWorker_ = nullptr;
  RouterConfiguration* router_cfg_ = nullptr;
  AbstractDRGraphics* graphics_ = nullptr;  // owned by FlexDR

#ifdef DEBUG_DRT_UNDERFLOW
  static constexpr int cost_bits = 16;
#else
  static constexpr int cost_bits = 8;
#endif

  struct Node
  {
    Node() { std::memset(this, 0, sizeof(Node)); }
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
    frUInt4 routeShapeCostPlanar : cost_bits;
    // Byte 3
    frUInt4 routeShapeCostVia : cost_bits;
    // Byte4
    frUInt4 markerCostPlanar : cost_bits;
    // Byte5
    frUInt4 markerCostVia : cost_bits;
    // Byte6
    frUInt4 fixedShapeCostVia : cost_bits;
    // Byte7
    frUInt4 fixedShapeCostPlanarHorz : cost_bits;
    // Byte8
    frUInt4 fixedShapeCostPlanarVert : cost_bits;
    // Byte9
    frUInt4 routeShapeCostPlanarNDR : cost_bits;
    // Byte10
    frUInt4 routeShapeCostViaNDR : cost_bits;
    // Byte 11
    frUInt4 fixedShapeCostViaNDR : cost_bits;
    // Byte 12
    frUInt4 fixedShapeCostPlanarHorzNDR : cost_bits;
    // Byte 13
    frUInt4 fixedShapeCostPlanarVertNDR : cost_bits;
  };
#ifndef DEBUG_DRT_UNDERFLOW
  static_assert(sizeof(Node) == 16);
#endif
  frVector<Node> nodes_;
  std::vector<bool> prevDirs_;
  std::vector<bool> srcs_;
  std::vector<bool> dsts_;
  std::vector<bool> guides_;
  frVector<frCoord> xCoords_;
  frVector<frCoord> yCoords_;
  frVector<frLayerNum> zCoords_;
  frVector<frCoord> zHeights_;  // accumulated Z diff
  std::vector<odb::dbTechLayerDir> layerRouteDirections_;
  odb::Rect dieBox_;
  frUInt4 ggDRCCost_ = 0;
  frUInt4 ggMarkerCost_ = 0;
  frUInt4 ggFixedShapeCost_ = 0;
  // temporary variables
  FlexWavefront wavefront_;
  const std::vector<std::pair<frCoord, frCoord>>* halfViaEncArea_
      = nullptr;  // std::pair<layer1area, layer2area>
  // ndr related
  frNonDefaultRule* ndr_ = nullptr;
  const frBox3D* dstTaperBox_
      = nullptr;  // taper box for the current dest pin in the search

  // locations of access points. The vector is indexed by layer number.
  frVector<std::set<odb::Point>> ap_locs_;
  std::ofstream dump_file_;
  bool debug_{false};
  frUInt4 curr_id_{1};

  void printExpansion(const FlexWavefrontGrid& currGrid,
                      const std::string& keyword);
  // unsafe access, no idx check
  void setPrevAstarNodeDir(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir)
  {
    auto baseIdx = 3 * getIdx(x, y, z);
    prevDirs_[baseIdx] = ((uint16_t) dir >> 2) & 1;
    prevDirs_[baseIdx + 1] = ((uint16_t) dir >> 1) & 1;
    prevDirs_[baseIdx + 2] = ((uint16_t) dir) & 1;
  }

  // unsafe access, no check
  frDirEnum getPrevAstarNodeDir(const FlexMazeIdx& idx) const
  {
    auto baseIdx = 3 * getIdx(idx.x(), idx.y(), idx.z());
    return (frDirEnum) (((uint16_t) (prevDirs_[baseIdx]) << 2)
                        + ((uint16_t) (prevDirs_[baseIdx + 1]) << 1)
                        + ((uint16_t) (prevDirs_[baseIdx + 2]) << 0));
  }

  // unsafe access, no check
  bool isSrc(frMIdx x, frMIdx y, frMIdx z) const
  {
    return srcs_[getIdx(x, y, z)];
  }
  // unsafe access, no check
  bool isDst(frMIdx x, frMIdx y, frMIdx z) const
  {
    return dsts_[getIdx(x, y, z)];
  }
  bool isDst(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    getNextGrid(x, y, z, dir);
    bool b = dsts_[getIdx(x, y, z)];
    getPrevGrid(x, y, z, dir);
    return b;
  }

  // internal getters
  frMIdx getIdx(frMIdx xIdx, frMIdx yIdx, frMIdx zIdx) const
  {
    auto xSize = xCoords_.size();
    auto ySize = yCoords_.size();

    frMIdx zDirModifier = (getZDir(zIdx) == odb::dbTechLayerDir::HORIZONTAL)
                              ? (xIdx + yIdx * xSize)
                              : (yIdx + xIdx * ySize);
    frMIdx partialCoordinates = zIdx * xSize * ySize;

    return zDirModifier + partialCoordinates;
  }

  frUInt4 addToByte(frUInt4 augend, frUInt4 summand)
  {
    frUInt4 result = augend + summand;
    constexpr frUInt4 limit = (1u << cost_bits) - 1;
#ifdef DEBUG_DRT_UNDERFLOW
    if (result > limit) {
      logger_->error(utl::DRT, 550, "addToByte overflow");
    }
#else
    result = std::min(result, limit);
#endif
    return result;
  }

  frUInt4 subFromByte(frUInt4 minuend, frUInt4 subtrahend)
  {
#ifdef DEBUG_DRT_UNDERFLOW
    if (subtrahend > minuend) {
      logger_->error(utl::DRT, 551, "subFromByte underflow");
    }
#endif
    return std::max((int) (minuend - subtrahend), 0);
  }

  // internal utility
  void correct(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir) const
  {
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
      default:;
    }
  }
  void correctU(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir) const
  {
    switch (dir) {
      case frDirEnum::D:
        z--;
        dir = frDirEnum::U;
        break;
      default:;
    }
  }
  void reverse(frMIdx& x, frMIdx& y, frMIdx& z, frDirEnum& dir) const
  {
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
      default:;
    }
  }
  frMIdx getLowerBoundIndex(const frVector<frCoord>& tracks, frCoord v) const;
  frMIdx getUpperBoundIndex(const frVector<frCoord>& tracks, frCoord v) const;

  void getPrevGrid(frMIdx& gridX,
                   frMIdx& gridY,
                   frMIdx& gridZ,
                   frDirEnum dir) const;
  void getNextGrid(frMIdx& gridX,
                   frMIdx& gridY,
                   frMIdx& gridZ,
                   frDirEnum dir) const;
  bool isValid(frMIdx x, frMIdx y, frMIdx z) const
  {
    if (x < 0 || y < 0 || z < 0 || x >= (frMIdx) xCoords_.size()
        || y >= (frMIdx) yCoords_.size() || z >= (frMIdx) zCoords_.size()) {
      return false;
    }
    return true;
  }
  bool isValid(frMIdx x, frMIdx y, frMIdx z, frDirEnum dir) const
  {
    auto sol = isValid(x, y, z);
    reverse(x, y, z, dir);
    return sol && isValid(x, y, z);
  }
  // internal init utility
  void initTracks(const frDesign* design,
                  frLayerCoordTrackPatternMap& horLoc2TrackPatterns,
                  frLayerCoordTrackPatternMap& vertLoc2TrackPatterns,
                  frLayerDirMap& layerNum2PreRouteDir,
                  const odb::Rect& bbox);
  void initGrids(const frLayerCoordTrackPatternMap& xMap,
                 const frLayerCoordTrackPatternMap& yMap,
                 const frLayerDirMap& zMap,
                 bool followGuide);
  void initEdges(const frDesign* design,
                 frLayerCoordTrackPatternMap& xMap,
                 frLayerCoordTrackPatternMap& yMap,
                 const frLayerDirMap& zMap,
                 const odb::Rect& bbox,
                 bool initDR);
  frCost getEstCost(const FlexMazeIdx& src,
                    const FlexMazeIdx& dstMazeIdx1,
                    const FlexMazeIdx& dstMazeIdx2,
                    const frDirEnum& dir) const;
  frCost getNextPathCost(const FlexWavefrontGrid& currGrid,
                         const frDirEnum& dir,
                         bool route_with_jumpers) const;
  frDirEnum getLastDir(const std::bitset<WAVEFRONTBITSIZE>& buffer) const;
  void traceBackPath(const FlexWavefrontGrid& currGrid,
                     std::vector<FlexMazeIdx>& path,
                     std::vector<FlexMazeIdx>& root,
                     FlexMazeIdx& ccMazeIdx1,
                     FlexMazeIdx& ccMazeIdx2) const;
  void expandWavefront(FlexWavefrontGrid& currGrid,
                       const FlexMazeIdx& dstMazeIdx1,
                       const FlexMazeIdx& dstMazeIdx2,
                       const odb::Point& centerPt,
                       bool route_with_jumpers);
  bool isExpandable(const FlexWavefrontGrid& currGrid, frDirEnum dir) const;
  FlexMazeIdx getTailIdx(const FlexMazeIdx& currIdx,
                         const FlexWavefrontGrid& currGrid) const;
  void expand(FlexWavefrontGrid& currGrid,
              const frDirEnum& dir,
              const FlexMazeIdx& dstMazeIdx1,
              const FlexMazeIdx& dstMazeIdx2,
              const odb::Point& centerPt,
              bool route_with_jumpers);
  bool hasAlignedUpDefTrack(
      frLayerNum layerNum,
      const std::map<frLayerNum, frTrackPattern*>& xSubMap,
      const std::map<frLayerNum, frTrackPattern*>& ySubMap) const;

 private:
  bool outOfDieVia(frMIdx x, frMIdx y, frMIdx z, const odb::Rect& dieBox);
  bool hasOutOfDieViol(frMIdx x, frMIdx y, frMIdx z);
  bool isWorkerBorder(frMIdx v, bool isVert);

  friend class FlexDRWorker;
};

}  // namespace drt
