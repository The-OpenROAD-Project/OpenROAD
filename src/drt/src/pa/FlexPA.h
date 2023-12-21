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

#pragma once

#include <boost/polygon/polygon.hpp>
#include <cstdint>

#include "FlexPA_unique.h"
#include "frDesign.h"
namespace gtl = boost::polygon;

namespace odb {
class dbDatabase;
}

namespace dst {
class Distributed;
}

namespace boost::serialization {
class access;
}

namespace fr {
// not default via, upperWidth, lowerWidth, not align upper, upperArea,
// lowerArea, not align lower, via name
using ViaRawPriorityTuple
    = std::tuple<bool, frCoord, frCoord, bool, frCoord, frCoord, bool>;

class FlexPinAccessPattern;
class FlexDPNode;
class FlexPAGraphics;

class FlexPA
{
 public:
  enum PatternType
  {
    Edge,
    Commit
  };

  FlexPA(frDesign* in, Logger* logger, dst::Distributed* dist);
  ~FlexPA();

  void setDebug(frDebugSettings* settings, odb::dbDatabase* db);
  void setTargetInstances(const frCollection<odb::dbInst*>& insts);
  void setDistributed(const std::string& rhost,
                      uint16_t rport,
                      const std::string& shared_vol,
                      int cloud_sz);

  int main();

 private:
  frDesign* design_;
  Logger* logger_;
  dst::Distributed* dist_;

  std::unique_ptr<FlexPAGraphics> graphics_;
  std::string debugPinName_;

  int stdCellPinGenApCnt_ = 0;
  int stdCellPinValidPlanarApCnt_ = 0;
  int stdCellPinValidViaApCnt_ = 0;
  int stdCellPinNoApCnt_ = 0;
  int instTermValidViaApCnt_ = 0;
  int macroCellPinGenApCnt_ = 0;
  int macroCellPinValidPlanarApCnt_ = 0;
  int macroCellPinValidViaApCnt_ = 0;
  int macroCellPinNoApCnt_ = 0;
  std::vector<std::vector<std::unique_ptr<FlexPinAccessPattern>>>
      uniqueInstPatterns_;

  UniqueInsts unique_insts_;

  // helper structures
  std::vector<std::map<frCoord, frAccessPointEnum>> trackCoords_;
  std::map<frLayerNum, std::map<int, std::map<ViaRawPriorityTuple, frViaDef*>>>
      layerNum2ViaDefs_;
  frCollection<odb::dbInst*> target_insts_;

  std::string remote_host_;
  uint16_t remote_port_;
  std::string shared_vol_;
  int cloud_sz_;

  // helper functions
  frDesign* getDesign() const { return design_; }
  frTechObject* getTech() const { return design_->getTech(); }
  void setDesign(frDesign* in)
  {
    design_ = in;
    unique_insts_.setDesign(in);
  }
  void applyPatternsFile(const char* file_path);
  void getViaRawPriority(frViaDef* viaDef, ViaRawPriorityTuple& priority);
  bool isSkipInstTerm(frInstTerm* in);
  bool isDistributed() const { return !remote_host_.empty(); }

  // init
  void init();
  void initTrackCoords();
  void initViaRawPriority();
  // prep
  void prep();
  void prepPoint();
  void getViasFromMetalWidthMap(
      const Point& pt,
      const frLayerNum layerNum,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      std::vector<std::pair<int, frViaDef*>>& viaDefs);
  template <typename T>
  int prepPoint_pin(T* pin, frInstTerm* instTerm = nullptr);
  template <typename T>
  void prepPoint_pin_mergePinShapes(
      std::vector<gtl::polygon_90_set_data<frCoord>>& pinShapes,
      T* pin,
      frInstTerm* instTerm,
      bool isShrink = false);
  // type 0 -- on-grid; 1 -- half-grid; 2 -- center; 3 -- via-enc-opt
  template <typename T>
  void prepPoint_pin_genPoints(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      T* pin,
      frInstTerm* instTerm,
      const std::vector<gtl::polygon_90_set_data<frCoord>>& pinShapes,
      frAccessPointEnum lowerType,
      frAccessPointEnum upperType);
  void prepPoint_pin_genPoints_layerShapes(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      frInstTerm* instTerm,
      const gtl::polygon_90_set_data<frCoord>& layerShapes,
      frLayerNum layerNum,
      bool allowVia,
      frAccessPointEnum lowerType,
      frAccessPointEnum upperType);
  bool enclosesOnTrackPlanarAccess(const gtl::rectangle_data<frCoord>& rect,
                                   frLayerNum layerNum);
  void prepPoint_pin_genPoints_rect(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      const gtl::rectangle_data<frCoord>& rect,
      frLayerNum layerNum,
      bool allowPlanar,
      bool allowVia,
      frAccessPointEnum lowerType,
      frAccessPointEnum upperType,
      bool isMacroCellPin);
  void prepPoint_pin_genPoints_rect_genGrid(
      std::map<frCoord, frAccessPointEnum>& coords,
      const std::map<frCoord, frAccessPointEnum>& trackCoords,
      frCoord low,
      frCoord high,
      bool useNearbyGrid = false);
  void prepPoint_pin_genPoints_rect_genCenter(
      std::map<frCoord, frAccessPointEnum>& coords,
      frLayerNum layerNum,
      frCoord low,
      frCoord high);
  void prepPoint_pin_genPoints_rect_genEnc(
      std::map<frCoord, frAccessPointEnum>& coords,
      const gtl::rectangle_data<frCoord>& rect,
      frLayerNum layerNum,
      bool isCurrLayerHorz);
  void prepPoint_pin_genPoints_rect_ap(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      const gtl::rectangle_data<frCoord>& rect,
      frLayerNum layerNum,
      bool allowPlanar,
      bool allowVia,
      bool isLayer1Horz,
      const std::map<frCoord, frAccessPointEnum>& xCoords,
      const std::map<frCoord, frAccessPointEnum>& yCoords,
      frAccessPointEnum lowerType,
      frAccessPointEnum upperType);
  void prepPoint_pin_genPoints_rect_ap_helper(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      const gtl::rectangle_data<frCoord>& maxrect,
      frCoord x,
      frCoord y,
      frLayerNum layerNum,
      bool allowPlanar,
      bool allowVia,
      frAccessPointEnum lowCost,
      frAccessPointEnum highCost);
  template <typename T>
  void prepPoint_pin_checkPoints(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      const std::vector<gtl::polygon_90_set_data<frCoord>>& pinShapes,
      T* pin,
      frInstTerm* instTerm,
      const bool& isStdCellPin);
  template <typename T>
  void prepPoint_pin_checkPoint(
      frAccessPoint* ap,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      const std::vector<gtl::polygon_90_data<frCoord>>& polys,
      T* pin,
      frInstTerm* instTerm,
      bool deepSearch = false);
  template <typename T>
  void prepPoint_pin_checkPoint_planar(
      frAccessPoint* ap,
      const std::vector<gtl::polygon_90_data<frCoord>>& layerPolys,
      frDirEnum dir,
      T* pin,
      frInstTerm* instTerm);
  bool prepPoint_pin_checkPoint_planar_ep(
      Point& ep,
      const std::vector<gtl::polygon_90_data<frCoord>>& layerPolys,
      const Point& bp,
      frLayerNum layerNum,
      frDirEnum dir,
      bool isBlock);
  template <typename T>
  void prepPoint_pin_checkPoint_via(
      frAccessPoint* ap,
      const std::vector<gtl::polygon_90_data<frCoord>>& layerPolys,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      frDirEnum dir,
      T* pin,
      frInstTerm* instTerm,
      bool deepSearch = false);
  template <typename T>
  bool prepPoint_pin_checkPoint_via_helper(
      frAccessPoint* ap,
      frVia* via,
      T* pin,
      frInstTerm* instTerm,
      const std::vector<gtl::polygon_90_data<frCoord>>& layerPolys);
  template <typename T>
  bool prepPoint_pin_checkPoint_viaDir_helper(
      frAccessPoint* ap,
      frVia* via,
      T* pin,
      frInstTerm* instTerm,
      const std::vector<gtl::polygon_90_data<frCoord>>& layerPolys,
      frDirEnum dir);
  template <typename T>
  void prepPoint_pin_updateStat(
      const std::vector<std::unique_ptr<frAccessPoint>>& tmpAps,
      T* pin,
      frInstTerm* instTerm);
  template <typename T>
  bool prepPoint_pin_helper(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      std::vector<gtl::polygon_90_set_data<frCoord>>& pinShapes,
      T* pin,
      frInstTerm* instTerm,
      frAccessPointEnum lowerType,
      frAccessPointEnum upperType);

  void prepPattern();
  void prepPatternInstRows(std::vector<std::vector<frInst*>> inst_rows);
  int prepPattern_inst(frInst* inst,
                       const int currUniqueInstIdx,
                       const double xWeight);
  int genPatterns(const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
                  int currUniqueInstIdx);
  void genPatterns_init(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      std::set<std::vector<int>>& instAccessPatterns,
      std::set<std::pair<int, int>>& usedAccessPoints,
      std::set<std::pair<int, int>>& violAccessPoints,
      int maxAccessPointSize);
  void genPatterns_reset(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      int maxAccessPointSize);
  void genPatterns_perform(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      std::vector<int>& vioEdges,
      const std::set<std::pair<int, int>>& usedAccessPoints,
      const std::set<std::pair<int, int>>& violAccessPoints,
      int currUniqueInstIdx,
      int maxAccessPointSize);
  int getEdgeCost(int prevNodeIdx,
                  int currNodeIdx,
                  const std::vector<FlexDPNode>& nodes,
                  const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
                  std::vector<int>& vioEdges,
                  const std::set<std::pair<int, int>>& usedAccessPoints,
                  const std::set<std::pair<int, int>>& violAccessPoints,
                  int currUniqueInstIdx,
                  int maxAccessPointSize);
  bool genPatterns_commit(
      const std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      bool& isValid,
      std::set<std::vector<int>>& instAccessPatterns,
      std::set<std::pair<int, int>>& usedAccessPoints,
      std::set<std::pair<int, int>>& violAccessPoints,
      int currUniqueInstIdx,
      int maxAccessPointSize);
  void genPatterns_print_debug(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      int maxAccessPointSize);
  void genPatterns_print(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      int maxAccessPointSize);
  int getFlatIdx(int idx1, int idx2, int idx2Dim);
  void getNestedIdx(int flatIdx, int& idx1, int& idx2, int idx2Dim);
  int getFlatEdgeIdx(int prevIdx1, int prevIdx2, int currIdx2, int idx2Dim);

  bool genPatterns_gc(
      const std::set<frBlockObject*>& targetObjs,
      const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
      const PatternType patternType,
      std::set<frBlockObject*>* owners = nullptr);

  void getInsts(std::vector<frInst*>& insts);
  void genInstRowPattern(std::vector<frInst*>& insts);
  void genInstRowPattern_init(std::vector<FlexDPNode>& nodes,
                              const std::vector<frInst*>& insts);
  void genInstRowPattern_perform(std::vector<FlexDPNode>& nodes,
                                 const std::vector<frInst*>& insts);
  void genInstRowPattern_commit(std::vector<FlexDPNode>& nodes,
                                const std::vector<frInst*>& insts);
  void genInstRowPattern_print(std::vector<FlexDPNode>& nodes,
                               const std::vector<frInst*>& insts);
  int getEdgeCost(int prevNodeIdx,
                  int currNodeIdx,
                  const std::vector<FlexDPNode>& nodes,
                  const std::vector<frInst*>& insts);
  void revertAccessPoints();
  void addAccessPatternObj(
      frInst* inst,
      FlexPinAccessPattern* accessPattern,
      std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
      std::vector<std::unique_ptr<frVia>>& vias,
      bool isPrev);

  friend class RoutingCallBack;
};

class FlexPinAccessPattern
{
 public:
  // constructor
  FlexPinAccessPattern()
      : pattern_(),
        left_(nullptr),
        right_(nullptr),
        cost_(std::numeric_limits<int>::max())
  {
  }
  FlexPinAccessPattern(const FlexPinAccessPattern& rhs)
      : pattern_(rhs.pattern_),
        left_(rhs.left_),
        right_(rhs.right_),
        cost_(rhs.cost_)
  {
  }
  // getter
  const std::vector<frAccessPoint*>& getPattern() const { return pattern_; }
  frAccessPoint* getBoundaryAP(bool isLeft) const
  {
    return isLeft ? left_ : right_;
  }
  int getCost() const { return cost_; }
  // setter
  void addAccessPoint(frAccessPoint* in) { pattern_.push_back(in); }
  void setBoundaryAP(bool isLeft, frAccessPoint* in)
  {
    if (isLeft) {
      left_ = in;
    } else {
      right_ = in;
    }
  }
  void updateCost()
  {
    cost_ = 0;
    for (auto& ap : pattern_) {
      if (ap)
        cost_ += ap->getCost();
    }
  }

 private:
  std::vector<frAccessPoint*> pattern_;
  frAccessPoint* left_;
  frAccessPoint* right_;
  int cost_;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
  friend class boost::serialization::access;
};

// dynamic programming related
class FlexDPNode
{
 public:
  // constructor
  FlexDPNode()
      : pathCost_(std::numeric_limits<int>::max()),
        nodeCost_(std::numeric_limits<int>::max()),
        prevNodeIdx_(-1)
  {
  }

  // getters
  int getPathCost() const { return pathCost_; }
  int getNodeCost() const { return nodeCost_; }
  int getPrevNodeIdx() const { return prevNodeIdx_; }

  // setters
  void setPathCost(int in) { pathCost_ = in; }
  void setNodeCost(int in) { nodeCost_ = in; }
  void setPrevNodeIdx(int in) { prevNodeIdx_ = in; }

 private:
  int pathCost_;
  int nodeCost_;
  int prevNodeIdx_;
};
}  // namespace fr
