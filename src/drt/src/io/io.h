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

#include <boost/icl/interval_set.hpp>
#include <list>
#include <memory>

#include "frDesign.h"

namespace odb {
class dbDatabase;
class dbTechNonDefaultRule;
class dbBlock;
class dbTech;
class dbSBox;
class dbTechLayer;
}  // namespace odb
namespace utl {
class Logger;
}

namespace fr::io {
using viaRawPriorityTuple = std::tuple<bool,          // not default via
                                       frCoord,       // lowerWidth
                                       frCoord,       // upperWidth
                                       bool,          // not align upper
                                       frCoord,       // cutArea
                                       frCoord,       // upperArea
                                       frCoord,       // lowerArea
                                       bool,          // not align lower
                                       std::string>;  // via name

class Parser
{
 public:
  // constructors
  Parser(odb::dbDatabase* dbIn, frDesign* designIn, Logger* loggerIn);

  // others
  void readDesign(odb::dbDatabase*);
  void readTechAndLibs(odb::dbDatabase*);
  bool readGuide();
  void postProcess();
  void postProcessGuide();
  void initDefaultVias();
  void initRPin();
  auto& getTrackOffsetMap() { return trackOffsetMap_; }
  std::vector<frTrackPattern*>& getPrefTrackPatterns()
  {
    return prefTrackPatterns_;
  }

 private:
  void setMasters(odb::dbDatabase*);
  void setTechVias(odb::dbTech*);
  void setTechViaRules(odb::dbTech*);
  void setDieArea(odb::dbBlock*);
  void setTracks(odb::dbBlock*);
  void setInsts(odb::dbBlock*);
  void setObstructions(odb::dbBlock*);
  void setBTerms(odb::dbBlock*);
  odb::Rect getViaBoxForTermAboveMaxLayer(odb::dbBTerm* term,
                                          frLayerNum& finalLayerNum);
  void setBTerms_addPinFig_helper(frBPin* pinIn,
                                  odb::Rect bbox,
                                  frLayerNum finalLayerNum);
  void setVias(odb::dbBlock*);
  void setNets(odb::dbBlock*);
  void setAccessPoints(odb::dbDatabase*);
  void getSBoxCoords(odb::dbSBox*,
                     frCoord&,
                     frCoord&,
                     frCoord&,
                     frCoord&,
                     frCoord&);
  void setLayers(odb::dbTech*);
  void addDefaultMasterSliceLayer();
  void addDefaultCutLayer();
  void addRoutingLayer(odb::dbTechLayer*);
  void addCutLayer(odb::dbTechLayer*);
  void addMasterSliceLayer(odb::dbTechLayer*);
  void setRoutingLayerProperties(odb::dbTechLayer* layer, frLayer* tmpLayer);
  void setCutLayerProperties(odb::dbTechLayer* layer, frLayer* tmpLayer);

  void setNDRs(odb::dbDatabase* db);
  void createNDR(odb::dbTechNonDefaultRule* ndr);
  void convertLef58MinCutConstraints();

  // postProcess functions
  void buildGCellPatterns(odb::dbDatabase* db);
  void buildGCellPatterns_helper(frCoord& GCELLGRIDX,
                                 frCoord& GCELLGRIDY,
                                 frCoord& GCELLOFFSETX,
                                 frCoord& GCELLOFFSETY);
  void buildGCellPatterns_getWidth(frCoord& GCELLGRIDX, frCoord& GCELLGRIDY);
  void buildGCellPatterns_getOffset(frCoord GCELLGRIDX,
                                    frCoord GCELLGRIDY,
                                    frCoord& GCELLOFFSETX,
                                    frCoord& GCELLOFFSETY);
  void getViaRawPriority(frViaDef* viaDef, viaRawPriorityTuple& priority);
  void initDefaultVias_GF14(const std::string& in);
  void initCutLayerWidth();
  void initConstraintLayerIdx();

  // instance analysis
  void instAnalysis();

  // postProcessGuide functions
  void genGuides(frNet* net, std::vector<frRect>& rects);
  void genGuides_addCoverGuide(frNet* net, std::vector<frRect>& rects);
  template <typename T>
  void genGuides_addCoverGuide_helper(frBlockObject* term,
                                      T* trueTerm,
                                      frInst* inst,
                                      dbTransform& shiftXform,
                                      vector<frRect>& rects);
  void patchGuides(frNet* net, frBlockObject* pin, std::vector<frRect>& rects);
  static int distL1(const Rect& b, const Point& p);
  static void getClosestPoint(const frRect& r,
                              const Point3D& p,
                              Point3D& result);
  void genGuides_pinEnclosure(frNet* net, std::vector<frRect>& rects);
  void checkPinForGuideEnclosure(frBlockObject* pin,
                                 frNet* net,
                                 std::vector<frRect>& guides);
  void genGuides_merge(
      std::vector<frRect>& rects,
      std::vector<std::map<frCoord, boost::icl::interval_set<frCoord>>>& intvs);
  void genGuides_split(
      std::vector<frRect>& rects,
      std::vector<std::map<frCoord, boost::icl::interval_set<frCoord>>>& intvs,
      std::map<std::pair<Point, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      std::map<frBlockObject*,
               std::set<std::pair<Point, frLayerNum>>,
               frBlockObjectComp>& pin2GCellMap,
      bool isRetry);
  void genGuides_gCell2PinMap(
      frNet* net,
      std::map<std::pair<Point, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap);
  template <typename T>
  void genGuides_gCell2TermMap(
      std::map<std::pair<Point, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      T* term,
      frBlockObject* origTerm);
  bool genGuides_gCell2APInstTermMap(
      std::map<std::pair<Point, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      frInstTerm* instTerm);
  bool genGuides_gCell2APTermMap(
      std::map<std::pair<Point, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      frBTerm* term);
  void genGuides_initPin2GCellMap(
      frNet* net,
      std::map<frBlockObject*,
               std::set<std::pair<Point, frLayerNum>>,
               frBlockObjectComp>& pin2GCellMap);
  void genGuides_buildNodeMap(
      std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap,
      int& gCnt,
      int& nCnt,
      std::vector<frRect>& rects,
      std::map<frBlockObject*,
               std::set<std::pair<Point, frLayerNum>>,
               frBlockObjectComp>& pin2GCellMap);
  bool genGuides_astar(
      frNet* net,
      std::vector<bool>& adjVisited,
      std::vector<int>& adjPrevIdx,
      std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap,
      int& gCnt,
      int& nCnt,
      bool forceFeedThrough,
      bool retry);
  void genGuides_final(frNet* net,
                       std::vector<frRect>& rects,
                       std::vector<bool>& adjVisited,
                       std::vector<int>& adjPrevIdx,
                       int gCnt,
                       int nCnt,
                       std::map<frBlockObject*,
                                std::set<std::pair<Point, frLayerNum>>,
                                frBlockObjectComp>& pin2GCellMap);

  // temp init functions
  void initRPin_rpin();
  void initRPin_rq();

  // write guide
  void saveGuidesUpdates();

  // misc
  void addFakeNets();

  odb::dbDatabase* db_;
  frDesign* design_;
  frTechObject* tech_;
  Logger* logger_;
  std::unique_ptr<frBlock> tmpBlock_;
  // temporary variables
  int readLayerCnt_;
  odb::dbTechLayer* masterSliceLayer_;
  std::map<frNet*, std::vector<frRect>, frBlockObjectComp> tmpGuides_;
  std::vector<std::pair<frBlockObject*, Point>> tmpGRPins_;
  std::map<frMaster*,
           std::map<dbOrientType,
                    std::map<std::vector<frCoord>,
                             std::set<frInst*, frBlockObjectComp>>>,
           frBlockObjectComp>
      trackOffsetMap_;
  std::vector<frTrackPattern*> prefTrackPatterns_;
  int numMasters_;
  int numInsts_;
  int numTerms_;      // including instterm and term
  int numNets_;       // including snet and net
  int numBlockages_;  // including instBlockage and blockage
};

class Writer
{
 public:
  // constructors
  Writer(frDesign* designIn, Logger* loggerIn)
      : tech_(designIn->getTech()), design_(designIn), logger_(loggerIn)
  {
  }
  // getters
  frTechObject* getTech() const { return tech_; }
  frDesign* getDesign() const { return design_; }
  // others
  void updateDb(odb::dbDatabase* db,
                bool pin_access = false,
                bool snapshot = false);
  void updateTrackAssignment(odb::dbBlock* block);

 private:
  void fillViaDefs();
  void fillConnFigs(bool isTA);
  void fillConnFigs_net(frNet* net, bool isTA);
  void mergeSplitConnFigs(std::list<std::shared_ptr<frConnFig>>& connFigs);
  void splitVia_helper(
      frLayerNum layerNum,
      int isH,
      frCoord trackLoc,
      frCoord x,
      frCoord y,
      std::vector<std::vector<
          std::map<frCoord, std::vector<std::shared_ptr<frPathSeg>>>>>&
          mergedPathSegs);
  void updateDbConn(odb::dbBlock* block, odb::dbTech* db_tech, bool snapshot);
  void updateDbVias(odb::dbBlock* block, odb::dbTech* db_tech);
  void updateDbAccessPoints(odb::dbBlock* block, odb::dbTech* db_tech);

  frTechObject* tech_;
  frDesign* design_;
  Logger* logger_;
  std::map<frString, std::list<std::shared_ptr<frConnFig>>>
      connFigs_;  // all connFigs ready to def
  std::vector<frViaDef*> viaDefs_;
};

}  // namespace fr::io
