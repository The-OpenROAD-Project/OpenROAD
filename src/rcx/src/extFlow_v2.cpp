// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "gseq.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/geom.h"
#include "parse.h"
#include "rcx/array1.h"
#include "rcx/dbUtil.h"
#include "rcx/extMeasureRC.h"
#include "rcx/extRCap.h"
#include "rcx/extSegment.h"
#include "rcx/grids.h"
#include "rcx/util.h"
#include "utl/Logger.h"

using odb::dbCapNode;
using odb::dbInst;
using odb::dbInstShapeItr;
using odb::dbITermShapeItr;
using odb::dbNet;
using odb::dbSet;
using odb::dbWirePath;
using odb::Rect;

namespace rcx {

void extMain::initRunEnv(extMeasureRC& m)
{
  m._extMain = this;
  m._block = _block;
  m._diagFlow = _diagFlow;

  m._resFactor = _resFactor;
  m._resModify = _resModify;
  m._ccFactor = _ccFactor;
  m._ccModify = _ccModify;
  m._gndcFactor = _gndcFactor;
  m._gndcModify = _gndcModify;

  m._dgContextArray = _dgContextArray;
  m._dgContextDepth = &_dgContextDepth;
  m._dgContextPlanes = &_dgContextPlanes;
  m._dgContextTracks = &_dgContextTracks;
  m._dgContextBaseLvl = &_dgContextBaseLvl;
  m._dgContextLowLvl = &_dgContextLowLvl;
  m._dgContextHiLvl = &_dgContextHiLvl;
  m._dgContextBaseTrack = _dgContextBaseTrack;
  m._dgContextLowTrack = _dgContextLowTrack;
  m._dgContextHiTrack = _dgContextHiTrack;
  m._dgContextTrackBase = _dgContextTrackBase;

  m._dgContextCnt = 0;

  m._ccContextArray = _ccContextArray;

  m._pixelTable = _geomSeq;
  m._minModelIndex = 0;  // couplimg threshold will be appled to this cap
  m._maxModelIndex = 0;
  m._currentModel = _currentModel;
  m._diagModel = _currentModel[0].getDiagModel();
  for (uint32_t ii = 0; ii < _modelMap.getCnt(); ii++) {
    uint32_t jj = _modelMap.get(ii);
    m._metRCTable.add(_currentModel->getMetRCTable(jj));
  }
  const uint32_t techLayerCnt = getExtLayerCnt(_tech) + 1;
  const uint32_t modelLayerCnt = _currentModel->getLayerCnt();
  m._layerCnt = techLayerCnt < modelLayerCnt ? techLayerCnt : modelLayerCnt;
  if (techLayerCnt == 5 && modelLayerCnt == 8) {
    m._layerCnt = modelLayerCnt;
  }
  m.getMinWidth(_tech);
  m.allocOUpool();

  m._debugFP = nullptr;
  m._netId = 0;
  uint32_t debugNetId = this->_debug_net_id;

  if (debugNetId > 0) {
    m._netId = debugNetId;
    char bufName[32];
    sprintf(bufName, "%d", debugNetId);
    m._debugFP = fopen(bufName, "w");
  }
}

void extMain::initializeLayerTables(LayerDimensionData& tables)
{
  for (uint32_t ii = 0; ii < 32; ii++) {
    tables.pitchTable[ii] = 0;
    tables.widthTable[ii] = 0;
  }
}
void extMain::setupBoundaries(BoundaryData& bounds, const Rect& extRect)
{
  bounds.ll[0] = extRect.xMin();
  bounds.ll[1] = extRect.yMin();
  bounds.ur[0] = extRect.xMax();
  bounds.ur[1] = extRect.yMax();
}
void extMain::updateBoundaries(BoundaryData& bounds,
                               uint32_t dir,
                               uint32_t ccDist,
                               uint32_t maxPitch)
{
  bounds.lo_gs[!dir] = bounds.ll[!dir];
  bounds.hi_gs[!dir] = bounds.ur[!dir];
  bounds.lo_search[!dir] = bounds.ll[!dir];
  bounds.hi_search[!dir] = bounds.ur[!dir];

  int hiXY = bounds.ur[dir] + 5 * ccDist * maxPitch;
  int gs_limit = bounds.ll[dir];

  bounds.lo_gs[dir] = gs_limit;
  bounds.hi_gs[dir] = hiXY;

  bounds.hi_search[dir] = hiXY;
  bounds.lo_search[dir] = bounds.ll[dir];
}

int extMain::initSearch(LayerDimensionData& tables,
                        Rect& extRect,
                        uint32_t& totWireCnt)
{
  int layerCnt = initSearchForNets(tables.baseX,
                                   tables.baseY,
                                   tables.pitchTable,
                                   tables.widthTable,
                                   tables.dirTable,
                                   extRect,
                                   false);

  layerCnt = std::max(layerCnt, _currentModel->getLayerCnt());

  uint32_t maxWidth = 0;
  uint32_t totPowerWireCnt = powerWireCounter(maxWidth);
  totWireCnt = signalWireCounter(maxWidth);
  totWireCnt += totPowerWireCnt;

  tables.maxWidth = maxWidth;

  logger_->info(utl::RCX, 43, "{} wires to be extracted", totWireCnt);

  return layerCnt;
}
void extMeasureRC::resetTrackIndices(uint32_t dir)
{
  for (uint32_t ii = 0; ii < _trackLevelCnt; ii++) {
    _lowTrackToExtract[dir][ii] = 0;
    _hiTrackToExtract[dir][ii] = 0;
    _lowTrackToFree[dir][ii] = 0;
    _hiTrackToFree[dir][ii] = 0;
    _lowTrackSearch[dir][ii] = 0;
    _hiTrackSearch[dir][ii] = 0;
  }
}
uint32_t extMain::couplingFlow_v2(Rect& extRect,
                                  uint32_t ccDist,
                                  extMeasure* m1)
{
  getPeakMemory("Start Coupling Flow: ");

  if (_ccContextDepth) {
    initContextArray();
  }
  // Wire Tables for Diagonal Coupling for v1 modeling
  initDgContextArray();

  extMeasureRC* mrc = new extMeasureRC(logger_);
  initRunEnv(*mrc);

  // Setup boundaries and steps
  BoundaryData bounds;
  setupBoundaries(bounds, extRect);

  // Get Width and Pitch for all  layers
  LayerDimensionData tables;
  initializeLayerTables(tables);

  uint32_t totWireCnt;
  int layerCnt = initSearch(tables, extRect, totWireCnt);
  _search->setV2(_v2);

  setExtControl_v2(mrc->_seqPool);
  _seqPool = mrc->_seqPool;

  uint32_t maxPitch = tables.pitchTable[layerCnt - 1];

  // TODO mrc->_progressTracker =
  // std::make_unique<ExtProgressTracker>(totWireCnt);

  mrc->_seqmentPool = new AthPool<extSegment>(1024);

  uint32_t totalWiresExtracted = 0;
  float previous_percent_extracted = 0.0;
  for (int dir = 1; dir >= 0; dir--) {  // dir==1 Horizontal wires

    if (dir == 0) {
      enableRotatedFlag();
    }
    updateBoundaries(bounds, dir, ccDist, maxPitch);
    _search->initCouplingCapLoops_v2(dir, ccDist);

    // Add all shapes on a compressed bit-based structure for quick cross
    // overlap calculation
    fill_gs4(dir,
             bounds.ll,
             bounds.ur,
             bounds.lo_gs,
             bounds.hi_gs,
             layerCnt,
             tables.dirTable,
             tables.pitchTable,
             tables.widthTable);

    mrc->_rotatedGs = getRotatedFlag();
    mrc->_pixelTable = _geomSeq;
    getPeakMemory("End fill_gs4 Dir: ", dir);

    // Add all shapes on a fast track based structure for quick wire coupling
    // detection
    addPowerNets(dir, bounds.lo_search, bounds.hi_search, 11);  // pwrtype = 11
    addSignalNets(dir, bounds.lo_search, bounds.hi_search, 9);  // sigtype = 9

    getPeakMemory("End WiresOnSearch Dir: ", dir);

    mrc->_search = this->_search.get();

    // Create single lists of wires on every track/level/direction
    mrc->ConnectWires(dir);

    // Find immediate coupling neighbor wires in all directions and levels
    mrc->FindCouplingNeighbors(dir, 10, 5);

    mrc->CouplingFlow(dir,
                      10,
                      5,
                      totWireCnt,
                      totalWiresExtracted,
                      previous_percent_extracted);

    float tmpCnt = -10;
    mrc->printProgress(totalWiresExtracted, totWireCnt, tmpCnt);
    getPeakMemory("End CouplingFlow Dir:", dir);
  }
  if (_geomSeq != nullptr) {
    delete _geomSeq;
    _geomSeq = nullptr;
  }
  // delete wire tables  used during diagonal coupling in v1 modeling
  removeDgContextArray();

  delete mrc;
  getPeakMemory("Exit CouplingFlow ----------------------------------------- ");

  return 0;
}
uint32_t extMain::couplingFlow_v2_opt(Rect& extRect,
                                      uint32_t ccDist,
                                      extMeasure* m1)
{
  ccDist
      = 10;  // TODO -- test for different  values and adjust regression tests

  if (_ccContextDepth) {
    initContextArray();
  }
  // Wire Tables for Diagonal Coupling for v1 modeling
  initDgContextArray();

  extMeasureRC* mrc = new extMeasureRC(logger_);
  initRunEnv(*mrc);
  mrc->resetTrackIndices(0);
  mrc->resetTrackIndices(1);

  // Get Width and Pitch for all  layers
  LayerDimensionData tables;
  initializeLayerTables(tables);

  uint32_t totWireCnt;
  int layerCnt = initSearch(tables, extRect, totWireCnt);
  _search->setV2(_v2);

  setExtControl_v2(mrc->_seqPool);
  _seqPool = mrc->_seqPool;

  // Setup boundaries and extraction steps
  BoundaryData bounds;
  bounds.setBBox(extRect);
  bounds.init(ccDist,
              5,
              tables.pitchTable[1],
              tables.pitchTable[layerCnt - 1],
              tables.maxWidth,
              1000);

  mrc->_seqmentPool = new AthPool<extSegment>(1024);

  uint32_t totalWiresExtracted = 0;
  float previous_percent_extracted = 0.0;

  // TODO mrc->_progressTracker =
  // std::make_unique<ExtProgressTracker>(totWireCnt);

  for (int dir = 1; dir >= 0; dir--)  // dir==1 Horizontal wires
  {
    if (dir == 0) {
      enableRotatedFlag();
    }
    mrc->resetTrackIndices(dir);
    // TODO -- need it?
    _search->initCouplingCapLoops_v2(dir, ccDist);

    bounds.setBBox(extRect);
    while (true)  // Extraction Iteration Loop
    {
      bool lastIteration = bounds.update(dir);

      // Add all shapes on a compressed bit-based structure for quick cross
      // overlap calculation
      fill_gs4(dir,
               bounds.ll,
               bounds.ur,
               bounds.lo_gs,
               bounds.hi_gs,
               layerCnt,
               tables.dirTable,
               tables.pitchTable,
               tables.widthTable);

      // TODO move up
      mrc->_rotatedGs = getRotatedFlag();
      mrc->_pixelTable = _geomSeq;

      // Add all shapes on a fast track based structure for quick wire
      // coupling detection
      addPowerNets(
          dir, bounds.lo_search, bounds.hi_search, 11);  // pwrtype = 11
      addSignalNets(dir, bounds.lo_search, bounds.hi_search, 9);  // sigtype = 9

      // TODO move up
      mrc->_search = this->_search.get();

      // Create single lists of wires on every track/level/direction
      // Set Boundaries for all tracks
      mrc->ConnectWires(dir, bounds);

      // Find immediate coupling neighbor wires in all directions and levels
      // mrc->FindCouplingNeighbors(dir, 10, 5);
      mrc->FindCouplingNeighbors(dir, bounds);

      // Lateral and diagonal coupling
      mrc->CouplingFlow_opt(dir,
                            bounds,
                            totWireCnt,
                            totalWiresExtracted,
                            previous_percent_extracted);
      float tmpCnt = -10;
      mrc->printProgress(totalWiresExtracted, totWireCnt, tmpCnt);

      _search->dealloc(dir, bounds.releaseMemoryLimitXY);
      if (lastIteration) {
        break;
      }
    }
  }
  if (_geomSeq != nullptr) {
    delete _geomSeq;
    _geomSeq = nullptr;
  }
  // delete wire tables  used during diagonal coupling in v1 modeling
  removeDgContextArray();
  delete mrc->_seqmentPool;
  mrc->_seqmentPool = nullptr;

  delete mrc;
  return 0;
}

void extMain::setExtControl_v2(AthPool<SEQ>* seqPool)
{
  OverlapAdjust overlapAdj = Z_noAdjust;
  _useDbSdb = true;
  _search->setExtControl_v2(_block,
                            _useDbSdb,
                            (uint32_t) overlapAdj,
                            _ccNoPowerSource,
                            _ccNoPowerTarget,
                            _ccUp,
                            _allNet,
                            _ccContextDepth,
                            _ccContextArray,
                            _ccContextLength,
                            _dgContextArray,
                            &_dgContextDepth,
                            &_dgContextPlanes,
                            &_dgContextTracks,
                            &_dgContextBaseLvl,
                            &_dgContextLowLvl,
                            &_dgContextHiLvl,
                            _dgContextBaseTrack,
                            _dgContextLowTrack,
                            _dgContextHiTrack,
                            _dgContextTrackBase,
                            seqPool);
}

void extMain::printUpdateCoup(uint32_t netId1,
                              uint32_t netId2,
                              double v,
                              double org,
                              double totCC)
{
  if (netId1 == _debug_net_id || netId2 == _debug_net_id) {
    fprintf(stdout,
            "updateCoupCap: Nets %d %d -- add_cc= %10.6f  org  %10.6f  totCC  "
            "%10.6f\n",
            netId1,
            netId2,
            org,
            v,
            totCC);
  }
}
bool extMeasure::IsDebugNet1()
{
  if (_no_debug) {
    return false;
  }

  if (!(_extMain->_debug_net_id > 0)) {
    return false;
  }

  return _netSrcId == _extMain->_debug_net_id
         || _netTgtId == _extMain->_debug_net_id;
}
void GridTable::initCouplingCapLoops_v2(uint32_t dir,
                                        uint32_t couplingDist,
                                        int* startXY)
{
  setCCFlag(couplingDist);

  for (uint32_t jj = 1; jj < _colCnt; jj++) {
    Grid* netGrid = _gridTable[dir][jj];
    if (netGrid == nullptr) {
      continue;
    }

    if (startXY == nullptr) {
      netGrid->initCouplingCapLoops_v2(couplingDist);
    } else {
      netGrid->initCouplingCapLoops_v2(couplingDist, false, startXY[jj]);
    }
  }
}
int Grid::initCouplingCapLoops_v2(uint32_t couplingDist,
                                  bool startSearchTrack,
                                  int startXY)
{
  uint32_t TargetHighMarkedNet = _gridtable->targetHighMarkedNet();
  bool allNet = _gridtable->allNet();

  uint32_t domainAdjust = allNet || !TargetHighMarkedNet ? 0 : couplingDist;

  initContextGrids();
  setSearchDomain(domainAdjust);
  if (startSearchTrack) {
    _currentTrack = _searchLowTrack;
  } else {
    _currentTrack = (startXY - _base) / _pitch;
  }
  _lastFreeTrack = 0;

  return _base + _pitch * _searchHiTrack;
}
uint32_t Wire::getLevel()
{
  return this->_track->getGrid()->getLevel();
}
uint32_t Wire::getPitch()
{
  return this->_track->getGrid()->getPitch();
}
uint32_t Grid::placeWire_v2(SearchBox* bb)
{
  uint32_t d = !_dir;

  int xy1 = bb->loXY(d);

  int ll[2] = {bb->loXY(0), bb->loXY(1)};
  int ur[2] = {bb->hiXY(0), bb->hiXY(1)};

  uint32_t m1 = getBucketNum(xy1);

#ifdef SINGLE_WIRE
  uint32_t width = bb->hiXY(_dir) - bb->loXY(_dir);
  uint32_t trackNum1 = getMinMaxTrackNum((bb->loXY(_dir) + bb->loXY(_dir)) / 2);
  uint32_t trackNum2 = trackNum1;
  if (width > _pitch) {
    trackNum2 = getMinMaxTrackNum(bb->hiXY(_dir));
  }
  // ** wire base is not always at track base
#else

  // ---------------------------------------------------
  // DELETE --- v2= true  bool NO_SUB_TRACKS= getGridTable()->_no_sub_tracks; //
  // old flow=false
  uint32_t trackNum1 = getMinMaxTrackNum(bb->loXY(_dir));
  // uint32_t trackNum2 = trackNum1;
  // DELETE if NO_SUB_TRACKS=true (!NO_SUB_TRACKS)
  //  trackNum2 = getMinMaxTrackNum(bb->hiXY(_dir));

#endif

  uint32_t wireType = bb->getType();

  Wire* w
      = makeWire(_dir, ll, ur, bb->getOwnerId(), bb->getOtherId(), wireType);
  Track* track = getTrackPtr(trackNum1, _markerCnt);
  /* DELETE
    int TTTsubt = NO_SUB_TRACKS ? 0 : 1;
    if (TTTsubt>0)
      track = getTrackPtr(trackNum1, _markerCnt, w->_base);
    else
      track = getTrackPtr(trackNum1, _markerCnt);
  */
  // track->place2(w, m1, m2);
  track->place(w, m1);
  /* DELETE
  uint32_t wCnt = 1;
  for (uint32_t ii = trackNum1 + 1; ii <= trackNum2; ii++) {
    Wire* w1 = makeWire(w, wireType);
    w1->_srcId = w->_id;
    w1->_srcWire = w;
    _gridtable->incrMultiTrackWireCnt(w->isPower());
    Track* track = getTrackPtr(ii, _markerCnt);
    track->place(w1, m1);
    wCnt++;
  }
*/
  return trackNum1;
}
extDistRC* extMeasureRC::getDiagUnderCC(extMetRCTable* rcModel,
                                        uint32_t dist,
                                        uint32_t overMet)
{
  if (rcModel->_capDiagUnder[_met] == nullptr) {
    return nullptr;
  }

  uint32_t n = getUnderIndex(overMet);
  extDistRC* rc = rcModel->_capDiagUnder[_met]->getRC(n, _width, dist);
  return rc;
}

/* CHRECK if called in v2 flow
void extMeasure::OverSubRC(dbRSeg * rseg1,
                             dbRSeg * rseg2,
                             int ouCovered,
                             int diagCovered,
                             int srcCovered)
  {
    int res_lenOverSub = _len - ouCovered;  // 0228
    res_lenOverSub = 0;                     // 0315 -- new calc
    bool SCALING_RES = false;

    double SUB_MULT_CAP
        = 1.0;  // Open ended resitance should account by 1/4 -- 11/15

    double SUB_MULT_RES = 1.0;
    if (SCALING_RES) {
      double dist_track = 0.0;
      SUB_MULT_RES = ScaleResbyTrack(true, dist_track);
      res_lenOverSub = _len;
    }
    int lenOverSub = _len - ouCovered;
    if (lenOverSub < 0)
      lenOverSub = 0;

    bool rvia1 = rseg1 != nullptr && isVia(rseg1->getId());

    if (!((lenOverSub > 0) || (res_lenOverSub > 0))) {
      return;
    }

    _underMet = 0;
    for (uint32_t jj = 0; jj < _metRCTable.getCnt(); jj++) {
      extDistRC* rc = _metRCTable.get(jj)->getOverFringeRC(this);
      if (rc == nullptr)
        continue;
      double cap = 0;
      if (lenOverSub > 0) {
        cap = SUB_MULT_CAP * rc->getFringe() * lenOverSub;
        _extMain->updateTotalCap(rseg1, cap, jj);
      }
      double res = 0;
      if (!_extMain->_lef_res && !rvia1) {
        if (res_lenOverSub > 0) {
          extDistRC* rc0 = _metRCTable.get(jj)->getOverFringeRC(this, 0);
          extDistRC* rc_last
              = _metRCTable.get(jj)->getOverFringeRC_last(_met, _width);
          double delta0 = rc0->_res - rc_last->_res;
          if (delta0 < 0)
            delta0 = -delta0;
          if (delta0 < 0.000001)
            SUB_MULT_RES = 1.0;
          // if (lenOverSub>0) {
          res = rc->getRes() * res_lenOverSub;
          res *= SUB_MULT_RES;
          _extMain->updateRes(rseg1, res, jj);
        }
      }
      const char* msg = "OverSubRC (No Neighbor)";
      OverSubDebug(rc, lenOverSub, res_lenOverSub, res, cap, msg);
    }
  }

*/

void extMain::setBranchCapNodeId(dbNet* net, uint32_t junction)
{
  int capId = _nodeTable->geti(junction);
  if (capId != 0) {
    return;
  }

  dbCapNode* cap = dbCapNode::create(net, 0, _foreign);

  cap->setInternalFlag();
  cap->setBranchFlag();

  cap->setNode(junction);

  capId = cap->getId();

  _nodeTable->set(junction, -capId);
}

void extMain::markPathHeadTerm(dbWirePath& path)
{
  if (path.bterm) {
    _connectedBTerm.push_back(path.bterm);
    path.bterm->setMark(1);
  } else if (path.iterm) {
    _connectedITerm.push_back(path.iterm);
    path.iterm->setMark(1);
  }
}
bool extRCModel::isRulesFile_v2(char* name, bool bin)
{
  bool res_over = false;
  bool Over = false;
  bool Under = false;
  bool OverUnder = false;
  bool diag_under = false;
  bool over0 = false;
  bool over1 = false;
  bool under0 = false;
  bool under1 = false;
  bool overunder0 = false;
  bool overunder1 = false;

  bool via_res = false;

  spotModelsInRules(name,
                    bin,
                    res_over,
                    Over,
                    Under,
                    OverUnder,
                    diag_under,
                    over0,
                    over1,
                    under0,
                    under1,
                    overunder0,
                    overunder1,
                    via_res);
  bool ret = over0 || over1 || under0 || under1 || overunder0 || overunder1
             || via_res;
  return ret;
}
bool extRCModel::spotModelsInRules(char* name,
                                   bool bin,
                                   bool& res_over,
                                   bool& over,
                                   bool& under,
                                   bool& overUnder,
                                   bool& diag_under,
                                   bool& over0,
                                   bool& over1,
                                   bool& under0,
                                   bool& under1,
                                   bool& overunder0,
                                   bool& overunder1,
                                   bool& via_res)
{
  free(_ruleFileName);
  _ruleFileName = strdup(name);
  Parser parser(logger_);
  // parser.setDbg(1);
  parser.addSeparator("\r");
  parser.openFile(name);
  while (parser.parseNextLine() > 0) {
    if (parser.getWordCnt() == 3 && parser.isKeyword(0, "Metal")) {
      // DBG int met= parser.getInt(1);
      // if (met==3)
      //    break;

      if (parser.isKeyword(2, "RESOVER")) {
        res_over = true;
      } else if (parser.isKeyword(2, "OVER")) {
        over = true;
      } else if (parser.isKeyword(2, "UNDER")) {
        under = true;
      } else if (parser.isKeyword(2, "DIAGUNDER")) {
        diag_under = true;
      } else if (parser.isKeyword(2, "OVERUNDER")) {
        overUnder = true;
      } else if (parser.isKeyword(2, "OVER1")) {
        over1 = true;
      } else if (parser.isKeyword(2, "OVER0")) {
        over0 = true;
      } else if (parser.isKeyword(2, "UNDER1")) {
        under1 = true;
      } else if (parser.isKeyword(2, "UNDER0")) {
        under0 = true;
      } else if (parser.isKeyword(2, "OVERUNDER1")) {
        overunder1 = true;
      } else if (parser.isKeyword(2, "OVERUNDER0")) {
        overunder0 = true;
      }
    } else if (parser.isKeyword(0, "VIARES")) {
      via_res = true;
    }
  }
  return true;
}
bool extRCModel::readRules(char* name,
                           bool bin,
                           bool over,
                           bool under,
                           bool overUnder,
                           bool diag,
                           uint32_t cornerCnt,
                           const uint32_t* cornerTable,
                           double dbFactor)
{
  bool res_over = false;
  // DELETE bool exclude_res_over= true;
  bool Over = false;
  bool Under = false;
  bool OverUnder = false;
  bool diag_under = false;
  bool over0 = false;
  bool over1 = false;
  bool under0 = false;
  bool under1 = false;
  bool overunder0 = false;
  bool overunder1 = false;

  bool via_res = false;

  spotModelsInRules(name,
                    bin,
                    res_over,
                    Over,
                    Under,
                    OverUnder,
                    diag_under,
                    over0,
                    over1,
                    under0,
                    under1,
                    overunder0,
                    overunder1,
                    via_res);

  diag = false;
  free(_ruleFileName);
  _ruleFileName = strdup(name);
  Parser parser(logger_);
  // parser.setDbg(1);
  parser.addSeparator("\r");
  parser.openFile(name);
  while (parser.parseNextLine() > 0) {
    if (parser.isKeyword(0, "OUREVERSEORDER")) {
    }
    if (parser.isKeyword(0, "DIAGMODEL")) {
      if (strcmp(parser.get(1), "ON") == 0) {
        _diagModel = 1;
        diag = true;
      } else if (strcmp(parser.get(1), "TRUE") == 0) {
        _diagModel = 2;
        diag = true;
      }
      continue;
    }
    if (parser.isKeyword(0, "Layer")) {
      _layerCnt = parser.getInt(2);
      continue;
    }
    if (parser.isKeyword(0, "LayerCount")) {
      _layerCnt = parser.getInt(1) + 1;
      _verticalDiag = true;
      continue;
    }
    if (parser.isKeyword(0, "DensityRate")) {
      uint32_t rulesFileModelCnt = parser.getInt(1);
      if (cornerCnt > 0) {
        if ((rulesFileModelCnt > 0) && (rulesFileModelCnt < cornerCnt)) {
          logger_->warn(
              utl::RCX,
              226,
              "There were {} extraction models defined but only {} exists "
              "in the extraction rules file {}",
              cornerCnt,
              rulesFileModelCnt,
              name);
          return false;
        }
        // createModelTable(cornerCnt, _layerCnt);
        createModelTable(rulesFileModelCnt, _layerCnt);

        for (uint32_t jj = 0; jj < cornerCnt; jj++) {
          uint32_t modelIndex = cornerTable[jj];

          uint32_t kk;
          for (kk = 0; kk < rulesFileModelCnt; kk++) {
            if (modelIndex != kk) {
              continue;
            }
            _dataRateTable->add(parser.getDouble(kk + 2));
            break;
          }
          if (kk == rulesFileModelCnt) {
            logger_->warn(utl::RCX,
                          228,
                          "Cannot find model index {} in extRules file {}",
                          modelIndex,
                          name);
            return false;
          }
        }
      }
      continue;
    }
    // parser.setDbg(1);

    if (parser.isKeyword(0, "DensityModel")) {
      uint32_t m = parser.getInt(1);
      uint32_t modelIndex = m;
      bool skipModel = false;
      /*
      if (cornerCnt > 0) {
        uint32_t jj = 0;
        for (; jj < cornerCnt; jj++) {
          if (m == cornerTable[jj])
            break;
        }
        if (jj == cornerCnt) {
          skipModel = true;
          modelIndex = 0;
        } else {
          skipModel = false;
          modelIndex = jj;
        }
      } else {  // david 7.20
        if (modelIndex)
          skipModel = true;
      }
      */
      // skipModel= true;
      bool res_skipModel = false;

      for (uint32_t ii = 1; ii < _layerCnt; ii++) {
        if (res_over) {
          readRules_v2(&parser,
                       modelIndex,
                       ii,
                       "RESOVER",
                       "WIDTH",
                       over,
                       false,
                       bin,
                       false,
                       res_skipModel,
                       dbFactor);
        }
        readRules_v2(&parser,
                     modelIndex,
                     ii,
                     "OVER",
                     "WIDTH",
                     over,
                     false,
                     bin,
                     false,
                     skipModel,
                     dbFactor);
        if (over0) {
          readRules_v2(&parser,
                       modelIndex,
                       ii,
                       "OVER0",
                       "WIDTH",
                       over,
                       false,
                       bin,
                       false,
                       skipModel,
                       dbFactor);
        }
        if (over1) {
          readRules_v2(&parser,
                       modelIndex,
                       ii,
                       "OVER1",
                       "WIDTH",
                       over,
                       false,
                       bin,
                       false,
                       skipModel,
                       dbFactor);
        }

        if (ii < _layerCnt - 1) {
          readRules_v2(&parser,
                       modelIndex,
                       ii,
                       "UNDER",
                       "WIDTH",
                       false,
                       under,
                       bin,
                       false,
                       skipModel,
                       dbFactor);
          if (under0) {
            readRules_v2(&parser,
                         modelIndex,
                         ii,
                         "UNDER0",
                         "WIDTH",
                         false,
                         under,
                         bin,
                         false,
                         skipModel,
                         dbFactor);
          }
          if (under1) {
            readRules_v2(&parser,
                         modelIndex,
                         ii,
                         "UNDER1",
                         "WIDTH",
                         false,
                         under,
                         bin,
                         false,
                         skipModel,
                         dbFactor);
          }
          if (diag) {
            readRules_v2(&parser,
                         modelIndex,
                         ii,
                         "DIAGUNDER",
                         "WIDTH",
                         false,
                         false,
                         bin,
                         diag,
                         skipModel,
                         dbFactor);
          }
        }
        if ((ii > 1) && (ii < _layerCnt - 1)) {
          readRules_v2(&parser,
                       modelIndex,
                       ii,
                       "OVERUNDER",
                       "WIDTH",
                       overUnder,
                       overUnder,
                       bin,
                       false,
                       skipModel,
                       dbFactor);
          if (overunder0) {
            readRules_v2(&parser,
                         modelIndex,
                         ii,
                         "OVERUNDER0",
                         "WIDTH",
                         overUnder,
                         overUnder,
                         bin,
                         false,
                         skipModel,
                         dbFactor);
          }
          if (overunder1) {
            readRules_v2(&parser,
                         modelIndex,
                         ii,
                         "OVERUNDER1",
                         "WIDTH",
                         overUnder,
                         overUnder,
                         bin,
                         false,
                         skipModel,
                         dbFactor);
          }
        }
      }

      if (!via_res) {
        continue;
      }

      while (parser.parseNextLine()) {
        // if (parser.isKeyword(0, "END") && parser.isKeyword(1,
        // "DensityModel"))
        //  break;

        if (parser.isKeyword(0, "VIARES")) {
          _modelTable[modelIndex]->ReadRules(&parser);
          break;
        }
      }
    }
  }
  return true;
}
bool extRCModel::readRules_v2(char* name,
                              bool bin,
                              bool over,
                              bool under,
                              bool overUnder,
                              bool diag,
                              double dbFactor)
{
  // clean v2 flow

  bool res_over = false;
  bool Over = false;
  bool Under = false;
  bool OverUnder = false;
  bool diag_under = false;
  bool over0 = false;
  bool over1 = false;
  bool under0 = false;
  bool under1 = false;
  bool overunder0 = false;
  bool overunder1 = false;

  bool via_res = false;

  spotModelsInRules(name,
                    bin,
                    res_over,
                    Over,
                    Under,
                    OverUnder,
                    diag_under,
                    over0,
                    over1,
                    under0,
                    under1,
                    overunder0,
                    overunder1,
                    via_res);

  diag = false;
  free(_ruleFileName);
  _ruleFileName = strdup(name);
  Parser parser(logger_);
  // parser.setDbg(1);
  parser.addSeparator("\r");
  parser.openFile(name);
  while (parser.parseNextLine() > 0) {
    if (parser.isKeyword(0, "OUREVERSEORDER")) {
    }
    if (parser.isKeyword(0, "DIAGMODEL")) {
      if (strcmp(parser.get(1), "ON") == 0) {
        _diagModel = 1;
        diag = true;
      } else if (strcmp(parser.get(1), "TRUE") == 0) {
        _diagModel = 2;
        diag = true;
      }
      continue;
    }
    if (parser.isKeyword(0, "Layer")) {
      _layerCnt = parser.getInt(2);
      continue;
    }
    if (parser.isKeyword(0, "LayerCount")) {
      _layerCnt = parser.getInt(1) + 1;
      _verticalDiag = true;
      continue;
    }
    if (parser.isKeyword(0, "DensityRate")) {
      uint32_t rulesFileModelCnt = parser.getInt(1);
      // _modelTable holds process corners
      createModelTable(rulesFileModelCnt, _layerCnt);
      continue;
    }
    // Density Model is equivalent to Process Corner
    if (parser.isKeyword(0, "DensityModel")) {
      uint32_t m = parser.getInt(1);
      uint32_t modelIndex = m;
      bool skipModel = false;
      bool res_skipModel = false;

      // Loop to read all sections of the Model file per Metal Level
      for (uint32_t ii = 1; ii < _layerCnt; ii++) {
        if (res_over) {
          readRules_v2(&parser,
                       modelIndex,
                       ii,
                       "RESOVER",
                       "WIDTH",
                       over,
                       false,
                       bin,
                       false,
                       res_skipModel,
                       dbFactor);
        }
        readRules_v2(&parser,
                     modelIndex,
                     ii,
                     "OVER",
                     "WIDTH",
                     over,
                     false,
                     bin,
                     false,
                     skipModel,
                     dbFactor);
        if (over0) {
          readRules_v2(&parser,
                       modelIndex,
                       ii,
                       "OVER0",
                       "WIDTH",
                       over,
                       false,
                       bin,
                       false,
                       skipModel,
                       dbFactor);
        }
        if (over1) {
          readRules_v2(&parser,
                       modelIndex,
                       ii,
                       "OVER1",
                       "WIDTH",
                       over,
                       false,
                       bin,
                       false,
                       skipModel,
                       dbFactor);
        }

        if (ii < _layerCnt - 1) {
          readRules_v2(&parser,
                       modelIndex,
                       ii,
                       "UNDER",
                       "WIDTH",
                       false,
                       under,
                       bin,
                       false,
                       skipModel,
                       dbFactor);
          if (under0) {
            readRules_v2(&parser,
                         modelIndex,
                         ii,
                         "UNDER0",
                         "WIDTH",
                         false,
                         under,
                         bin,
                         false,
                         skipModel,
                         dbFactor);
          }
          if (under1) {
            readRules_v2(&parser,
                         modelIndex,
                         ii,
                         "UNDER1",
                         "WIDTH",
                         false,
                         under,
                         bin,
                         false,
                         skipModel,
                         dbFactor);
          }
          if (diag) {
            readRules_v2(&parser,
                         modelIndex,
                         ii,
                         "DIAGUNDER",
                         "WIDTH",
                         false,
                         false,
                         bin,
                         diag,
                         skipModel,
                         dbFactor);
          }
        }
        if ((ii > 1) && (ii < _layerCnt - 1)) {
          readRules_v2(&parser,
                       modelIndex,
                       ii,
                       "OVERUNDER",
                       "WIDTH",
                       overUnder,
                       overUnder,
                       bin,
                       false,
                       skipModel,
                       dbFactor);
          if (overunder0) {
            readRules_v2(&parser,
                         modelIndex,
                         ii,
                         "OVERUNDER0",
                         "WIDTH",
                         overUnder,
                         overUnder,
                         bin,
                         false,
                         skipModel,
                         dbFactor);
          }
          if (overunder1) {
            readRules_v2(&parser,
                         modelIndex,
                         ii,
                         "OVERUNDER1",
                         "WIDTH",
                         overUnder,
                         overUnder,
                         bin,
                         false,
                         skipModel,
                         dbFactor);
          }
        }
      }
      // v1 flow can only handle single process corners and NO Via modeling
      if (!_v2_flow) {  // v1 flow can only handle one corner
        break;
      }

      if (!via_res) {
        continue;
      }

      while (parser.parseNextLine()) {
        if (parser.isKeyword(0, "VIARES")) {
          _modelTable[modelIndex]->ReadRules(&parser);
          break;
        }
      }
    }
  }
  return true;
}

uint32_t extRCModel::readRules_v2(Parser* parser,
                                  uint32_t m,
                                  uint32_t ii,
                                  const char* ouKey,
                                  const char* wKey,
                                  bool over,
                                  bool under,
                                  bool bin,
                                  bool diag,
                                  bool ignore,
                                  double dbFactor)
{
  uint32_t cnt = 0;
  uint32_t met = 0;
  Array1D<double>* wTable
      = readHeaderAndWidth(parser, met, ouKey, wKey, bin, false);

  if (wTable == nullptr) {
    return 0;
  }

  uint32_t widthCnt = wTable->getCnt();

  extDistWidthRCTable* dummy = nullptr;
  if (ignore) {
    dummy = new extDistWidthRCTable(
        true, met, _layerCnt, widthCnt, OUReverseOrder_);
  }

  uint32_t diagWidthCnt = 0;
  uint32_t diagDistCnt = 0;

  if (diag && strcmp(ouKey, "DIAGUNDER") == 0 && _diagModel == 2) {
    parser->parseNextLine();
    if (parser->isKeyword(0, "DIAG_WIDTH")) {
      diagWidthCnt = parser->getInt(3);
    }
    parser->parseNextLine();
    if (parser->isKeyword(0, "DIAG_DIST")) {
      diagDistCnt = parser->getInt(3);
    }
  }
  if (over && under && (met > 1)) {
    if (!ignore) {
      if (strcmp(ouKey, "OVERUNDER") == 0) {
        _modelTable[m]->allocOverUnderTable(met, false, wTable, dbFactor);
        _modelTable[m]->_capOverUnder[met]->readRulesOverUnder(
            parser, widthCnt, bin, ignore, dbFactor);
      } else if (strcmp(ouKey, "OVERUNDER0") == 0) {
        _modelTable[m]->allocOverUnderTable(met, true, wTable, dbFactor);
        _modelTable[m]->_capOverUnder_open[met][0]->readRulesOverUnder(
            parser, widthCnt, bin, ignore, dbFactor);
      } else if (strcmp(ouKey, "OVERUNDER1") == 0) {
        _modelTable[m]->_capOverUnder_open[met][1]->readRulesOverUnder(
            parser, widthCnt, bin, ignore, dbFactor);
      }
    } else {
      dummy->readRulesOverUnder(parser, widthCnt, bin, ignore, dbFactor);
    }
  } else if (over) {
    if (strcmp(ouKey, "OVER") == 0) {
      _modelTable[m]->_capOver[met]->readRulesOver(
          parser, widthCnt, bin, ignore, "OVER", dbFactor);
    } else if (strcmp(ouKey, "OVER0") == 0) {
      _modelTable[m]->_capOver_open[met][0]->readRulesOver(
          parser, widthCnt, bin, ignore, "OVER0", dbFactor);
    } else if (strcmp(ouKey, "OVER1") == 0) {
      _modelTable[m]->_capOver_open[met][1]->readRulesOver(
          parser, widthCnt, bin, ignore, "OVER1", dbFactor);
    } else {  // RESOVER ---- first in rules
      _modelTable[m]->allocOverTable(
          met, wTable, dbFactor);  // TODO: remove assumption -- RESOVER first
      _modelTable[m]->_resOver[met]->readRulesOver(
          parser, widthCnt, bin, ignore, "RESOVER", dbFactor);
    }
  } else if (under) {
    if (!ignore) {
      if (strcmp(ouKey, "UNDER") == 0) {
        _modelTable[m]->allocUnderTable(met, false, wTable, dbFactor);
        _modelTable[m]->_capUnder[met]->readRulesUnder(
            parser, widthCnt, bin, ignore, "UNDER", dbFactor);
      } else if (strcmp(ouKey, "UNDER0") == 0) {
        _modelTable[m]->allocUnderTable(
            met, true, wTable, dbFactor);  // should be before UNDER1
        _modelTable[m]->_capUnder_open[met][0]->readRulesUnder(
            parser, widthCnt, bin, ignore, "UNDER0", dbFactor);
      } else if (strcmp(ouKey, "UNDER1") == 0) {
        _modelTable[m]->_capUnder_open[met][1]->readRulesUnder(
            parser, widthCnt, bin, ignore, "UNDER1", dbFactor);
      }
    } else {
      dummy->readRulesUnder(
          parser, widthCnt, bin, ignore, "OPENUNDER", dbFactor);
    }
  } else if (diag) {
    if (!ignore && _diagModel == 2) {
      _modelTable[m]->allocDiagUnderTable(
          met, wTable, diagWidthCnt, diagDistCnt, dbFactor);
      _modelTable[m]->_capDiagUnder[met]->readRulesDiagUnder(
          parser, widthCnt, diagWidthCnt, diagDistCnt, bin, ignore, dbFactor);
    } else if (!ignore && _diagModel == 1) {
      _modelTable[m]->allocDiagUnderTable(met, wTable, dbFactor);
      _modelTable[m]->_capDiagUnder[met]->readRulesDiagUnder(
          parser, widthCnt, bin, ignore, dbFactor);
    } else if (ignore) {
      if (_diagModel == 2) {
        dummy->readRulesDiagUnder(
            parser, widthCnt, diagWidthCnt, diagDistCnt, bin, ignore, dbFactor);
      } else if (_diagModel == 1) {
        dummy->readRulesDiagUnder(parser, widthCnt, bin, ignore, dbFactor);
      }
    }
  }
  if (ignore) {
    delete dummy;
  }

  delete wTable;

  return cnt;
}
uint32_t extDistWidthRCTable::readRulesUnder(Parser* parser,
                                             uint32_t widthCnt,
                                             bool bin,
                                             bool ignore,
                                             const char* keyword,
                                             double dbFactor)
{
  uint32_t cnt = 0;
  for (uint32_t ii = _met + 1; ii < _layerCnt; ii++) {
    uint32_t met = 0;
    if (readMetalHeader(parser, met, keyword, bin, ignore) <= 0) {
      return 0;
    }

    uint32_t metIndex = getMetIndexUnder(ii);
    if (ignore) {
      metIndex = 0;
    }

    parser->getInt(3);

    for (uint32_t jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[metIndex][jj]->readRules(
          parser, _rcPoolPtr, true, bin, ignore, dbFactor);
    }
  }
  return cnt;
}
uint32_t extRCModel::calcMinMaxRC(odb::dbTech* tech, const char* out_file)
{
  dbSet<odb::dbTechLayer> layers = tech->getLayers();
  dbSet<odb::dbTechLayer>::iterator itr;

  FILE* fp = openFile(out_file, "", "", "w");
  uint32_t cnt = 0;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    odb::dbTechLayer* layer = *itr;

    if (layer->getRoutingLevel() == 0) {
      continue;
    }

    cnt++;

    int met = layer->getRoutingLevel();
    int width = layer->getWidth();
    int dist = layer->getSpacing();
    if (dist == 0) {
      dist = layer->getPitch() - layer->getWidth();
    }

    for (uint32_t jj = 0; jj < _modelCnt; jj++) {
      extMetRCTable* corner_model = _modelTable[jj];
      extDistWidthRCTable* rcTable = corner_model->_capOver[met];

      extDistRC* res_rc_min
          = corner_model->_resOver[met]->getRes(0, width, 0, 0);
      extDistRC* res_rc_max
          = corner_model->_resOver[met]->getRes(0, width, dist, dist);

      extDistRC* rcMin = rcTable->getFringeRC(0, width);

      int underMet = met - 1;
      int overMet = met + 1;

      extDistRC* rcMax = nullptr;
      if (met == _layerCnt - 1) {  // over
        overMet = 0;
        rcMax = corner_model->_capOver[met]->getFringeRC(underMet, width);
      } else if (met == 1) {  // over
        uint32_t n = overMet - met - 1;
        rcMax = corner_model->_capUnder[met]->getRC(n, width, dist);
      } else {
        uint32_t maxOverUnderIndex = corner_model->_capOverUnder[met]->_metCnt;
        uint32_t n = extRCModel::getMetIndexOverUnder(
            met, underMet, overMet, _layerCnt, maxOverUnderIndex);
        rcMax = corner_model->_capOverUnder[met]->getRC(n, width, dist);
      }
      // rcMin->printBound(stdout, "LO", layer->getConstName(), met, jj,
      // res_rc_min->getRes());
      rcMin->printBound(
          fp, "LO", layer->getConstName(), met, jj, res_rc_min->getRes());
      // rcMax->printBound(stdout, "HI", layer->getConstName(), met, jj,
      // res_rc_max->getRes());
      rcMax->printBound(
          fp, "HI", layer->getConstName(), met, jj, res_rc_max->getRes());
    }
  }
  fclose(fp);
  return cnt;
}
void extDistRC::printBound(FILE* fp,
                           const char* loHi,
                           const char* layer_name,
                           uint32_t met,
                           uint32_t corner,
                           double res)
{
  fprintf(fp,
          "%-10s Metal %2d  Corner %2d  %s_Bound cap:aF/nm: total %9.6f "
          "coupling %9.6f  Res:mOhm/nm: %10.6f\n",
          layer_name,
          met,
          corner,
          loHi,
          2 * (coupling_ + fringe_ + diag_) * 10e+3,
          2 * coupling_ * 10e+3,
          res * 1000);
}

void extMain::addInstsGeometries(const Array1D<uint32_t>* instTable,
                                 Array1D<uint32_t>* tmpInstIdTable,
                                 const uint32_t dir)
{
  if (instTable == nullptr) {
    return;
  }

  const bool rotatedGs = getRotatedFlag();

  const uint32_t instCnt = instTable->getCnt();

  for (uint32_t ii = 0; ii < instCnt; ii++) {
    const uint32_t instId = instTable->get(ii);
    dbInst* inst = dbInst::getInst(_block, instId);

    if (tmpInstIdTable != nullptr) {
      if (inst->getUserFlag1()) {
        continue;
      }

      inst->setUserFlag1();
      tmpInstIdTable->add(instId);
    }

    addItermShapesOnPlanes(inst, rotatedGs, !dir);
    addObsShapesOnPlanes(inst, rotatedGs, !dir);
  }
  if (tmpInstIdTable != nullptr) {
    for (uint32_t jj = 0; jj < tmpInstIdTable->getCnt(); jj++) {
      const uint32_t instId = instTable->get(jj);
      dbInst::getInst(_block, instId)->clearUserFlag1();
    }
  }
}

void extMain::addItermShapesOnPlanes(dbInst* inst,
                                     const bool rotatedFlag,
                                     const bool swap_coords)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    odb::dbShape s;
    dbITermShapeItr term_shapes;
    for (term_shapes.begin(iterm); term_shapes.next(s);) {
      if (s.isVia()) {
        continue;
      }

      const uint32_t level = s.getTechLayer()->getRoutingLevel();

      if (!rotatedFlag) {
        _geomSeq->box(s.xMin(), s.yMin(), s.xMax(), s.yMax(), level);
      } else {
        addShapeOnGs(&s, swap_coords);
      }
    }
  }
}

void extMain::addShapeOnGs(odb::dbShape* s, const bool swap_coords)
{
  const int level = s->getTechLayer()->getRoutingLevel();

  if (!swap_coords) {  // horizontal
    _geomSeq->box(s->xMin(), s->yMin(), s->xMax(), s->yMax(), level);
  } else {
    _geomSeq->box(s->yMin(), s->xMin(), s->yMax(), s->xMax(), level);
  }
}

void extMain::addObsShapesOnPlanes(dbInst* inst,
                                   const bool rotatedFlag,
                                   const bool swap_coords)
{
  dbInstShapeItr obs_shapes;
  odb::dbShape s;

  for (obs_shapes.begin(inst, dbInstShapeItr::OBSTRUCTIONS);
       obs_shapes.next(s);) {
    if (s.isVia()) {
      continue;
    }

    uint32_t level = s.getTechLayer()->getRoutingLevel();

    if (!rotatedFlag) {
      _geomSeq->box(s.xMin(), s.yMin(), s.xMax(), s.yMax(), level);
    } else {
      addShapeOnGs(&s, swap_coords);
    }
  }
}

}  // namespace rcx
