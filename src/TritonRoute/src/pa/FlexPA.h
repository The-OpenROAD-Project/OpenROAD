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

#ifndef _FR_FLEXPA_H_
#define _FR_FLEXPA_H_

#include <boost/polygon/polygon.hpp>

#include "frDesign.h"
namespace gtl = boost::polygon;

namespace odb {
class dbDatabase;
}

namespace fr {
// not default via, upperWidth, lowerWidth, not align upper, upperArea,
// lowerArea, not align lower
typedef std::tuple<bool, frCoord, frCoord, bool, frCoord, frCoord, bool>
    viaRawPriorityTuple;
class FlexPinAccessPattern;
class FlexDPNode;
class FlexPAGraphics;

class FlexPA
{
 public:
  // constructor
  FlexPA(frDesign* in, Logger* logger);
  ~FlexPA();
  // getters
  frDesign* getDesign() const { return design_; }
  frTechObject* getTech() const { return design_->getTech(); }
  // setters
  int main();
  void setDebug(frDebugSettings* settings, odb::dbDatabase* db);

 protected:
  frDesign* design_;
  Logger* logger_;
  std::unique_ptr<FlexPAGraphics> graphics_;
  std::string debugPinName_;

  int stdCellPinGenApCnt_;
  int stdCellPinValidPlanarApCnt_;
  int stdCellPinValidViaApCnt_;
  int stdCellPinNoApCnt_;
  int instTermValidViaApCnt_ = 0;
  int macroCellPinGenApCnt_;
  int macroCellPinValidPlanarApCnt_;
  int macroCellPinValidViaApCnt_;
  int macroCellPinNoApCnt_;

  std::vector<frInst*> uniqueInstances_;
  std::map<frInst*, frInst*, frBlockObjectComp> inst2unique_;
  std::map<frInst*, int, frBlockObjectComp>
      unique2paidx_;  // unique instance to pinaccess index
  std::map<frInst*, int, frBlockObjectComp> unique2Idx_;
  std::vector<std::vector<std::unique_ptr<FlexPinAccessPattern>>>
      uniqueInstPatterns_;

  int maxAccessPatternSize_;

  // helper strutures
  std::vector<std::map<frCoord, frAccessPointEnum>> trackCoords_;
  std::map<frLayerNum, std::map<int, std::map<viaRawPriorityTuple, frViaDef*>>>
      layerNum2ViaDefs_;

  // helper functions
  void getPrefTrackPatterns(std::vector<frTrackPattern*>& prefTrackPatterns);
  bool hasTrackPattern(frTrackPattern* tp, const frBox& box);
  void getViaRawPriority(frViaDef* viaDef, viaRawPriorityTuple& priority);
  bool isSkipTerm(frTerm* in);
  bool isSkipInstTerm(frInstTerm* in);

  // init
  void init();
  void initUniqueInstance();
  void initUniqueInstance_refBlock2PinLayerRange(
      std::map<frBlock*, std::tuple<frLayerNum, frLayerNum>, frBlockObjectComp>&
          refBlock2PinLayerRange);
  void initUniqueInstance_main(
      const std::map<frBlock*,
                     std::tuple<frLayerNum, frLayerNum>,
                     frBlockObjectComp>& refBlock2PinLayerRange,
      const std::vector<frTrackPattern*>& prefTrackPatterns);
  bool isNDRInst(frInst& inst);
  void initPinAccess();
  void initTrackCoords();
  void initViaRawPriority();
  // prep
  void prep();
  void prepPoint();
  void prepPoint_pin(frPin* pin, frInstTerm* instTerm = nullptr);
  void prepPoint_pin_mergePinShapes(
      std::vector<gtl::polygon_90_set_data<frCoord>>& pinShapes,
      frPin* pin,
      frInstTerm* instTerm,
      bool isShrink = false);
  // type 0 -- on-grid; 1 -- half-grid; 2 -- center; 3 -- via-enc-opt
  void prepPoint_pin_genPoints(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<frPoint, frLayerNum>>& apset,
      frPin* pin,
      frInstTerm* instTerm,
      const std::vector<gtl::polygon_90_set_data<frCoord>>& pinShapes,
      frAccessPointEnum lowerType,
      frAccessPointEnum upperType);
  void prepPoint_pin_genPoints_layerShapes(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<frPoint, frLayerNum>>& apset,
      frPin* pin,
      frInstTerm* instTerm,
      const gtl::polygon_90_set_data<frCoord>& layerShapes,
      frLayerNum layerNum,
      bool allowVia,
      frAccessPointEnum lowerType,
      frAccessPointEnum upperType);
  void prepPoint_pin_genPoints_rect(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<frPoint, frLayerNum>>& apset,
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
      std::set<std::pair<frPoint, frLayerNum>>& apset,
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
      std::set<std::pair<frPoint, frLayerNum>>& apset,
      const gtl::rectangle_data<frCoord>& maxrect,
      frCoord x,
      frCoord y,
      frLayerNum layerNum,
      bool allowPlanar,
      bool allowVia,
      frAccessPointEnum lowCost,
      frAccessPointEnum highCost);
  void prepPoint_pin_checkPoints(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      const std::vector<gtl::polygon_90_set_data<frCoord>>& pinShapes,
      frPin* pin,
      frInstTerm* instTerm);
  void prepPoint_pin_checkPoint(
      frAccessPoint* ap,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      const std::vector<gtl::polygon_90_data<frCoord>>& polys,
      frPin* pin,
      frInstTerm* instTerm);
  void prepPoint_pin_checkPoint_planar(
      frAccessPoint* ap,
      const std::vector<gtl::polygon_90_data<frCoord>>& layerPolys,
      frDirEnum dir,
      frPin* pin,
      frInstTerm* instTerm);
  bool prepPoint_pin_checkPoint_planar_ep(
      frPoint& ep,
      const std::vector<gtl::polygon_90_data<frCoord>>& layerPolys,
      const frPoint& bp,
      frLayerNum layerNum,
      frDirEnum dir,
      int stepSizeMultiplier = 2);
  void prepPoint_pin_checkPoint_via(
      frAccessPoint* ap,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      frDirEnum dir,
      frPin* pin,
      frInstTerm* instTerm);
  bool prepPoint_pin_checkPoint_via_helper(frAccessPoint* ap,
                                           frVia* via,
                                           frPin* pin,
                                           frInstTerm* instTerm);
  void prepPoint_pin_updateStat(
      const std::vector<std::unique_ptr<frAccessPoint>>& tmpAps,
      frPin* pin,
      frInstTerm* instTerm);
  bool prepPoint_pin_helper(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<frPoint, frLayerNum>>& apset,
      std::vector<gtl::polygon_90_set_data<frCoord>>& pinShapes,
      frPin* pin,
      frInstTerm* instTerm,
      frAccessPointEnum lowerType,
      frAccessPointEnum upperType);

  void prepPattern();
  void prepPattern_inst(frInst* inst, int currUniqueInstIdx);
  void genPatterns(const std::vector<std::pair<frPin*, frInstTerm*>>& pins,
                   int currUniqueInstIdx);
  void genPatterns_init(std::vector<FlexDPNode>& nodes,
                        const std::vector<std::pair<frPin*, frInstTerm*>>& pins,
                        std::set<std::vector<int>>& instAccessPatterns,
                        std::set<std::pair<int, int>>& usedAccessPoints,
                        std::set<std::pair<int, int>>& violAccessPoints,
                        int maxAccessPointSize);
  void genPatterns_reset(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frPin*, frInstTerm*>>& pins,
      int maxAccessPointSize);
  void genPatterns_perform(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frPin*, frInstTerm*>>& pins,
      std::vector<int>& vioEdges,
      const std::set<std::pair<int, int>>& usedAccessPoints,
      const std::set<std::pair<int, int>>& violAccessPoints,
      int currUniqueInstIdx,
      int maxAccessPointSize);
  int getEdgeCost(int prevNodeIdx,
                  int currNodeIdx,
                  const std::vector<FlexDPNode>& nodes,
                  const std::vector<std::pair<frPin*, frInstTerm*>>& pins,
                  std::vector<int>& vioEdges,
                  const std::set<std::pair<int, int>>& usedAccessPoints,
                  const std::set<std::pair<int, int>>& violAccessPoints,
                  int currUniqueInstIdx,
                  int maxAccessPointSize);
  bool genPatterns_commit(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frPin*, frInstTerm*>>& pins,
      bool& isValid,
      std::set<std::vector<int>>& instAccessPatterns,
      std::set<std::pair<int, int>>& usedAccessPoints,
      std::set<std::pair<int, int>>& violAccessPoints,
      int currUniqueInstIdx,
      int maxAccessPointSize);
  void genPatterns_print_debug(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frPin*, frInstTerm*>>& pins,
      int maxAccessPointSize);
  void genPatterns_print(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frPin*, frInstTerm*>>& pins,
      int maxAccessPointSize);
  int getFlatIdx(int idx1, int idx2, int idx2Dim);
  void getNestedIdx(int flatIdx, int& idx1, int& idx2, int idx2Dim);
  int getFlatEdgeIdx(int prevIdx1, int prevIdx2, int currIdx2, int idx2Dim);

  bool genPatterns_gc(frBlockObject* targetObj,
                      std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
                      std::set<frBlockObject*>* owners = nullptr);

  void getInsts(std::vector<frInst*>& insts);
  void genInstPattern(std::vector<frInst*>& insts);
  void genInstPattern_init(std::vector<FlexDPNode>& nodes,
                           const std::vector<frInst*>& insts);
  void genInstPattern_perform(std::vector<FlexDPNode>& nodes,
                              const std::vector<frInst*>& insts);
  void genInstPattern_commit(std::vector<FlexDPNode>& nodes,
                             const std::vector<frInst*>& insts);
  void genInstPattern_print(std::vector<FlexDPNode>& nodes,
                            const std::vector<frInst*>& insts);
  int getEdgeCost(int prevNodeIdx,
                  int currNodeIdx,
                  std::vector<FlexDPNode>& nodes,
                  const std::vector<frInst*>& insts);
  void revertAccessPoints();
  void addAccessPatternObj(
      frInst* inst,
      FlexPinAccessPattern* accessPattern,
      std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
      std::vector<std::unique_ptr<frVia>>& vias,
      bool isPrev);
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
      cost_ += ap->getCost();
    }
  }

 private:
  std::vector<frAccessPoint*> pattern_;
  frAccessPoint* left_;
  frAccessPoint* right_;
  int cost_;
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

#endif
