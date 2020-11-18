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

#ifndef _FR_FLEXPA_H_
#define _FR_FLEXPA_H_

#include "frDesign.h"
#include <boost/polygon/polygon.hpp>
namespace gtl = boost::polygon;

namespace fr {
  // not default via, upperWidth, lowerWidth, not align upper, upperArea, lowerArea, not align lower
  typedef std::tuple<bool, frCoord, frCoord, bool, frCoord, frCoord, bool> viaRawPriorityTuple;
  class FlexPinAccessPattern;
  class FlexDPNode;

  class FlexPA {
  public:
    // constructor
    FlexPA(frDesign* in): design(in), stdCellPinGenApCnt(0), stdCellPinValidPlanarApCnt(0), stdCellPinValidViaApCnt(0), stdCellPinNoApCnt(0),
                          macroCellPinGenApCnt(0), macroCellPinValidPlanarApCnt(0), macroCellPinValidViaApCnt(0), macroCellPinNoApCnt(0), maxAccessPatternSize(0) {}
    // getters
    frDesign* getDesign() const {
      return design;
    }
    // setters
    int main();
  protected:
    frDesign* design;

    int stdCellPinGenApCnt;
    int stdCellPinValidPlanarApCnt;
    int stdCellPinValidViaApCnt;
    int stdCellPinNoApCnt;
    int instTermValidViaApCnt = 0;
    int macroCellPinGenApCnt;
    int macroCellPinValidPlanarApCnt;
    int macroCellPinValidViaApCnt;
    int macroCellPinNoApCnt;

    std::vector<frInst*>                          uniqueInstances;
    std::map<frInst*, frInst*, frBlockObjectComp> inst2unique;
    std::map<frInst*, int,     frBlockObjectComp> unique2paidx; //unique instance to pinaccess index;
    std::map<frInst*, int,     frBlockObjectComp> unique2Idx;
    std::vector<std::vector<std::unique_ptr<FlexPinAccessPattern> > > uniqueInstPatterns;

    int maxAccessPatternSize;

    // helper strutures
    std::vector<std::map<frCoord, int> > trackCoords; // 0 -- on grid; 1 -- half-grid; 2 -- center; 3 -- 1/4 grid
    std::map<frLayerNum, std::map<int, std::map<viaRawPriorityTuple, frViaDef*> > > layerNum2ViaDefs;

    // helper functions
    void getPrefTrackPatterns(std::vector<frTrackPattern*> &prefTrackPatterns);
    bool hasTrackPattern(frTrackPattern* tp, const frBox &box);
    void getViaRawPriority(frViaDef* viaDef, viaRawPriorityTuple &priority);
    bool isSkipTerm(frTerm *in);
    bool isSkipInstTerm(frInstTerm *in);


    // init
    void init();
    void initUniqueInstance();
    void initUniqueInstance_refBlock2PinLayerRange(std::map<frBlock*, 
                                                            std::tuple<frLayerNum, frLayerNum>, 
                                                            frBlockObjectComp> &refBlock2PinLayerRange);
    void initUniqueInstance_main(const std::map<frBlock*, std::tuple<frLayerNum, frLayerNum>, frBlockObjectComp> &refBlock2PinLayerRange,
                                 const std::vector<frTrackPattern*> &prefTrackPatterns);
    void initPinAccess();
    void initTrackCoords();
    void initViaRawPriority();
    // prep
    void prep();
    void prepPoint();
    void prepPoint_pin(frPin *pin, frInstTerm* instTerm = nullptr);
    void prepPoint_pin_mergePinShapes(std::vector<gtl::polygon_90_set_data<frCoord> > &pinShapes, frPin* pin, frInstTerm* instTerm, bool isShrink = false);
    // type 0 -- on-grid; 1 -- half-grid; 2 -- center; 3 -- via-enc-opt
    void prepPoint_pin_genPoints(std::vector<std::unique_ptr<frAccessPoint> > &aps, std::set<std::pair<frPoint, frLayerNum> > &apset, frPin* pin, 
                                 frInstTerm* instTerm, const std::vector<gtl::polygon_90_set_data<frCoord> > &pinShapes, int lowerType, int upperType);
    void prepPoint_pin_genPoints_layerShapes(std::vector<std::unique_ptr<frAccessPoint> > &aps, std::set<std::pair<frPoint, frLayerNum> > &apset,
                                             frPin* pin, frInstTerm* instTerm, const gtl::polygon_90_set_data<frCoord> &layerShapes,
                                             frLayerNum layerNum, bool allowVia, int lowerType, int upperType);
    void prepPoint_pin_genPoints_rect(std::vector<std::unique_ptr<frAccessPoint> > &aps, std::set<std::pair<frPoint, frLayerNum> > &apset,
                                      const gtl::rectangle_data<frCoord> &rect,
                                      frLayerNum layerNum, bool allowPlanar, bool allowVia, int lowerType, int upperType, bool isMacroCellPin);
    void prepPoint_pin_genPoints_rect_genGrid(std::map<frCoord, int> &coords, const std::map<frCoord, int> &trackCoords, frCoord low, frCoord high);
    void prepPoint_pin_genPoints_rect_genCenter(std::map<frCoord, int> &coords, frLayerNum layerNum, frCoord low, frCoord high);
    void prepPoint_pin_genPoints_rect_genEnc(std::map<frCoord, int> &coords, const gtl::rectangle_data<frCoord> &rect, 
                                             frLayerNum layerNum, bool isCurrLayerHorz);
    void prepPoint_pin_genPoints_rect_ap(std::vector<std::unique_ptr<frAccessPoint> > &aps, std::set<std::pair<frPoint, frLayerNum> > &apset, 
                                         const gtl::rectangle_data<frCoord> &rect, 
                                         frLayerNum layerNum, bool allowPlanar, bool allowVia, bool isLayer1Horz, 
                                         const std::map<frCoord, int> &xCoords, const std::map<frCoord, int> &yCoords, int lowerType, int upperType);
    void prepPoint_pin_genPoints_rect_ap_helper(std::vector<std::unique_ptr<frAccessPoint> > &aps, std::set<std::pair<frPoint, frLayerNum> > &apset,
                                                const gtl::rectangle_data<frCoord> &maxrect,
                                                frCoord x, frCoord y, frLayerNum layerNum, bool allowPlanar, bool allowVia, int lowCost, int highCost);
    void prepPoint_pin_checkPoints(std::vector<std::unique_ptr<frAccessPoint> > &aps, 
                                   const std::vector<gtl::polygon_90_set_data<frCoord> > &pinShapes,
                                   frPin* pin, frInstTerm* instTerm);
    void prepPoint_pin_checkPoint(frAccessPoint* ap, const gtl::polygon_90_set_data<frCoord> &polyset,
                                  const std::vector<gtl::polygon_90_data<frCoord> > &polys, frPin* pin, frInstTerm* instTerm);
    void prepPoint_pin_checkPoint_planar(frAccessPoint* ap, const std::vector<gtl::polygon_90_data<frCoord> > &layerPolys, 
                                         frDirEnum dir, frPin* pin, frInstTerm* instTerm);
    bool prepPoint_pin_checkPoint_planar_ep(frPoint &ep, 
                                            const std::vector<gtl::polygon_90_data<frCoord> > &layerPolys,
                                            const frPoint &bp, frLayerNum layerNum, frDirEnum dir, int stepSizeMultiplier = 2);
    void prepPoint_pin_checkPoint_print_helper(frAccessPoint* ap, frPin* pin, frInstTerm* instTerm, frDirEnum dir, int typeGC, int typeDRC, 
                                               frPoint bp, frPoint ep, frViaDef* viaDef);
    void prepPoint_pin_checkPoint_via(frAccessPoint* ap, const gtl::polygon_90_set_data<frCoord> &polyset, frDirEnum dir, frPin* pin, frInstTerm* instTerm);
    bool prepPoint_pin_checkPoint_via_helper(frAccessPoint* ap, frVia* via, frPin* pin, frInstTerm* instTerm);
    void prepPoint_pin_updateStat(const std::vector<std::unique_ptr<frAccessPoint> > &tmpAps, frPin* pin, frInstTerm* instTerm);
    bool prepPoint_pin_helper(std::vector<std::unique_ptr<frAccessPoint> > &aps, std::set<std::pair<frPoint, frLayerNum> > &apset,
                              std::vector<gtl::polygon_90_set_data<frCoord> > &pinShapes,
                              frPin* pin, frInstTerm* instTerm, int lowerType, int upperType);
    
    void prepPattern();
    void prepPattern_inst(frInst *inst, int currUniqueInstIdx);
    void genPatterns(const std::vector<std::pair<frPin*, frInstTerm*> > &pins, int currUniqueInstIdx);
    void genPatterns_init(std::vector<FlexDPNode> &nodes,
                          const std::vector<std::pair<frPin*, frInstTerm*> > &pins,
                          std::set<std::vector<int> > &instAccessPatterns,
                          std::set<std::pair<int, int> > &usedAccessPoints,
                          std::set<std::pair<int, int> > &violAccessPoints,
                          int maxAccessPointSize);
    void genPatterns_reset(std::vector<FlexDPNode> &nodes,
                           const std::vector<std::pair<frPin*, frInstTerm*> > &pins,
                           int maxAccessPointSize);
    void genPatterns_perform(std::vector<FlexDPNode> &nodes,
                             const std::vector<std::pair<frPin*, frInstTerm*> > &pins,
                             std::vector<int> &vioEdges,
                             const std::set<std::pair<int, int> > &usedAccessPoints,
                             const std::set<std::pair<int, int> > &violAccessPoints,
                             int currUniqueInstIdx,
                             int maxAccessPointSize);
    int getEdgeCost(int prevNodeIdx, int currNodeIdx, 
                    const std::vector<FlexDPNode> &nodes,
                    const std::vector<std::pair<frPin*, frInstTerm*> > &pins,
                    std::vector<int> &vioEdges,
                    const std::set<std::pair<int, int> > &usedAccessPoints,
                    const std::set<std::pair<int, int> > &violAccessPoints,
                    int currUniqueInstIdx,
                    int maxAccessPointSize);
    bool genPatterns_commit(std::vector<FlexDPNode> &nodes,
                            const std::vector<std::pair<frPin*, frInstTerm*> > &pins, bool &isValid,
                            std::set<std::vector<int> > &instAccessPatterns,
                            std::set<std::pair<int, int> > &usedAccessPoints,
                            std::set<std::pair<int, int> > &violAccessPoints,
                            int currUniqueInstIdx,
                            int maxAccessPointSize);
    void genPatterns_print_debug(std::vector<FlexDPNode> &nodes,
                           const std::vector<std::pair<frPin*, frInstTerm*> > &pins,
                           int maxAccessPointSize);
    void genPatterns_print(std::vector<FlexDPNode> &nodes,
                           const std::vector<std::pair<frPin*, frInstTerm*> > &pins,
                           int maxAccessPointSize);
    int getFlatIdx(int idx1, int idx2, int idx2Dim);
    void getNestedIdx(int flatIdx, int &idx1, int &idx2, int idx2Dim);
    int getFlatEdgeIdx(int prevIdx1, int prevIdx2, int currIdx2, int idx2Dim);
    
    bool genPatterns_gc(frBlockObject* targetObj, std::vector<std::pair<frConnFig*, frBlockObject*> > &objs,
                        std::set<frBlockObject*> *owners = nullptr);
    
    void getInsts(std::vector<frInst*> &insts);
    void genInstPattern(std::vector<frInst*> &insts);
    void genInstPattern_init(std::vector<FlexDPNode> &nodes,
                             const std::vector<frInst*> &insts);
    void genInstPattern_perform(std::vector<FlexDPNode> &nodes,
                                const std::vector<frInst*> &insts);
    void genInstPattern_commit(std::vector<FlexDPNode> &nodes,
                               const std::vector<frInst*> &insts);
    void genInstPattern_print(std::vector<FlexDPNode> &nodes,
                              const std::vector<frInst*> &insts);
    int getEdgeCost(int prevNodeIdx, int currNodeIdx, 
                    std::vector<FlexDPNode> &nodes,
                    const std::vector<frInst*> &insts);
    void revertAccessPoints();
    void addAccessPatternObj(frInst* inst,
                             FlexPinAccessPattern* accessPattern,
                             std::vector<std::pair<frConnFig*, frBlockObject*> > &objs,
                             std::vector<std::unique_ptr<frVia> > &vias, bool isPrev);
  };

  class FlexPinAccessPattern {
  public:
    // constructor
    FlexPinAccessPattern(): pattern(), left(nullptr), right(nullptr), cost(std::numeric_limits<int>::max()) {}
    // getter
    const std::vector<frAccessPoint*> &getPattern() const {
      return pattern;
    }
    frAccessPoint* getBoundaryAP(bool isLeft) const {
      return isLeft? left : right;
    }
    int getCost() const {
      return cost;
    }
    // setter
    void addAccessPoint(frAccessPoint *in) {
      pattern.push_back(in);
    }
    void setBoundaryAP(bool isLeft, frAccessPoint* in) {
      if (isLeft) {
        left = in;
      } else {
        right = in;
      }
    }
    void updateCost() {
      cost = 0;
      for (auto &ap: pattern) {
        cost += ap->getCost();
      }
    }
  private:
    std::vector<frAccessPoint*> pattern;
    frAccessPoint* left;
    frAccessPoint* right;
    int cost;
  };

  // dynamic programming related
  class FlexDPNode {
  public:
    // constructor
    FlexDPNode() : pathCost(std::numeric_limits<int>::max()), 
                   nodeCost(std::numeric_limits<int>::max()),
                   prevNodeIdx(-1) {}

    // getters
    int getPathCost() const {
      return pathCost;
    }
    int getNodeCost() const {
      return nodeCost;
    }
    int getPrevNodeIdx() const {
      return prevNodeIdx;
    }

    // setters
    void setPathCost(int in) {
      pathCost = in;
    }
    void setNodeCost(int in) {
      nodeCost = in;
    }
    void setPrevNodeIdx(int in) {
      prevNodeIdx = in;
    }

  private:
    int pathCost;
    int nodeCost;
    int prevNodeIdx;
  };
}


#endif
