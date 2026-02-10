// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "gseq.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "rcx/array1.h"
#include "rcx/dbUtil.h"
#include "rcx/ext2dBox.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

using odb::dbBTerm;
using odb::dbCapNode;
using odb::dbCCSeg;
using odb::dbNet;
using odb::dbRSeg;
using odb::dbSet;
using odb::dbShape;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbWire;
using odb::dbWirePath;
using odb::dbWirePathItr;
using odb::dbWirePathShape;
using utl::RCX;

namespace rcx {

bool extMeasure::getFirstShape(dbNet* net, dbShape& s)
{
  dbWirePath path;
  dbWirePathShape pshape;

  dbWirePathItr pitr;
  dbWire* wire = net->getWire();

  bool status = false;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    pitr.getNextShape(pshape);
    s = pshape.shape;
    status = true;
    break;
  }
  return status;
}

dbRSeg* extMeasure::getRseg(const char* netname,
                            const char* capMsg,
                            const char* tableEntryName)
{
  dbNet* net = _block->findNet(netname);
  if (net == nullptr) {
    logger_->warn(RCX,
                  74,
                  "Cannot find net {} from the {} table entry {}",
                  netname,
                  capMsg,
                  tableEntryName);
    return nullptr;
  }
  dbRSeg* r = getFirstDbRseg(net->getId());
  if (r == nullptr) {
    logger_->warn(RCX,
                  460,
                  "Cannot find dbRseg for net {} from the {} table entry {}",
                  netname,
                  capMsg,
                  tableEntryName);
  }
  return r;
}

void extMeasure::getMinWidth(dbTech* tech)
{
  dbSet<dbTechLayer> layers = tech->getLayers();
  dbSet<dbTechLayer>::iterator litr;
  dbTechLayer* layer;
  for (litr = layers.begin(); litr != layers.end(); ++litr) {
    layer = *litr;
    if (layer->getRoutingLevel() == 0) {
      continue;
    }

    uint32_t level = layer->getRoutingLevel();
    uint32_t pitch = layer->getPitch();
    uint32_t minWidth = layer->getWidth();
    _minSpaceTable[level] = pitch - minWidth;
  }
}

void extMeasure::updateBox(uint32_t w_layout, uint32_t s_layout, int dir)
{
  uint32_t d = _dir;
  if (dir >= 0) {
    d = dir;
  }

  _ll[d] = _ur[d] + s_layout;
  _ur[d] = _ll[d] + w_layout;
}

uint32_t extMeasure::createNetSingleWire(char* dirName,
                                         uint32_t idCnt,
                                         uint32_t w_layout,
                                         uint32_t s_layout,
                                         int dir)
{
  dbTechLayer* layer = _create_net_util.getRoutingLayer()[_met];

  if (w_layout == 0) {
    w_layout = layer->getWidth();
  }
  if (s_layout == 0) {
    uint32_t d = _dir;
    if (dir >= 0) {
      d = dir;
    }
    _ur[d] = _ll[d] + w_layout;
  } else {
    updateBox(w_layout, s_layout, dir);
  }

  int ll[2];
  int ur[2];
  ll[0] = _ll[0];
  ll[1] = _ll[1];
  ur[0] = _ur[0];
  ur[1] = _ur[1];

  ur[!_dir] = ur[!_dir] - w_layout / 2;
  ll[!_dir] = ll[!_dir] + w_layout / 2;

  char left, right;
  _block->getBusDelimiters(left, right);

  char netName[1024];
  sprintf(netName, "%s%c%d%c", dirName, left, idCnt, right);
  if (_skip_delims) {
    sprintf(netName, "%s_%d", dirName, idCnt);
  }

  assert(_create_net_util.getBlock() == _block);

  dbNet* net;
  if (layer->getNumMasks() > 1) {
    // Alternate mask colors based on id
    uint8_t mask_color = (idCnt % layer->getNumMasks()) + 1;
    net = _create_net_util.createNetSingleWire(netName,
                                               ll[0],
                                               ll[1],
                                               ur[0],
                                               ur[1],
                                               _met,
                                               /*skipBTerms=*/false,
                                               /*skipExistsNet=*/false,
                                               mask_color);
  } else {
    net = _create_net_util.createNetSingleWire(
        netName, ll[0], ll[1], ur[0], ur[1], _met);
  }

  dbBTerm* in1 = net->get1stBTerm();
  if (in1 != nullptr) {
    in1->rename(net->getConstName());
  }

  uint32_t netId = net->getId();
  addNew2dBox(net, ll, ur, _met, false);

  _extMain->makeNetRCsegs(net);

  return netId;
}

uint32_t extMeasure::createNetSingleWire_cntx(int met,
                                              char* dirName,
                                              uint32_t idCnt,
                                              int d,
                                              int ll[2],
                                              int ur[2],
                                              int s_layout)
{
  char netName[1024];

  sprintf(netName, "%s_cntxM%d_%d", dirName, met, idCnt);

  assert(_create_net_util.getBlock() == _block);
  dbNet* net = _create_net_util.createNetSingleWire(
      netName, ll[0], ll[1], ur[0], ur[1], met);
  dbBTerm* in1 = net->get1stBTerm();
  if (in1 != nullptr) {
    in1->rename(net->getConstName());
  }
  _extMain->makeNetRCsegs(net);

  return net->getId();
}

uint32_t extMeasure::createDiagNetSingleWire(char* dirName,
                                             uint32_t idCnt,
                                             int begin,
                                             int w_layout,
                                             int s_layout,
                                             int dir)
{
  int ll[2], ur[2];
  ll[!_dir] = _ll[!_dir];
  ll[_dir] = begin;
  ur[!_dir] = _ur[!_dir];
  ur[_dir] = begin + w_layout;

  int met = 0;
  if (_overMet > 0) {
    met = _overMet;
  } else if (_underMet > 0) {
    met = _underMet;
  }

  char left, right;
  _block->getBusDelimiters(left, right);

  char netName[1024];
  sprintf(netName, "%s%c%d%c", dirName, left, idCnt, right);
  if (_skip_delims) {
    sprintf(netName, "%s_%d", dirName, idCnt);
  }

  assert(_create_net_util.getBlock() == _block);
  dbNet* net = _create_net_util.createNetSingleWire(
      netName, ll[0], ll[1], ur[0], ur[1], met);
  addNew2dBox(net, ll, ur, met, false);

  _extMain->makeNetRCsegs(net);

  return net->getId();
}

ext2dBox* extMeasure::addNew2dBox(dbNet* net,
                                  int* ll,
                                  int* ur,
                                  uint32_t m,
                                  bool cntx)
{
  ext2dBox* bb = _2dBoxPool->alloc();

  std::array<int, 2> bb_ll;
  std::array<int, 2> bb_ur;
  dbShape s;
  if ((net != nullptr) && _extMain->getFirstShape(net, s)) {
    bb_ll = {s.xMin(), s.yMin()};
    bb_ur = {s.xMax(), s.yMax()};
  } else {
    bb_ll = {ll[0], ll[1]};
    bb_ur = {ur[0], ur[1]};
  }

  new (bb) ext2dBox(bb_ll, bb_ur);

  if (cntx) {  // context net
    _2dBoxTable[1][m].add(bb);
  } else {  // main net
    _2dBoxTable[0][m].add(bb);
  }

  return bb;
}

void extMeasure::clean2dBoxTable(int met, bool cntx)
{
  if (met <= 0) {
    return;
  }
  for (uint32_t ii = 0; ii < _2dBoxTable[cntx][met].getCnt(); ii++) {
    ext2dBox* bb = _2dBoxTable[cntx][met].get(ii);
    _2dBoxPool->free(bb);
  }
  _2dBoxTable[cntx][met].resetCnt();
}

void extMeasure::getBox(int met,
                        bool cntx,
                        int& xlo,
                        int& ylo,
                        int& xhi,
                        int& yhi)
{
  if (met <= 0) {
    return;
  }

  int cnt = _2dBoxTable[cntx][met].getCnt();
  if (cnt <= 0) {
    return;
  }

  ext2dBox* bbLo = _2dBoxTable[cntx][met].get(0);
  ext2dBox* bbHi = _2dBoxTable[cntx][met].get(cnt - 1);

  xlo = std::min(bbLo->loX(), bbHi->loX());
  ylo = std::min(bbLo->loY(), bbHi->loY());

  xhi = std::max(bbLo->ur0(), bbHi->ur0());
  yhi = std::max(bbLo->ur1(), bbHi->ur1());
}

void extMeasure::writeRaphaelPointXY(FILE* fp, double X, double Y)
{
  fprintf(fp, "  %6.3f,%6.3f ; ", X, Y);
}

uint32_t extMeasure::createContextNets(char* dirName,
                                       const int bboxLL[2],
                                       const int bboxUR[2],
                                       int met,
                                       double pitchMult)
{
  if (met <= 0) {
    return 0;
  }

  dbTechLayer* layer = _tech->findRoutingLayer(met);
  dbTechLayer* mlayer = _tech->findRoutingLayer(_met);
  uint32_t minWidth = layer->getWidth();
  uint32_t minSpace = layer->getSpacing();
  int pitch = lround(1000 * ((minWidth + minSpace) * pitchMult) / 1000);

  int ll[2];
  int ur[2];

  uint32_t offset = 0;
  ll[_dir] = bboxLL[_dir] - offset;
  ur[_dir] = bboxUR[_dir] + offset;

  _ur[_dir] = ur[_dir];

  uint32_t cnt = 1;

  uint32_t not_dir = !_dir;
  int start = bboxLL[not_dir] + offset;
  int end = bboxUR[not_dir];
  for (int lenXY = (int) (start + minWidth); (int) (lenXY + minWidth) <= end;
       lenXY += pitch) {
    ll[not_dir] = lenXY;
    ur[not_dir] = lenXY + minWidth;

    char netName[1024];
    sprintf(netName, "%s_m%d_cntxt_%d", dirName, met, cnt++);
    dbNet* net;
    assert(_create_net_util.getBlock() == _block);
    if (mlayer->getDirection() != dbTechLayerDir::HORIZONTAL) {
      net = _create_net_util.createNetSingleWire(netName,
                                                 ll[0],
                                                 ll[1],
                                                 ur[0],
                                                 ur[1],
                                                 met,
                                                 dbTechLayerDir::HORIZONTAL,
                                                 false);
    } else {
      net = _create_net_util.createNetSingleWire(netName,
                                                 ll[0],
                                                 ll[1],
                                                 ur[0],
                                                 ur[1],
                                                 met,
                                                 mlayer->getDirection(),
                                                 false);
    }
    addNew2dBox(net, ll, ur, met, true);
  }
  return cnt - 1;
}

dbRSeg* extMeasure::getFirstDbRseg(uint32_t netId)
{
  dbNet* net = dbNet::getNet(_block, netId);

  dbSet<dbRSeg> rSet = net->getRSegs();
  dbSet<dbRSeg>::iterator rc_itr;

  dbRSeg* rseg = nullptr;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    rseg = *rc_itr;
    break;
  }

  return rseg;
}

void extMeasure::printBox(FILE* fp)
{
  fprintf(fp, "( %8d %8d ) ( %8d %8d )\n", _ll[0], _ll[1], _ur[0], _ur[1]);
}

uint32_t extMeasure::initWS_box(extMainOptions* opt, uint32_t gridCnt)
{
  dbTechLayer* layer = opt->_tech->findRoutingLayer(_met);
  _minWidth = layer->getWidth();
  _pitch = layer->getPitch();
  _minSpace = _pitch - _minWidth;
  _dir = layer->getDirection() == dbTechLayerDir::HORIZONTAL ? 1 : 0;

  uint32_t patternSep = gridCnt * (_minWidth + _minSpace);

  _ll[0] = opt->_ur[0] + patternSep;
  _ll[1] = 0;

  _ur[_dir] = _ll[_dir];
  _ur[!_dir]
      = _ll[!_dir] + opt->_len * _minWidth / 1000;  // _len is in nm per ext.ti

  return patternSep;
}

void extMeasure::updateForBench(extMainOptions* opt, extMain* extMain)
{
  _len = opt->_len;
  _wireCnt = opt->_wireCnt;
  _block = opt->_block;
  _tech = opt->_tech;
  _extMain = extMain;
  _create_net_util.setBlock(_block, false);
  _dbunit = _block->getDbUnitsPerMicron();
}

uint32_t extMeasure::defineBox(CoupleOptions& options)
{
  _no_debug = false;
  _met = options[0];

  _len = options[3];
  _dist = options[4];
  _s_nm = options[4];

  int xy = options[5];
  _dir = options[6];

  _width = options[7];
  _w_nm = options[7];
  int base = options[9];
  // _dir= 1 horizontal
  // _dir= 0 vertical

  if (_dir == 0) {
    _ll[1] = xy;
    _ll[0] = base;
    _ur[1] = xy + _len;
    _ur[0] = base + _width;
  } else {
    _ll[0] = xy;
    _ll[1] = base;
    _ur[0] = xy + _len;
    _ur[1] = base + _width;
  }
  for (uint32_t ii = 0; ii < _metRCTable.getCnt(); ii++) {
    _rc[ii]->coupling_ = 0.0;
    _rc[ii]->fringe_ = 0.0;
    _rc[ii]->diag_ = 0.0;
    _rc[ii]->res_ = 0.0;
    _rc[ii]->sep_ = 0;
  }
  dbTechLayer* layer = _extMain->_tech->findRoutingLayer(_met);
  _minWidth = layer->getWidth();
  _toHi = (options[11] > 0) ? true : false;

  return _len;
}

void extMeasure::tableCopyP(Array1D<int>* src, Array1D<int>* dst)
{
  for (uint32_t ii = 0; ii < src->getCnt(); ii++) {
    dst->add(src->get(ii));
  }
}

void extMeasure::tableCopyP(Array1D<SEQ*>* src, Array1D<SEQ*>* dst)
{
  for (uint32_t ii = 0; ii < src->getCnt(); ii++) {
    dst->add(src->get(ii));
  }
}

void extMeasure::tableCopy(Array1D<SEQ*>* src,
                           Array1D<SEQ*>* dst,
                           gs* pixelTable)
{
  for (uint32_t ii = 0; ii < src->getCnt(); ii++) {
    copySeq(src->get(ii), dst, pixelTable);
  }
}

void extMeasure::release(Array1D<SEQ*>* seqTable, gs* pixelTable)
{
  if (pixelTable == nullptr) {
    pixelTable = _pixelTable;
  }

  for (uint32_t ii = 0; ii < seqTable->getCnt(); ii++) {
    pixelTable->release(seqTable->get(ii));
  }

  seqTable->resetCnt();
}

int extMeasure::calcDist(const int* ll, const int* ur)
{
  int d = ll[_dir] - _ur[_dir];
  if (d >= 0) {
    return d;
  }

  d = _ll[_dir] - ur[_dir];
  if (d >= 0) {
    return d;
  }
  return 0;
}

SEQ* extMeasure::addSeq(const int* ll, const int* ur)
{
  SEQ* s = _pixelTable->salloc();
  for (uint32_t ii = 0; ii < 2; ii++) {
    s->_ll[ii] = ll[ii];
    s->_ur[ii] = ur[ii];
  }
  s->type = 0;
  return s;
}

void extMeasure::addSeq(const int* ll,
                        const int* ur,
                        Array1D<SEQ*>* seqTable,
                        gs* pixelTable)
{
  if (pixelTable == nullptr) {
    pixelTable = _pixelTable;
  }

  SEQ* s = pixelTable->salloc();
  for (uint32_t ii = 0; ii < 2; ii++) {
    s->_ll[ii] = ll[ii];
    s->_ur[ii] = ur[ii];
  }
  s->type = 0;
  if (seqTable != nullptr) {
    seqTable->add(s);
  }
}

void extMeasure::addSeq(Array1D<SEQ*>* seqTable, gs* pixelTable)
{
  SEQ* s = pixelTable->salloc();
  for (uint32_t ii = 0; ii < 2; ii++) {
    s->_ll[ii] = _ll[ii];
    s->_ur[ii] = _ur[ii];
  }
  s->type = 0;

  seqTable->add(s);
}

void extMeasure::copySeq(SEQ* t, Array1D<SEQ*>* seqTable, gs* pixelTable)
{
  SEQ* s = pixelTable->salloc();
  for (uint32_t ii = 0; ii < 2; ii++) {
    s->_ll[ii] = t->_ll[ii];
    s->_ur[ii] = t->_ur[ii];
  }
  s->type = t->type;

  seqTable->add(s);
}

void extMeasure::copySeqUsingPool(SEQ* t, Array1D<SEQ*>* seqTable)
{
  SEQ* s = _seqPool->alloc();
  for (uint32_t ii = 0; ii < 2; ii++) {
    s->_ll[ii] = t->_ll[ii];
    s->_ur[ii] = t->_ur[ii];
  }
  s->type = t->type;

  seqTable->add(s);
}

uint32_t extMeasure::getOverUnderIndex()
{
  int n = _layerCnt - _met - 1;
  n *= _underMet - 1;
  n += _overMet - _met - 1;

  if ((n < 0) || (n >= (int) _layerCnt + 1)) {
    logger_->info(RCX,
                  459,
                  "getOverUnderIndex: out of range n= {}   m={} u= {} o= {}",
                  n,
                  _met,
                  _underMet,
                  _overMet);
  }

  return n;
}

extDistRC* extMeasure::getFringe(uint32_t len, double* valTable)
{
  extDistRC* rcUnit = nullptr;

  for (uint32_t ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    rcUnit = rcModel->getOverFringeRC(this);

    if (rcUnit == nullptr) {
      continue;
    }

    valTable[ii] = rcUnit->getFringe() * len;
  }
  return rcUnit;
}

void extLenOU::addOverOrUnderLen(int met, bool over, uint32_t len)
{
  _overUnder = false;
  _under = false;
  if (over) {
    _overMet = -1;
    _underMet = met;
    _over = true;
  } else {
    _overMet = met;
    _underMet = -1;
    _over = false;
    _under = true;
  }
  _len = len;
}

void extLenOU::addOULen(int underMet, int overMet, uint32_t len)
{
  _overUnder = true;
  _under = false;
  _over = false;

  _overMet = overMet;
  _underMet = underMet;

  _len = len;
}

uint32_t extMeasure::getLength(SEQ* s, int dir)
{
  return s->_ur[dir] - s->_ll[dir];
}

uint32_t extMeasure::blackCount(uint32_t start, Array1D<SEQ*>* resTable)
{
  uint32_t cnt = 0;
  for (uint32_t jj = start; jj < resTable->getCnt(); jj++) {
    SEQ* s = resTable->get(jj);

    if (s->type > 0) {  // Black
      cnt++;
    }
  }
  return cnt;
}

extDistRC* extMeasure::computeOverFringe(uint32_t overMet,
                                         uint32_t overWidth,
                                         uint32_t len,
                                         uint32_t dist)
{
  extDistRC* rcUnit = nullptr;

  for (uint32_t ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    rcUnit = rcModel->_capOver[overMet]->getRC(_met, overWidth, dist);

    if (IsDebugNet()) {
      rcUnit->printDebugRC(_met, overMet, 0, _width, dist, len, logger_);
    }

    if (rcUnit != nullptr) {
      _rc[ii]->fringe_ += rcUnit->fringe_ * len;
      _rc[ii]->res_ += rcUnit->res_ * len;
    }
  }
  return rcUnit;
}

extDistRC* extMeasure::computeUnderFringe(uint32_t underMet,
                                          uint32_t underWidth,
                                          uint32_t len,
                                          uint32_t dist)
{
  extDistRC* rcUnit = nullptr;

  uint32_t n = _met - underMet - 1;

  for (uint32_t ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    if (rcModel->_capUnder[underMet] == nullptr) {
      continue;
    }

    rcUnit = rcModel->_capUnder[underMet]->getRC(n, underWidth, dist);
    if (IsDebugNet()) {
      rcUnit->printDebugRC(_met, 0, underMet, _width, dist, len, logger_);
    }

    if (rcUnit != nullptr) {
      _rc[ii]->fringe_ += rcUnit->fringe_ * len;
      _rc[ii]->res_ += rcUnit->res_ * len;
    }
  }
  return rcUnit;
}

void extMeasure::swap_coords(SEQ* s)
{
  int xy = s->_ll[1];
  s->_ll[1] = s->_ll[0];
  s->_ll[0] = xy;

  xy = s->_ur[1];
  s->_ur[1] = s->_ur[0];
  s->_ur[0] = xy;
}

uint32_t extMeasure::swap_coords(uint32_t initCnt,
                                 uint32_t endCnt,
                                 Array1D<SEQ*>* resTable)
{
  for (uint32_t ii = initCnt; ii < endCnt; ii++) {
    swap_coords(resTable->get(ii));
  }

  return endCnt - initCnt;
}

uint32_t extMeasure::getOverlapSeq(uint32_t met,
                                   SEQ* s,
                                   Array1D<SEQ*>* resTable)
{
  return getOverlapSeq(met, s->_ll, s->_ur, resTable);
}

uint32_t extMeasure::getOverlapSeq(uint32_t met,
                                   int* ll,
                                   int* ur,
                                   Array1D<SEQ*>* resTable)
{
  uint32_t len1 = 0;

  if (!_rotatedGs) {
    len1 = _pixelTable->getSeq(ll, ur, _dir, met, resTable);
  } else {
    if (_dir > 0) {  // extracting horizontal segments
      len1 = _pixelTable->getSeq(ll, ur, _dir, met, resTable);
    } else {
      int sll[2];
      int sur[2];

      sll[0] = ll[1];
      sll[1] = ll[0];
      sur[0] = ur[1];
      sur[1] = ur[0];

      uint32_t initCnt = resTable->getCnt();

      len1 = _pixelTable->getSeq(sll, sur, !_dir, met, resTable);

      swap_coords(initCnt, resTable->getCnt(), resTable);
    }
  }

  if ((len1 >= 0) && (len1 <= _len)) {
    return len1;
  }
  return 0;
}

uint32_t extMeasure::computeOverOrUnderSeq(Array1D<SEQ*>* seqTable,
                                           uint32_t met,
                                           Array1D<SEQ*>* resTable,
                                           bool over)
{
  uint32_t len = 0;
  for (uint32_t ii = 0; ii < seqTable->getCnt(); ii++) {
    SEQ* s = seqTable->get(ii);

    if (s->type > 0) {  // Black
      continue;
    }
    if ((s->_ll[0] < _ll[0]) || (s->_ll[1] < _ll[1]) || (s->_ur[0] > _ur[0])
        || (s->_ur[1] > _ur[1])) {
      continue;
    }
    len += getOverlapSeq(met, s, resTable);

    uint32_t startIndex = resTable->getCnt();
    int maxDist = 1000;  // TO_TEST

    if (blackCount(startIndex, resTable) > 0) {
      for (uint32_t jj = startIndex; jj < resTable->getCnt(); jj++) {
        SEQ* q = resTable->get(jj);

        if (q->type > 0) {  // Black
          continue;
        }

        int dist = getLength(q, !_dir);

        if (dist < 0) {
          continue;  // TO_TEST
        }
        if (dist > maxDist) {
          continue;
        }

        if (over) {
          computeUnderFringe(met, _width, _width, dist);
        } else {
          computeOverFringe(met, _width, _width, dist);
        }
      }
    }
  }
  if (len > _len) {
    return 0;
  }

  if (len <= 0) {
    return 0;
  }

  if (over) {
    computeOverRC(len);
  } else {
    computeUnderRC(len);
  }

  return len;
}

uint32_t extMeasure::computeOUwith2planes(int* ll,
                                          int* ur,
                                          Array1D<SEQ*>* resTable)
{
  Array1D<SEQ*> met1Table(16);

  uint32_t met1 = _underMet;
  uint32_t met2 = _overMet;
  if (_met - _underMet > _overMet - _met) {
    met2 = _underMet;
    met1 = _overMet;
  }
  getOverlapSeq(met1, ll, ur, &met1Table);

  uint32_t len = 0;
  for (uint32_t ii = 0; ii < met1Table.getCnt(); ii++) {
    SEQ* s = met1Table.get(ii);

    if (s->type == 0) {  // white
      resTable->add(s);
      continue;
    }
    len += getOverlapSeq(met2, s, resTable);
    _pixelTable->release(s);
  }
  return len;
}

void extMeasure::calcOU(uint32_t len)
{
  computeOverUnderRC(len);
}

uint32_t extMeasure::computeOverUnder(int* ll, int* ur, Array1D<SEQ*>* resTable)
{
  uint32_t ouLen = computeOUwith2planes(ll, ur, resTable);

  if ((ouLen < 0) || (ouLen > _len)) {
    logger_->info(RCX,
                  456,
                  "pixelTable gave len {}, bigger than expected {}",
                  ouLen,
                  _len);
    return 0;
  }
  if (ouLen > 0) {
    calcOU(ouLen);
  }

  return ouLen;
}

uint32_t extMeasure::computeOverOrUnderSeq(Array1D<int>* seqTable,
                                           uint32_t met,
                                           Array1D<int>* resTable,
                                           bool over)
{
  if (seqTable->getCnt() <= 0) {
    return 0;
  }

  uint32_t len = 0;

  bool black = true;
  for (uint32_t ii = 0; ii < seqTable->getCnt() - 1; ii++) {
    int xy1 = seqTable->get(ii);
    int xy2 = seqTable->get(ii + 1);

    black = !black;

    if (black) {
      continue;
    }

    if (xy1 == xy2) {
      continue;
    }

    uint32_t len1 = mergeContextArray(
        _ccContextArray[met], _minSpaceTable[met], xy1, xy2, resTable);

    if (len1 >= 0) {
      len += len1;
    }
  }
  if (len > _len) {
    return 0;
  }

  if (len <= 0) {
    return 0;
  }

  if (over) {
    computeOverRC(len);
  } else {
    computeUnderRC(len);
  }

  return len;
}

uint32_t extMeasure::computeOverUnder(int xy1, int xy2, Array1D<int>* resTable)
{
  uint32_t ouLen
      = intersectContextArray(xy1, xy2, _underMet, _overMet, resTable);

  if ((ouLen < 0) || (ouLen > _len)) {
    logger_->info(RCX,
                  458,
                  "pixelTable gave len {}, bigger than expected {}",
                  ouLen,
                  _len);
    return 0;
  }
  if (ouLen > 0) {
    computeOverUnderRC(ouLen);
  }

  return ouLen;
}

uint32_t extMeasure::mergeContextArray(Array1D<int>* srcContext,
                                       int minS,
                                       Array1D<int>* tgtContext)
{
  tgtContext->resetCnt(0);
  uint32_t ssize = srcContext->getCnt();
  if (ssize < 4) {
    return 0;
  }
  uint32_t contextLength = 0;
  tgtContext->add(srcContext->get(0));
  int p1 = srcContext->get(1);
  int p2 = srcContext->get(2);
  int n1, n2;
  uint32_t jj = 3;
  while (jj < ssize - 2) {
    n1 = srcContext->get(jj++);
    n2 = srcContext->get(jj++);
    if (n1 - p2 <= minS) {
      p2 = n2;
    } else {
      tgtContext->add(p1);
      tgtContext->add(p2);
      contextLength += p2 - p1;
      p1 = n1;
      p2 = n2;
    }
  }
  tgtContext->add(p1);
  tgtContext->add(p2);
  tgtContext->add(srcContext->get(ssize - 1));
  contextLength += p2 - p1;
  return contextLength;
}

uint32_t extMeasure::mergeContextArray(Array1D<int>* srcContext,
                                       int minS,
                                       int pmin,
                                       int pmax,
                                       Array1D<int>* tgtContext)
{
  tgtContext->resetCnt(0);
  uint32_t ssize = srcContext->getCnt();
  if (ssize < 4) {
    return 0;
  }
  tgtContext->add(pmin);
  uint32_t contextLength = 0;
  int p1, p2, n1, n2;
  uint32_t jj;
  for (jj = 2; jj < ssize - 1; jj += 2) {
    if (srcContext->get(jj) > pmin) {
      break;
    }
  }
  if (jj >= ssize - 1) {
    tgtContext->add(pmax);
    return 0;
  }
  p1 = srcContext->get(jj - 1);
  p1 = std::max(p1, pmin);
  p2 = srcContext->get(jj++);
  p2 = std::min(p2, pmax);
  while (jj < ssize - 2) {
    n1 = srcContext->get(jj++);
    if (n1 >= pmax) {
      break;
    }
    n2 = srcContext->get(jj++);
    n2 = std::min(n2, pmax);
    if (n1 - p2 <= minS) {
      p2 = n2;
    } else {
      tgtContext->add(p1);
      tgtContext->add(p2);
      contextLength += p2 - p1;
      p1 = n1;
      p2 = n2;
    }
  }
  tgtContext->add(p1);
  tgtContext->add(p2);
  contextLength += p2 - p1;
  tgtContext->add(pmax);
  return contextLength;
}

uint32_t extMeasure::intersectContextArray(int pmin,
                                           int pmax,
                                           uint32_t met1,
                                           uint32_t met2,
                                           Array1D<int>* tgtContext)
{
  int minS1 = _minSpaceTable[met1];
  int minS2 = _minSpaceTable[met2];

  Array1D<int> t1Context(1024);
  mergeContextArray(_ccContextArray[met1], minS1, pmin, pmax, &t1Context);
  Array1D<int> t2Context(1024);
  mergeContextArray(_ccContextArray[met2], minS2, pmin, pmax, &t2Context);

  tgtContext->resetCnt(0);
  tgtContext->add(pmin);
  uint32_t tsize1 = t1Context.getCnt();
  uint32_t tsize2 = t2Context.getCnt();
  if (!tsize1 || !tsize2) {
    tgtContext->add(pmax);
    return 0;
  }
  uint32_t readc1 = 1;
  uint32_t readc2 = 1;
  uint32_t jj1 = 1;
  uint32_t jj2 = 1;
  uint32_t icontextLength = 0;
  int p1min, p1max, p2min, p2max, ptmin, ptmax;
  while (true) {
    if (readc1) {
      if (jj1 + 2 >= tsize1) {
        break;
      }
      p1min = t1Context.get(jj1++);
      p1max = t1Context.get(jj1++);
      readc1 = 0;
    }
    if (readc2) {
      if (jj2 + 2 >= tsize2) {
        break;
      }
      p2min = t2Context.get(jj2++);
      p2max = t2Context.get(jj2++);
      readc2 = 0;
    }
    if (p1min >= p2max) {
      readc2 = 1;
      continue;
    }
    if (p2min >= p1max) {
      readc1 = 1;
      continue;
    }
    ptmin = p1min > p2min ? p1min : p2min;
    ptmax = p1max < p2max ? p1max : p2max;
    tgtContext->add(ptmin);
    tgtContext->add(ptmax);
    icontextLength += ptmax - ptmin;
    if (p1max > p2max) {
      readc2 = 1;
    } else if (p1max < p2max) {
      readc1 = 1;
    } else {
      readc1 = readc2 = 1;
    }
  }
  tgtContext->add(pmax);
  return icontextLength;
}

uint32_t extMeasure::measureOverUnderCap()
{
  int ll[2] = {_ll[0], _ll[1]};
  int ur[2] = {_ur[0], _ur[1]};
  ur[_dir] = ll[_dir];

  _tmpTable->resetCnt();
  _ouTable->resetCnt();
  _overTable->resetCnt();
  _underTable->resetCnt();

  uint32_t ouLen = 0;
  uint32_t underLen = 0;
  uint32_t overLen = 0;

  if ((_met > 1) && (_met < (int) _layerCnt - 1)) {
    _underMet = _met - 1;
    _overMet = _met + 1;

    ouLen = computeOverUnder(ll, ur, _tmpTable);
  } else {
    addSeq(_tmpTable, _pixelTable);
    _underMet = _met;
    _overMet = _met;
  }
  uint32_t totUnderLen = underLen;
  uint32_t totOverLen = overLen;

  _underMet = _met;
  _overMet = _met;

  int remainderLength = _len - ouLen;

  while (remainderLength > 0) {
    _underMet--;
    _overMet++;

    underLen = 0;
    overLen = 0;
    if (_underMet > 0) {
      // cap over _underMet
      underLen = computeOverOrUnderSeq(_tmpTable, _underMet, _underTable, true);
      release(_tmpTable);

      if (_overMet < (int) _layerCnt) {
        // cap under _overMet
        overLen
            = computeOverOrUnderSeq(_underTable, _overMet, _overTable, false);
        release(_underTable);

        tableCopyP(_overTable, _tmpTable);
        _overTable->resetCnt();
        totOverLen += overLen;
      } else {
        tableCopyP(_underTable, _tmpTable);
        _underTable->resetCnt();
      }
      totUnderLen += underLen;
    } else if (_overMet < (int) _layerCnt) {
      overLen = computeOverOrUnderSeq(_tmpTable, _overMet, _overTable, false);
      release(_tmpTable);
      tableCopyP(_overTable, _tmpTable);
      _overTable->resetCnt();

      totOverLen += overLen;
    } else {
      break;
    }

    remainderLength -= (underLen + overLen);
  }
  release(_tmpTable);
  uint32_t totLen = ouLen + totOverLen + totUnderLen;

  return totLen;
}

bool extMeasure::updateLengthAndExit(int& remainder, int& totCovered, int len)
{
  if (len <= 0) {
    return false;
  }

  totCovered += len;
  remainder -= len;

  return remainder <= 0;
}

int extMeasure::getDgPlaneAndTrackIndex(uint32_t tgt_met,
                                        int trackDist,
                                        int& loTrack,
                                        int& hiTrack)
{
  int n = tgt_met - *_dgContextBaseLvl + *_dgContextDepth;
  assert(n >= 0);
  if (n >= (int) *_dgContextPlanes) {
    return -1;
  }

  loTrack
      = _dgContextLowTrack[n] < -trackDist ? -trackDist : _dgContextLowTrack[n];
  hiTrack = _dgContextHiTrack[n] > trackDist ? trackDist : _dgContextHiTrack[n];

  return n;
}

// CJ doc notes
// suppose we do "ext ectract -cc_model 3", then
//   *_dgContextTracks is 7 (3*2 + 1), and *_dgContextTracks/2 is 3
//   _dgContextLowTrack[planeIndex] can be -3 ~ 0
//   _dgContextHiTrack[planeIndex] can be 0 ~ 3
// if we want to handle all Dg tracks in the plane, we can say:
//   int lowTrack = _dgContextLowTrack[planeIndex];
//   int hiTrack = _dgContextHiTrack[planeIndex];
// or if we want only from the lower 2 tracks to the higher 2, we can say:
//   int lowTrack = _dgContextLowTrack[planeIndex] < -2 ? -2 :
//   _dgContextLowTrack[planeIndex]; int hiTrack = _dgContextHiTrack[planeIndex]
//   > 2 ? 2 : _dgContextHiTrack[planeIndex];
//
void extMeasure::seq_release(Array1D<SEQ*>* table)
{
  for (uint32_t jj = 0; jj < table->getCnt(); jj++) {
    SEQ* s = table->get(jj);
    _seqPool->free(s);
  }
  table->resetCnt();
}

uint32_t extMeasure::computeDiag(SEQ* s,
                                 uint32_t targetMet,
                                 uint32_t dir,
                                 uint32_t planeIndex,
                                 uint32_t trackn,
                                 Array1D<SEQ*>* residueSeq)
{
  Array1D<SEQ*>* dgContext = _dgContextArray[planeIndex][trackn];
  if (dgContext->getCnt() <= 1) {
    return 0;
  }

  Array1D<SEQ*> overlapSeq(16);
  getDgOverlap(s, _dir, dgContext, &overlapSeq, residueSeq);

  uint32_t len = 0;
  for (uint32_t jj = 0; jj < overlapSeq.getCnt(); jj++) {
    SEQ* tgt = overlapSeq.get(jj);
    uint32_t diagDist = calcDist(tgt->_ll, tgt->_ur);
    uint32_t tgWidth = tgt->_ur[_dir] - tgt->_ll[_dir];
    uint32_t len1 = getLength(tgt, !_dir);

    len += len1;
    bool skip_high_acc = true;
    bool verticalOverlap = false;
    if (_dist < 0 && !skip_high_acc) {
      if (diagDist <= _width && diagDist >= 0 && (int) _width < 10 * _minWidth
          && _verticalDiag) {
        verticalCap(_rsegSrcId, tgt->type, len1, tgWidth, diagDist, targetMet);
        verticalOverlap = true;
      } else if (((int) tgWidth > 10 * _minWidth)
                 && ((int) _width > 10 * _minWidth)
                 && (tgWidth >= 2 * diagDist)) {
        areaCap(_rsegSrcId, tgt->type, len1, targetMet);
        verticalOverlap = true;
      } else if ((int) tgWidth > 2 * _minWidth && tgWidth >= 2 * diagDist) {
        calcDiagRC(_rsegSrcId, tgt->type, len1, 1000000, targetMet);
      } else if (_diagModel == 2) {
        if (_verticalDiag) {
          verticalCap(
              _rsegSrcId, tgt->type, len1, tgWidth, diagDist, targetMet);
        } else {
          calcDiagRC(_rsegSrcId, tgt->type, len1, tgWidth, diagDist, targetMet);
        }
      }
      if (!verticalOverlap && _overMet > _met + 1) {
        addSeq(tgt->_ll, tgt->_ur, residueSeq);
      }

      continue;
    }
    if (_diagModel == 2) {
      calcDiagRC(_rsegSrcId, tgt->type, len1, tgWidth, diagDist, targetMet);
    }
    if (_diagModel == 1) {
      calcDiagRC(_rsegSrcId, tgt->type, len1, diagDist, targetMet);
    }
  }
  seq_release(&overlapSeq);
  return len;
}

int extMeasure::computeDiagOU(SEQ* s,
                              uint32_t trackMin,
                              uint32_t trackMax,
                              uint32_t targetMet,
                              Array1D<SEQ*>* diagTable)
{
  int trackDist = 2;
  int loTrack;
  int hiTrack;
  int planeIndex
      = getDgPlaneAndTrackIndex(targetMet, trackDist, loTrack, hiTrack);
  if (planeIndex < 0) {
    return 0;
  }

  uint32_t len = 0;

  Array1D<SEQ*> tmpTable(16);
  copySeqUsingPool(s, &tmpTable);

  Array1D<SEQ*> residueTable(16);

  int trackTable[200];
  uint32_t cnt = 0;
  for (int kk = (int) trackMin; kk <= (int) trackMax;
       kk++)  // skip overlapping track
  {
    if (kk <= _dgContextHiTrack[planeIndex]) {
      trackTable[cnt++] = *_dgContextTracks / 2 + kk;
    }

    if (!kk) {
      continue;
    }
    if (-kk >= _dgContextLowTrack[planeIndex]) {
      trackTable[cnt++] = *_dgContextTracks / 2 - kk;
    }
  }
  if (cnt == 0) {
    trackTable[cnt++] = *_dgContextTracks / 2;
  }

  for (uint32_t ii = 0; ii < cnt; ii++) {
    int trackn = trackTable[ii];

    if (_dgContextArray[planeIndex][trackn]->getCnt() <= 1) {
      continue;
    }
    bool add_all_diag = false;
    if (!add_all_diag) {
      for (uint32_t jj = 0; jj < tmpTable.getCnt(); jj++) {
        len += computeDiag(tmpTable.get(jj),
                           targetMet,
                           _dir,
                           planeIndex,
                           trackn,
                           &residueTable);
      }
    } else {
      len += computeDiag(s, targetMet, _dir, planeIndex, trackn, &residueTable);
    }

    seq_release(&tmpTable);
    tableCopyP(&residueTable, &tmpTable);
    residueTable.resetCnt();
  }
  if (diagTable != nullptr) {
    tableCopyP(&tmpTable, diagTable);
  } else {
    seq_release(&tmpTable);
  }
  return len;
}

int extMeasure::compute_Diag_Over_Under(Array1D<SEQ*>* seqTable,
                                        Array1D<SEQ*>* resTable)
{
  bool overUnder = true;
  int met1 = _underMet;
  int met2 = _overMet;

  if (_met - _underMet > _overMet - _met) {
    met2 = _underMet;
    met1 = _overMet;
    overUnder = false;
  }

  int totCovered = 0;
  for (uint32_t ii = 0; ii < seqTable->getCnt(); ii++) {
    SEQ* s = seqTable->get(ii);

    if (s->type > 0) {  // Black
      continue;
    }

    uint32_t len = s->_ur[!_dir] - s->_ll[!_dir];
    int remainder = len;

    if (_diagFlow) {
      if (_overMet < (int) _layerCnt) {
        computeDiagOU(s, 0, 3, _overMet, nullptr);
      }
    }

    addSeq(s->_ll, s->_ur, _diagTable);

    uint32_t len1
        = computeOverOrUnderSeq(_diagTable, met1, _underTable, overUnder);

    if (updateLengthAndExit(remainder, totCovered, len1)) {
      break;
    }

    uint32_t len2
        = computeOverOrUnderSeq(_underTable, met2, resTable, !overUnder);

    if (updateLengthAndExit(remainder, totCovered, len2)) {
      break;
    }

    release(_diagTable, _pixelTable);
    release(_underTable, _pixelTable);
  }
  release(_diagTable, _pixelTable);
  release(_underTable, _pixelTable);

  return totCovered;
}

int extMeasure::compute_Diag_OverOrUnder(Array1D<SEQ*>* seqTable,
                                         bool over,
                                         uint32_t met,
                                         Array1D<SEQ*>* resTable)
{
  int totCovered = 0;
  int diagTotLen = 0;
  for (uint32_t ii = 0; ii < seqTable->getCnt(); ii++) {
    SEQ* s = seqTable->get(ii);

    if (s->type > 0) {  // Black
      continue;
    }

    uint32_t len = s->_ur[!_dir] - s->_ll[!_dir];
    int remainder = len;

    if (_diagFlow) {
      if (!over) {
        computeDiagOU(s, 0, 3, met, nullptr);
      }
    }
    addSeq(s->_ll, s->_ur, _diagTable);

    uint32_t len1 = computeOverOrUnderSeq(_diagTable, met, resTable, over);

    if (updateLengthAndExit(remainder, totCovered, len1)) {
      break;
    }

    release(_diagTable);
  }
  release(_diagTable);
  release(_underTable);

  return totCovered - diagTotLen;
}

uint32_t extMeasure::measureUnderOnly(bool diagFlag)
{
  int totCovered = 0;
  int remainderLen = _len;

  uint32_t overLen = 0;
  _underMet = 0;
  for (_overMet = _met + 1; _overMet < (int) _layerCnt; _overMet++) {
    overLen
        = compute_Diag_OverOrUnder(_tmpSrcTable, false, _overMet, _tmpDstTable);

    if (updateLengthAndExit(remainderLen, totCovered, overLen)) {
      break;
    }

    release(_tmpSrcTable);
    tableCopyP(_tmpDstTable, _tmpSrcTable);
    _tmpDstTable->resetCnt();
  }
  release(_tmpSrcTable);
  release(_tmpDstTable);

  return totCovered;
}

uint32_t extMeasure::measureOverOnly(bool diagFlag)
{
  int totCovered = 0;
  int remainder = _len;

  _overMet = -1;
  for (_underMet = _met - 1; _underMet > 0; _underMet--) {
    uint32_t underLen
        = compute_Diag_OverOrUnder(_tmpSrcTable, true, _underMet, _tmpDstTable);

    if (updateLengthAndExit(remainder, totCovered, underLen)) {
      break;
    }

    release(_tmpSrcTable);
    tableCopyP(_tmpDstTable, _tmpSrcTable);
    _tmpDstTable->resetCnt();
  }
  release(_tmpSrcTable);
  release(_tmpDstTable);

  return totCovered;
}

uint32_t extMeasure::ouFlowStep(Array1D<SEQ*>* overTable)
{
  Array1D<SEQ*> tmpTable(32);
  uint32_t len = 0;
  for (uint32_t ii = 0; ii < overTable->getCnt(); ii++) {
    SEQ* s = overTable->get(ii);

    uint32_t ouLen = getOverlapSeq(_underMet, s, &tmpTable);

    if (ouLen > 0) {
      calcOU(ouLen);
      len += ouLen;
    }
  }
  release(overTable);

  for (uint32_t jj = 0; jj < tmpTable.getCnt(); jj++) {
    SEQ* s = tmpTable.get(jj);
    if (s->type == 0) {
      overTable->add(s);
      // s->type= 1;
      continue;
    }
    _pixelTable->release(s);
  }
  return len;
}

int extMeasure::underFlowStep(Array1D<SEQ*>* srcTable, Array1D<SEQ*>* overTable)
{
  int totLen = 0;

  Array1D<SEQ*> whiteTable(32);
  Array1D<SEQ*> table1(32);
  Array1D<SEQ*> diagTable(32);

  for (uint32_t ii = 0; ii < srcTable->getCnt(); ii++) {
    SEQ* s1 = srcTable->get(ii);
    int oLen = getOverlapSeq(_overMet, s1, &table1);
    if (oLen > 0) {
      totLen += oLen;
    }
  }
  for (uint32_t jj = 0; jj < table1.getCnt(); jj++) {
    SEQ* s2 = table1.get(jj);

    if (s2->type == 0) {
      if (_diagFlow) {
        _diagLen += computeDiagOU(s2, 0, 3, _overMet, nullptr);
      }
      whiteTable.add(s2);
      continue;
    }
    overTable->add(s2);
    s2->type = 0;
  }
  release(srcTable);
  tableCopyP(&whiteTable, srcTable);

  return totLen;
}

uint32_t extMeasure::measureDiagFullOU()
{
  _tmpSrcTable->resetCnt();
  _tmpDstTable->resetCnt();

  int ll[2] = {_ll[0], _ll[1]};
  int ur[2] = {_ur[0], _ur[1]};
  ur[_dir] = ll[_dir];

  addSeq(ll, ur, _tmpSrcTable);

  if (_met == (int) _layerCnt - 1) {
    return measureOverOnly(false);
  }

  _tmpTable->resetCnt();
  _ouTable->resetCnt();
  _overTable->resetCnt();
  _underTable->resetCnt();
  _diagTable->resetCnt();

  int totCovered = 0;
  int remainder = _len;

  uint32_t maxDist = _extMain->_ccContextDepth;
  int upperLimit = _met + maxDist >= _layerCnt ? _layerCnt : _met + maxDist;
  int lowerLimit = _met - maxDist;
  lowerLimit = std::max(lowerLimit, 0);

  for (_overMet = _met + 1; _overMet < upperLimit; _overMet++) {
    int totUnderLen = underFlowStep(_tmpSrcTable, _overTable);
    if (totUnderLen <= 0) {
      release(_overTable);
      continue;
    }

    int underLen = totUnderLen;
    int totOUCovered = 0;
    bool skipUnderMetOverSub = false;
    for (_underMet = _met - 1; _underMet > lowerLimit; _underMet--) {
      uint32_t overLen = ouFlowStep(_overTable);

      if (updateLengthAndExit(underLen, totOUCovered, overLen)) {
        skipUnderMetOverSub = true;
        release(_overTable);
        break;
      }
    }
    if (!skipUnderMetOverSub) {
      computeOverOrUnderSeq(_overTable, _overMet, _underTable, false);
      release(_underTable);
    }
    release(_overTable);
    if (updateLengthAndExit(remainder, totCovered, totUnderLen)) {
      release(_overTable);
      break;
    }
  }

  _overMet = -1;
  for (_underMet = _met - 1; _underMet > 0; _underMet--) {
    uint32_t overLen
        = computeOverOrUnderSeq(_tmpSrcTable, _underMet, _tmpDstTable, true);

    if (updateLengthAndExit(remainder, totCovered, overLen)) {
      break;
    }

    release(_tmpSrcTable);
    tableCopyP(_tmpDstTable, _tmpSrcTable);
    _tmpDstTable->resetCnt();
  }
  release(_tmpSrcTable);
  release(_tmpDstTable);

  return totCovered;
}

uint32_t extMeasure::measureDiagOU(uint32_t ouLevelLimit,
                                   uint32_t diagLevelLimit)
{
  return measureDiagFullOU();

  _tmpSrcTable->resetCnt();
  _tmpDstTable->resetCnt();

  int ll[2] = {_ll[0], _ll[1]};
  int ur[2] = {_ur[0], _ur[1]};
  ur[_dir] = ll[_dir];

  addSeq(ll, ur, _tmpSrcTable);

  if (_met == 1) {
    return measureUnderOnly(true);
  }

  if (_met == (int) _layerCnt - 1) {
    return measureOverOnly(false);
  }

  _tmpTable->resetCnt();
  _ouTable->resetCnt();
  _overTable->resetCnt();
  _underTable->resetCnt();
  _diagTable->resetCnt();

  int totCovered = 0;
  int remainder = _len;

  uint32_t downDist = 1;
  uint32_t upDist = 1;

  _underMet = _met;
  downDist = 0;
  _overMet = _met;
  upDist = 0;
  while (true) {
    if (_underMet > 0) {
      _underMet--;
      downDist++;
    }

    if (_overMet < (int) _layerCnt) {
      _overMet++;
      upDist++;
    }
    if ((_underMet == 0) && (_overMet == (int) _layerCnt)) {
      break;
    }

    if ((_underMet > 0) && (downDist <= ouLevelLimit)
        && (upDist <= ouLevelLimit)) {
      uint32_t ouLen = 0;

      for (uint32_t ii = 0; ii < _tmpSrcTable->getCnt();
           ii++) {  // keep on adding inside loop
        SEQ* s = _tmpSrcTable->get(ii);

        if (s->type > 0) {  // Black
          continue;
        }

        ouLen += computeOverUnder(s->_ll, s->_ur, _ouTable);
      }
      release(_tmpSrcTable);
      if (updateLengthAndExit(remainder, totCovered, ouLen)) {
        break;
      }
    } else {
      tableCopyP(_tmpSrcTable, _ouTable);
      _tmpSrcTable->resetCnt();
    }

    uint32_t underLen = 0;
    uint32_t overUnderLen = 0;

    if (_underMet > 0) {
      overUnderLen = compute_Diag_Over_Under(_ouTable, _tmpDstTable);
    } else {
      underLen
          = compute_Diag_OverOrUnder(_ouTable, false, _overMet, _tmpDstTable);
    }

    release(_ouTable);
    release(_tmpSrcTable);
    tableCopyP(_tmpDstTable, _tmpSrcTable);
    _tmpDstTable->resetCnt();

    if (updateLengthAndExit(remainder, totCovered, underLen + overUnderLen)) {
      break;
    }
  }
  release(_tmpSrcTable);
  return totCovered;
}

void extMeasure::ccReportProgress()
{
  uint32_t repChunk = 1000000;
  if ((_totCCcnt > 0) && (_totCCcnt % repChunk == 0)) {
    logger_->info(RCX,
                  79,
                  "Have processed {} CC caps, and stored {} CC caps",
                  _totCCcnt,
                  _totBigCCcnt);
  }
}

void extMeasure::printNet(dbRSeg* rseg, uint32_t netId)
{
  if (rseg == nullptr) {
    return;
  }

  dbNet* net = rseg->getNet();

  if (netId == net->getId()) {
    _netId = netId;
    dbCapNode::getCapNode(_block, rseg->getTargetNode());
  }
}

bool extMain::updateCoupCap(dbRSeg* rseg1, dbRSeg* rseg2, int jj, double v)
{
  if (rseg1 != nullptr && rseg2 != nullptr) {
    dbCCSeg* ccap
        = dbCCSeg::create(dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
                          dbCapNode::getCapNode(_block, rseg2->getTargetNode()),
                          true);
    ccap->addCapacitance(v, jj);
    return true;
  }
  if (rseg1 != nullptr) {
    updateTotalCap(rseg1, v, jj);
  }
  if (rseg2 != nullptr) {
    updateTotalCap(rseg2, v, jj);
  }

  return false;
}

double extMain::calcFringe(extDistRC* rc, double deltaFr, bool includeCoupling)
{
  double ccCap = 0.0;
  if (includeCoupling) {
    ccCap = rc->coupling_;
  }

  double cap = rc->fringe_ + ccCap - deltaFr;

  if (_gndcModify) {
    cap *= _gndcFactor;
  }

  return cap;
}

double extMain::updateTotalCap(dbRSeg* rseg, double cap, uint32_t modelIndex)
{
  if (rseg == nullptr) {
    return 0;
  }

  int extDbIndex, sci, scDbIndex;
  extDbIndex = getProcessCornerDbIndex(modelIndex);
  double tot = rseg->getCapacitance(extDbIndex);
  tot += cap;

  rseg->setCapacitance(tot, extDbIndex);
  // return rseg->getCapacitance(extDbIndex);
  getScaledCornerDbIndex(modelIndex, sci, scDbIndex);
  if (sci == -1) {
    return tot;
  }
  getScaledGndC(sci, cap);
  double tots = rseg->getCapacitance(scDbIndex);
  tots += cap;
  rseg->setCapacitance(tots, scDbIndex);
  return tot;
}

void extDistRC::addRC(extDistRC* rcUnit, uint32_t len, bool addCC)
{
  if (rcUnit == nullptr) {
    return;
  }

  fringe_ += rcUnit->fringe_ * len;

  if (addCC) {  // dist based
    coupling_ += rcUnit->coupling_ * len;
  }
}

double extMain::updateRes(dbRSeg* rseg, double res, uint32_t model)
{
  if (rseg == nullptr) {
    return 0;
  }

  if (_resModify) {
    res *= _resFactor;
  }

  double tot = rseg->getResistance(model);
  tot += res;

  rseg->setResistance(tot, model);
  return rseg->getResistance(model);
}

bool extMeasure::isConnectedToBterm(dbRSeg* rseg1)
{
  if (rseg1 == nullptr) {
    return false;
  }

  dbCapNode* node1 = rseg1->getTargetCapNode();
  if (node1->isBTerm()) {
    return true;
  }
  dbCapNode* node2 = rseg1->getSourceCapNode();
  if (node2->isBTerm()) {
    return true;
  }

  return false;
}

dbCCSeg* extMeasure::makeCcap(dbRSeg* rseg1, dbRSeg* rseg2, double ccCap)
{
  if ((rseg1 != nullptr) && (rseg2 != nullptr)
      && rseg1->getNet() != rseg2->getNet()) {  // signal nets

    _totCCcnt++;  // TO_TEST

    if (ccCap >= _extMain->_coupleThreshold) {
      _totBigCCcnt++;

      dbCapNode* node1 = rseg1->getTargetCapNode();
      dbCapNode* node2 = rseg2->getTargetCapNode();

      return dbCCSeg::create(node1, node2, true);
    }
    _totSmallCCcnt++;
    return nullptr;
  }
  return nullptr;
}

void extMeasure::addCCcap(dbCCSeg* ccap, double v, uint32_t model)
{
  double coupling = _ccModify ? v * _ccFactor : v;
  ccap->addCapacitance(coupling, model);
}

void extMeasure::addFringe(dbRSeg* rseg1,
                           dbRSeg* rseg2,
                           double frCap,
                           uint32_t model)
{
  if (_gndcModify) {
    frCap *= _gndcFactor;
  }

  if (rseg1 != nullptr) {
    _extMain->updateTotalCap(rseg1, frCap, model);
  }

  if (rseg2 != nullptr) {
    _extMain->updateTotalCap(rseg2, frCap, model);
  }
}

void extMeasure::calcDiagRC(int rsegId1,
                            uint32_t rsegId2,
                            uint32_t len,
                            uint32_t diagWidth,
                            uint32_t diagDist,
                            uint32_t tgtMet)
{
  double capTable[10];
  uint32_t modelCnt = _metRCTable.getCnt();
  for (uint32_t ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    capTable[ii] = len * getDiagUnderCC(rcModel, diagWidth, diagDist, tgtMet);
    _rc[ii]->diag_ += capTable[ii];
    double ccTable[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (_dist > 0) {
      extDistRC* rc = getDiagUnderCC2(rcModel, diagWidth, diagDist, tgtMet);
      if (rc) {
        ccTable[ii] = len * rc->coupling_;
      }

      rc = rcModel->_capOver[_met]->getRC(0, _width, _dist);
      if (rc) {
        ccTable[ii] -= len * rc->coupling_;
        _rc[ii]->coupling_ += ccTable[ii];
      }
    }
  }
  dbRSeg* rseg1 = nullptr;
  dbRSeg* rseg2 = nullptr;
  if (rsegId1 > 0) {
    rseg1 = dbRSeg::getRSeg(_block, rsegId1);
  }
  if (rsegId2 > 0) {
    rseg2 = dbRSeg::getRSeg(_block, rsegId2);
  }

  dbCCSeg* ccCap = makeCcap(rseg1, rseg2, capTable[_minModelIndex]);

  for (uint32_t model = 0; model < modelCnt; model++) {
    if (ccCap != nullptr) {
      addCCcap(ccCap, capTable[model], model);
    } else {
      addFringe(nullptr, rseg2, capTable[model], model);
    }
  }
}

void extMeasure::createCap(int rsegId1, uint32_t rsegId2, double* capTable)
{
  dbRSeg* rseg1 = rsegId1 > 0 ? dbRSeg::getRSeg(_block, rsegId1) : nullptr;
  dbRSeg* rseg2 = rsegId2 > 0 ? dbRSeg::getRSeg(_block, rsegId2) : nullptr;

  dbCCSeg* ccCap = makeCcap(rseg1, rseg2, capTable[_minModelIndex]);

  uint32_t modelCnt = _metRCTable.getCnt();
  for (uint32_t model = 0; model < modelCnt; model++) {
    if (ccCap != nullptr) {
      addCCcap(ccCap, capTable[model], model);
    } else {
      _rc[model]->diag_ += capTable[model];
      addFringe(nullptr, rseg2, capTable[model], model);
      // FIXME IMPORTANT-TEST-FIRST addFringe(rseg1, rseg2, capTable[model],
      // model);
    }
  }
}

void extMeasure::areaCap(int rsegId1,
                         uint32_t rsegId2,
                         uint32_t len,
                         uint32_t tgtMet)
{
  double capTable[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t modelCnt = _metRCTable.getCnt();
  for (uint32_t ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    double area = 1.0 * _width;
    area *= len;
    extDistRC* rc = getUnderLastWidthDistRC(rcModel, tgtMet);
    capTable[ii] = 2 * area * rc->fringe_;  //_fringe always 1/2 of rulesGen

    dbRSeg* rseg2 = nullptr;
    if (rsegId2 > 0) {
      rseg2 = dbRSeg::getRSeg(_block, rsegId2);
    }

    if (rseg2 != nullptr) {
      uint32_t met = _met;
      _met = tgtMet;
      extDistRC* area_rc = areaCapOverSub(ii, rcModel);
      _met = met;
      double areaCapOverSub = 2 * area * area_rc->fringe_;
      _extMain->updateTotalCap(rseg2, 0.0, 0.0, areaCapOverSub, ii);
    }
  }
  createCap(rsegId1, rsegId2, capTable);
}

extDistRC* extMeasure::areaCapOverSub(uint32_t modelNum, extMetRCTable* rcModel)
{
  if (rcModel == nullptr) {
    rcModel = _metRCTable.get(modelNum);
  }

  extDistRC* rc = rcModel->getOverFringeRC(this);

  return rc;
}

bool extMeasure::verticalCap(int rsegId1,
                             uint32_t rsegId2,
                             uint32_t len,
                             uint32_t tgtWidth,
                             uint32_t diagDist,
                             uint32_t tgtMet)
{
  dbRSeg* rseg2 = nullptr;
  if (rsegId2 > 0) {
    rseg2 = dbRSeg::getRSeg(_block, rsegId2);
  }
  dbRSeg* rseg1 = nullptr;
  if (rsegId1 > 0) {
    rseg1 = dbRSeg::getRSeg(_block, rsegId1);
  }

  double capTable[10];
  uint32_t modelCnt = _metRCTable.getCnt();
  for (uint32_t ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    extDistRC* rc = getVerticalUnderRC(rcModel, diagDist, tgtWidth, tgtMet);
    if (rc == nullptr) {
      return false;
    }

    capTable[ii] = len * rc->fringe_;

    if ((rseg2 == nullptr) && (rseg1 == nullptr)) {
      continue;
    }

    extDistRC* overSubFringe
        = _metRCTable.get(ii)->_capOver[tgtMet]->getFringeRC(0, tgtWidth);
    if (overSubFringe == nullptr) {
      continue;
    }
    double frCap = len * overSubFringe->fringe_;

    if (diagDist > tgtWidth) {
      double scale = 0.25 * diagDist / tgtWidth;
      scale = 1.0 / scale;
      scale = std::min(scale, 0.5);
      frCap *= scale;
    }
    if (rseg2 != nullptr) {
      _extMain->updateTotalCap(rseg2, 0.0, 0.0, frCap, ii);
    }
    if (rseg1 != nullptr) {
      _extMain->updateTotalCap(rseg1, 0.0, 0.0, 0.5 * frCap, ii);
    }
  }
  createCap(rsegId1, rsegId2, capTable);
  return true;
}

void extMeasure::calcDiagRC(int rsegId1,
                            uint32_t rsegId2,
                            uint32_t len,
                            uint32_t dist,
                            uint32_t tgtMet)
{
  int DOUBLE_DIAG = 1;
  double capTable[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t modelCnt = _metRCTable.getCnt();
  for (uint32_t ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    if (dist != 1000000) {
      double cap = getDiagUnderCC(rcModel, dist, tgtMet);
      double diagCap = DOUBLE_DIAG * len * cap;
      capTable[ii] = diagCap;
      _rc[ii]->diag_ += diagCap;

      const char* msg = "calcDiagRC";
      Debug_DiagValues(0.0, diagCap, msg);
    } else {
      capTable[ii] = 2 * len * getUnderRC(rcModel)->fringe_;
    }
  }
  createCap(rsegId1, rsegId2, capTable);
}

void extMeasure::calcRC(dbRSeg* rseg1, dbRSeg* rseg2, uint32_t totLenCovered)
{
  int lenOverSub = _len - totLenCovered;

  uint32_t modelCnt = _metRCTable.getCnt();

  for (uint32_t model = 0; model < modelCnt; model++) {
    extMetRCTable* rcModel = _metRCTable.get(model);

    if (totLenCovered > 0) {
      for (uint32_t ii = 0; ii < _lenOUtable->getCnt(); ii++) {
        extLenOU* ou = _lenOUtable->get(ii);

        _overMet = ou->_overMet;
        _underMet = ou->_underMet;

        extDistRC* ouRC = nullptr;

        if (ou->_over) {
          ouRC = getOverRC(rcModel);
        } else if (ou->_under) {
          ouRC = getUnderRC(rcModel);
        } else if (ou->_overUnder) {
          ouRC = getOverUnderRC(rcModel);
        }

        _rc[model]->addRC(ouRC, ou->_len, _dist > 0);
      }
    }

    if (_dist >= 0) {  // dist based
      _underMet = 0;

      extDistRC* rcOverSub = getOverRC(rcModel);
      if (lenOverSub > 0) {
        _rc[model]->addRC(rcOverSub, lenOverSub, _dist > 0);
      }
      double res = 0.0;
      if (rcOverSub != nullptr) {
        res = rcOverSub->res_ * _len;
      }

      double deltaFr = 0.0;
      double deltaRes = 0.0;
      extDistRC* rcMaxDist = rcModel->getOverFringeRC(this);

      if (rcMaxDist != nullptr) {
        deltaFr = rcMaxDist->getFringe() * _len;
        deltaRes = rcMaxDist->res_ * _len;
        res -= deltaRes;
      }

      if (rseg1 != nullptr) {
        _extMain->updateRes(rseg1, res, model);
      }

      if (rseg2 != nullptr) {
        _extMain->updateRes(rseg2, res, model);
      }

      dbCCSeg* ccap = nullptr;
      bool includeCoupling = true;
      if ((rseg1 != nullptr) && (rseg2 != nullptr)) {  // signal nets

        _totCCcnt++;

        if (_rc[_minModelIndex]->coupling_ >= _extMain->_coupleThreshold) {
          ccap = dbCCSeg::create(
              dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
              dbCapNode::getCapNode(_block, rseg2->getTargetNode()),
              true);

          includeCoupling = false;
          _totBigCCcnt++;
        } else {
          _totSmallCCcnt++;
        }
      }
      extDistRC* finalRC = _rc[model];
      if (ccap != nullptr) {
        double coupling
            = _ccModify ? finalRC->coupling_ * _ccFactor : finalRC->coupling_;
        ccap->addCapacitance(coupling, model);
      }

      double frCap = _extMain->calcFringe(finalRC, deltaFr, includeCoupling);

      if (rseg1 != nullptr) {
        _extMain->updateTotalCap(rseg1, frCap, model);
      }

      if (rseg2 != nullptr) {
        _extMain->updateTotalCap(rseg2, frCap, model);
      }
    }
  }
  for (uint32_t ii = 0; ii < _lenOUtable->getCnt(); ii++) {
    _lenOUPool->free(_lenOUtable->get(ii));
  }

  _lenOUtable->resetCnt();
}

void extMeasure::OverSubRC(dbRSeg* rseg1,
                           dbRSeg* rseg2,
                           int ouCovered,
                           int diagCovered,
                           int srcCovered)
{
  int res_lenOverSub = 0;

  int lenOverSub = _len - ouCovered;
  lenOverSub = std::max(lenOverSub, 0);

  bool rvia1 = rseg1 != nullptr && isVia(rseg1->getId());

  if (!((lenOverSub > 0) || (res_lenOverSub > 0))) {
    return;
  }

  _underMet = 0;
  for (uint32_t jj = 0; jj < _metRCTable.getCnt(); jj++) {
    extDistRC* rc = _metRCTable.get(jj)->getOverFringeRC(this);
    if (rc == nullptr) {
      continue;
    }
    double cap = 0;
    if (lenOverSub > 0) {
      cap = rc->getFringe() * lenOverSub;
      _extMain->updateTotalCap(rseg1, cap, jj);
    }
    double res = 0;
    if (!_extMain->_lef_res && !rvia1) {
      if (res_lenOverSub > 0) {
        res = rc->getRes() * res_lenOverSub;
        _extMain->updateRes(rseg1, res, jj);
      }
    }
    const char* msg = "OverSubRC (No Neighbor)";
    OverSubDebug(rc, lenOverSub, res_lenOverSub, res, cap, msg);
  }
}

/**
 * ScaleResbyTrack scales the original calculation by
 * a coefficient depending on how many track(s) away the
 * nearest neighbors to the wire of interest.
 *
 * To scale the calculated resistance to be closer to
 * golden resistance from commercial tool.
 *
 * @return the scaling coefficient for the resistance.
 * @currently not used
 */
double extMeasure::ScaleResbyTrack(bool openEnded, double& dist_track)
{
  dist_track = 0.0;

  // Dividers: 1, 1.2, 2, 3 respectively for tracks: 1, 2, >=3  --- assumption:
  // extRules is 1/2 of total res
  double sub_mult_res = 1;
  if (openEnded) {
    sub_mult_res = 0.4;
    return sub_mult_res;
  }
  if (_extMain->_minDistTable[_met] > 0) {
    dist_track = _dist / _extMain->_minDistTable[_met];
    if (dist_track >= 3) {
      sub_mult_res = 2.0 / (1 + 4);
    } else if (dist_track > 1 && dist_track <= 2) {
      sub_mult_res = 1;
    } else if (dist_track > 2 && dist_track <= 4) {
      sub_mult_res = 2.0 / (1 + 3);
    }
  }
  return sub_mult_res;
}

void extMeasure::OverSubRC_dist(dbRSeg* rseg1,
                                dbRSeg* rseg2,
                                int ouCovered,
                                int diagCovered,
                                int srcCovered)
{
  double SUB_MULT = 1.0;
  double res_lenOverSub = 0;
  // -----------------------------------------
  int lenOverSub = _len - ouCovered;

  int lenOverSub_bot = _len - srcCovered;
  if (lenOverSub_bot > 0) {
    lenOverSub += lenOverSub_bot;
  }

  bool rvia1 = rseg1 != nullptr && isVia(rseg1->getId());
  bool rvia2 = rseg2 != nullptr && isVia(rseg2->getId());

  if (!((lenOverSub > 0) || (res_lenOverSub > 0))) {
    return;
  }
  _underMet = 0;
  for (uint32_t jj = 0; jj < _metRCTable.getCnt(); jj++) {
    extMetRCTable* rcModel = _metRCTable.get(jj);
    extDistRC* rc = getOverRC(rcModel);

    if (rc == nullptr) {
      continue;
    }

    double res = rc->getRes() * res_lenOverSub;

    if (!_extMain->_lef_res) {
      if (!rvia1) {
        _extMain->updateRes(rseg1, res, jj);
      }
      if (!rvia2) {
        _extMain->updateRes(rseg2, res, jj);
      }
    }

    double fr = 0;
    double cc = 0;
    if (lenOverSub > 0) {
      if (_sameNetFlag) {
        fr = SUB_MULT * rc->getFringe() * lenOverSub;
        _extMain->updateTotalCap(rseg1, fr, jj);
      } else {
        double fr = SUB_MULT * rc->getFringe() * lenOverSub;
        _extMain->updateTotalCap(rseg1, fr, jj);
        _extMain->updateTotalCap(rseg2, fr, jj);

        if (_dist > 0) {  // dist based
          cc = SUB_MULT * rc->getCoupling() * lenOverSub;
          _extMain->updateCoupCap(rseg1, rseg2, jj, cc);
        }
      }
    }
    const char* msg = "OverSubRC_dist (With Neighbor)";
    OverSubDebug(rc, lenOverSub, lenOverSub, res, fr + cc, msg);
  }
}

int extMeasure::computeAndStoreRC(dbRSeg* rseg1, dbRSeg* rseg2, int srcCovered)
{
  if (rseg1 == nullptr && rseg2 == nullptr) {
    return 0;
  }

  rcSegInfo();
  if (IsDebugNet()) {
    debugPrint(logger_,
               RCX,
               "debug_net",
               1,
               "measureRC:"
               "C"
               "\t[BEGIN-OUD] ----- OverUnder/Diagonal RC ----- BEGIN");
  }

  int totLenCovered = 0;
  _lenOUtable->resetCnt();
  if (_extMain->_usingMetalPlanes) {
    _diagLen = 0;
    if (_extMain->_ccContextDepth > 0) {
      if (!_diagFlow) {
        totLenCovered = measureOverUnderCap();
      } else {
        totLenCovered = measureDiagOU(1, 2);
      }
    }
  }
  ouCovered_debug(totLenCovered);

  // Case where the geometric search returns no neighbor found
  // _dist is infinit
  if (_dist < 0) {
    totLenCovered = std::max(totLenCovered, 0);

    _underMet = 0;

    _no_debug = true;
    _no_debug = false;

    for (uint32_t jj = 0; jj < _metRCTable.getCnt(); jj++) {
      bool ou = false;
      _rc[jj]->res_ = 0;  // Res non context based

      if (_rc[jj]->fringe_ > 0) {
        ou = true;
        _extMain->updateTotalCap(rseg1, _rc[jj]->fringe_, jj);
      }
      if (ou && IsDebugNet()) {
        _rc[jj]->printDebugRC_values("OverUnder Total Open");
      }
    }
    if (IsDebugNet()) {
      debugPrint(logger_,
                 RCX,
                 "debug_net",
                 1,
                 "measureRC:"
                 "C",
                 "\t[END-OUD] ----- OverUnder/Diagonal ----- END");
    }
    rcSegInfo();

    OverSubRC(rseg1, nullptr, totLenCovered, _diagLen, _len);
    return totLenCovered;
  }

  _underMet = 0;

  for (uint32_t jj = 0; jj < _metRCTable.getCnt(); jj++) {
    bool ou = false;
    if (_rc[jj]->fringe_ > 0) {
      ou = true;
      _extMain->updateTotalCap(rseg1, _rc[jj]->fringe_, jj);
      _extMain->updateTotalCap(rseg2, _rc[jj]->fringe_, jj);
    }
    if (_rc[jj]->coupling_ > 0) {
      ou = true;
      _extMain->updateCoupCap(rseg1, rseg2, jj, _rc[jj]->coupling_);
    }
    if (ou && IsDebugNet()) {
      _rc[jj]->printDebugRC_values("OverUnder Total Dist");
    }
  }
  rcSegInfo();
  if (IsDebugNet()) {
    debugPrint(logger_,
               RCX,
               "debug_net",
               1,
               "measureRC:"
               "C"
               "\t[END-OUD] ------ OverUnder/Diagonal RC ------ END");
  }

  OverSubRC_dist(rseg1, rseg2, totLenCovered, _diagLen, _len);
  return totLenCovered;
}

void extMeasure::measureRC(CoupleOptions& options)
{
  _totSegCnt++;

  int rsegId1 = options[1];  // dbRSeg id for SRC segment
  int rsegId2 = options[2];  // dbRSeg id for Target segment

  _rsegSrcId = rsegId1;
  _rsegTgtId = rsegId2;

  defineBox(options);

  dbRSeg* rseg1 = nullptr;
  dbNet* srcNet = nullptr;
  uint32_t netId1 = 0;
  if (rsegId1 > 0) {
    rseg1 = dbRSeg::getRSeg(_block, rsegId1);
    srcNet = rseg1->getNet();
    netId1 = srcNet->getId();
  }
  _netSrcId = netId1;

  dbRSeg* rseg2 = nullptr;
  dbNet* tgtNet = nullptr;
  uint32_t netId2 = 0;
  if (rsegId2 > 0) {
    rseg2 = dbRSeg::getRSeg(_block, rsegId2);
    tgtNet = rseg2->getNet();
    netId2 = tgtNet->getId();
  }
  _sameNetFlag = (srcNet == tgtNet);

  _netTgtId = netId2;

  _totSignalSegCnt++;

  if (_met >= (int) _layerCnt) {
    return;
  }

  _verticalDiag = _currentModel->getVerticalDiagFlag();
  int prevCovered = 0;

  _netId = _extMain->_debug_net_id;

  DebugStart();

  int totCovered = computeAndStoreRC(rseg1, rseg2, prevCovered);

  bool rvia1 = rseg1 != nullptr && isVia(rseg1->getId());
  if (!rvia1 && rseg1 != nullptr) {
    for (uint32_t jj = 0; jj < _metRCTable.getCnt(); jj++) {
      _rc[jj]->res_ = 0;
    }
    if (IsDebugNet()) {
      const char* netName = "";
      if (_netId > 0) {
        dbNet* net = dbNet::getNet(_block, _netId);
        netName = net->getConstName();
      }
      fprintf(stdout,
              " ---------------------------------------------------------------"
              "------------------\n");
      fprintf(stdout,
              "     %7d %7d %7d %7d    M%d  D%d  L%d   N%d N%d %s\n",
              _ll[0],
              _ur[0],
              _ll[1],
              _ur[1],
              _met,
              _dist,
              _len,
              _netSrcId,
              _netTgtId,
              netName);
      fprintf(stdout,
              " ---------------------------------------------------------------"
              "------------------\n");
    }
    double deltaRes[10];
    for (uint32_t jj = 0; jj < _metRCTable.getCnt(); jj++) {
      deltaRes[jj] = 0.0;
    }

    SEQ* s = addSeq(_ll, _ur);
    int len_covered = computeResDist(s, 1, 4, _met, nullptr);
    int len_down_not_coupled = _len - len_covered;

    if (_dist > 0 && len_down_not_coupled > 0) {
      calcRes(rseg1->getId(), len_down_not_coupled, 0, _dist, _met);
      len_covered += len_down_not_coupled;
    }
    if (len_covered > 0) {
      calcRes0(deltaRes, _met, len_covered);
    }

    for (uint32_t jj = 0; jj < _metRCTable.getCnt(); jj++) {
      double totR1 = _rc[jj]->res_;
      if (totR1 > 0) {
        totR1 -= deltaRes[jj];
        if (totR1 != 0.0) {
          _extMain->updateRes(rseg1, totR1, jj);
        }
      }
    }
  }
  if (IsDebugNet()) {
    debugPrint(logger_,
               RCX,
               "debug_net",
               1,
               "[END-DistRC:C]"
               "\tDistRC:C"
               " ----- measureRC: ----- END\n");
  }
  options[20] = totCovered;
}

void extMeasure::getDgOverlap(SEQ* sseq,
                              uint32_t dir,
                              Array1D<SEQ*>* dgContext,
                              Array1D<SEQ*>* overlapSeq,
                              Array1D<SEQ*>* residueSeq)
{
  int idx = dgContext->get(0)->_ll[0];
  uint32_t lp = dir ? 0 : 1;
  uint32_t wp = dir ? 1 : 0;
  SEQ* rseq;
  SEQ* tseq;
  SEQ* wseq;
  int covered = sseq->_ll[lp];

  if (idx == dgContext->getCnt()) {
    rseq = _seqPool->alloc();
    rseq->_ll[wp] = sseq->_ll[wp];
    rseq->_ur[wp] = sseq->_ur[wp];
    rseq->_ll[lp] = sseq->_ll[lp];
    rseq->_ur[lp] = sseq->_ur[lp];
    residueSeq->add(rseq);
    return;
  }

  for (; idx < dgContext->getCnt(); idx++) {
    tseq = dgContext->get(idx);
    if (tseq->_ur[lp] <= covered) {
      continue;
    }

    if (tseq->_ll[lp] >= sseq->_ur[lp]) {
      rseq = _seqPool->alloc();
      rseq->_ll[wp] = sseq->_ll[wp];
      rseq->_ur[wp] = sseq->_ur[wp];
      rseq->_ll[lp] = covered;
      rseq->_ur[lp] = sseq->_ur[lp];
      assert(rseq->_ur[lp] >= rseq->_ll[lp]);
      residueSeq->add(rseq);
      break;
    }
    wseq = _seqPool->alloc();
    wseq->type = tseq->type;
    wseq->_ll[wp] = tseq->_ll[wp];
    wseq->_ur[wp] = tseq->_ur[wp];
    if (tseq->_ur[lp] <= sseq->_ur[lp]) {
      wseq->_ur[lp] = tseq->_ur[lp];
    } else {
      wseq->_ur[lp] = sseq->_ur[lp];
    }
    if (tseq->_ll[lp] <= covered) {
      wseq->_ll[lp] = covered;
    } else {
      wseq->_ll[lp] = tseq->_ll[lp];
      rseq = _seqPool->alloc();
      rseq->_ll[wp] = sseq->_ll[wp];
      rseq->_ur[wp] = sseq->_ur[wp];
      rseq->_ll[lp] = covered;
      rseq->_ur[lp] = tseq->_ll[lp];
      assert(rseq->_ur[lp] >= rseq->_ll[lp]);
      residueSeq->add(rseq);
    }
    assert(wseq->_ur[lp] >= wseq->_ll[lp]);
    overlapSeq->add(wseq);
    covered = wseq->_ur[lp];
    if (tseq->_ur[lp] >= sseq->_ur[lp]) {
      break;
    }
    if (idx == dgContext->getCnt() - 1 && covered < sseq->_ur[lp]) {
      rseq = _seqPool->alloc();
      rseq->_ll[wp] = sseq->_ll[wp];
      rseq->_ur[wp] = sseq->_ur[wp];
      rseq->_ll[lp] = covered;
      rseq->_ur[lp] = sseq->_ur[lp];
      assert(rseq->_ur[lp] >= rseq->_ll[lp]);
      residueSeq->add(rseq);
    }
  }
  dgContext->get(0)->_ll[0] = idx;
}

void extMeasure::getDgOverlap(CoupleOptions& options)
{
  int ttttprintOverlap = 1;
  int srcseqcnt = 0;
  if (ttttprintOverlap && !_dgContextFile) {
    _dgContextFile = fopen("overlapSeq.1", "w");
    fprintf(_dgContextFile, "wire overlapping context:\n");
    srcseqcnt = 0;
  }
  uint32_t met = -options[0];
  srcseqcnt++;
  SEQ* seq = _seqPool->alloc();
  SEQ* pseq;
  int dir = options[6];
  uint32_t xidx = 0;
  uint32_t yidx = 1;
  uint32_t lidx, bidx;
  lidx = dir == 1 ? xidx : yidx;
  bidx = dir == 1 ? yidx : xidx;
  seq->_ll[lidx] = options[1];
  seq->_ll[bidx] = options[3];
  seq->_ur[lidx] = options[2];
  seq->_ur[bidx] = options[4];
  if (ttttprintOverlap) {
    fprintf(_dgContextFile,
            "\nSource Seq %d:ll_0=%d ll_1=%d ur_0=%d ur_1=%d met=%d dir=%d\n",
            srcseqcnt,
            seq->_ll[0],
            seq->_ll[1],
            seq->_ur[0],
            seq->_ur[1],
            met,
            dir);
  }
  Array1D<SEQ*> overlapSeq(16);
  Array1D<SEQ*> residueSeq(16);
  Array1D<SEQ*>* dgContext = nullptr;
  for (int jj = 1; jj <= *_dgContextHiLvl; jj++) {
    int gridn = *_dgContextDepth + jj;
    for (int kk = 0; kk <= _dgContextHiTrack[gridn]; kk++) {
      int trackn = *_dgContextTracks / 2 + kk;
      dgContext = _dgContextArray[gridn][trackn];
      overlapSeq.resetCnt();
      residueSeq.resetCnt();
      getDgOverlap(seq, dir, dgContext, &overlapSeq, &residueSeq);
      if (!ttttprintOverlap) {
        continue;
      }

      for (uint32_t ss = 0; ss < overlapSeq.getCnt(); ss++) {
        pseq = overlapSeq.get(ss);
        fprintf(
            _dgContextFile,
            "\n    overlap %d:ll_0=%d ll_1=%d ur_0=%d ur_1=%d met=%d trk=%d\n",
            ss,
            pseq->_ll[0],
            pseq->_ll[1],
            pseq->_ur[0],
            pseq->_ur[1],
            jj + met,
            kk + _dgContextBaseTrack[gridn]);
      }
      for (uint32_t ss1 = 0; ss1 < residueSeq.getCnt(); ss1++) {
        if (ss1 == 0 && overlapSeq.getCnt() == 0) {
          fprintf(_dgContextFile, "\n");
        }
        pseq = residueSeq.get(ss1);
        fprintf(
            _dgContextFile,
            "    residue %d:ll_0=%d ll_1=%d ur_0=%d ur_1=%d met=%d trk=%d\n",
            ss1,
            pseq->_ll[0],
            pseq->_ll[1],
            pseq->_ur[0],
            pseq->_ur[1],
            jj + met,
            kk + _dgContextBaseTrack[gridn]);
      }
    }
  }
}

void extMeasure::initTargetSeq()
{
  Array1D<SEQ*>* dgContext = nullptr;
  SEQ* seq;
  for (int jj = 1; jj <= *_dgContextHiLvl; jj++) {
    int gridn = *_dgContextDepth + jj;
    for (int kk = _dgContextLowTrack[gridn]; kk <= _dgContextHiTrack[gridn];
         kk++) {
      int trackn = *_dgContextTracks / 2 + kk;
      dgContext = _dgContextArray[gridn][trackn];
      seq = dgContext->get(0);
      seq->_ll[0] = 1;
    }
  }
}

void extMeasure::printDgContext()
{
  if (_dgContextFile == nullptr) {
    return;
  }

  _dgContextCnt++;
  fprintf(_dgContextFile,
          "diagonalContext %d: baseLevel %d\n",
          _dgContextCnt,
          *_dgContextBaseLvl);
  Array1D<SEQ*>* dgContext = nullptr;
  SEQ* seq = nullptr;
  for (int jj = *_dgContextLowLvl; jj <= *_dgContextHiLvl; jj++) {
    int gridn = *_dgContextDepth + jj;
    fprintf(_dgContextFile,
            "  level %d, plane %d, baseTrack %d\n",
            *_dgContextBaseLvl + jj,
            gridn,
            _dgContextBaseTrack[gridn]);
    int lowTrack = _dgContextLowTrack[gridn];
    int hiTrack = _dgContextHiTrack[gridn];
    for (int kk = lowTrack; kk <= hiTrack; kk++) {
      int trackn = *_dgContextTracks / 2 + kk;
      dgContext = _dgContextArray[gridn][trackn];
      fprintf(_dgContextFile,
              "    track %d (%d), %d seqs\n",
              _dgContextBaseTrack[gridn] + kk,
              trackn,
              dgContext->getCnt());
      for (uint32_t ii = 0; ii < dgContext->getCnt(); ii++) {
        seq = dgContext->get(ii);
        fprintf(_dgContextFile,
                "      seq %d: ll_0=%d ll_1=%d ur_0=%d ur_1=%d rseg=%d\n",
                ii,
                seq->_ll[0],
                seq->_ll[1],
                seq->_ur[0],
                seq->_ur[1],
                seq->type);
      }
    }
  }
}

}  // namespace rcx
