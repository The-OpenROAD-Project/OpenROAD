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

#include <memory>

#include "db/gcObj/gcNet.h"
#include "dr/FlexDR.h"
#include "frDesign.h"
#include "gc/FlexGC.h"

namespace odb {
class dbTechLayerCutSpacingTableDefRule;
}

namespace fr {
class FlexGCWorkerRegionQuery
{
 public:
  FlexGCWorkerRegionQuery(FlexGCWorker* in);
  ~FlexGCWorkerRegionQuery();
  FlexGCWorker* getGCWorker() const;
  void addPolygonEdge(gcSegment* edge);
  void addMaxRectangle(gcRect* rect);
  void addSpcRectangle(gcRect* rect);
  void removePolygonEdge(gcSegment* connFig);
  void removeMaxRectangle(gcRect* connFig);
  void removeSpcRectangle(gcRect* rect);
  void queryPolygonEdge(
      const box_t& box,
      const frLayerNum layerNum,
      std::vector<std::pair<segment_t, gcSegment*>>& result) const;
  void queryPolygonEdge(
      const Rect& box,
      const frLayerNum layerNum,
      std::vector<std::pair<segment_t, gcSegment*>>& result) const;
  void queryMaxRectangle(const box_t& box,
                         const frLayerNum layerNum,
                         std::vector<rq_box_value_t<gcRect*>>& result) const;
  void querySpcRectangle(const box_t& box,
                         const frLayerNum layerNum,
                         std::vector<rq_box_value_t<gcRect>>& result) const;
  void queryMaxRectangle(const Rect& box,
                         const frLayerNum layerNum,
                         std::vector<rq_box_value_t<gcRect*>>& result) const;
  void queryMaxRectangle(const gtl::rectangle_data<frCoord>& box,
                         const frLayerNum layerNum,
                         std::vector<rq_box_value_t<gcRect*>>& result) const;
  void init(int numLayers);
  void addToRegionQuery(gcNet* net);
  void removeFromRegionQuery(gcNet* net);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

class FlexGCWorker::Impl
{
  friend class FlexGCWorker;

 public:
  // constructors
  Impl();  // for serialization
  Impl(frTechObject* techIn,
       Logger* logger,
       FlexDRWorker* drWorkerIn,
       FlexGCWorker* gcWorkerIn);
  frLayerNum getMinLayerNum()  // inclusive
  {
    return std::max((frLayerNum) (getTech()->getBottomLayerNum()),
                    minLayerNum_);
  }
  frLayerNum getMaxLayerNum()  // inclusive
  {
    return std::min((frLayerNum) (getTech()->getTopLayerNum()), maxLayerNum_);
  }
  gcNet* addNet(frBlockObject* owner = nullptr)
  {
    auto uNet = std::make_unique<gcNet>(getTech()->getLayers().size());
    auto net = uNet.get();
    net->setOwner(owner);
    net->setId(nets_.size());
    nets_.push_back(std::move(uNet));
    owner2nets_[owner] = net;
    return net;
  }
  void addMarker(std::unique_ptr<frMarker> in);
  void clearMarkers()
  {
    mapMarkers_.clear();
    markers_.clear();
  }
  void addPAObj(frConnFig* obj, frBlockObject* owner);
  // getters
  frTechObject* getTech() const { return tech_; }
  FlexDRWorker* getDRWorker() const { return drWorker_; }
  const Rect& getExtBox() const { return extBox_; }
  std::vector<std::unique_ptr<gcNet>>& getNets() { return nets_; }
  // others
  void init(const frDesign* design);
  int main();
  void end();
  // initialization from FlexPA, initPA0 --> addPAObj --> initPA1
  void initPA0(const frDesign* design);
  void initPA1();
  void initNetsFromDesign(const frDesign* design);
  // update
  void updateGCWorker();

 private:
  frTechObject* tech_;
  Logger* logger_;
  FlexDRWorker* drWorker_;

  Rect extBox_;
  Rect drcBox_;

  std::map<frBlockObject*, gcNet*> owner2nets_;  // no order is assumed
  std::vector<std::unique_ptr<gcNet>> nets_;

  std::vector<std::unique_ptr<frMarker>> markers_;
  std::map<MarkerId, frMarker*> mapMarkers_;
  std::vector<std::unique_ptr<drPatchWire>> pwires_;

  FlexGCWorkerRegionQuery rq_;
  bool printMarker_;

  // temps
  std::vector<drNet*> modifiedDRNets_;

  // parameters
  gcNet* targetNet_;
  frLayerNum minLayerNum_;
  frLayerNum maxLayerNum_;

  // for pin prep
  std::set<frBlockObject*> targetObjs_;
  bool ignoreDB_;
  bool ignoreMinArea_;
  bool ignoreLongSideEOL_;
  bool ignoreCornerSpacing_;
  bool surgicalFixEnabled_;

  FlexGCWorkerRegionQuery& getWorkerRegionQuery() { return rq_; }

  void modifyMarkers();
  // init
  gcNet* getNet(frBlockObject* obj);
  gcNet* getNet(frNet* net);
  void initObj(const Rect& box,
               frLayerNum layerNum,
               frBlockObject* obj,
               bool isFixed);
  gcNet* initDRObj(drConnFig* obj, gcNet* currNet = nullptr);
  gcNet* initRouteObj(frBlockObject* obj, gcNet* currNet = nullptr);
  void initDesign(const frDesign* design, bool skipDR = false);
  bool initDesign_skipObj(frBlockObject* obj);
  void initDRWorker();
  void initNets();
  void initNet(gcNet* net);
  void initNet_pins_polygon(gcNet* net);
  void initNet_pins_polygonEdges(gcNet* net);
  void initNet_pins_polygonEdges_getFixedPolygonEdges(
      gcNet* net,
      std::vector<std::set<std::pair<Point, Point>>>& fixedPolygonEdges);
  void initNet_pins_polygonEdges_helper_outer(
      gcNet* net,
      gcPin* pin,
      gcPolygon* poly,
      frLayerNum i,
      const std::vector<std::set<std::pair<Point, Point>>>& fixedPolygonEdges);
  void initNet_pins_polygonEdges_helper_inner(
      gcNet* net,
      gcPin* pin,
      const gtl::polygon_90_data<frCoord>& hole_poly,
      frLayerNum i,
      const std::vector<std::set<std::pair<Point, Point>>>& fixedPolygonEdges);
  void initNet_pins_polygonCorners(gcNet* net);
  void initNet_pins_polygonCorners_helper(gcNet* net, gcPin* pin);
  void initNet_pins_maxRectangles(gcNet* net);
  void initNet_pins_maxRectangles_getFixedMaxRectangles(
      gcNet* net,
      std::vector<std::set<std::pair<Point, Point>>>& fixedMaxRectangles);
  void initNet_pins_maxRectangles_helper(
      gcNet* net,
      gcPin* pin,
      const gtl::rectangle_data<frCoord>& rect,
      frLayerNum i,
      const std::vector<std::set<std::pair<Point, Point>>>& fixedMaxRectangles);

  void initRegionQuery();

  void checkMetalSpacing();
  void checkMetalSpacing_wrongDir_getQueryBox(gcSegment* edge,
                                              frCoord spcVal,
                                              box_t& queryBox);
  frCoord getPrl(gcSegment* edge,
                 gcSegment* ptr,
                 const gtl::orientation_2d& orient) const;
  void checkMetalSpacing_wrongDir(gcPin* pin, frLayer* layer);
  frCoord checkMetalSpacing_getMaxSpcVal(frLayerNum layerNum,
                                         bool checkNDRs = true);
  void myBloat(const gtl::rectangle_data<frCoord>& rect,
               frCoord val,
               box_t& box);
  void checkMetalSpacing_main(gcRect* rect,
                              bool checkNDRs = true,
                              bool isSpcRect = false);
  void checkMetalSpacing_main(gcRect* rect1,
                              gcRect* rect2,
                              bool checkNDRs = true,
                              bool isSpcRect = false);
  void checkMetalSpacing_short(gcRect* rect1,
                               gcRect* rect2,
                               const gtl::rectangle_data<frCoord>& markerRect);

  bool checkMetalSpacing_short_skipFixed(
      gcRect* rect1,
      gcRect* rect2,
      const gtl::rectangle_data<frCoord>& markerRect);

  bool checkMetalSpacing_short_skipSameNet(
      gcRect* rect1,
      gcRect* rect2,
      const gtl::rectangle_data<frCoord>& markerRect);

  void checkMetalSpacing_short_obs(
      gcRect* rect1,
      gcRect* rect2,
      const gtl::rectangle_data<frCoord>& markerRect);
  frCoord checkMetalSpacing_prl_getReqSpcVal(gcRect* rect1,
                                             gcRect* rect2,
                                             frCoord prl);
  bool checkMetalSpacing_prl_hasPolyEdge(
      gcRect* rect1,
      gcRect* rect2,
      const gtl::rectangle_data<frCoord>& markerRect,
      int type,
      frCoord prl);
  void checkMetalSpacing_prl(gcRect* rect1,
                             gcRect* rect2,
                             const gtl::rectangle_data<frCoord>& markerRect,
                             frCoord prl,
                             frCoord distX,
                             frCoord distY,
                             bool checkNDRs = true,
                             bool checkPolyEdge = true);
  box_t checkMetalCornerSpacing_getQueryBox(gcCorner* corner,
                                            frCoord& maxSpcValX,
                                            frCoord& maxSpcValY);
  void checkMetalCornerSpacing();
  void checkMetalCornerSpacing_getMaxSpcVal(frLayerNum layerNum,
                                            frCoord& maxSpcValX,
                                            frCoord& maxSpcValY);

  void checkMetalCornerSpacing_main(gcCorner* corner);
  void checkMetalCornerSpacing_main(gcCorner* corner,
                                    gcRect* rect,
                                    frLef58CornerSpacingConstraint* con);

  void checkMetalShape(bool allow_patching = false);
  void checkMetalShape_main(gcPin* pin, bool allow_patching);
  void checkMetalShape_minWidth(const gtl::rectangle_data<frCoord>& rect,
                                frLayerNum layerNum,
                                gcNet* net,
                                bool isH);
  void checkMetalShape_offGrid(gcPin* pin);
  void checkMetalShape_minEnclosedArea(gcPin* pin);
  void checkMetalShape_minStep(gcPin* pin);
  void checkMetalShape_minStep_helper(const Rect& markerBox,
                                      frLayerNum layerNum,
                                      gcNet* net,
                                      frMinStepConstraint* con,
                                      bool hasInsideCorner,
                                      bool hasOutsideCorner,
                                      int currEdges,
                                      frCoord currLength,
                                      bool hasRoute);
  void checkMetalShape_rectOnly(gcPin* pin);
  void checkMetalShape_minArea(gcPin* pin);

  void checkMetalEndOfLine();
  void checkMetalEndOfLine_main(gcPin* pin);
  void checkMetalEndOfLine_eol(gcSegment* edge, frConstraint* constraint);
  void checkMetalEndOfLine_eol_TN(gcSegment* edge, frConstraint* constraint);
  bool qualifiesAsEol(gcSegment* edge,
                      frConstraint* constraint,
                      bool& hasRoute);
  void getMetalEolExtQueryRegion(gcSegment* edge,
                                 const gtl::rectangle_data<frCoord>& extRect,
                                 frCoord spacing,
                                 box_t& queryBox,
                                 gtl::rectangle_data<frCoord>& queryRect);
  void getMetalEolExtRect(gcSegment* edge,
                          frCoord extension,
                          gtl::rectangle_data<frCoord>& rect);
  void checkMetalEndOfLine_ext_helper(
      gcSegment* edge1,
      gcSegment* edge2,
      const gtl::rectangle_data<frCoord>& rect1,
      const gtl::rectangle_data<frCoord>& queryRect,
      frLef58EolExtensionConstraint* constraint);
  void checkMetalEndOfLine_ext(gcSegment* edge,
                               frLef58EolExtensionConstraint* constraint);
  void checkMetalEOLkeepout_helper(gcSegment* edge,
                                   gcRect* rect,
                                   gtl::rectangle_data<frCoord> queryRect,
                                   frLef58EolKeepOutConstraint* constraint);
  void checkMetalEOLkeepout_main(gcSegment* edge,
                                 frLef58EolKeepOutConstraint* constraint);
  void getEolKeepOutExceptWithinRects(gcSegment* edge,
                                      frLef58EolKeepOutConstraint* constraint,
                                      gtl::rectangle_data<frCoord>& rect1,
                                      gtl::rectangle_data<frCoord>& rect2);
  void getEolKeepOutQueryBox(gcSegment* edge,
                             frLef58EolKeepOutConstraint* constraint,
                             box_t& queryBox,
                             gtl::rectangle_data<frCoord>& queryRect);
  bool checkMetalEndOfLine_eol_isEolEdge(gcSegment* edge,
                                         frConstraint* constraint);
  bool checkMetalEndOfLine_eol_hasMinMaxLength(gcSegment* edge,
                                               frConstraint* constraint);
  bool checkMetalEndOfLine_eol_hasEncloseCut(gcSegment* edge1,
                                             gcSegment* edge2,
                                             frConstraint* constraint);
  void checkMetalEndOfLine_eol_hasEncloseCut_getQueryBox(
      gcSegment* edge,
      frLef58SpacingEndOfLineWithinEncloseCutConstraint* constraint,
      box_t& queryBox);
  bool checkMetalEndOfLine_eol_hasParallelEdge(gcSegment* edge,
                                               frConstraint* constraint,
                                               bool& hasRoute);
  bool checkMetalEndOfLine_eol_hasParallelEdge_oneDir(gcSegment* edge,
                                                      frConstraint* constraint,
                                                      bool isSegLow,
                                                      bool& hasRoute);
  void checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getQueryBox(
      gcSegment* edge,
      frConstraint* constraint,
      bool isSegLow,
      box_t& queryBox,
      gtl::rectangle_data<frCoord>& queryRect);
  void checkMetalEndOfLine_eol_hasParallelEdge_oneDir_getParallelEdgeRect(
      gcSegment* edge,
      gtl::rectangle_data<frCoord>& rect);
  void checkMetalEndOfLine_eol_hasEol(gcSegment* edge,
                                      frConstraint* constraint,
                                      bool hasRoute);
  void checkMetalEndOfLine_eol_hasEol_getQueryBox(
      gcSegment* edge,
      frConstraint* constraint,
      box_t& queryBox,
      gtl::rectangle_data<frCoord>& queryRect,
      frCoord& eolNonPrlSpacing,
      frCoord& endPrlSpacing,
      frCoord& endPrl,
      bool isEolEdge = true);

  void checkMetalEndOfLine_eol_hasEol_check(
      gcSegment* edge,
      gcSegment* ptr,
      const gtl::rectangle_data<frCoord>& queryRect,
      frConstraint* constraint,
      frCoord endPrlSpacing,
      frCoord eolNonPrlSpacing,
      frCoord endPrl,
      bool hasRoute);

  void checkMetalEndOfLine_eol_hasEol_helper(gcSegment* edge1,
                                             gcSegment* edge2,
                                             frConstraint* constraint);
  bool checkMetalEndOfLine_eol_hasEol_endToEndHelper(gcSegment* edge1,
                                                     gcSegment* edge2,
                                                     frConstraint* constraint);

  void checkCutSpacing();
  void checkCutSpacing_main(gcRect* rect);
  void checkLef58CutSpacingTbl(gcRect* viaRect,
                               frLef58CutSpacingTableConstraint* con);
  bool checkLef58CutSpacingTbl_prlValid(
      const gtl::rectangle_data<frCoord>& viaRect1,
      const gtl::rectangle_data<frCoord>& viaRect2,
      const gtl::rectangle_data<frCoord>& markerRect,
      std::string cutClass1,
      std::string cutClass2,
      frCoord& prl,
      odb::dbTechLayerCutSpacingTableDefRule* dbRule);
  bool checkLef58CutSpacingTbl_sameMetal(gcRect* viaRect1, gcRect* viaRect2);
  bool checkLef58CutSpacingTbl_stacked(gcRect* viaRect1, gcRect* viaRect2);
  void checkLef58CutSpacingTbl_main(gcRect* viaRect1,
                                    gcRect* viaRect2,
                                    frLef58CutSpacingTableConstraint* con);
  bool checkLef58CutSpacingTbl_helper(
      gcRect* viaRect1,
      gcRect* viaRect2,
      frString cutClass1,
      frString cutClass2,
      const frDirEnum dir,
      frSquaredDistance distSquare,
      frSquaredDistance c2cSquare,
      bool prlValid,
      frCoord prl,
      odb::dbTechLayerCutSpacingTableDefRule* dbRule);
  void checkCutSpacing_main(gcRect* rect, frCutSpacingConstraint* con);
  bool checkCutSpacing_main_hasAdjCuts(gcRect* rect,
                                       frCutSpacingConstraint* con);
  frCoord checkCutSpacing_getMaxSpcVal(frCutSpacingConstraint* con);
  void checkCutSpacing_main(gcRect* ptr1,
                            gcRect* ptr2,
                            frCutSpacingConstraint* con);
  void checkCutSpacing_short(gcRect* rect1,
                             gcRect* rect2,
                             const gtl::rectangle_data<frCoord>& markerRect);
  void checkCutSpacing_spc(gcRect* rect1,
                           gcRect* rect2,
                           const gtl::rectangle_data<frCoord>& markerRect,
                           frCutSpacingConstraint* con,
                           frCoord prl);
  void checkCutSpacing_spc_diff_layer(
      gcRect* rect1,
      gcRect* rect2,
      const gtl::rectangle_data<frCoord>& markerRect,
      frCutSpacingConstraint* con);
  frCoord checkCutSpacing_spc_getReqSpcVal(gcRect* ptr1,
                                           gcRect* ptr2,
                                           frCutSpacingConstraint* con);

  // LEF58
  void checkLef58CutSpacing_main(gcRect* rect);
  void checkLef58CutSpacing_main(gcRect* rect,
                                 frLef58CutSpacingConstraint* con,
                                 bool skipSameNet = false);
  void checkLef58CutSpacing_spc_parallelOverlap(
      gcRect* rect1,
      gcRect* rect2,
      frLef58CutSpacingConstraint* con,
      const gtl::rectangle_data<frCoord>& markerRect);
  void checkLef58CutSpacing_main(gcRect* rect1,
                                 gcRect* rect2,
                                 frLef58CutSpacingConstraint* con);
  void checkLef58CutSpacing_spc_layer(
      gcRect* rect1,
      gcRect* rect2,
      const gtl::rectangle_data<frCoord>& markerRect,
      frLef58CutSpacingConstraint* con);
  void checkLef58CutSpacing_spc_adjCut(
      gcRect* rect1,
      gcRect* rect2,
      const gtl::rectangle_data<frCoord>& markerRect,
      frLef58CutSpacingConstraint* con);
  bool checkLef58CutSpacing_spc_hasAdjCuts(gcRect* rect,
                                           frLef58CutSpacingConstraint* con);
  bool checkLef58CutSpacing_spc_hasTwoCuts(gcRect* rect1,
                                           gcRect* rect2,
                                           frLef58CutSpacingConstraint* con);
  bool checkLef58CutSpacing_spc_hasTwoCuts_helper(
      gcRect* rect,
      frLef58CutSpacingConstraint* con);
  frCoord checkLef58CutSpacing_spc_getReqSpcVal(
      gcRect* ptr1,
      gcRect* ptr2,
      frLef58CutSpacingConstraint* con);
  // LEF58_KEEPOUTZONE
  void checKeepOutZone_main(gcRect* rect, frLef58KeepOutZoneConstraint* con);

  frCoord checkLef58CutSpacing_getMaxSpcVal(frLef58CutSpacingConstraint* con);
  void checkMetalShape_lef58MinStep(gcPin* pin);
  void checkMetalShape_lef58MinStep_noBetweenEol(gcPin* pin,
                                                 frLef58MinStepConstraint* con);
  void checkMetalSpacingTableInfluence();
  void checkPinMetSpcTblInf(gcPin*);
  void checkRectMetSpcTblInf(gcRect*, frSpacingTableInfluenceConstraint*);
  void checkOrthRectsMetSpcTblInf(const std::vector<gcRect*>& rects,
                                  const gtl::rectangle_data<frCoord>& queryRect,
                                  const frCoord spacing,
                                  const gtl::orientation_2d orient);
  void checkRectMetSpcTblInf_queryBox(const gtl::rectangle_data<frCoord>& rect,
                                      const frCoord dist,
                                      const frDirEnum dir,
                                      box_t& box);
  void checkMinimumCut_marker(gcRect* wideRect,
                              gcRect* viaRect,
                              frMinimumcutConstraint* con);
  void checkMinimumCut_main(gcRect* rect);
  void checkMinimumCut();
  void checkMetalShape_lef58Area(gcPin* pin);
  bool checkMetalShape_lef58Area_exceptRectangle(
      gcPolygon* poly,
      odb::dbTechLayerAreaRule* db_rule);
  bool checkMetalShape_lef58Area_rectWidth(gcPolygon* poly,
                                           odb::dbTechLayerAreaRule* db_rule,
                                           bool& check_rect_width);
  void checkMetalShape_addPatch(gcPin* pin, int min_area);
  void checkMetalShape_patchOwner_helper(drPatchWire* patch,
                                         const std::vector<drNet*>* dr_nets);

  void checkMetalWidthViaTable();
  void checkMetalWidthViaTable_main(gcRect* rect);
  // surgical fix
  void patchMetalShape();
  void patchMetalShape_minStep();
  void patchMetalShape_cornerSpacing();

  // utility
  bool isCornerOverlap(gcCorner* corner, const Rect& box);
  bool isCornerOverlap(gcCorner* corner,
                       const gtl::rectangle_data<frCoord>& rect);
  bool isOppositeDir(gcCorner* corner, gcSegment* seg);
  bool isWrongDir(gcSegment* edge);
};
}  // namespace fr
