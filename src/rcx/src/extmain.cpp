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

#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "utl/Logger.h"

namespace rcx {

using odb::dbBlock;
using odb::dbCapNode;
using odb::dbCCSeg;
using odb::dbDatabase;
using odb::dbNet;
using odb::dbRSeg;
using odb::dbSet;
using odb::dbTech;
using odb::dbTechLayer;
using utl::RCX;

void extMain::init(odb::dbDatabase* db, Logger* logger)
{
  _db = db;
  _block = nullptr;
  _blockId = 0;
  logger_ = logger;
}

void extMain::addDummyCorners(dbBlock* block, uint cnt, Logger* logger)
{
  extMain* tmiExt = (extMain*) block->getExtmi();
  if (tmiExt == nullptr) {
    logger->error(RCX, 252, "Ext object on dbBlock is nullptr!");
    return;
  }
  tmiExt->addDummyCorners(cnt);
}

void extMain::initExtractedCorners(dbBlock* block)
{
  extMain* tmiExt = (extMain*) block->getExtmi();
  if (tmiExt == nullptr) {
    tmiExt = new extMain;
    tmiExt->init((dbDatabase*) block->getDataBase(), logger_);
  }
  if (tmiExt->_processCornerTable) {
    return;
  }
  tmiExt->getPrevControl();
  tmiExt->getExtractedCorners();
}

int extMain::getExtCornerIndex(dbBlock* block, const char* cornerName)
{
  extMain* tmiExt = (extMain*) block->getExtmi();
  if (tmiExt == nullptr) {
    tmiExt = new extMain;
    tmiExt->init((dbDatabase*) block->getDataBase(), logger_);
  }
  int idx = tmiExt->getDbCornerIndex(cornerName);
  return idx;
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
  if (_btermTable) {
    return;
  }
  uint btermCnt = 0;
  if ((itermCnt == 0) && (_block != nullptr)) {
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

extMain::extMain()
{
  _modelTable = new Ath__array1D<extRCModel*>(8);
}

extMain::~extMain()
{
  delete _modelTable;
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
  if (_diagFlow) {
    _dgContextTracks = _couplingFlag * 2 + 1;
  } else {
    _dgContextTracks = _couplingFlag * 2 + 1;
  }
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
  if (!_dgContextPlanes || !_dgContextArray) {
    return;
  }
  delete[] _dgContextBaseTrack;
  delete[] _dgContextLowTrack;
  delete[] _dgContextHiTrack;
  for (uint jj = 0; jj < _dgContextPlanes; jj++) {
    delete[] _dgContextTrackBase[jj];
    for (uint tt = 0; tt < _dgContextTracks; tt++) {
      delete _dgContextArray[jj][tt];
    }
    delete[] _dgContextArray[jj];
  }
  delete[] _dgContextTrackBase;
  delete[] _dgContextArray;
  _dgContextArray = nullptr;
}

void extMain::initContextArray()
{
  if (_ccContextArray) {
    return;
  }
  uint layerCnt = getExtLayerCnt(_tech);
  _ccContextArray = new Ath__array1D<int>*[layerCnt + 1];
  _ccContextArray[0] = nullptr;
  uint ii;
  for (ii = 1; ii <= layerCnt; ii++) {
    _ccContextArray[ii] = new Ath__array1D<int>(1024);
  }
  _ccMergedContextArray = new Ath__array1D<int>*[layerCnt + 1];
  _ccMergedContextArray[0] = nullptr;
  for (ii = 1; ii <= layerCnt; ii++) {
    _ccMergedContextArray[ii] = new Ath__array1D<int>(1024);
  }
}

uint extMain::getExtLayerCnt(dbTech* tech)
{
  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;

  uint n = 0;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;

    if (layer->getRoutingLevel() == 0) {
      continue;
    }

    n++;
  }
  return n;
}

extRCModel* extMain::getRCmodel(uint n)
{
  if (_modelTable->getCnt() <= 0) {
    return nullptr;
  }

  return _modelTable->get(n);
}

uint extMain::getResCapTable()
{
  calcMinMaxRC();
  _currentModel = getRCmodel(0);

  extMeasure m(logger_);
  m._underMet = 0;
  m._overMet = 0;

  uint cnt = 0;
  for (dbTechLayer* layer : _tech->getLayers()) {
    if (layer->getRoutingLevel() == 0) {
      continue;
    }

    const uint n = layer->getRoutingLevel();

    const uint w = layer->getWidth();  // nm
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
    for (uint jj = 0; jj < _modelMap.getCnt(); jj++) {
      resTable[jj] = 0.0;
    }
    calcRes0(resTable, n, w, 1);
    for (uint jj = 0; jj < _modelMap.getCnt(); jj++) {
      const uint modelIndex = _modelMap.get(jj);
      extMetRCTable* rcModel = _currentModel->getMetRCTable(modelIndex);

      const double res = layer->getResistance();  // OHMS per square
      _resistanceTable[jj][n] = res;

      _capacitanceTable[jj][n] = 0.0;

      extDistRC* rc = rcModel->getOverFringeRC(&m);

      if (rc != nullptr) {
        const double r1 = rc->getRes();
        _capacitanceTable[jj][n] = rc->getFringe();
        debugPrint(logger_,
                   RCX,
                   "extrules",
                   1,
                   "EXT_RES: R "
                   "Layer= {} met= {}   w= {} cc= {:g} fr= {:g} res= {:g} "
                   "model_res= {:g} new_model_res= {:g}",
                   layer->getConstName(),
                   n,
                   w,
                   rc->getCoupling(),
                   rc->getFringe(),
                   res,
                   r1,
                   resTable[jj]);
      }

      if (!_lef_res) {
        _resistanceTable[jj][n] = resTable[jj];
      } else {
        debugPrint(logger_,
                   RCX,
                   "extrules",
                   1,
                   "EXT_RES_LEF: R Layer= {} met= {}  lef_res= {:g}",
                   layer->getConstName(),
                   n,
                   res);
      }
    }
    cnt++;
  }
  return cnt;
}

bool extMain::checkLayerResistance()
{
  dbSet<dbTechLayer> layers = _tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;

  uint cnt = 0;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;

    if (layer->getRoutingLevel() == 0) {
      continue;
    }

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

double extMain::getLefResistance(const uint level,
                                 const uint width,
                                 const uint len,
                                 const uint model)
{
  double n = len;

  if (_lef_res) {
    n /= width;
  }

  return n * _resistanceTable[model][level];
}

double extMain::getResistance(const uint level,
                              const uint width,
                              const uint len,
                              const uint model)
{
  return getLefResistance(level, width, len, model);
}

void extMain::setBlockFromChip()
{
  if (_db->getChip() == nullptr) {
    logger_->error(RCX, 497, "No design is loaded.");
  }
  _tech = _db->getTech();
  _block = _db->getChip()->getBlock();
  _blockId = _block->getId();
  _prevControl = _block->getExtControl();
  _block->setExtmi(this);

  if (_spef != nullptr) {
    _spef = nullptr;
    _extracted = false;
  }

  _bufSpefCnt = 0;
  _origSpefFilePrefix = nullptr;
  _newSpefFilePrefix = nullptr;
}

void extMain::setBlock(dbBlock* block)
{
  _block = block;
  _prevControl = _block->getExtControl();
  _block->setExtmi(this);
  _blockId = _block->getId();
  if (_spef) {
    _spef = nullptr;
    _extracted = false;
  }
  _bufSpefCnt = 0;
  _origSpefFilePrefix = nullptr;
  _newSpefFilePrefix = nullptr;
}

double extMain::getLoCoupling()
{
  return _coupleThreshold;
}

double extMain::getFringe(const uint met,
                          const uint width,
                          const uint modelIndex,
                          double& areaCap)
{
  areaCap = 0.0;
  if (_noModelRC) {
    return 0.0;
  }

  if (width == _minWidthTable[met]) {
    return _capacitanceTable[modelIndex][met];
  }

  // just in case

  extMeasure m(logger_);

  m._met = met;
  m._width = width;
  m._underMet = 0;
  m._ccContextArray = _ccContextArray;
  m._ccMergedContextArray = _ccMergedContextArray;

  extDistRC* rc = _metRCTable.get(modelIndex)->getOverFringeRC(&m);

  if (rc == nullptr) {
    return 0.0;
  }
  return rc->getFringe();
}

void extMain::updateTotalCap(dbRSeg* rseg,
                             double frCap,
                             double ccCap,
                             double deltaFr,
                             uint modelIndex)
{
  double cap = frCap + ccCap - deltaFr;

  double tot = rseg->getCapacitance(modelIndex);
  tot += cap;

  rseg->setCapacitance(tot, modelIndex);
}

void extMain::updateTotalRes(dbRSeg* rseg1,
                             dbRSeg* rseg2,
                             extMeasure* m,
                             const double* delta,
                             uint modelCnt)
{
  for (uint modelIndex = 0; modelIndex < modelCnt; modelIndex++) {
    extDistRC* rc = m->_rc[modelIndex];

    double res = rc->_res - delta[modelIndex];
    if (_resModify) {
      res *= _resFactor;
    }

    if (rseg1 != nullptr) {
      double tot = rseg1->getResistance(modelIndex);
      tot += res;

      rseg1->setResistance(tot, modelIndex);
    }
    if (rseg2 != nullptr) {
      double tot = rseg2->getResistance(modelIndex);
      tot += res;

      rseg2->setResistance(tot, modelIndex);
    }
  }
}

void extMain::updateTotalCap(dbRSeg* rseg,
                             extMeasure* m,
                             const double* deltaFr,
                             uint modelCnt,
                             bool includeCoupling,
                             bool includeDiag)
{
  double tot, cap;
  int extDbIndex, sci, scDbIdx;
  for (uint modelIndex = 0; modelIndex < modelCnt; modelIndex++) {
    extDistRC* rc = m->_rc[modelIndex];

    double frCap = rc->_fringe;

    double ccCap = 0.0;
    if (includeCoupling) {
      ccCap = rc->_coupling;
    }

    double diagCap = 0.0;
    if (includeDiag) {
      diagCap = rc->_diag;
    }

    cap = frCap + ccCap + diagCap - deltaFr[modelIndex];
    if (_gndcModify) {
      cap *= _gndcFactor;
    }

    extDbIndex = getProcessCornerDbIndex(modelIndex);
    tot = rseg->getCapacitance(extDbIndex);
    tot += cap;

    rseg->setCapacitance(tot, extDbIndex);
    getScaledCornerDbIndex(modelIndex, sci, scDbIdx);
    if (sci == -1) {
      continue;
    }
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
  bool mergeParallel = true;

  uint lcnt = 1;
  for (uint ii = 0; ii < lcnt; ii++) {
    if (mergeParallel) {
      ccap->addCapacitance(ccCap, ii);
    } else {
      ccap->setCapacitance(ccCap, ii);
    }
  }
}

void extMain::ccReportProgress()
{
  uint repChunk = 1000000;
  if ((_totSegCnt > 0) && (_totSegCnt % repChunk == 0)) {
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
  if (netId == net->getId()) {
    net->printNetName(stdout);
  }
}

void extMain::measureRC(CoupleOptions& options)
{
  _totSegCnt++;
  int rsegId1 = options[1];  // dbRSeg id for SRC segment
  int rsegId2 = options[2];  // dbRSeg id for Target segment

  if ((rsegId1 < 0) && (rsegId2 < 0)) {  // power nets
    return;
  }

  extMeasure m(logger_);
  m.defineBox(options);
  m._ccContextArray = _ccContextArray;
  m._ccMergedContextArray = _ccMergedContextArray;

  dbRSeg* rseg1 = nullptr;
  dbNet* srcNet = nullptr;
  if (rsegId1 > 0) {
    rseg1 = dbRSeg::getRSeg(_block, rsegId1);
    srcNet = rseg1->getNet();
    printNet(srcNet, _debug_net_id);
  }

  dbRSeg* rseg2 = nullptr;
  dbNet* tgtNet = nullptr;
  if (rsegId2 > 0) {
    rseg2 = dbRSeg::getRSeg(_block, rsegId2);
    tgtNet = rseg2->getNet();
    printNet(tgtNet, _debug_net_id);
  }

  m._pixelTable = _geomSeq;

  _totSignalSegCnt++;

  _currentModel = getRCmodel(0);
  m._currentModel = _currentModel;
  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    m._metRCTable.add(_metRCTable.get(ii));
  }
  if (m._met >= _currentModel->getLayerCnt()) {  // TO_TEST
    return;
  }

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
        for (jj = 0; jj < _ccContextArray[ii + m._met]->getCnt(); jj++) {
          logger_->info(RCX,
                        476,
                        "    {}: {}",
                        jj,
                        _ccContextArray[ii + m._met]->get(jj));
        }
      }
      for (ii = 1; ii <= _ccContextDepth && m._met - ii > 0; ii++) {
        logger_->info(RCX, 65, "  layer {}", m._met - ii);
        for (jj = 0; jj < _ccContextArray[m._met - ii]->getCnt(); jj++) {
          logger_->info(RCX,
                        143,
                        "    {}: {}",
                        jj,
                        _ccContextArray[m._met - ii]->get(jj));
        }
      }
    }
    totLenCovered = m.measureOverUnderCap();
  }
  int lenOverSub = m._len - totLenCovered;

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
    } else if (rseg1 != nullptr) {
      updateTotalCap(rseg1, &m, deltaFr, m._metRCTable.getCnt(), true);
    } else if (rseg2 != nullptr) {
      updateTotalCap(rseg2, &m, deltaFr, m._metRCTable.getCnt(), true);
    }
  }
  ccReportProgress();
}

extern CoupleOptions coupleOptionsNull;

void extCompute1(CoupleOptions& options, void* computePtr)
{
  extMeasure* mmm = (extMeasure*) computePtr;
  if (options != coupleOptionsNull && options[0] < 0) {
    if (options[5] == 1) {
      mmm->initTargetSeq();
    } else {
      mmm->getDgOverlap(options);
    }
  } else if (options != coupleOptionsNull) {
    mmm->measureRC(options);
  } else {
    mmm->printDgContext();
  }
}

}  // namespace rcx
