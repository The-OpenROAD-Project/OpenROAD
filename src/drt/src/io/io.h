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

#ifndef _FR_IO_H_
#define _FR_IO_H_

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

namespace fr {
namespace io {
// not default via, upperWidth, lowerWidth, not align upper, upperArea,
// lowerArea, not align lower
typedef std::tuple<bool, frCoord, frCoord, bool, frCoord, frCoord, bool>
    viaRawPriorityTuple;

class Parser
{
 public:
  // constructors
  Parser(frDesign* designIn, Logger* loggerIn)
      : design(designIn),
        tech(design->getTech()),
        logger(loggerIn),
        tmpBlock(nullptr),
        readLayerCnt(0),
        tmpGuides(),
        tmpGRPins(),
        trackOffsetMap(),
        prefTrackPatterns(),
        numRefBlocks(0),
        numInsts(0),
        numTerms(0),
        numNets(0),
        numBlockages(0)
  {
  }
  // others
  void readDb(odb::dbDatabase* db);
  void readGuide();
  void postProcess();
  void postProcessGuide();
  void initDefaultVias();
  void initRPin();
  std::map<frBlock*,
           std::map<frOrient,
                    std::map<std::vector<frCoord>,
                             std::set<frInst*, frBlockObjectComp>>>,
           frBlockObjectComp>&
  getTrackOffsetMap()
  {
    return trackOffsetMap;
  }
  std::vector<frTrackPattern*>& getPrefTrackPatterns()
  {
    return prefTrackPatterns;
  }

 protected:
  void readDesign(odb::dbDatabase*);
  void readTechAndLibs(odb::dbDatabase*);
  void setMacros(odb::dbDatabase*);
  void setTechVias(odb::dbTech*);
  void setTechViaRules(odb::dbTech*);
  void setDieArea(odb::dbBlock*);
  void setTracks(odb::dbBlock*);
  void setInsts(odb::dbBlock*);
  void setObstructions(odb::dbBlock*);
  void setBTerms(odb::dbBlock*);
  void setVias(odb::dbBlock*);
  void setNets(odb::dbBlock*);
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

  frDesign* design;
  frTechObject* tech;
  Logger* logger;
  std::unique_ptr<frBlock> tmpBlock;
  odb::dbDatabase* db;
  // temporary variables
  int readLayerCnt;
  std::string masterSliceLayerName;
  std::map<frNet*, std::vector<frRect>, frBlockObjectComp> tmpGuides;
  std::vector<std::pair<frBlockObject*, frPoint>> tmpGRPins;
  std::map<frBlock*,
           std::map<frOrient,
                    std::map<std::vector<frCoord>,
                             std::set<frInst*, frBlockObjectComp>>>,
           frBlockObjectComp>
      trackOffsetMap;
  std::vector<frTrackPattern*> prefTrackPatterns;
  int numRefBlocks;
  int numInsts;
  int numTerms;      // including instterm and term
  int numNets;       // including snet and net
  int numBlockages;  // including instBlockage and blockage

  // postProcess functions
  void buildGCellPatterns();
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
  void initDefaultVias_N16(const std::string& in);
  void initDefaultVias_GF14(const std::string& in);
  void initCutLayerWidth();
  void initConstraintLayerIdx();

  // instance analysis
  void instAnalysis();

  // postProcessGuide functions
  void genGuides(frNet* net, std::vector<frRect>& rects);
  void genGuides_addCoverGuide(frNet* net, std::vector<frRect>& rects);
  void genGuides_merge(
      std::vector<frRect>& rects,
      std::vector<std::map<frCoord, boost::icl::interval_set<frCoord>>>& intvs);
  void genGuides_split(
      std::vector<frRect>& rects,
      std::vector<std::map<frCoord, boost::icl::interval_set<frCoord>>>& intvs,
      std::map<std::pair<frPoint, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      std::map<frBlockObject*,
               std::set<std::pair<frPoint, frLayerNum>>,
               frBlockObjectComp>& pin2GCellMap,
      bool isRetry);
  void genGuides_gCell2PinMap(
      frNet* net,
      std::map<std::pair<frPoint, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap);
  void genGuides_gCell2TermMap(
      std::map<std::pair<frPoint, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      frTerm* term,
      frBlockObject* origTerm);
  bool genGuides_gCell2APInstTermMap(
      std::map<std::pair<frPoint, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      frInstTerm* instTerm);
  bool genGuides_gCell2APTermMap(
      std::map<std::pair<frPoint, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      frTerm* instTerm);
  void genGuides_initPin2GCellMap(
      frNet* net,
      std::map<frBlockObject*,
               std::set<std::pair<frPoint, frLayerNum>>,
               frBlockObjectComp>& pin2GCellMap);
  void genGuides_buildNodeMap(
      std::map<std::pair<frPoint, frLayerNum>, std::set<int>>& nodeMap,
      int& gCnt,
      int& nCnt,
      std::vector<frRect>& rects,
      std::map<frBlockObject*,
               std::set<std::pair<frPoint, frLayerNum>>,
               frBlockObjectComp>& pin2GCellMap);
  bool genGuides_astar(
      frNet* net,
      std::vector<bool>& adjVisited,
      std::vector<int>& adjPrevIdx,
      std::map<std::pair<frPoint, frLayerNum>, std::set<int>>& nodeMap,
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
                                std::set<std::pair<frPoint, frLayerNum>>,
                                frBlockObjectComp>& pin2GCellMap);

  // temp init functions
  void initRPin_rpin();
  void initRPin_rq();

  // write guide
  void writeGuideFile();

  // misc
  void addFakeNets();
};
class Writer
{
 public:
  // constructors
  Writer(frDesign* designIn, Logger* loggerIn)
      : tech(designIn->getTech()), design(designIn), logger(loggerIn)
  {
  }
  // getters
  frTechObject* getTech() const { return tech; }
  frDesign* getDesign() const { return design; }
  // others
  void updateDb(odb::dbDatabase* db);
  std::map<frString, std::list<std::shared_ptr<frConnFig>>>
      connFigs;  // all connFigs ready to def
  std::vector<frViaDef*> viaDefs;

 protected:
  frTechObject* tech;
  frDesign* design;
  Logger* logger;

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
  void updateDbConn(odb::dbBlock* block, odb::dbTech* tech);
  void updateDbVias(odb::dbBlock* block, odb::dbTech* tech);
};
}  // namespace io
}  // namespace fr

#endif
