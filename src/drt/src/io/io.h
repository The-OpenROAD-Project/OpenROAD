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
namespace drt::io {
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
  Parser(odb::dbDatabase* dbIn,
         frDesign* design,
         Logger* loggerIn,
         RouterConfiguration* router_cfg);

  // others
  void readDesign(odb::dbDatabase*);
  void readTechAndLibs(odb::dbDatabase*);
  void postProcess();
  frLayerNum getTopPinLayer();
  void initDefaultVias();
  /**
   * Initializes secondary viadefs.
   *
   * This function initializes 'frLayer::secondaryViaDefs_', which are needed to
   * replace lonely vias ('frLayer::defaultViaDef_') according to
   * LEF58_MAXSPACING constraints. Usage of 'frLayer::secondaryViaDefs_' is in
   * FlexDRWorker::route_queue_main(std::queue<RouteQueueEntry>& rerouteQueue)
   *
   */
  void initSecondaryVias();
  void initRPin();
  auto& getTrackOffsetMap() { return trackOffsetMap_; }
  std::vector<frTrackPattern*>& getPrefTrackPatterns()
  {
    return prefTrackPatterns_;
  }
  void updateDesign();

 private:
  frDesign* getDesign() const;
  frBlock* getBlock() const;
  frTechObject* getTech() const;
  void setMasters(odb::dbDatabase*);
  void setTechVias(odb::dbTech*);
  void setTechViaRules(odb::dbTech*);
  void setDieArea(odb::dbBlock*);
  void setTracks(odb::dbBlock*);
  void setInsts(odb::dbBlock*);
  void setInst(odb::dbInst*);
  void setObstructions(odb::dbBlock*);
  void setBTerms(odb::dbBlock*);
  odb::Rect getViaBoxForTermAboveMaxLayer(odb::dbBTerm* term,
                                          frLayerNum& finalLayerNum);
  void setBTerms_addPinFig_helper(frBPin* pinIn,
                                  odb::Rect bbox,
                                  frLayerNum finalLayerNum);
  void setVias(odb::dbBlock*);
  void updateNetRouting(frNet*, odb::dbNet*);
  void setNets(odb::dbBlock*);
  frNet* addNet(odb::dbNet* db_net);
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
  void checkFig(frPinFig* uFig,
                const frString& term_name,
                const dbTransform& xform,
                bool& foundTracks,
                bool& foundCenterTracks,
                bool& hasPolys);
  void checkPins();
  void getViaRawPriority(const frViaDef* viaDef, viaRawPriorityTuple& priority);
  void initDefaultVias_GF14(const std::string& node);
  void initCutLayerWidth();
  void initConstraintLayerIdx();

  // instance analysis
  void instAnalysis();

  // temp init functions
  void initRPin_rpin();
  void initRPin_rq();
  // misc
  void addFakeNets();

  odb::dbDatabase* db_;
  frDesign* design_;
  Logger* logger_;
  RouterConfiguration* router_cfg_;
  // temporary variables
  int readLayerCnt_;
  odb::dbTechLayer* masterSliceLayer_;
  std::map<frMaster*,
           std::map<dbOrientType,
                    std::map<std::vector<frCoord>,
                             std::set<frInst*, frBlockObjectComp>>>,
           frBlockObjectComp>
      trackOffsetMap_;
  std::vector<frTrackPattern*> prefTrackPatterns_;
};

class Writer
{
 public:
  // constructors
  Writer(frDesign* design, Logger* loggerIn)
      : design_(design), logger_(loggerIn)
  {
  }
  // getters
  frTechObject* getTech() const;
  frDesign* getDesign() const;
  // others
  void updateDb(odb::dbDatabase* db,
                RouterConfiguration* router_cfg,
                bool pin_access = false,
                bool snapshot = false);
  void updateTrackAssignment(odb::dbBlock* block);

 private:
  void fillConnFigs(bool isTA, int verbose);
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
  void writeViaDefToODB(odb::dbBlock* block,
                        odb::dbTech* db_tech,
                        const frViaDef* via);
  void updateDbAccessPoints(odb::dbBlock* block, odb::dbTech* db_tech);
  void updateDbAccessPoint(odb::dbAccessPoint* db_ap,
                           frAccessPoint* ap,
                           odb::dbTech* db_tech,
                           odb::dbBlock* block);

  frDesign* design_;
  Logger* logger_;
  std::map<frString, std::list<std::shared_ptr<frConnFig>>>
      connFigs_;  // all connFigs ready to def
  std::vector<frViaDef*> viaDefs_;
};

/**
 * This class handles BTerms above top routing layer. It creates a stack of vias
 * from the lowest pin shape down to the top routing layer so that the pin is
 * accessible to the router.
 */
class TopLayerBTermHandler
{
 public:
  TopLayerBTermHandler(frDesign* design,
                       odb::dbDatabase* db,
                       Logger* logger,
                       RouterConfiguration* router_cfg)
      : design_(design), db_(db), logger_(logger), router_cfg_(router_cfg)
  {
  }
  void processBTermsAboveTopLayer(bool has_routing = false);

 private:
  void stackVias(odb::dbBTerm* bterm,
                 int top_layer_idx,
                 int bterm_bottom_layer_idx,
                 bool has_routing);
  int countNetBTermsAboveMaxLayer(odb::dbNet* net);
  bool netHasStackedVias(odb::dbNet* net);
  /**
   * @brief Finds the best track on the TOP_ROUTING_LAYER to be the via position
   *
   * @param pin_rect The BTerm pin rectange shape.
   * @returns The chosen via location for stacking the vias up to the
   * TOP_ROUTING_LAYER
   */
  Point getBestViaPosition(Rect pin_rect);
  frDesign* design_;
  odb::dbDatabase* db_;
  Logger* logger_;
  RouterConfiguration* router_cfg_;
};
}  // namespace drt::io
