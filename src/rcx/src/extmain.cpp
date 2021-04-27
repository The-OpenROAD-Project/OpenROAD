///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#include "OpenRCX/extRCap.h"
#include "OpenRCX/extSpef.h"
#include "OpenRCX/exttree.h"
#include "darr.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

using odb::dbBlock;
using odb::dbBox;
using odb::dbBTerm;
using odb::dbCapNode;
using odb::dbCCSeg;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbNet;
using odb::dbRSeg;
using odb::dbSet;
using odb::dbShape;
using odb::dbSigType;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbTechLayerType;
using odb::dbWire;
using odb::dbWirePath;
using odb::dbWirePathItr;
using odb::dbWirePathShape;
using odb::gs;
using odb::ISdb;
using odb::Rect;
using odb::SEQ;
using odb::ZPtr;

void extMain::destroyExtSdb(std::vector<dbNet*>& nets, void* _ext)
{
  if (_ext == NULL)
    return;
  extMain* ext = (extMain*) _ext;
  ext->removeSdb(nets);
}

void extMain::addDummyCorners(dbBlock* block, uint cnt, Logger* logger)
{
  extMain* tmiExt = (extMain*) block->getExtmi();
  if (tmiExt == NULL) {
    logger->error(RCX, 252, "Ext object on dbBlock is NULL!");
    return;
  }
  tmiExt->addDummyCorners(cnt);
}

void extMain::initExtractedCorners(dbBlock* block)
{
  extMain* tmiExt = (extMain*) block->getExtmi();
  if (tmiExt == NULL) {
    tmiExt = new extMain(0);
    tmiExt->setDB((dbDatabase*) block->getDataBase());
  }
  if (tmiExt->_processCornerTable)
    return;
  tmiExt->getPrevControl();
  tmiExt->getExtractedCorners();
}

int extMain::getExtCornerIndex(dbBlock* block, const char* cornerName)
{
  extMain* tmiExt = (extMain*) block->getExtmi();
  if (tmiExt == NULL) {
    tmiExt = new extMain(0);
    tmiExt->setDB((dbDatabase*) block->getDataBase());
  }
  int idx = tmiExt->getDbCornerIndex(cornerName);
  return idx;
}

// wis 1
void extMain::writeIncrementalSpef(Darr<dbNet*>& buf_nets,
                                   dbBlock* block,
                                   INCR_SPEF_TYPE isftype,
                                   bool coupled_rc,
                                   bool dual_incr_spef)
{
  std::vector<dbNet*> bnets;
  int nn;
  for (nn = 0; nn < buf_nets.n(); nn++)
    bnets.push_back(buf_nets.get(nn));
  if (isftype == ISPEF_ORIGINAL_PLUS_HALO || isftype == ISPEF_NEW_PLUS_HALO) {
    std::vector<dbNet*> ccHaloNets;
    block->getCcHaloNets(bnets, ccHaloNets);
    INCR_SPEF_TYPE type
        = isftype == ISPEF_ORIGINAL_PLUS_HALO ? ISPEF_ORIGINAL : ISPEF_NEW;
    writeIncrementalSpef(
        bnets, ccHaloNets, block, type, coupled_rc, dual_incr_spef);  // wis 3
  } else
    writeIncrementalSpef(
        bnets, block, isftype, coupled_rc, dual_incr_spef);  // wis 4
}

// wis 2
void extMain::writeIncrementalSpef(Darr<dbNet*>& buf_nets,
                                   std::vector<dbNet*>& ccHaloNets,
                                   dbBlock* block,
                                   INCR_SPEF_TYPE isftype,
                                   bool coupled_rc,
                                   bool dual_incr_spef)
{
  std::vector<dbNet*> bnets;
  int nn;
  for (nn = 0; nn < buf_nets.n(); nn++)
    bnets.push_back(buf_nets.get(nn));
  uint jj;
  for (jj = 0; jj < ccHaloNets.size(); jj++)
    bnets.push_back(ccHaloNets[jj]);
  writeIncrementalSpef(
      bnets, block, isftype, coupled_rc, dual_incr_spef);  // wis 4
}

// wis 3
void extMain::writeIncrementalSpef(std::vector<dbNet*>& buf_nets,
                                   std::vector<dbNet*>& ccHaloNets,
                                   dbBlock* block,
                                   INCR_SPEF_TYPE isftype,
                                   bool coupled_rc,
                                   bool dual_incr_spef)
{
  std::vector<dbNet*> bnets;
  uint jj;
  for (jj = 0; jj < buf_nets.size(); jj++)
    bnets.push_back(buf_nets[jj]);
  for (jj = 0; jj < ccHaloNets.size(); jj++)
    bnets.push_back(ccHaloNets[jj]);
  writeIncrementalSpef(
      bnets, block, isftype, coupled_rc, dual_incr_spef);  // wis 4
}

// wis 4
void extMain::writeIncrementalSpef(std::vector<dbNet*>& buf_nets,
                                   dbBlock* block,
                                   INCR_SPEF_TYPE isftype,
                                   bool coupled_rc,
                                   bool dual_incr_spef)
{
  extMain* tmiExt = (extMain*) block->getExtmi();
  if (tmiExt == NULL) {
    tmiExt = new extMain(0);
    tmiExt->setDB((dbDatabase*) block->getDataBase());
    // tmiExt -> setDesign((char *)block->getConstName());
  }
  tmiExt->writeIncrementalSpef(
      buf_nets, isftype, coupled_rc, dual_incr_spef);  // wis 5
}

// wis 5
void extMain::writeIncrementalSpef(std::vector<dbNet*>& bnets,
                                   INCR_SPEF_TYPE isftype,
                                   bool coupled_rc,
                                   bool dual_incr_spef)
{
  if (isftype == ISPEF_ORIGINAL && !_origSpefFilePrefix)
    return;
  if (isftype == ISPEF_NEW && !_newSpefFilePrefix)
    return;
  std::vector<dbNet*> dumnet;
  bool fullIncrSpef
      = coupled_rc && !dual_incr_spef && !_noFullIncrSpef ? true : false;
  std::vector<dbNet*>* pbnets = fullIncrSpef ? &dumnet : &bnets;
  if (!_spef || _spef->getBlock() != _block) {
    if (_spef)
      delete _spef;
    _spef = new extSpef(_tech, _block, logger_, this);
    // copy block name for incremental spef - needed for magma
    // Mattias - Nov 19/07
    _spef->setDesign((char*) _block->getName().c_str());
  }
  _spef->_writeNameMap = _writeNameMap;
  _spef->preserveFlag(_prevControl->_foreign);
  _spef->_independentExtCorners = _prevControl->_independentExtCorners;
  char filename[1024];
  char cName[128];
  char seqName[64];
  const char* newp = "/tmp/new";
  if (_newSpefFilePrefix)
    newp = _newSpefFilePrefix;
  const char* oldp = "/tmp/orig";
  if (_origSpefFilePrefix)
    oldp = _origSpefFilePrefix;
  uint cCnt = _block->getCornerCount();
  uint nn;
  for (nn = 0; nn < cCnt; nn++) {
    _spef->_active_corner_cnt = 1;
    _spef->_active_corner_number[0] = nn;
    // _spef->_db_ext_corner= nn;
    _block->getExtCornerName(nn, &cName[0]);
    if (cName[0] == '\0')
      sprintf(&cName[0], "MinMax%d", nn);
    if (_bufSpefCnt && (1 == 0))
      sprintf(&seqName[0], "%d.%s", _bufSpefCnt++, &cName[0]);
    else
      sprintf(&seqName[0], "%s", &cName[0]);
    if (isftype == ISPEF_ORIGINAL)
      sprintf(filename, "%s.%s", oldp, &seqName[0]);
    else
      sprintf(filename, "%s.%s", newp, &seqName[0]);
    writeIncrementalSpef(filename, *pbnets, nn, dual_incr_spef);  // wis 6
  }
  delete _spef;
  _spef = NULL;
}

// wis 6
void extMain::writeIncrementalSpef(char* filename,
                                   std::vector<dbNet*>& bnets,
                                   uint nn,
                                   bool dual_incr_spef)
{
  uint cnt;
  char fname[1200];
  if (!dual_incr_spef) {
    debugPrint(logger_,
               RCX,
               "spef_out",
               1,
               "EXT_SPEF:"
               "I "
               "Writing Spef to File {}",
               filename);
    sprintf(&fname[0], "%s.%d.spef", filename, nn);
    if (openSpefFile(fname, 1) > 0)
      logger_->info(
          RCX, 137, "Can't open file \"{}\" to write spef.", filename);
    else
      cnt = _spef->writeBlock(NULL /*nodeCoord*/,
                              _excludeCells,
                              "PF" /*capUnit*/,
                              "OHM" /*resUnit*/,
                              false /*stopAfterMap*/,
                              bnets /*tnets*/,
                              false /*wClock*/,
                              false /*wConn*/,
                              false /*wCap*/,
                              false /*wOnlyCCcap*/,
                              false /*wRes*/,
                              false /*noCnum*/,
                              false /*initOnly*/,
                              _incrNoBackSlash /*noBackSlash*/,
                              false /*flatten*/,
                              false /*parallel*/);
    return;
  }
  std::vector<uint> oldNetCap;
  std::vector<uint> oldNetRseg;
  _block->replaceOldParasitics(bnets, oldNetCap, oldNetRseg);
  sprintf(&fname[0], "%s.1.%d.spef", filename, nn);
  if (openSpefFile(fname, 1) > 0)
    logger_->info(RCX, 137, "Can't open file \"{}\" to write spef.", fname);
  else
    cnt = _spef->writeBlock(NULL /*nodeCoord*/,
                            _excludeCells,
                            "PF" /*capUnit*/,
                            "OHM" /*resUnit*/,
                            false /*stopAfterMap*/,
                            bnets /*tnets*/,
                            false /*wClock*/,
                            false /*wConn*/,
                            false /*wCap*/,
                            false /*wOnlyCCcap*/,
                            false /*wRes*/,
                            false /*noCnum*/,
                            false /*initOnly*/,
                            _incrNoBackSlash /*noBackSlash*/,
                            false /*flatten*/,
                            false /*parallel*/);
  _block->restoreOldParasitics(bnets, oldNetCap, oldNetRseg);
  sprintf(&fname[0], "%s.2.%d.spef", filename, nn);
  if (openSpefFile(fname, 1) > 0)
    logger_->info(RCX, 137, "Can't open file \"{}\" to write spef.", fname);
  else
    cnt = _spef->writeBlock(NULL /*nodeCoord*/,
                            _excludeCells,
                            "PF" /*capUnit*/,
                            "OHM" /*resUnit*/,
                            false /*stopAfterMap*/,
                            bnets /*tnets*/,
                            false /*wClock*/,
                            false /*wConn*/,
                            false /*wCap*/,
                            false /*wOnlyCCcap*/,
                            false /*wRes*/,
                            false /*noCnum*/,
                            false /*initOnly*/,
                            _incrNoBackSlash /*noBackSlash*/,
                            false /*flatten*/,
                            false /*parallel*/);
}

void writeSpef(dbBlock* block,
               char* filename,
               char* nets,
               int corner,
               char* coord)
{
  extMain* tmiExt = (extMain*) block->getExtmi();
  if (tmiExt == NULL) {
    tmiExt = new extMain(0);
    tmiExt->setDB((dbDatabase*) block->getDataBase());
  }
  std::vector<dbNet*> tnets;
  block->findSomeNet(nets, tnets);
  tmiExt->writeSpef(filename, tnets, corner, coord);
}

void extMain::writeSpef(char* filename,
                        std::vector<dbNet*>& tnets,
                        int corner,
                        char* coord)
{
  if (!_spef || _spef->getBlock() != _block) {
    if (_spef)
      delete _spef;
    _spef = new extSpef(_tech, _block, logger_, this);
  }
  _spef->setDesign((char*) _block->getConstName());
  uint cCnt = _block->getCornerCount();
  if (corner >= 0) {
    _spef->_active_corner_cnt = 1;
    _spef->_active_corner_number[0] = corner;
  } else {
    _spef->_active_corner_cnt = cCnt;
    for (uint nn = 0; nn < cCnt; nn++)
      _spef->_active_corner_number[nn] = nn;
  }
  uint cnt;
  if (openSpefFile(filename, 1) > 0) {
    logger_->info(RCX, 137, "Can't open file \"{}\" to write spef.", filename);
    return;
  } else
    cnt = _spef->writeBlock(coord /*nodeCoord*/,
                            NULL /*excludeCell*/,
                            "PF" /*capUnit*/,
                            "OHM" /*resUnit*/,
                            false /*stopAfterMap*/,
                            tnets /*tnets*/,
                            false /*wClock*/,
                            false /*wConn*/,
                            false /*wCap*/,
                            false /*wOnlyCCcap*/,
                            false /*wRes*/,
                            false /*noCnum*/,
                            false /*initOnly*/,
                            false /*noBackSlash*/,
                            false /*flatten*/,
                            false /*parallel*/);
  delete _spef;
  _spef = NULL;
}

void extMain::adjustRC(double resFactor, double ccFactor, double gndcFactor)
{
  double res_factor = resFactor / _resFactor;
  _resFactor = resFactor;
  _resModify = resFactor == 1.0 ? false : true;
  double cc_factor = ccFactor / _ccFactor;
  _ccFactor = ccFactor;
  _ccModify = ccFactor == 1.0 ? false : true;
  double gndc_factor = gndcFactor / _gndcFactor;
  _gndcFactor = gndcFactor;
  _gndcModify = gndcFactor == 1.0 ? false : true;
  _block->adjustRC(res_factor, cc_factor, gndc_factor);
}

uint extMain::getMultiples(uint cnt, uint base)
{
  return ((cnt / base) + 1) * base;
}
void extMain::setupMapping(uint itermCnt)
{
  if (_btermTable)
    return;
  uint btermCnt = 0;
  if ((itermCnt == 0) && (_block != NULL)) {
    btermCnt = _block->getBTerms().size();
    btermCnt = getMultiples(btermCnt, 1024);

    itermCnt = _block->getITerms().size();
    itermCnt = getMultiples(itermCnt, 1024);
  } else if (itermCnt == 0) {
    btermCnt = 512;
    itermCnt = 64000;
  }
  _btermTable = new Ath__array1D<int>(btermCnt);
  _itermTable = new Ath__array1D<int>(itermCnt);
  _nodeTable = new Ath__array1D<int>(16000);
}
extMain::extMain(uint menuId)
    : _db(nullptr),
      _tech(nullptr),
      _block(nullptr),
      _spef(nullptr),
      _origSpefFilePrefix(nullptr),
      _newSpefFilePrefix(nullptr),
      _excludeCells(nullptr),
      _ibox(nullptr),
      _seqPool(nullptr),
      _dgContextBaseTrack(nullptr),
      _dgContextLowTrack(nullptr),
      _dgContextHiTrack(nullptr),
      _dgContextTrackBase(nullptr),
      _prevControl(nullptr),
      _wireBinTable(nullptr),
      _cntxBinTable(nullptr),
      _cntxInstTable(nullptr),
      _tiles(nullptr),
      _blkInfoVDD(nullptr),
      _viaInfoVDD(nullptr),
      _blkInfoGND(nullptr),
      _viaInfoGND(nullptr),
      _stdCirVDD(nullptr),
      _globCirVDD(nullptr),
      _globGeomVDD(nullptr),
      _stdCirGND(nullptr),
      _globCirGND(nullptr),
      _stdCirHeadVDD(nullptr),
      _globCirHeadVDD(nullptr),
      _globGeomGND(nullptr),
      _stdCirHeadGND(nullptr),
      _globCirHeadGND(nullptr),
      _blkInfo(nullptr),
      _viaInfo(nullptr),
      _globCir(nullptr),
      _globGeom(nullptr),
      _stdCir(nullptr),
      _globCirHead(nullptr),
      _stdCirHead(nullptr),
      _viaStackGlobCir(nullptr),
      _viaStackGlobVDD(nullptr),
      _viaStackGlobGND(nullptr),
      _junct2viaMap(nullptr),
      _netUtil(nullptr),
      _viaM1Table(nullptr),
      _viaUpTable(nullptr),
      _via2JunctionMap(nullptr),
      _supplyViaMap{nullptr, nullptr},
      _supplyViaTable{nullptr, nullptr},
      _coordsFP(nullptr),
      _coordsGND(nullptr),
      _coordsVDD(nullptr),
      _subCktNodeFP{{nullptr, nullptr}, {nullptr, nullptr}},
      _junct2iterm(nullptr)
{
  _previous_percent_extracted = 0;
  _power_extract_only = false;
  _skip_power_stubs = false;
  _power_exclude_cell_list = NULL;

  _modelTable = new Ath__array1D<extRCModel*>(8);

  _btermTable = NULL;
  _itermTable = NULL;
  _nodeTable = NULL;

  _extNetSDB = NULL;
  _extCcapSDB = NULL;
  _reExtCcapSDB = NULL;

  _reuseMetalFill = false;
  _usingMetalPlanes = 0;
  _alwaysNewGs = true;
  _ccUp = 0;
  _couplingFlag = 0;
  _debug = 0;
  _ccContextDepth = 0;
  _mergeViaRes = false;
  _mergeResBound = 0.0;
  _mergeParallelCC = false;
  _unifiedMeasureInit = true;
  _reportNetNoWire = false;
  _netNoWireCnt = 0;

  _resFactor = 1.0;
  _resModify = false;
  _ccFactor = 1.0;
  _ccModify = false;
  _gndcFactor = 1.0;
  _gndcModify = false;

  _menuId = menuId;
  _dbPowerId = 1;
  _dbSignalId = 2;
  _CCsegId = 3;
  /*
  _dbPowerId= ZSUBMENUID(_menuId, 1);
  _dbSignalId= ZSUBMENUID(_menuId, 2);
  _CCsegId= ZSUBMENUID(_menuId, 3);
  */

  _CCnoPowerSource = 0;
  _CCnoPowerTarget = 0;

  _coupleThreshold = 0.1;  // fF
  _lefRC = false;

  _singlePlaneLayerMap = NULL;
  _overUnderPlaneLayerMap = NULL;
  _usingMetalPlanes = false;
  _alwaysNewGs = true;
  _geomSeq = NULL;

  _dgContextArray = NULL;

  _ccContextLength = NULL;
  _ccContextArray = NULL;
  _ccMergedContextLength = NULL;
  _ccMergedContextArray = NULL;
  _tContextArray = NULL;

  _noModelRC = false;

  _currentModel = NULL;
  _geoThickTable = NULL;

  _measureRcCnt = -1;
  _shapeRcCnt = -1;
  _updateTotalCcnt = -1;
  _printFile = NULL;
  _ptFile = NULL;

  _extRun = 0;
  _independentExtCorners = false;
  _foreign = false;
  _overCell = false;
  _diagFlow = false;
  _processCornerTable = NULL;
  _scaledCornerTable = NULL;
  _batchScaleExt = true;
  _cornerCnt = 0;
  _rotatedGs = false;

  _getBandWire = false;
  _searchFP = NULL;
  _search = NULL;
  _printBandInfo = false;

  _writeNameMap = true;
  _fullIncrSpef = false;
  _noFullIncrSpef = false;
  _adjust_colinear = false;
  _power_source_file = NULL;
}

void extMain::initDgContextArray()
{
  _dgContextDepth = 3;
  _dgContextPlanes = _dgContextDepth * 2 + 1;
  _dgContextArray = new Ath__array1D<SEQ*>**[_dgContextPlanes];
  _dgContextBaseTrack = new uint[_dgContextPlanes];
  _dgContextLowTrack = new int[_dgContextPlanes];
  _dgContextHiTrack = new int[_dgContextPlanes];
  _dgContextTrackBase = new int*[_dgContextPlanes];
  if (_diagFlow)
    //		_dgContextTracks = (_couplingFlag%10)*2 + 1;
    _dgContextTracks = _couplingFlag * 2 + 1;
  else
    _dgContextTracks = _couplingFlag * 2 + 1;
  for (uint jj = 0; jj < _dgContextPlanes; jj++) {
    _dgContextTrackBase[jj] = new int[1024];
    _dgContextArray[jj] = new Ath__array1D<SEQ*>*[_dgContextTracks];
    for (uint tt = 0; tt < _dgContextTracks; tt++) {
      _dgContextArray[jj][tt] = new Ath__array1D<SEQ*>(1024);
    }
  }
}

void extMain::removeDgContextArray()
{
  if (!_dgContextPlanes || !_dgContextArray)
    return;
  delete[] _dgContextBaseTrack;
  delete[] _dgContextLowTrack;
  delete[] _dgContextHiTrack;
  for (uint jj = 0; jj < _dgContextPlanes; jj++) {
    delete[] _dgContextTrackBase[jj];
    for (uint tt = 0; tt < _dgContextTracks; tt++)
      delete _dgContextArray[jj][tt];
    delete[] _dgContextArray[jj];
  }
  delete[] _dgContextTrackBase;
  delete[] _dgContextArray;
  _dgContextArray = NULL;
}

void extMain::initContextArray()
{
  if (_ccContextArray)
    return;
  uint layerCnt = getExtLayerCnt(_tech);
  _ccContextLength = (uint*) calloc(sizeof(uint), layerCnt + 1);
  _ccContextArray = new Ath__array1D<int>*[layerCnt + 1];
  _ccContextArray[0] = NULL;
  uint ii;
  for (ii = 1; ii <= layerCnt; ii++)
    _ccContextArray[ii] = new Ath__array1D<int>(1024);
  _ccMergedContextLength = (uint*) calloc(sizeof(uint), layerCnt + 1);
  _ccMergedContextArray = new Ath__array1D<int>*[layerCnt + 1];
  _ccMergedContextArray[0] = NULL;
  for (ii = 1; ii <= layerCnt; ii++)
    _ccMergedContextArray[ii] = new Ath__array1D<int>(1024);
  _tContextArray = new Ath__array1D<int>(1024);
}

uint extMain::getExtLayerCnt(dbTech* tech)
{
  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;

  uint n = 0;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;
    dbTechLayerType type = layer->getType();

    if (type.getValue() != dbTechLayerType::ROUTING)
      continue;

    n++;
  }
  return n;
}
uint extMain::addExtModel(dbTech* tech)
{
  _lefRC = true;

  if (tech == NULL)
    tech = _tech;

  _extDbCnt = 3;

  uint layerCnt = getExtLayerCnt(tech);

  extRCModel* m = NULL;
  /*
  if (_modelTable->getCnt()>0)
          m= _modelTable->get(0);
*/
  if (m == NULL) {
    m = new extRCModel(layerCnt, "TYPICAL", logger_);
    _modelTable->add(m);
  }

  int dbunit = _block->getDbUnitsPerMicron();
  double dbFactor = 1;
  if (dbunit > 1000)
    dbFactor = dbunit * 0.001;

  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;

  uint n = 0;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;
    dbTechLayerType type = layer->getType();

    if (type.getValue() != dbTechLayerType::ROUTING)
      continue;

    n = layer->getRoutingLevel();

    uint w = layer->getWidth();  // nm

    double areacap = layer->getCapacitance() / (dbFactor * dbFactor);
    double cap = layer->getCapacitance();

    // double cap
    //    = layer->getCapacitance()
    //      / (dbFactor * dbFactor);  // PF per square micron : totCap= cap * LW
    double res = layer->getResistance();  // OHMS per square
    // uint   w   = layer->getWidth();       // nm
    //		res /= w; // OHMS per nm
    //		cap *= 0.001 * w; // FF per nm : 0.00
    cap *= 0.001 * 2;

    m->addLefTotRC(n, 0, cap, res);

    double c1 = m->getTotCapOverSub(n);
    double r1 = m->getRes(n);

    _minWidthTable[n] = w;

    _resistanceTable[0][n] = r1;
    _capacitanceTable[0][n] = c1;
  }
  return layerCnt;
}
extRCModel* extMain::getRCmodel(uint n)
{
  if (_modelTable->getCnt() <= 0)
    return NULL;

  return _modelTable->get(n);
}
uint extMain::getResCapTable(bool lefRC)
{
  calcMinMaxRC();
  _currentModel = getRCmodel(0);

  dbSet<dbTechLayer> layers = _tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;

  extMeasure m;
  m._underMet = 0;
  m._overMet = 0;

  uint cnt = 0;
  uint n = 0;
  bool allRzero = true;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;
    dbTechLayerType type = layer->getType();

    if (type.getValue() != dbTechLayerType::ROUTING)
      continue;

    n = layer->getRoutingLevel();

    uint w = layer->getWidth();  // nm
    _minWidthTable[n] = w;

    m._width = w;
    m._met = n;

    uint sp = layer->getSpacing();  // nm

    _minDistTable[n] = sp;
    if (sp == 0) {
      sp = layer->getPitch() - layer->getWidth();
      _minDistTable[n] = sp;
    }
    double resTable[20];
    bool newResModel = true;
    if (newResModel) {
      for (uint jj = 0; jj < _modelMap.getCnt(); jj++) {
        resTable[jj] = 0.0;
      }
      calcRes0(resTable, n, w, 1);
    }
    for (uint jj = 0; jj < _modelMap.getCnt(); jj++) {
      uint modelIndex = _modelMap.get(jj);
      extMetRCTable* rcModel = _currentModel->getMetRCTable(modelIndex);

      double res = layer->getResistance();  // OHMS per square
      if (res != 0.0)
        allRzero = false;
      _resistanceTable[jj][n] = res;

      _capacitanceTable[jj][n] = 0.0;

      extDistRC* rc = rcModel->getOverFringeRC(&m);

      if (rc != NULL) {
        double r1 = rc->getRes();
        _capacitanceTable[jj][n] = rc->getFringe();
        debugPrint(logger_,
                   RCX,
                   "extrules",
                   1,
                   "EXT_RES: "
                   "R "
                   "Layer= {} met= {}   w= {} cc= {:g} fr= {:g} res= {:g} "
                   "model_res= {:g} new_model_res= {:g} ",
                   layer->getConstName(),
                   n,
                   w,
                   rc->getCoupling(),
                   rc->getFringe(),
                   res,
                   r1,
                   resTable[jj]);
      }

      extDistRC* rc0 = rcModel->getOverFringeRC(&m, 0);

      if (!_lef_res) {
        if (newResModel) {
          _resistanceTable[jj][n] = resTable[jj];
        } else {
          if (rc0 != NULL) {
            double r1 = rc->getRes();
            _resistanceTable[jj][n] = r1;
          }
        }
      } else {
        debugPrint(logger_,
                   RCX,
                   "extrules",
                   1,
                   "EXT_RES_LEF: "
                   "R "
                   "Layer= {} met= {}  lef_res= {:g}\n",
                   layer->getConstName(),
                   n,
                   res);
      }
    }
    cnt++;
  }
  //	if (allRzero)
  //	{
  //		logger_->info(RCX, 0, "Warning: No RESISTANCE from all layers in
  // the
  // lef files. Can't do extraction.\n\n"); 		return 0;
  //	}
  return cnt;
}
bool extMain::checkLayerResistance()
{
  dbSet<dbTechLayer> layers = _tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;

  uint cnt = 0;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;
    dbTechLayerType type = layer->getType();

    if (type.getValue() != dbTechLayerType::ROUTING)
      continue;

    double res = layer->getResistance();  // OHMS per square

    if (res <= 0.0) {
      logger_->warn(RCX,
                    139,
                    "Missing Resistance value for layer {}",
                    layer->getConstName());
      cnt++;
    }
  }
  if (cnt > 0) {
    logger_->warn(RCX,
                  138,
                  "{} layers are missing resistance value; Check LEF file. "
                  "Extraction cannot proceed! Exiting",
                  cnt);
    return false;
  }
  return true;
}
double extMain::getLefResistance(uint level, uint width, uint len, uint model)
{
  double res = _resistanceTable[model][level];
  double n = 1.0 * len;

  if (_lefRC || _lef_res)
    n /= width;

  double r = n * res;

  return r;
}
double extMain::getResistance(uint level, uint width, uint len, uint model)
{
  /*
          if (_lefRC) {
          double res= _resistanceTable[0][level];
          // double res= _modelTable->get(0)->getRes(level);
          res *= len;
          return res;

      }
      return getLefResistance(level, width, len, model);
  */
  return getLefResistance(level, width, len, model);
  /*This the flow to use extrule esistance. Disable now but will be used in
     future. if (_lefRC) {
                  // TO TEST regrs. test and relpace :
                   return getLefResistance(level, width, len, model);
          }
          else {
                  if (_minWidthTable[level]==width) {
                          double res= _resistanceTable[0][level];
                          res *= 2*len;
                          return res;
                  } else {
                          extMeasure m;
                          m._underMet= 0;
                          m._overMet= 0;
                          m._width= width;
                          m._met= level;
                          uint modelIndex= _modelMap.get(model);
                          extDistRC
     *rc=_currentModel->getMetRCTable(modelIndex)->getOverFringeRC(&m); double
     res= rc->getRes(); res *= 2*len; return res;
                  }
          }
  */
}
void extMain::setDB(dbDatabase* db)
{
  _db = db;
  _tech = db->getTech();
  _block = NULL;
  _blockId = 0;
  if (db->getChip() != NULL) {
    _block = db->getChip()->getBlock();
    _blockId = _block->getId();
    _prevControl = _block->getExtControl();
#ifndef _WIN32
    _block->setExtmi(this);
#endif
  }
  _spef = NULL;
  _bufSpefCnt = 0;
  _origSpefFilePrefix = NULL;
  _newSpefFilePrefix = NULL;
  _excludeCells = NULL;
  _extracted = false;
}
void extMain::setBlock(dbBlock* block)
{
  _block = block;
  _prevControl = _block->getExtControl();
#ifndef _WIN32
  _block->setExtmi(this);
#endif
  _blockId = _block->getId();
  if (_spef) {
    _spef = NULL;
    _extracted = false;
  }
  _bufSpefCnt = 0;
  _origSpefFilePrefix = NULL;
  _newSpefFilePrefix = NULL;
  _excludeCells = NULL;
}
uint extMain::makeGuiBoxes(uint extGuiBoxType)
{
  uint cnt = 0;
  return cnt;
}

uint extMain::computeXcaps(uint boxType)
{
  ZPtr<ISdb> ccCapSdb = _reExtract ? _reExtCcapSDB : _extCcapSDB;
  if (ccCapSdb == NULL)
    return 0;

  uint cnt = 0;
  ccCapSdb->searchWireIds(_x1, _y1, _x2, _y2, true, NULL);

  ccCapSdb->startIterator();

  bool mergeParallel = _mergeParallelCC;
  uint wireId = 0;
  while ((wireId = ccCapSdb->getNextWireId())) {
    uint len, dist, id1, id2;
    ccCapSdb->getCCdist(wireId, &dist, &len, &id1, &id2);

    uint extId1;
    uint junctionId1;
    uint wtype1;
    _extNetSDB->getIds(id1, &extId1, &junctionId1, &wtype1);

    if (wtype1 == _dbPowerId)
      continue;

    uint extId2, junctionId2, wtype2;
    _extNetSDB->getIds(id2, &extId2, &junctionId2, &wtype2);

    if (wtype2 == _dbPowerId)
      continue;

    dbRSeg* rseg1 = dbRSeg::getRSeg(_block, extId1);
    dbRSeg* rseg2 = dbRSeg::getRSeg(_block, extId2);

    dbNet* srcNet = rseg1->getNet();
    dbNet* tgtNet = rseg2->getNet();

    if (srcNet == tgtNet)
      continue;

    // dbCCSeg *ccap= dbCCSeg::create(srcNet, rseg1->getTargetNode(),
    //		tgtNet, rseg2->getTargetNode(), mergeParallel);
    dbCCSeg* ccap
        = dbCCSeg::create(dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
                          dbCapNode::getCapNode(_block, rseg2->getTargetNode()),
                          mergeParallel);

    uint lcnt = _block->getCornerCount();
    for (uint ii = 0; ii < lcnt; ii++) {
      if (mergeParallel)
        ccap->addCapacitance(len / dist + ii, ii);
      else
        ccap->setCapacitance(len / dist + ii, ii);
    }

    cnt++;
  }
  return cnt;
}

double extMain::getLoCoupling()
{
  return _coupleThreshold;
}

double extMain::getFringe(uint met,
                          uint width,
                          uint modelIndex,
                          double& areaCap)
{
  areaCap = 0.0;
  if (_noModelRC)
    return 0.0;

  if (_lefRC)
    // return _modelTable->get(0)->getTotCapOverSub(met);
    return _capacitanceTable[0][met];

  if (width == _minWidthTable[met])
    return _capacitanceTable[modelIndex][met];

  // just in case

  extMeasure m;

  m._met = met;
  m._width = width;
  m._underMet = 0;
  m._ccContextLength = _ccContextLength;
  m._ccContextArray = _ccContextArray;
  m._ccMergedContextLength = _ccMergedContextLength;
  m._ccMergedContextArray = _ccMergedContextArray;

  extDistRC* rc = _metRCTable.get(modelIndex)->getOverFringeRC(&m);

  /* TODO 10292011
          if (width>10*_minWidthTable[met]) {
                  extDistRC *rc1= m.areaCapOverSub(modelIndex,
     _metRCTable.get(modelIndex)); areaCap = rc1->_fringe; return 0.0;
          }
  */
  if (rc == NULL)
    return 0.0;
  return rc->getFringe();
}
void extMain::updateTotalCap(dbRSeg* rseg,
                             double frCap,
                             double ccCap,
                             double deltaFr,
                             uint modelIndex)
{
  if (_eco && !rseg->getNet()->isWireAltered())
    return;

  double cap = frCap + ccCap - deltaFr;

  double tot = rseg->getCapacitance(modelIndex);
  tot += cap;

  rseg->setCapacitance(tot, modelIndex);
  //	double T= rseg->getCapacitance(modelIndex);
}
void extMain::updateTotalRes(dbRSeg* rseg1,
                             dbRSeg* rseg2,
                             extMeasure* m,
                             double* delta,
                             uint modelCnt)
{
  for (uint modelIndex = 0; modelIndex < modelCnt; modelIndex++) {
    extDistRC* rc = m->_rc[modelIndex];

    double res = rc->_res - delta[modelIndex];
    if (_resModify)
      res *= _resFactor;

    if ((rseg1 != NULL) && !(_eco && !rseg1->getNet()->isWireAltered())) {
      double tot = rseg1->getResistance(modelIndex);
      tot += res;

      /*
      if (_updateTotalCcnt >= 0)
      {
      if (_printFile == NULL)
      _printFile =fopen ("updateRes.1", "w");
      _updateTotalCcnt++;
      fprintf (_printFile, "%d %d %g %g\n", _updateTotalCcnt, rseg->getId(),
      tot, cap);
      }
      */

      rseg1->setResistance(tot, modelIndex);
      //			double T= rseg1->getResistance(modelIndex);
    }
    if ((rseg2 != NULL) && !(_eco && !rseg2->getNet()->isWireAltered())) {
      double tot = rseg2->getResistance(modelIndex);
      tot += res;

      /*
      if (_updateTotalCcnt >= 0)
      {
      if (_printFile == NULL)
      _printFile =fopen ("updateRes.1", "w");
      _updateTotalCcnt++;
      fprintf (_printFile, "%d %d %g %g\n", _updateTotalCcnt, rseg->getId(),
      tot, cap);
      }
      */

      rseg2->setResistance(tot, modelIndex);
      //			double T= rseg2->getResistance(modelIndex);
    }
  }
}

void extMain::updateTotalCap(dbRSeg* rseg,
                             extMeasure* m,
                             double* deltaFr,
                             uint modelCnt,
                             bool includeCoupling,
                             bool includeDiag)
{
  if (_eco && !rseg->getNet()->isWireAltered())
    return;

  double tot, cap;
  int extDbIndex, sci, scDbIdx;
  for (uint modelIndex = 0; modelIndex < modelCnt; modelIndex++) {
    extDistRC* rc = m->_rc[modelIndex];

    double frCap = rc->_fringe;

    double ccCap = 0.0;
    if (includeCoupling)
      ccCap = rc->_coupling;

    double diagCap = 0.0;
    if (includeDiag)
      diagCap = rc->_diag;

#ifdef HI_ACC_1
    cap = frCap + ccCap + diagCap - deltaFr[modelIndex];
#else
    cap = frCap + ccCap - deltaFr[modelIndex];
#endif
    if (_gndcModify)
      cap *= _gndcFactor;

    extDbIndex = getProcessCornerDbIndex(modelIndex);
    tot = rseg->getCapacitance(extDbIndex);
    tot += cap;
    if (_updateTotalCcnt >= 0) {
      if (_printFile == NULL)
        _printFile = fopen("updateCap.1", "w");
      _updateTotalCcnt++;
      fprintf(_printFile,
              "%d %d %g %g\n",
              _updateTotalCcnt,
              rseg->getId(),
              tot,
              cap);
    }

    rseg->setCapacitance(tot, extDbIndex);
    //		double T= rseg->getCapacitance(extDbIndex);
    getScaledCornerDbIndex(modelIndex, sci, scDbIdx);
    if (sci == -1)
      continue;
    getScaledGndC(sci, cap);
    tot = rseg->getCapacitance(scDbIdx);
    tot += cap;
    rseg->setCapacitance(tot, scDbIdx);
  }
}

void extMain::updateCCCap(dbRSeg* rseg1, dbRSeg* rseg2, double ccCap)
{
  dbCCSeg* ccap
      = dbCCSeg::create(dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
                        dbCapNode::getCapNode(_block, rseg2->getTargetNode()),
                        true);
  /*
dbCCSeg *ccap= dbCCSeg::create(rseg1->getNet(),
rseg1->getTargetNode(), rseg2->getNet(), rseg2->getTargetNode(), true);
*/
  bool mergeParallel = true;

  uint lcnt = _block->getCornerCount();
  lcnt = 1;
  for (uint ii = 0; ii < lcnt; ii++) {
    if (mergeParallel)
      ccap->addCapacitance(ccCap, ii);
    else
      ccap->setCapacitance(ccCap, ii);
  }
}
int extGeoThickTable::getRowCol(int xy, int base, uint bucket, uint bound)
{
  int delta = (xy - base);
  if (delta <= 0)
    return delta;

  int rowCol = delta / bucket;
  if (rowCol >= (int) bound)
    return -1;

  return rowCol;
}

extGeoVarTable* extGeoThickTable::getSquare(int x, int y, uint* rowCol)
{
  int row = getRowCol(y, _ll[1], _tileSize, _rowCnt);
  if (row < 0)
    return NULL;

  int col = getRowCol(x, _ll[0], _tileSize, _colCnt);
  if (col < 0)
    return NULL;

  rowCol[1] = row;
  rowCol[0] = col;

  return _thickTable[row][col];
}
int extGeoThickTable::getLowerBound(uint dir, uint* rowCol)
{
  return _thickTable[rowCol[0]][rowCol[1]]->getLowerBound(dir);
}
int extGeoThickTable::getUpperBound(uint dir, uint* rowCol)
{
  return _thickTable[rowCol[1]][rowCol[0]]->getLowerBound(dir) + _tileSize;
}
extGeoVarTable* extGeoThickTable::addVarTable(int x,
                                              int y,
                                              double nom,
                                              double e,
                                              Ath__array1D<double>* A,
                                              bool simpleVersion,
                                              bool calcDiff)
{
  uint row = (y - _ll[1]) / _tileSize;
  assert((row >= 0) && (row < _rowCnt));

  uint col = (x - _ll[0]) / _tileSize;
  assert((col >= 0) && (col < _colCnt));

  extGeoVarTable* b
      = new extGeoVarTable(x, y, nom, e, A, simpleVersion, calcDiff);

  _thickTable[row][col] = b;

  return b;
}

extGeoThickTable::extGeoThickTable(int x1,
                                   int y1,
                                   int x2,
                                   int y2,
                                   uint tileSize,
                                   Ath__array1D<double>* A,
                                   uint units)
{
  _tileSize = tileSize;
  // char *_layerName;

  int nm = units;
  _ll[0] = x1 * nm;
  _ll[1] = y1 * nm;
  _ur[0] = x2 * nm;
  _ur[1] = y2 * nm;
  _rowCnt = (_ur[1] - _ll[1]) / tileSize + 1;
  _colCnt = (_ur[0] - _ll[0]) / tileSize + 1;

  _thickTable = new extGeoVarTable**[_rowCnt];
  for (uint ii = 0; ii < _rowCnt; ii++) {
    _thickTable[ii] = new extGeoVarTable*[_colCnt];
    for (uint jj = 0; jj < _colCnt; jj++) {
      _thickTable[ii][jj] = NULL;
    }
  }
  _widthTable = NULL;
  if (A->getCnt() == 0)
    return;

  int n = A->getCnt() - 1;  // last is ( --- TO BE FIXED!!!
  if (n == 0)
    return;

  _widthTable = new Ath__array1D<uint>(n);
  for (int i = 0; i < n; i++) {
    int w = Ath__double2int(A->get(i));
    _widthTable->add(w);
  }
}
extGeoThickTable::~extGeoThickTable()
{
  if (_widthTable != NULL)
    delete _widthTable;

  if (_thickTable == NULL)
    return;

  for (uint ii = 0; ii < _rowCnt; ii++) {
    for (uint jj = 0; jj < _colCnt; jj++) {
      if (_thickTable[ii][jj] != NULL)
        delete _thickTable[ii][jj];
    }
    delete[] _thickTable[ii];
  }
  delete[] _thickTable;
}
double extGeoVarTable::getVal(uint n, double& nom)
{
  nom = _nominal;
  return _diffTable->get(n);
}
bool extGeoVarTable::getThicknessDiff(int n, double& delta_th)
{
  if (_fractionDiff) {
    if (n < 0)
      n = 0;
    delta_th = _diffTable->get(n);
    return true;
  }
  double thickDiff = _diffTable->get(n);
  if (_simpleVersion) {
    delta_th = (thickDiff - _nominal) / _nominal;
    return true;
  } else {
    delta_th = 0.001 * _nominal * (1 + thickDiff);
    return true;

    // double diff= (th-thRef)/thRef;
  }
}
bool extGeoThickTable::getThicknessDiff(int x, int y, uint w, double& delta_th)
{
  uint rowCol[2];

  extGeoVarTable* sq1 = getSquare(x, y, rowCol);
  if (sq1 == NULL)
    return false;

  int n = -1;
  if (_widthTable != NULL)
    n = _widthTable->findNextBiggestIndex(w);

  return sq1->getThicknessDiff(n, delta_th);
}
int extGeoVarTable::getLowerBound(uint dir)
{
  if (dir > 0)
    return _y;
  else
    return _x;
}

extGeoVarTable::extGeoVarTable(int x,
                               int y,
                               double nom,
                               double e,
                               Ath__array1D<double>* A,
                               bool simpleVersion,
                               bool calcDiff)
{
  _x = x;
  _y = y;
  _nominal = nom;
  _epsilon = e;

  _fractionDiff = false;
  _varCoeffTable = NULL;
  _diffTable = NULL;

  uint n = A->getCnt();
  if (n == 0)
    return;

  _varCoeffTable = new Ath__array1D<double>(n);
  _diffTable = new Ath__array1D<double>(n);

  if (simpleVersion && calcDiff) {
    for (uint ii = 0; ii < n; ii++) {
      double v = A->get(ii);

      _varCoeffTable->add(v);

      double th = (v - _nominal) / _nominal;
      _diffTable->add(th);
    }
    _fractionDiff = true;
    return;
  }
  _varCoeffTable = new Ath__array1D<double>(n);
  for (uint ii = 0; ii < n; ii++)
    _varCoeffTable->add(A->get(ii));
}
extGeoVarTable::~extGeoVarTable()
{
  if (_diffTable != NULL)
    delete _diffTable;
  if (_varCoeffTable != NULL)
    delete _varCoeffTable;
}
void extMain::ccReportProgress()
{
  uint repChunk = 1000000;
  if ((_totSegCnt > 0) && (_totSegCnt % repChunk == 0)) {
    // if ((_totSignalSegCnt>0)&&(_totSignalSegCnt%5000000==0))
    //		fprintf(stdout, "Have processed %d total segments, %d signal
    // segments, %d CC caps, and stored %d CC caps\n", _totSegCnt,
    //_totSignalSegCnt, _totCCcnt, _totBigCCcnt);
    logger_->info(RCX,
                  140,
                  "Have processed {} total segments, {} signal segments, {} CC "
                  "caps, and stored {} CC caps",
                  _totSegCnt,
                  _totSignalSegCnt,
                  _totCCcnt,
                  _totBigCCcnt);
  }
}
int ttttsrcnet = 66;
int tttttgtnet = 66;
int ttttm = 0;
void extMain::printNet(dbNet* net, uint netId)
{
  if (netId == net->getId())
    net->printNetName(stdout);
}
bool IsDebugNets(dbNet* srcNet, dbNet* tgtNet, uint debugNetId)
{
  if (srcNet != NULL && srcNet->getId() == debugNetId)
    return true;
  if (tgtNet != NULL && tgtNet->getId() == debugNetId)
    return true;

  return false;
}
void extMain::measureRC(int* options)
{
  _totSegCnt++;
  int rsegId1 = options[1];  // dbRSeg id for SRC segment
  int rsegId2 = options[2];  // dbRSeg id for Target segment

  if ((rsegId1 < 0) && (rsegId2 < 0))  // power nets
    return;

  extMeasure m;
  m.defineBox(options);
  m._ccContextLength = _ccContextLength;
  m._ccContextArray = _ccContextArray;
  m._ccMergedContextLength = _ccMergedContextLength;
  m._ccMergedContextArray = _ccMergedContextArray;

  //	fprintf(stdout, "extCompute:: met= %d  len= %d  dist= %d  <===>
  // modelCnt= %d  layerCnt= %d\n", 		met, len, dist,
  // m->getModelCnt(), m->getLayerCnt());

  uint debugNetId = _debug_net_id;

  dbRSeg* rseg1 = NULL;
  dbNet* srcNet = NULL;
  if (rsegId1 > 0) {
    rseg1 = dbRSeg::getRSeg(_block, rsegId1);
    srcNet = rseg1->getNet();
    printNet(srcNet, debugNetId);
  }

  dbRSeg* rseg2 = NULL;
  dbNet* tgtNet = NULL;
  if (rsegId2 > 0) {
    rseg2 = dbRSeg::getRSeg(_block, rsegId2);
    tgtNet = rseg2->getNet();
    printNet(tgtNet, debugNetId);
  }
  if (_lefRC)
    return;

  bool watchNets = IsDebugNets(srcNet, tgtNet, debugNetId);
  m._ouPixelTableIndexMap = _overUnderPlaneLayerMap;
  m._pixelTable = _geomSeq;

  _totSignalSegCnt++;

  _currentModel = getRCmodel(0);
  m._currentModel = _currentModel;
  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    m._metRCTable.add(_metRCTable.get(ii));
  }
  if (m._met >= _currentModel->getLayerCnt())  // TO_TEST
    return;

  m._layerCnt = _currentModel->getLayerCnt();

  double deltaFr[20];
  for (uint jj = 0; jj < m._metRCTable.getCnt(); jj++) {
    deltaFr[jj] = 0.0;
    m._rc[jj]->_coupling = 0.0;
    m._rc[jj]->_fringe = 0.0;
    m._rc[jj]->_diag = 0.0;
    m._rc[jj]->_res = 0.0;
    m._rc[jj]->_sep = 0;
  }

  uint totLenCovered = 0;
  if (_usingMetalPlanes) {
    if (_ccContextArray
        && ((!srcNet || (int) srcNet->getId() == ttttsrcnet)
            || (!tgtNet || (int) tgtNet->getId() == tttttgtnet))
        && (!ttttm || m._met == ttttm)) {
      int pxy = m._dir ? m._ll[0] : m._ll[1];
      int pbase = m._dir ? m._ur[1] : m._ur[0];
      //		  fprintf(stdout, "Context of layer %d, xy=%d len=%d
      // base=%d width=%d :\n", m._met, pxy, m._len, pbase, m._s_nm);
      logger_->info(RCX,
                    141,
                    "Context of layer {} xy={} len={} base={} width={}",
                    m._met,
                    pxy,
                    m._len,
                    pbase,
                    m._s_nm);
      uint ii, jj;
      for (ii = 1; ii <= _ccContextDepth
                   && (int) ii + m._met < _currentModel->getLayerCnt();
           ii++) {
        logger_->info(RCX, 142, "  layer {}", ii + m._met);
        for (jj = 0; jj < _ccContextArray[ii + m._met]->getCnt(); jj++)
          logger_->info(RCX,
                        143,
                        "    {}: {}",
                        jj,
                        _ccContextArray[ii + m._met]->get(jj));
      }
      for (ii = 1; ii <= _ccContextDepth && m._met - ii > 0; ii++) {
        logger_->info(RCX, 142, "  layer {}", m._met - ii);
        for (jj = 0; jj < _ccContextArray[m._met - ii]->getCnt(); jj++)
          logger_->info(RCX,
                        143,
                        "    {}: {}",
                        jj,
                        _ccContextArray[m._met - ii]->get(jj));
      }
    }
    //		for (uint ii= 0; _ccContextArray!=NULL && m._met>1 &&
    // ii<_ccContextArray[m._met]->getCnt(); ii++) {
    // fprintf(stdout, "ii= %d
    // -- %d\n", ii, _ccContextArray[m._met]->get(ii));
    //		}
    totLenCovered = m.measureOverUnderCap();
  }
  int lenOverSub = m._len - totLenCovered;

  //	int mUnder= m._underMet; // will be replaced
  if (m._dist < 0) {  // dist is infinit

    if (totLenCovered > 0) {
      m._underMet = 0;
      for (uint jj = 0; jj < m._metRCTable.getCnt(); jj++) {
        extDistRC* rc = m._metRCTable.get(jj)->getOverFringeRC(&m);
        deltaFr[jj] = rc->getFringe() * totLenCovered;
      }
    }
  } else {  // dist based

    if (lenOverSub > 0) {
      m._underMet = 0;
      m.computeOverRC(lenOverSub);
    }
    // deltaFr[jj]= getFringe(m._met, m._width, jj) * m._len; TO_TEST
    m.getFringe(m._len, deltaFr);

    if ((rsegId1 > 0) && (rsegId2 > 0)) {  // signal nets

      _totCCcnt++;  // TO_TEST

      if (m._rc[_minModelIndex]->_coupling < _coupleThreshold) {  // TO_TEST
        updateTotalCap(rseg1, &m, deltaFr, m._metRCTable.getCnt(), true);
        updateTotalCap(rseg2, &m, deltaFr, m._metRCTable.getCnt(), true);

        _totSmallCCcnt++;

        return;
      }
      _totBigCCcnt++;

      dbCCSeg* ccap = dbCCSeg::create(
          dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
          dbCapNode::getCapNode(_block, rseg2->getTargetNode()),
          true);
      // dbCCSeg *ccap= dbCCSeg::create(srcNet,
      // rseg1->getTargetNode(), tgtNet, rseg2->getTargetNode(), true);

      int extDbIndex, sci, scDbIdx;
      for (uint jj = 0; jj < m._metRCTable.getCnt(); jj++) {
        extDbIndex = getProcessCornerDbIndex(jj);
        ccap->addCapacitance(m._rc[jj]->_coupling, extDbIndex);
        getScaledCornerDbIndex(jj, sci, scDbIdx);
        if (sci != -1) {
          double cap = m._rc[jj]->_coupling;
          getScaledGndC(sci, cap);
          ccap->addCapacitance(cap, scDbIdx);
        }
      }
      updateTotalCap(rseg1, &m, deltaFr, m._metRCTable.getCnt(), false);
      updateTotalCap(rseg2, &m, deltaFr, m._metRCTable.getCnt(), false);
    } else if (rseg1 != NULL) {
      updateTotalCap(rseg1, &m, deltaFr, m._metRCTable.getCnt(), true);
    } else if (rseg2 != NULL) {
      updateTotalCap(rseg2, &m, deltaFr, m._metRCTable.getCnt(), true);
    }
  }
  ccReportProgress();
}
void extCompute(int* options, void* computePtr)
{
  extMain* mmm = (extMain*) computePtr;
  mmm->measureRC(options);
}
void extCompute1(int* options, void* computePtr)
{
  extMeasure* mmm = (extMeasure*) computePtr;
  if (options && options[0] < 0) {
    if (options[5] == 1)
      mmm->initTargetSeq();
    else
      mmm->getDgOverlap(options);
  } else if (options)
    mmm->measureRC(options);
  else
    mmm->printDgContext();
}

uint extMain::makeTree(uint netId)
{
  if (netId == 0)
    return 0;

  extRcTree* tree = new extRcTree(_block, logger_);

  uint test = 0;
  double max_cap = 10.00;
  tree->makeTree(dbNet::getNet(_block, netId),
                 max_cap,
                 test,
                 true,
                 true,
                 1.0,
                 _block->getSearchDb());
  return 0;
}

}  // namespace rcx
