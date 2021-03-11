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

#include <wire.h>

#include "OpenRCX/extRCap.h"
#include "OpenRCX/extprocess.h"

#ifdef _WIN32
#include "direct.h"
#endif

#include <map>
#include <vector>

#include "utility/Logger.h"

//#define SKIP_SOLVER
namespace rcx {

using utl::RCX;

bool OUREVERSEORDER = false;

static int getMetIndexOverUnder(uint met,
                                uint mUnder,
                                uint mOver,
                                uint layerCnt,
                                uint maxCnt,
                                Logger* logger)
{
  int n = layerCnt - met - 1;
  n *= mUnder - 1;
  n += mOver - met - 1;

  if ((n < 0) || (n >= (int) maxCnt)) {
    logger->info(RCX,
                 206,
                 "getOverUnderIndex: out of range n= {}   m={} u= {} o= {}",
                 n,
                 met,
                 mUnder,
                 mOver);
    return -1;
  }

  return n;
}
static uint getMaxMetIndexOverUnder(uint met, uint layerCnt, Logger* logger)
{
  uint n = 0;
  for (uint u = met - 1; u > 0; u--) {
    for (uint o = met + 1; o < layerCnt; o++) {
      uint metIndex = getMetIndexOverUnder(met, u, o, layerCnt, 10000, logger);
      if (n < metIndex)
        n = metIndex;
    }
  }
  return n;
}

static double lineSegment(double X, double x1, double x2, double y1, double y2)
{
  double slope = (y2 - y1) / (x2 - x1);
  double retVal = y2 - slope * (x2 - X);

  return retVal;
}
void extDistRC::interpolate(uint d, extDistRC* rc1, extDistRC* rc2)
{
  _sep = d;
  _coupling
      = lineSegment(d, rc1->_sep, rc2->_sep, rc1->_coupling, rc2->_coupling);
  _fringe = lineSegment(d, rc1->_sep, rc2->_sep, rc1->_fringe, rc2->_fringe);
  _res = lineSegment(d, rc1->_sep, rc2->_sep, rc1->_res, rc2->_res);
}
void extDistRC::set(uint d, double cc, double fr, double a, double r)
{
  _sep = d;
  _coupling = cc;
  _fringe = fr;
  //	_area= a;
  _diag = a;
  _res = r;
}
void extDistRC::readRC(Ath__parser* parser, double dbFactor)
{
  _sep = Ath__double2int(dbFactor * 1000 * parser->getDouble(0));
  _coupling = parser->getDouble(1) / dbFactor;
  _fringe = parser->getDouble(2) / dbFactor;
  //	_area= a;
  _res = parser->getDouble(3) / dbFactor;
}
void extDistRC::readRC_res2(Ath__parser* parser, double dbFactor)
{
  _sep =      Ath__double2int(dbFactor * 1000 * parser->getDouble(0));
  _coupling = Ath__double2int(dbFactor * 1000 * parser->getDouble(1));
  _fringe = parser->getDouble(2) / dbFactor;
  //	_area= a;
  _res = parser->getDouble(3) / dbFactor;
}
double extDistRC::getCoupling()
{
  return _coupling;
}
double extDistRC::getFringe()
{
  return _fringe;
}
double extDistRC::getDiag()
{
  return _diag;
}
double extDistRC::getRes()
{
  return _res;
}
void extDistRC::writeRC()
{
  logger_->info(RCX,
                208,
                "{} {} {} {}  {}",
                0.001 * _sep,
                _coupling,
                _fringe,
                _res,
                _coupling + _fringe);
}
void extDistRC::writeRC(FILE* fp, bool bin)
{
  // fprintf(fp, "%g %g %g %g\n", _dist, _coupling, _fringe, _res);
  fprintf(fp, "%g %g %g %g\n", 0.001 * _sep, _coupling, _fringe, _res);
}
void extRCTable::makeCapTableOver()
{
  _over = true;

  for (uint jj = 1; jj < _maxCnt1; jj++) {
    _inTable[jj] = new Ath__array1D<extDistRC*>*[jj];
    _table[jj] = new Ath__array1D<extDistRC*>*[jj];

    for (uint kk = 0; kk < jj; kk++) {
      _inTable[jj][kk] = new Ath__array1D<extDistRC*>(32);
      _table[jj][kk] = new Ath__array1D<extDistRC*>(512);
    }
  }
}
void extRCTable::makeCapTableUnder()
{
  _over = false;
  for (uint jj = 1; jj < _maxCnt1; jj++) {
    _inTable[jj] = new Ath__array1D<extDistRC*>*[_maxCnt1];
    _table[jj] = new Ath__array1D<extDistRC*>*[_maxCnt1];

    for (uint ii = 0; ii < jj; ii++) {
      _inTable[jj][ii] = NULL;
      _table[jj][ii] = NULL;
    }
    for (uint kk = jj + 1; kk < _maxCnt1; kk++) {
      _inTable[jj][kk] = new Ath__array1D<extDistRC*>(32);
      _table[jj][kk] = new Ath__array1D<extDistRC*>(512);
    }
  }
}

extDistRCTable::extDistRCTable(uint distCnt)
{
  uint n = 16 * (distCnt / 16 + 1);
  _measureTable = new Ath__array1D<extDistRC*>(n);

  _computeTable = NULL;
}

extDistRCTable::~extDistRCTable()
{
  if (_measureTable != NULL)
    delete _measureTable;
  if (_computeTable != NULL)
    delete _computeTable;
}
uint extDistRCTable::mapExtrapolate(uint loDist,
                                    extDistRC* rc2,
                                    uint distUnit,
                                    AthPool<extDistRC>* rcPool)
{
  uint cnt = 0;
  uint d1 = loDist;
  uint d2 = rc2->_sep;

  for (uint d = d1; d <= d2; d += distUnit) {
    extDistRC* rc = rcPool->alloc();

    rc->_sep = d;
    rc->_coupling = rc2->_coupling;
    rc->_fringe = rc2->_fringe;
    rc->_res = rc2->_res;

    // rc->_coupling= lineSegment(d, rc1->_dist, rc2->_dist, rc1->_coupling,
    // rc1->_coupling); rc->_fringe= lineSegment(d,   rc1->_dist, rc2->_dist,
    // rc1->_coupling, rc1->_coupling); rc->_res= lineSegment(d, rc1->_dist,
    // rc2->_dist, rc1->_res, rc1->_res);

    uint n = d / distUnit;

    _computeTable->set(n, rc);

    cnt++;
  }
  return cnt;
}
uint extDistRCTable::mapInterpolate(extDistRC* rc1,
                                    extDistRC* rc2,
                                    uint distUnit,
                                    int maxDist,
                                    AthPool<extDistRC>* rcPool)
{
  uint cnt = 0;
  uint d1 = rc1->_sep;
  uint d2 = rc2->_sep;

  if ((int) d2 > maxDist)
    d2 = maxDist;

  for (uint d = d1; d <= d2; d += distUnit) {
    extDistRC* rc = rcPool->alloc();

    rc->_sep = d;
    rc->interpolate(rc->_sep, rc1, rc2);

    uint n = d / distUnit;

    _computeTable->set(n, rc);

    cnt++;
  }
  return cnt;
}
uint extDistRCTable::interpolate(uint distUnit,
                                 int maxDist,
                                 AthPool<extDistRC>* rcPool)
{
  uint cnt = _measureTable->getCnt();
  uint Cnt = cnt;
  if (cnt == 0)
    return 0;

  if (maxDist < 0) {
    extDistRC* lastRC = _measureTable->get(cnt - 1);
    maxDist = lastRC->_sep;
    if (maxDist == 100000) {
      maxDist = _measureTable->get(cnt - 2)->_sep;
      if (maxDist == 99000) {
        maxDist = _measureTable->get(cnt - 3)->_sep;
        Cnt = cnt - 2;
      } else
        Cnt = cnt - 1;
      extDistRC* rc31 = rcPool->alloc();
      rc31->set(0, 0.0, 0.0, 0.0, 0.0);
      _measureTable->set(31, rc31);
    }
  }

  makeComputeTable(maxDist, distUnit);

  mapExtrapolate(0, _measureTable->get(0), distUnit, rcPool);

  for (uint ii = 0; ii < Cnt - 1; ii++) {
    extDistRC* rc1 = _measureTable->get(ii);
    extDistRC* rc2 = _measureTable->get(ii + 1);

    mapInterpolate(rc1, rc2, distUnit, maxDist, rcPool);
  }
  if (Cnt != cnt) {
    extDistRC* rc1 = _measureTable->get(Cnt);
    extDistRC* rc = rcPool->alloc();
    rc->set(rc1->_sep, rc1->_coupling, rc1->_fringe, 0.0, rc1->_res);
    _computeTable->set(_computeTable->getSize() - 1, rc);
  }

  return _computeTable->getCnt();
}
uint extDistRCTable::writeRules(FILE* fp,
                                Ath__array1D<extDistRC*>* table,
                                double w,
                                bool bin)
{
  uint cnt = table->getCnt();
  if (cnt > 0) {
    extDistRC* rc1 = table->get(cnt - 1);
    if (rc1 != NULL)
      rc1->set(rc1->_sep, 0, rc1->_coupling + rc1->_fringe, 0.0, rc1->_res);
  }

  fprintf(fp, "DIST count %d width %g\n", cnt, w);

  for (uint ii = 0; ii < cnt; ii++)
    table->get(ii)->writeRC(fp, bin);

  fprintf(fp, "END DIST\n");
  return cnt;
}
uint extDistRCTable::writeDiagRules(FILE* fp,
                                    Ath__array1D<extDistRC*>* table,
                                    double w1,
                                    double w2,
                                    double s,
                                    bool bin)
{
  uint cnt = table->getCnt();

  fprintf(fp,
          "DIST count %d width %g diag_width %g diag_dist %g\n",
          cnt,
          w1,
          w2,
          s);
  for (uint ii = 0; ii < cnt; ii++)
    table->get(ii)->writeRC(fp, bin);

  fprintf(fp, "END DIST\n");
  return cnt;
}
uint extDistRCTable::writeRules(FILE* fp, double w, bool compute, bool bin)
{
  if (compute)
    return writeRules(fp, _computeTable, w, bin);
  else
    return writeRules(fp, _measureTable, w, bin);
}
uint extDistRCTable::writeDiagRules(FILE* fp,
                                    double w1,
                                    double w2,
                                    double s,
                                    bool compute,
                                    bool bin)
{
  if (compute)
    return writeDiagRules(fp, _computeTable, w1, w2, s, bin);
  else
    return writeDiagRules(fp, _measureTable, w1, w2, s, bin);
}
uint extMetRCTable::readRCstats(Ath__parser* parser)
{
  uint cnt = 0;

  extMeasure m;

  while (parser->parseNextLine() > 0) {
    cnt++;

    m._overUnder = false;
    m._over = false;

    if ((parser->isKeyword(2, "OVER")) && (parser->isKeyword(4, "UNDER"))) {
      m._met = parser->getInt(1);
      m._overMet = parser->getInt(5);
      m._underMet = parser->getInt(3);
      m._overUnder = true;

      m._w_m = parser->getDouble(7);
      m._w_nm = Ath__double2int(m._w_m * 1000);

      m._s_m = parser->getDouble(9);
      m._s_nm = Ath__double2int(m._s_m * 1000);

      extDistRC* rc = _rcPoolPtr->alloc();

      rc->set(m._s_nm,
              parser->getDouble(10),
              parser->getDouble(11),
              0.0,
              parser->getDouble(12));

      m._tmpRC = rc;
    } else if (parser->isKeyword(2, "UNDER")
               || parser->isKeyword(2, "DIAGUNDER")) {
      m._met = parser->getInt(1);
      m._overMet = parser->getInt(4);
      m._underMet = -1;

      m._w_m = parser->getDouble(6);
      m._w_nm = Ath__double2int(m._w_m * 1000);

      m._s_m = parser->getDouble(8);
      m._s_nm = Ath__double2int(m._s_m * 1000);

      extDistRC* rc = _rcPoolPtr->alloc();

      rc->set(m._s_nm,
              parser->getDouble(9),
              parser->getDouble(10),
              0.0,
              parser->getDouble(11));

      m._tmpRC = rc;
    } else if (parser->isKeyword(2, "OVER")) {
      m._met = parser->getInt(1);
      m._underMet = parser->getInt(4);
      m._overMet = -1;
      m._over = true;

      m._w_m = parser->getDouble(6);
      m._w_nm = Ath__double2int(m._w_m * 1000);

      m._s_m = parser->getDouble(8);
      m._s_nm = Ath__double2int(m._s_m * 1000);

      extDistRC* rc = _rcPoolPtr->alloc();

      rc->set(m._s_nm,
              parser->getDouble(9),
              parser->getDouble(10),
              0.0,
              parser->getDouble(11));

      m._tmpRC = rc;
    }
    addRCw(&m);
  }
  mkWidthAndSpaceMappings();
  return cnt;
}
uint extDistRCTable::readRules_res2(Ath__parser* parser,
                               AthPool<extDistRC>* rcPool,
                               bool compute,
                               bool bin,
                               bool ignore,
                               double dbFactor)
{
  parser->parseNextLine();
  uint cnt = parser->getInt(2);
  if (cnt < 32)
    cnt = 32;
  // _measureTable= NULL;

  Ath__array1D<extDistRC*>* table = NULL;
  if (!ignore)
    table = new Ath__array1D<extDistRC*>(cnt);

  while (parser->parseNextLine() > 0) {
    if (parser->isKeyword(0, "END"))
      break;

    if (ignore)
      continue;

    extDistRC* rc = rcPool->alloc();
    rc->readRC_res2(parser, dbFactor);
    table->add(rc);
  }
  if (ignore)
    return cnt;

  _measureTable = table;

  return cnt;
}
uint extDistRCTable::readRules(Ath__parser* parser,
                               AthPool<extDistRC>* rcPool,
                               bool compute,
                               bool bin,
                               bool ignore,
                               double dbFactor)
{
  parser->parseNextLine();
  uint cnt = parser->getInt(2);
  if (cnt < 32)
    cnt = 32;
  // _measureTable= NULL;

  Ath__array1D<extDistRC*>* table = NULL;
  if (!ignore)
    table = new Ath__array1D<extDistRC*>(cnt);

  while (parser->parseNextLine() > 0) {
    if (parser->isKeyword(0, "END"))
      break;

    if (ignore)
      continue;

    extDistRC* rc = rcPool->alloc();
    rc->readRC(parser, dbFactor);
    table->add(rc);
  }
  bool SCALE_RES_ON_MAX_DIST = true;
  if (SCALE_RES_ON_MAX_DIST) {
    double SUB_MULT_RES = 0.5;
    // ScaleRes(SUB_MULT_RES, table);
  }
  if (ignore)
    return cnt;

  _measureTable = table;

  if (compute)
#ifdef HI_ACC_1
    // interpolate(12, -1, rcPool);
    interpolate(4, -1, rcPool);
#else
    interpolate(4, -1, rcPool);
#endif

  return cnt;
}
void extDistRCTable::ScaleRes(double SUB_MULT_RES,
                              Ath__array1D<extDistRC*>* table)
{
  uint cnt = table->getCnt();
  if (cnt == 0)
    return;

  extDistRC* rc_last = table->get(cnt - 1);

  for (uint jj = 0; jj < cnt; jj++) {
    extDistRC* rc = table->get(jj);
    double delta = rc->_res - rc_last->_res;
    if (delta < 0)
      delta = -delta;
    if (delta > 0.000001)
      continue;

    rc->_res *= SUB_MULT_RES;
  }
}
void extDistRCTable::makeComputeTable(uint maxDist, uint distUnit)
{
  _unit = distUnit;  // in nm
  uint n = maxDist / distUnit;
  n = 16 * (n / 16 + 1);

  _computeTable = new Ath__array1D<extDistRC*>(n + 1);
  // TODO
}
uint extDistRCTable::addMeasureRC(extDistRC* rc)
{
  return _measureTable->add(rc);
}
extDistRC* extDistRCTable::getRC_99()
{
  if (_measureTable == NULL)
    return NULL;

  uint cnt = _measureTable->getCnt();
  if (cnt < 2)
    return NULL;

  extDistRC* before_lastRC = _measureTable->get(cnt - 2);
  if (before_lastRC->_sep == 99000)
    return before_lastRC;

  extDistRC* lastRC
      = _measureTable->getLast();  // assuming last is 100 equivalent to inf
  if (lastRC->_sep == 99000)
    return lastRC;

  return NULL;
}
extDistRC* extDistRCTable::getComputeRC(uint dist)
{
  if (_measureTable == NULL)
    return NULL;

  if (_measureTable->getCnt() <= 0)
    return NULL;

  extDistRC* firstRC = _measureTable->get(0);
  uint firstDist = firstRC->_sep;
  if (dist <= firstDist) {
    return firstRC;
  }

  /*
  extDistRC* secondRC= _measureTable->get(1);
        if (dist<=secondRC->_sep)
                return secondRC;
  */
  if (_measureTable->getLast()->_sep == 100000) {
    extDistRC* before_lastRC = _measureTable->getLast()
                               - 1;  // assuming last is 100 equivalent to inf
    uint lastDist = before_lastRC->_sep;

    if (lastDist == 99000)
      before_lastRC = before_lastRC - 1;

    lastDist = before_lastRC->_sep;
    if (dist >= lastDist) {  // send Inf dist
      if (dist == lastDist)  // send Inf dist
        return before_lastRC;
      if (dist <= 2 * lastDist) {  // send Inf dist

        uint cnt = _measureTable->getCnt();
        extDistRC* rc31 = _measureTable->geti(31);
        extDistRC* rc2 = _measureTable->get(cnt - 2);
        extDistRC* rc3 = _measureTable->get(cnt - 3);

        rc31->_sep = dist;
        rc31->interpolate(dist, rc3, rc2);

        rc31->_coupling
            = (before_lastRC->_coupling / dist) * before_lastRC->_sep;
        rc31->_fringe = before_lastRC->_fringe;
        // rc31->_fringe= (before_lastRC->_fringe / dist) * before_lastRC->_sep;
        return rc31;
        // return before_lastRC;
      }
      if (dist > lastDist) {  // send Inf dist
        return _measureTable->getLast();
      }
    }
  } else {
    extDistRC* before_lastRC
        = _measureTable->getLast();  // assuming last is 100 equivalent to inf
    uint lastDist = before_lastRC->_sep;
    if (dist >= lastDist - _unit && lastDist > 0)  // send Inf dist
      return _measureTable->getLast();
  }

  uint n = dist / _unit;
  return _computeTable->geti(n);
}
/*
extDistRC* extDistRCTable::getComputeRC(double dist)
{
        uint n= dist/_unit);
        return _computeTable->get(n);
}
*/
uint extDistWidthRCTable::getWidthIndex(uint w)
{
  if ((int) w >= _lastWidth)
    return _widthTable->getCnt() - 1;

  int v = w - _firstWidth;
  if (v < 0)
    return 0;

  return _widthMapTable->geti(v / _modulo);
}
uint extDistWidthRCTable::getDiagWidthIndex(uint m, uint w)
{
  if (_lastDiagWidth == NULL)  // TO_DEBUG 620
    return -1;

  if ((int) w >= _lastDiagWidth->geti(m))
    return _diagWidthTable[m]->getCnt() - 1;

  int v = w - _firstDiagWidth->geti(m);
  if (v < 0)
    return 0;

  return _diagWidthMapTable[m]->geti(v / _modulo);
}
uint extDistWidthRCTable::getDiagDistIndex(uint m, uint s)
{
  if ((int) s >= _lastDiagDist->geti(m))
    return _diagDistTable[m]->getCnt() - 1;

  int v = s - _firstDiagDist->geti(m);
  if (v < 0)
    return 0;

  return _diagDistMapTable[m]->geti(v / _modulo);
}
/*
void extDistWidthRCTable::setOUReverseOrder()
{
        _ouReadReverse = true;
}
*/
extDistWidthRCTable::extDistWidthRCTable(bool over,
                                         uint met,
                                         uint layerCnt,
                                         uint metCnt,
                                         uint maxWidthCnt,
                                         AthPool<extDistRC>* rcPool,
                                         Logger* logger)
{
  logger_ = logger;
  _ouReadReverse = OUREVERSEORDER;
  _over = over;
  _layerCnt = layerCnt;
  _met = met;

  _widthTable = new Ath__array1D<int>(maxWidthCnt);

  _firstWidth = 0;
  _lastWidth = 0;
  _modulo = 4;

  _widthTableAllocFlag = false;
  _widthMapTable = NULL;

  _metCnt = metCnt;

  _rcDistTable = new extDistRCTable**[_metCnt];
  uint jj;
  for (jj = 0; jj < _metCnt; jj++) {
    _rcDistTable[jj] = new extDistRCTable*[maxWidthCnt];
    for (uint ii = 0; ii < maxWidthCnt; ii++)
      _rcDistTable[jj][ii] = new extDistRCTable(10);
  }
  _rcPoolPtr = rcPool;

  _firstDiagWidth = NULL;
  _lastDiagWidth = NULL;
  _firstDiagDist = NULL;
  _lastDiagDist = NULL;

  for (jj = 0; jj < 16; jj++) {
    _diagWidthMapTable[jj] = NULL;
    _diagDistMapTable[jj] = NULL;
    _diagWidthTable[jj] = NULL;
    _diagDistTable[jj] = NULL;
  }
  _rcDiagDistTable = NULL;
  _rc31 = rcPool->alloc();
}
void extDistWidthRCTable::createWidthMap()
{
  uint widthCnt = _widthTable->getCnt();
  if (widthCnt == 0)
    return;

  _firstWidth = _widthTable->get(0);
  _lastWidth = _widthTable->getLast();
  _modulo = 4;

  _widthTableAllocFlag = true;
  _widthMapTable = new Ath__array1D<uint>(10 * widthCnt);

  uint jj;
  for (jj = 0; jj < widthCnt - 1; jj++) {
    double v1 = _widthTable->get(jj);
    double v2 = _widthTable->get(jj + 1);

    int w1 = Ath__double2int(v1);
    int w2 = Ath__double2int(v2);

    for (int w = w1; w <= w2; w += _modulo) {
      if (w >= _lastWidth)
        continue;

      uint n = 0;
      int v = w - _firstWidth;
      if (v > 0)
        n = v / _modulo;

      _widthMapTable->set(n, jj);
    }
  }
}
void extDistWidthRCTable::makeWSmapping()
{
  createWidthMap();

  for (uint jj = 0; jj < _metCnt; jj++)
    for (uint ii = 0; ii < _widthTable->getCnt(); ii++)
      _rcDistTable[jj][ii]->interpolate(4, -1, _rcPoolPtr);
}
extDistWidthRCTable::extDistWidthRCTable(bool dummy,
                                         uint met,
                                         uint layerCnt,
                                         uint widthCnt,
                                         Logger* logger)
{
  logger_ = logger;
  _ouReadReverse = OUREVERSEORDER;
  _met = met;
  _layerCnt = layerCnt;

  _widthTable = new Ath__array1D<int>(widthCnt);
  for (uint ii = 0; ii < widthCnt; ii++) {
    _widthTable->add(0);
  }

  _widthMapTable = NULL;
  _widthTableAllocFlag = true;

  // _metCnt= 1;
  _metCnt = layerCnt;

  _rcDistTable = new extDistRCTable**[_metCnt];
  uint jj;
  for (jj = 0; jj < _metCnt; jj++) {
    _rcDistTable[jj] = new extDistRCTable*[widthCnt];
    for (uint ii = 0; ii < widthCnt; ii++)
      _rcDistTable[jj][ii] = new extDistRCTable(1);
  }
  _firstDiagWidth = NULL;
  _lastDiagWidth = NULL;
  _firstDiagDist = NULL;
  _rcDiagDistTable = NULL;
  _lastDiagDist = NULL;
  for (jj = 0; jj < 32; jj++) {
    _diagWidthMapTable[jj] = NULL;
    _diagDistMapTable[jj] = NULL;
    _diagWidthTable[jj] = NULL;
    _diagDistTable[jj] = NULL;
  }
  _rc31 = NULL;
}
extDistWidthRCTable::extDistWidthRCTable(bool over,
                                         uint met,
                                         uint layerCnt,
                                         uint metCnt,
                                         Ath__array1D<double>* widthTable,
                                         AthPool<extDistRC>* rcPool,
                                         double dbFactor,
                                         Logger* logger)
{
  logger_ = logger;
  _ouReadReverse = OUREVERSEORDER;
  _over = over;
  _layerCnt = layerCnt;
  _met = met;

  if (widthTable->getCnt() == 0)
    return;

  int widthCnt = widthTable->getCnt();
  _widthTable = new Ath__array1D<int>(widthCnt);
  for (uint ii = 0; ii < widthCnt; ii++) {
    int w = Ath__double2int(dbFactor * 1000 * widthTable->get(ii));
    _widthTable->add(w);
  }
  if (widthCnt > 0) {
    _firstWidth = _widthTable->get(0);
    _lastWidth = _widthTable->get(widthCnt - 1);
  }

  _modulo = 4;

  _widthTableAllocFlag = true;
  _widthMapTable = new Ath__array1D<uint>(10 * widthCnt);

  uint jj;
  for (jj = 0; jj < widthCnt - 1; jj++) {
    double v1 = _widthTable->get(jj);
    double v2 = _widthTable->get(jj + 1);

    int w1 = Ath__double2int(v1);
    int w2 = Ath__double2int(v2);

    for (int w = w1; w <= w2; w += _modulo) {
      if (w >= _lastWidth)
        continue;

      uint n = 0;
      int v = w - _firstWidth;
      if (v > 0)
        n = v / _modulo;

      _widthMapTable->set(n, jj);
    }
  }

  _metCnt = metCnt;

  _rcDistTable = new extDistRCTable**[_metCnt];
  for (jj = 0; jj < _metCnt; jj++) {
    _rcDistTable[jj] = new extDistRCTable*[widthCnt];
    for (uint ii = 0; ii < widthCnt; ii++)
      _rcDistTable[jj][ii] = new extDistRCTable(10);
  }
  _rcPoolPtr = rcPool;

  _firstDiagWidth = NULL;
  _lastDiagWidth = NULL;
  _firstDiagDist = NULL;
  _lastDiagDist = NULL;
  for (jj = 0; jj < 12; jj++) {
    _diagWidthMapTable[jj] = NULL;
    _diagDistMapTable[jj] = NULL;
    _diagWidthTable[jj] = NULL;
    _diagDistTable[jj] = NULL;
  }
  _rcDiagDistTable = NULL;
  _rc31 = rcPool->alloc();
}
extDistWidthRCTable::extDistWidthRCTable(bool over,
                                         uint met,
                                         uint layerCnt,
                                         uint metCnt,
                                         Ath__array1D<double>* widthTable,
                                         int diagWidthCnt,
                                         int diagDistCnt,
                                         AthPool<extDistRC>* rcPool,
                                         double dbFactor,
                                         Logger* logger)
{
  logger_ = logger;
  _ouReadReverse = OUREVERSEORDER;
  _over = over;
  _layerCnt = layerCnt;
  _met = met;

  uint widthCnt = widthTable->getCnt();
  _widthTable = new Ath__array1D<int>(widthCnt);
  for (uint ii = 0; ii < widthCnt; ii++) {
    int w = Ath__double2int(dbFactor * 1000 * widthTable->get(ii));
    _widthTable->add(w);
  }
  for (uint i = 0; i < layerCnt; i++) {
    _diagWidthTable[i] = new Ath__array1D<int>(diagWidthCnt);
    _diagDistTable[i] = new Ath__array1D<int>(diagDistCnt);
    _diagWidthMapTable[i] = new Ath__array1D<uint>(10 * diagWidthCnt);
    _diagDistMapTable[i] = new Ath__array1D<uint>(10 * diagDistCnt);
  }

  _firstWidth = _widthTable->get(0);
  _lastWidth = _widthTable->get(widthCnt - 1);
  _firstDiagWidth = new Ath__array1D<int>(layerCnt);
  _lastDiagWidth = new Ath__array1D<int>(layerCnt);
  _firstDiagDist = new Ath__array1D<int>(layerCnt);
  _lastDiagDist = new Ath__array1D<int>(layerCnt);

  _modulo = 4;

  _widthTableAllocFlag = true;
  _widthMapTable = new Ath__array1D<uint>(10 * widthCnt);
  uint jj;
  for (jj = 0; jj < widthCnt - 1; jj++) {
    double v1 = _widthTable->get(jj);
    double v2 = _widthTable->get(jj + 1);

    int w1 = Ath__double2int(v1);
    int w2 = Ath__double2int(v2);

    for (int w = w1; w <= w2; w += _modulo) {
      if (w >= _lastWidth)
        continue;

      uint n = 0;
      int v = w - _firstWidth;
      if (v > 0)
        n = v / _modulo;

      _widthMapTable->set(n, jj);
    }
  }

  _metCnt = metCnt;
  _rcDiagDistTable = new extDistRCTable****[_metCnt];
  for (jj = 0; jj < _metCnt; jj++) {
    _rcDiagDistTable[jj] = new extDistRCTable***[widthCnt];
    for (uint ii = 0; ii < widthCnt; ii++) {
      _rcDiagDistTable[jj][ii] = new extDistRCTable**[diagWidthCnt];
      for (int kk = 0; kk < diagWidthCnt; kk++) {
        _rcDiagDistTable[jj][ii][kk] = new extDistRCTable*[diagDistCnt];
        for (int ll = 0; ll < diagDistCnt; ll++)
          _rcDiagDistTable[jj][ii][kk][ll] = new extDistRCTable(10);
      }
    }
  }
  _rcPoolPtr = rcPool;
  _rcDistTable = NULL;

  _rc31 = rcPool->alloc();
}
void extDistWidthRCTable::setDiagUnderTables(
    uint met,
    Ath__array1D<double>* diagWidthTable,
    Ath__array1D<double>* diagDistTable,
    double dbFactor)
{
  uint diagWidthCnt = diagWidthTable->getCnt();
  _diagWidthTable[met]->resetCnt();
  uint ii;
  for (ii = 0; ii < diagWidthCnt; ii++) {
    int w = Ath__double2int(dbFactor * 1000 * diagWidthTable->get(ii));
    _diagWidthTable[met]->add(w);
  }
  _firstDiagWidth->set(met, _diagWidthTable[met]->get(0));
  _lastDiagWidth->set(met, _diagWidthTable[met]->get(diagWidthCnt - 1));
  uint diagDistCnt = diagDistTable->getCnt();
  _diagDistTable[met]->resetCnt();
  for (ii = 0; ii < diagDistCnt; ii++) {
    int s = Ath__double2int(dbFactor * 1000 * diagDistTable->get(ii));
    _diagDistTable[met]->add(s);
  }
  _firstDiagDist->set(met, _diagDistTable[met]->get(0));
  _lastDiagDist->set(met, _diagDistTable[met]->get(diagDistCnt - 1));
  uint jj;
  for (jj = 0; jj < diagWidthCnt - 1; jj++) {
    double v1 = _diagWidthTable[met]->get(jj);
    double v2 = _diagWidthTable[met]->get(jj + 1);

    int w1 = Ath__double2int(v1);
    int w2 = Ath__double2int(v2);

    for (int w = w1; w <= w2; w += _modulo) {
      if (w >= _lastDiagWidth->geti(met))
        continue;

      uint n = 0;
      int v = w - _firstDiagWidth->geti(met);
      if (v > 0)
        n = v / _modulo;

      _diagWidthMapTable[met]->set(n, jj);
    }
  }
  for (jj = 0; jj < diagDistCnt - 1; jj++) {
    double v1 = _diagDistTable[met]->get(jj);
    double v2 = _diagDistTable[met]->get(jj + 1);

    int s1 = Ath__double2int(v1);
    int s2 = Ath__double2int(v2);

    for (int s = s1; s <= s2; s += _modulo) {
      if (s >= _lastDiagDist->geti(met))
        continue;

      int d = (s2 - s1) / 2;

      uint n = 0;
      int v = s - _firstDiagDist->geti(met);
      if (v > 0)
        n = v / _modulo;

      if (v < s1 + d)
        _diagDistMapTable[met]->set(n, jj);
      else
        _diagDistMapTable[met]->set(n, jj + 1);
    }
  }
}
extDistWidthRCTable::~extDistWidthRCTable()
{
  logger_ = nullptr;
  uint ii, jj, kk, ll;
  if (_rcDistTable) {
    for (jj = 0; jj < _metCnt; jj++) {
      for (ii = 0; ii < _widthTable->getCnt(); ii++)
        if (_rcDistTable[jj][ii])
          delete _rcDistTable[jj][ii];

      if (_rcDistTable[jj])
        delete[] _rcDistTable[jj];
    }
    delete[] _rcDistTable;
  }

  if (_rcDiagDistTable) {
    for (jj = 0; jj < _metCnt; jj++) {
      for (ii = 0; ii < _widthTable->getCnt(); ii++) {
        for (kk = 0; kk < _diagWidthTable[jj]->getCnt(); kk++) {
          for (ll = 0; ll < _diagDistTable[jj]->getCnt(); ll++)
            delete _rcDiagDistTable[jj][ii][kk][ll];
          delete[] _rcDiagDistTable[jj][ii][kk];
        }
        delete[] _rcDiagDistTable[jj][ii];
      }
      delete[] _rcDiagDistTable[jj];
    }
    delete[] _rcDiagDistTable;
  }

  //	if (_widthTableAllocFlag)
  if (_widthTable)
    delete _widthTable;
  if (_widthMapTable)
    delete _widthMapTable;
  if (_firstDiagWidth)
    delete _firstDiagWidth;
  if (_lastDiagWidth)
    delete _lastDiagWidth;
  if (_firstDiagDist)
    delete _firstDiagDist;
  if (_lastDiagDist)
    delete _lastDiagDist;
  for (uint i = 0; i < _layerCnt; i++) {
    if (_diagWidthTable[i] != NULL)
      delete _diagWidthTable[i];
    if (_diagDistTable[i] != NULL)
      delete _diagDistTable[i];
    if (_diagWidthMapTable[i] != NULL)
      delete _diagWidthMapTable[i];
    if (_diagDistMapTable[i] != NULL)
      delete _diagDistMapTable[i];
  }
}
uint extDistWidthRCTable::writeWidthTable(FILE* fp, bool bin)
{
  uint widthCnt = _widthTable->getCnt();
  fprintf(fp, "WIDTH Table %d entries: ", widthCnt);
  for (uint ii = 0; ii < widthCnt; ii++)
    fprintf(fp, " %g", 0.001 * _widthTable->get(ii));
  fprintf(fp, "\n");
  return widthCnt;
}
uint extDistWidthRCTable::writeDiagWidthTable(FILE* fp, uint met, bool bin)
{
  uint diagWidthCnt = _diagWidthTable[met]->getCnt();
  fprintf(fp, "DIAG_WIDTH Table %d entries: ", diagWidthCnt);
  for (uint ii = 0; ii < diagWidthCnt; ii++)
    fprintf(fp, " %g", 0.001 * _diagWidthTable[met]->get(ii));
  fprintf(fp, "\n");
  return diagWidthCnt;
}
void extDistWidthRCTable::writeDiagTablesCnt(FILE* fp, uint met, bool bin)
{
  uint diagWidthCnt = _diagWidthTable[met]->getCnt();
  uint diagDistCnt = _diagDistTable[met]->getCnt();
  fprintf(fp, "DIAG_WIDTH Table Count: %d\n", diagWidthCnt);
  fprintf(fp, "DIAG_DIST Table Count: %d\n", diagDistCnt);
}
uint extDistWidthRCTable::writeDiagDistTable(FILE* fp, uint met, bool bin)
{
  uint diagDistCnt = _diagDistTable[met]->getCnt();
  fprintf(fp, "DIAG_DIST Table %d entries: ", diagDistCnt);
  for (uint ii = 0; ii < diagDistCnt; ii++)
    fprintf(fp, " %g", 0.001 * _diagDistTable[met]->get(ii));
  fprintf(fp, "\n");
  return diagDistCnt;
}
uint extDistWidthRCTable::writeRulesOver(FILE* fp, bool bin)
{
  uint cnt = 0;
  fprintf(fp, "\nMetal %d OVER\n", _met);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();

  for (uint ii = 0; ii < _met; ii++) {
    fprintf(fp, "\nMetal %d OVER %d\n", _met, ii);

    for (uint jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[ii][jj]->writeRules(
          fp, 0.001 * _widthTable->get(jj), false, bin);
    }
  }
  return cnt;
}
uint extDistWidthRCTable::writeRulesOver_res(FILE* fp, bool bin)
{
  uint cnt = 0;
  fprintf(fp, "\nMetal %d RESOVER\n", _met);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();

  for (uint ii = 0; ii < _met; ii++) {
    fprintf(fp, "\nMetal %d RESOVER %d\n", _met, ii);

    for (uint jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[ii][jj]->writeRules(
          fp, 0.001 * _widthTable->get(jj), false, bin);
    }
  }
  return cnt;
}
uint extDistWidthRCTable::readMetalHeader(Ath__parser* parser,
                                          uint& met,
                                          const char* keyword,
                                          bool bin,
                                          bool ignore)
{
  if (!(parser->parseNextLine() > 0))
    return 0;

  //	uint cnt= 0;
  if (parser->isKeyword(0, "Metal") && (strcmp(parser->get(2), keyword) == 0)) {
    met = parser->getInt(1);
    return 1;
  }

  return 0;
}
uint extDistWidthRCTable::readRulesOver(Ath__parser* parser,
                                        uint widthCnt,
                                        bool bin,
                                        bool ignore,
                                        const char *OVER,
                                        double dbFactor)
{
  bool res= strcmp(OVER, "RESOVER")==0;
  uint cnt = 0;
  for (uint ii = 0; ii < _met; ii++) {
    uint met = 0;
    if (readMetalHeader(parser, met, OVER, bin, ignore) <= 0)
      return 0;

    parser->getInt(3);

    
    for (uint jj = 0; jj < widthCnt; jj++) {
      if (res) {
        if (!ignore)
          cnt += _rcDistTable[ii][jj]->readRules_res2(
                    parser, _rcPoolPtr, true, bin, ignore, dbFactor);
        else
          cnt += _rcDistTable[0][0]->readRules_res2(
                    parser, _rcPoolPtr, true, bin, ignore, dbFactor);
      } else {
      if (!ignore)
        cnt += _rcDistTable[ii][jj]->readRules(
            parser, _rcPoolPtr, true, bin, ignore, dbFactor);
      else
        cnt += _rcDistTable[0][0]->readRules(
            parser, _rcPoolPtr, true, bin, ignore, dbFactor);
      }
    }
  }
  return cnt;
}
uint extDistWidthRCTable::readRulesUnder(Ath__parser* parser,
                                         uint widthCnt,
                                         bool bin,
                                         bool ignore,
                                         double dbFactor)
{
  uint cnt = 0;
  for (uint ii = _met + 1; ii < _layerCnt; ii++) {
    uint met = 0;
    if (readMetalHeader(parser, met, "UNDER", bin, ignore) <= 0)
      return 0;

    uint metIndex = getMetIndexUnder(ii);
    if (ignore)
      metIndex = 0;

    parser->getInt(3);

    for (uint jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[metIndex][jj]->readRules(
          parser, _rcPoolPtr, true, bin, ignore, dbFactor);
    }
  }
  return cnt;
}
uint extDistWidthRCTable::readRulesDiagUnder(Ath__parser* parser,
                                             uint widthCnt,
                                             uint diagWidthCnt,
                                             uint diagDistCnt,
                                             bool bin,
                                             bool ignore,
                                             double dbFactor)
{
  uint cnt = 0;
  for (uint ii = _met + 1; ii < _met + 5 && ii < _layerCnt; ii++) {
    uint met = 0;
    if (readMetalHeader(parser, met, "DIAGUNDER", bin, ignore) <= 0)
      return 0;
    Ath__array1D<double>* dwTable = NULL;
    Ath__array1D<double>* ddTable = NULL;
    parser->parseNextLine();
    dwTable = parser->readDoubleArray("DIAG_WIDTH", 4);
    parser->parseNextLine();
    ddTable = parser->readDoubleArray("DIAG_DIST", 4);
    uint diagWidthCnt = dwTable->getCnt();
    uint diagDistCnt = ddTable->getCnt();
    //		setDiagUnderTables(ii, dwTable, ddTable);

    uint metIndex = getMetIndexUnder(ii);

    if (!ignore)
      setDiagUnderTables(metIndex, dwTable, ddTable);

    parser->getInt(3);

    for (uint jj = 0; jj < widthCnt; jj++) {
      for (uint kk = 0; kk < diagWidthCnt; kk++) {
        for (uint ll = 0; ll < diagDistCnt; ll++) {
          if (!ignore)
            cnt += _rcDiagDistTable[metIndex][jj][kk][ll]->readRules(
                parser, _rcPoolPtr, true, bin, ignore, dbFactor);
          else
            cnt += _rcDistTable[0][0]->readRules(
                parser, _rcPoolPtr, true, bin, ignore, dbFactor);
        }
      }
    }
    delete dwTable;
    delete ddTable;
  }
  return cnt;
}
uint extDistWidthRCTable::readRulesDiagUnder(Ath__parser* parser,
                                             uint widthCnt,
                                             bool bin,
                                             bool ignore,
                                             double dbFactor)
{
  uint cnt = 0;
  for (uint ii = _met + 1; ii < _layerCnt; ii++) {
    uint met = 0;
    if (readMetalHeader(parser, met, "DIAGUNDER", bin, ignore) <= 0)
      return 0;

    uint metIndex = getMetIndexUnder(ii);
    parser->getInt(3);

    for (uint jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[metIndex][jj]->readRules(
          parser, _rcPoolPtr, true, bin, ignore, dbFactor);
    }
  }
  return cnt;
}
uint extDistWidthRCTable::readRulesOverUnder(Ath__parser* parser,
                                             uint widthCnt,
                                             bool bin,
                                             bool ignore,
                                             double dbFactor)
{
  uint cnt = 0;
  for (uint u = 1; u < _met; u++) {
    for (uint o = _met + 1; o < _layerCnt; o++) {
      uint mUnder = u;
      uint mOver = o;

      uint met = 0;
      if (readMetalHeader(parser, met, "OVER", bin, ignore) <= 0)
        return 0;

      if (_ouReadReverse)
        mOver = parser->getInt(5);

      mUnder = parser->getInt(3);

      // Commented out this code per Dimitris...
      // The variable mOver is already defined above...
      // uint mOver= parser->getInt(5);

      int metIndex = 0;
      if (!ignore)
        metIndex = getMetIndexOverUnder(
            _met, mUnder, mOver, _layerCnt, _metCnt, logger_);
      int mcnt = 0;
      for (uint jj = 0; jj < widthCnt; jj++) {
        if (!ignore)
          mcnt += _rcDistTable[metIndex][jj]->readRules(
              parser, _rcPoolPtr, true, bin, ignore, dbFactor);
        else
          mcnt += _rcDistTable[0][0]->readRules(
              parser, _rcPoolPtr, true, bin, ignore, dbFactor);
      }
      cnt += mcnt;
      // logger_->info(RCX, 0,"OU metIndex={} met={}  mUnder={}  mOver={}
      // layerCnt={} _metCnt={} widthCnt={} cnt={} ", metIndex, _met, mUnder,
      // mOver, _layerCnt, _metCnt, widthCnt, mcnt);
    }
  }
  return cnt;
}
uint extDistWidthRCTable::getMetIndexUnder(uint mOver)
{
  return mOver - _met - 1;
}
uint extDistWidthRCTable::writeRulesUnder(FILE* fp, bool bin)
{
  uint cnt = 0;
  fprintf(fp, "\nMetal %d UNDER\n", _met);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();

  for (uint ii = _met + 1; ii < _layerCnt; ii++) {
    fprintf(fp, "\nMetal %d UNDER %d\n", _met, ii);

    uint metIndex = getMetIndexUnder(ii);

    for (uint jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[metIndex][jj]->writeRules(
          fp, 0.001 * _widthTable->get(jj), false, bin);
    }
  }
  return cnt;
}
uint extDistWidthRCTable::writeRulesDiagUnder2(FILE* fp, bool bin)
{
  uint cnt = 0;
  fprintf(fp, "\nMetal %d DIAGUNDER\n", _met);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();
  writeDiagTablesCnt(fp, _met + 1, bin);

  for (uint ii = _met + 1; ii < _met + 5 && ii < _layerCnt; ii++) {
    fprintf(fp, "\nMetal %d DIAGUNDER %d\n", _met, ii);
    writeDiagWidthTable(fp, ii, bin);
    uint diagWidthCnt = _diagWidthTable[ii]->getCnt();
    writeDiagDistTable(fp, ii, bin);
    uint diagDistCnt = _diagDistTable[ii]->getCnt();

    uint metIndex = getMetIndexUnder(ii);

    for (uint jj = 0; jj < widthCnt; jj++) {
      for (uint kk = 0; kk < diagWidthCnt; kk++) {
        for (uint ll = 0; ll < diagDistCnt; ll++) {
          cnt += _rcDiagDistTable[metIndex][jj][kk][ll]->writeDiagRules(
              fp,
              0.001 * _widthTable->get(jj),
              0.001 * _diagWidthTable[ii]->get(kk),
              0.001 * _diagDistTable[ii]->get(ll),
              false,
              bin);
        }
      }
    }
  }
  return cnt;
}
uint extDistWidthRCTable::writeRulesDiagUnder(FILE* fp, bool bin)
{
  uint cnt = 0;
  fprintf(fp, "\nMetal %d DIAGUNDER\n", _met);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();

  for (uint ii = _met + 1; ii < _layerCnt; ii++) {
    fprintf(fp, "\nMetal %d DIAGUNDER %d\n", _met, ii);

    uint metIndex = getMetIndexUnder(ii);

    for (uint jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[metIndex][jj]->writeRules(
          fp, 0.001 * _widthTable->get(jj), false, bin);
    }
  }
  return cnt;
}
uint extDistWidthRCTable::writeRulesOverUnder(FILE* fp, bool bin)
{
  uint cnt = 0;
  fprintf(fp, "\nMetal %d OVERUNDER\n", _met);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();

  for (uint mUnder = 1; mUnder < _met; mUnder++) {
    for (uint mOver = _met + 1; mOver < _layerCnt; mOver++) {
      fprintf(fp, "\nMetal %d OVER %d UNDER %d\n", _met, mUnder, mOver);

      int metIndex = getMetIndexOverUnder(
          _met, mUnder, mOver, _layerCnt, _metCnt, logger_);
      assert(metIndex >= 0);

      for (uint jj = 0; jj < widthCnt; jj++) {
        cnt += _rcDistTable[metIndex][jj]->writeRules(
            fp, 0.001 * _widthTable->get(jj), false, bin);
      }
    }
  }
  return cnt;
}
extMetRCTable::extMetRCTable(uint layerCnt,
                             AthPool<extDistRC>* rcPool,
                             Logger* logger)
{
  logger_ = logger;
  _layerCnt = layerCnt;

  _resOver = new extDistWidthRCTable*[layerCnt];
  _capOver = new extDistWidthRCTable*[layerCnt];
  _capDiagUnder = new extDistWidthRCTable*[layerCnt];
  _capUnder = new extDistWidthRCTable*[layerCnt];
  _capOverUnder = new extDistWidthRCTable*[layerCnt];
  for (uint ii = 0; ii < layerCnt; ii++) {
    _resOver[ii] = NULL;
    _capOver[ii] = NULL;
    _capDiagUnder[ii] = NULL;
    _capUnder[ii] = NULL;
    _capOverUnder[ii] = NULL;
  }
  _rcPoolPtr = rcPool;
  _rate = -1000.0;
}
extMetRCTable::~extMetRCTable()
{
  for (uint ii = 0; ii < _layerCnt; ii++) {
    if (_capUnder[ii] != NULL)
      delete _capUnder[ii];
    if (_capDiagUnder[ii] != NULL)
      delete _capDiagUnder[ii];
    if (_resOver[ii] != NULL)
      delete _resOver[ii];
    if (_capOver[ii] != NULL)
      delete _capOver[ii];
    if (_capOverUnder[ii] != NULL)
      delete _capOverUnder[ii];
  }
  delete[] _resOver;
  delete[] _capOver;
  delete[] _capDiagUnder;
  delete[] _capUnder;
  delete[] _capOverUnder;
}
void extMetRCTable::allocOverTable(uint met,
                                   Ath__array1D<double>* wTable,
                                   double dbFactor)
{
  _capOver[met] = new extDistWidthRCTable(
      true, met, _layerCnt, met, wTable, _rcPoolPtr, dbFactor, logger_);
  _resOver[met] = new extDistWidthRCTable(
      true, met, _layerCnt, met, wTable, _rcPoolPtr, dbFactor, logger_);
}
void extMetRCTable::allocDiagUnderTable(uint met,
                                        Ath__array1D<double>* wTable,
                                        int diagWidthCnt,
                                        int diagDistCnt,
                                        double dbFactor)
{
  _capDiagUnder[met] = new extDistWidthRCTable(false,
                                               met,
                                               _layerCnt,
                                               _layerCnt - met - 1,
                                               wTable,
                                               diagWidthCnt,
                                               diagDistCnt,
                                               _rcPoolPtr,
                                               dbFactor,
                                               logger_);
}
void extMetRCTable::setDiagUnderTables(uint met,
                                       uint overMet,
                                       Ath__array1D<double>* diagWTable,
                                       Ath__array1D<double>* diagSTable,
                                       double dbFactor)
{
  _capDiagUnder[met]->setDiagUnderTables(
      overMet, diagWTable, diagSTable, dbFactor);
}
void extMetRCTable::allocDiagUnderTable(uint met,
                                        Ath__array1D<double>* wTable,
                                        double dbFactor)
{
  _capDiagUnder[met] = new extDistWidthRCTable(false,
                                               met,
                                               _layerCnt,
                                               _layerCnt - met - 1,
                                               wTable,
                                               _rcPoolPtr,
                                               dbFactor,
                                               logger_);
}
void extMetRCTable::allocUnderTable(uint met,
                                    Ath__array1D<double>* wTable,
                                    double dbFactor)
{
  _capUnder[met] = new extDistWidthRCTable(false,
                                           met,
                                           _layerCnt,
                                           _layerCnt - met - 1,
                                           wTable,
                                           _rcPoolPtr,
                                           dbFactor,
                                           logger_);
}
void extMetRCTable::allocOverUnderTable(uint met,
                                        Ath__array1D<double>* wTable,
                                        double dbFactor)
{
  if (met < 2)
    return;

  // uint n= getMetIndexOverUnder(met, met-1, _layerCnt-1, _layerCnt, logger_);
  uint n = getMaxMetIndexOverUnder(met, _layerCnt, logger_);
  _capOverUnder[met] = new extDistWidthRCTable(
      false, met, _layerCnt, n + 1, wTable, _rcPoolPtr, dbFactor, logger_);
}
extRCTable::extRCTable(bool over, uint layerCnt)
{
  _maxCnt1 = layerCnt + 1;
  _inTable = new Ath__array1D<extDistRC*>**[_maxCnt1];
  _table = new Ath__array1D<extDistRC*>**[_maxCnt1];

  if (over)
    makeCapTableOver();
  else
    makeCapTableUnder();
}
extDistRC* extRCTable::getCapOver(uint met, uint metUnder)
{
  return _inTable[met][metUnder]->get(0);
}
double extRCModel::getTotCapOverSub(uint met)
{
  extDistRC* rc = _capOver->getCapOver(met, 0);
  return rc->getFringe();
}
/*
double extRCModel::getFringeOver(uint met, uint mUnder, uint w, uint s)
{

        if ((_tmpDataRate<=0)||(_modelTable!=NULL))
                return 0.0;
*/

extDistRC* extDistRCTable::getRC_index(int n)
{
  int cnt = _measureTable->getCnt();
  if (n >= cnt)
    return NULL;
  return _measureTable->get(n);
}

extDistRC* extDistRCTable::getLastRC()
{
  int cnt = _measureTable->getCnt();
  return _measureTable->get(cnt - 1);
}

extDistRC* extDistRCTable::getRC(uint s, bool compute)
{
  if (compute)
    return getComputeRC(s);
  else
    return NULL;
  // return interpolate _measureTable->findNextBiggestIndex((double) s);
}
void extDistRCTable::getFringeTable(Ath__array1D<int>* sTable,
                                    Ath__array1D<double>* rcTable,
                                    bool compute)
{
  Ath__array1D<extDistRC*>* table = _computeTable;
  if (!compute)
    table = _measureTable;

  for (uint ii = 0; ii < table->getCnt(); ii++) {
    extDistRC* rc = table->get(ii);
    sTable->add(rc->_sep);
    rcTable->add(rc->_fringe);
  }
}
void extDistWidthRCTable::getFringeTable(uint mou,
                                         uint w,
                                         Ath__array1D<int>* sTable,
                                         Ath__array1D<double>* rcTable,
                                         bool map)
{
  uint wIndex = 0;
  if (map) {
    wIndex = getWidthIndex(w);
  } else
    wIndex = _widthTable->findNextBiggestIndex(w);

  _rcDistTable[mou][wIndex]->getFringeTable(sTable, rcTable, true);
}
/*
extDistRC* extDistWidthRCTable::getRC(uint mou, uint w, uint s)
{
        int wIndex= _widthTable->findNextBiggestIndex(w);
        if ((wIndex<0) || (wIndex>=_widthTable->getCnt()))
                return NULL;

        if (mou>=_metCnt || wIndex>=_widthTable->getCnt() ||
_rcDistTable[mou][wIndex] == NULL) return NULL; return
_rcDistTable[mou][wIndex]->getRC( s, true);
}
*/
extDistRC* extDistWidthRCTable::getFringeRC(uint mou, uint w, int index_dist)
{
  int wIndex = getWidthIndex(w);
  if ((wIndex < 0) || (wIndex >= (int) _widthTable->getCnt()))
    return NULL;

  if (mou >= _metCnt || wIndex >= (int) _widthTable->getCnt()
      || _rcDistTable[mou][wIndex] == NULL)
    return NULL;

  extDistRC* rc;
  if (index_dist < 0)
    rc = _rcDistTable[mou][wIndex]->getLastRC();
  else
    rc = _rcDistTable[mou][wIndex]->getRC_index(index_dist);
  /*
  if (rc!=NULL) {
          rc->writeRC();
  }
  */
  return rc;
}
extDistRC* extDistWidthRCTable::getLastWidthFringeRC(uint mou)
{
  if (mou >= _metCnt)
    return NULL;

  int wIndex = _widthTable->getCnt() - 1;

  if (wIndex >= (int) _widthTable->getCnt()
      || _rcDistTable[mou][wIndex] == NULL)
    return NULL;

  return _rcDistTable[mou][wIndex]->getLastRC();
}
extDistRC* extDistWidthRCTable::getRC(uint mou, uint w, uint s)
{
  int wIndex = getWidthIndex(w);
  if (wIndex < 0)
    return NULL;

  return _rcDistTable[mou][wIndex]->getRC(s, true);
}
extDistRC* extDistWidthRCTable::getRC(uint mou,
                                      uint w,
                                      uint dw,
                                      uint ds,
                                      uint s)
{
  int wIndex = getWidthIndex(w);
  if (wIndex < 0)
    return NULL;
  int dwIndex = getDiagWidthIndex(mou, dw);
  if (dwIndex < 0)
    return NULL;
  int dsIndex = getDiagDistIndex(mou, ds);
  if (dsIndex < 0)
    return NULL;
  return _rcDiagDistTable[mou][wIndex][dwIndex][dsIndex]->getRC(s, true);
}
extDistRC* extDistWidthRCTable::getRC_99(uint mou, uint w, uint dw, uint ds)
{
  int wIndex = getWidthIndex(w);
  if (wIndex < 0)
    return NULL;

  int dwIndex = getDiagWidthIndex(mou, dw);
  if (dwIndex < 0)
    return NULL;

  int dsIndex = getDiagDistIndex(mou, ds);
  if (dsIndex < 0)
    return NULL;

  uint s2 = _diagDistTable[mou]->get(dsIndex);
  extDistRC* rc2 = _rcDiagDistTable[mou][wIndex][dwIndex][dsIndex]->getRC_99();
  if (dsIndex == 0)
    return rc2;

  if ((int) ds == _diagDistTable[mou]->get(dsIndex))
    return rc2;

  _rc31->_sep = ds;

  uint lastDist = _lastDiagDist->geti(mou);
  if (ds > lastDist) {  // extrapolate
    _rc31->_fringe = (rc2->_fringe / ds) * lastDist;

    return _rc31;
  }
  // interpolate;
  uint s1 = _diagDistTable[mou]->get(dsIndex - 1);

  if (ds <= (s1 - s2) / 4)  // too close!
    return rc2;

  extDistRC* rc1
      = _rcDiagDistTable[mou][wIndex][dwIndex][dsIndex - 1]->getRC_99();

  _rc31->_fringe = lineSegment(ds, s1, s2, rc1->_fringe, rc2->_fringe);

  return _rc31;
}
double extRCModel::getFringeOver(uint met, uint mUnder, uint w, uint s)
{
  extDistRC* rc = _modelTable[_tmpDataRate]->_capOver[met]->getRC(mUnder, w, s);

  return rc->getFringe();
}
double extRCModel::getCouplingOver(uint met, uint mUnder, uint w, uint s)
{
  extDistRC* rc = _modelTable[_tmpDataRate]->_capOver[met]->getRC(mUnder, w, s);

  return rc->getCoupling();
}
extDistRC* extRCModel::getOverRC(extMeasure* m)
{
  if (_modelTable[_tmpDataRate] == NULL
      || _modelTable[_tmpDataRate]->_capOver[m->_met] == NULL)
    return NULL;
  extDistRC* rc = _modelTable[_tmpDataRate]->_capOver[m->_met]->getRC(
      m->_underMet, m->_width, m->_dist);

  return rc;
}
extDistRC* extRCModel::getUnderRC(extMeasure* m)
{
  uint n = getUnderIndex(m);
  if (_modelTable[_tmpDataRate] == NULL
      || _modelTable[_tmpDataRate]->_capUnder[m->_met] == NULL)
    return NULL;
  extDistRC* rc = _modelTable[_tmpDataRate]->_capUnder[m->_met]->getRC(
      n, m->_width, m->_dist);

  return rc;
}
extDistRC* extRCModel::getOverUnderRC(extMeasure* m)
{
  uint maxOverUnderIndex
      = _modelTable[_tmpDataRate]->_capOverUnder[m->_met]->_metCnt;
  uint n = getOverUnderIndex(m, maxOverUnderIndex);
  extDistRC* rc = _modelTable[_tmpDataRate]->_capOverUnder[m->_met]->getRC(
      n, m->_width, m->_dist);

  return rc;
}
extDistRC* extRCModel::getOverFringeRC(uint met, uint underMet, uint width)
{
  if (met >= _layerCnt)
    return NULL;

  extDistRC* rc
      = _modelTable[_tmpDataRate]->_capOver[met]->getFringeRC(underMet, width);

  return rc;
}
extDistRC* extMetRCTable::getOverFringeRC(extMeasure* m, int index_dist)
{
  if (m->_met >= (int) _layerCnt)
    return NULL;

  extDistRC* rc
      = _capOver[m->_met]->getFringeRC(m->_underMet, m->_width, index_dist);

  return rc;
}
extDistRC* extMetRCTable::getOverFringeRC_last(int met, int width)
{
  if (met >= (int) _layerCnt)
    return NULL;

  extDistRC* rc = _capOver[met]->getFringeRC(0, width, -1);

  return rc;
}
extDistRC* extRCModel::getOverFringeRC(extMeasure* m)
{
  if (m->_met >= (int) _layerCnt)
    return NULL;

  extDistRC* rc = _modelTable[_tmpDataRate]->_capOver[m->_met]->getFringeRC(
      m->_underMet, m->_width);

  return rc;
}
extDistRC* extRCModel::getUnderFringeRC(extMeasure* m)
{
  uint n = getUnderIndex(m);
  if (_modelTable[_tmpDataRate] == NULL
      || _modelTable[_tmpDataRate]->_capUnder[m->_met] == NULL)
    return NULL;
  extDistRC* rc = _modelTable[_tmpDataRate]->_capUnder[m->_met]->getFringeRC(
      n, m->_width);

  return rc;
}
extDistRC* extRCModel::getOverUnderFringeRC(extMeasure* m)
{
  uint maxCnt = _modelTable[_tmpDataRate]->_capOverUnder[m->_met]->_metCnt;
  uint n = getOverUnderIndex(m, maxCnt);
  if (_modelTable[_tmpDataRate] == NULL
      || _modelTable[_tmpDataRate]->_capOverUnder[m->_met] == NULL)
    return NULL;
  extDistRC* rc
      = _modelTable[_tmpDataRate]->_capOverUnder[m->_met]->getFringeRC(
          n, m->_width);

  return rc;
}
extDistRC* extMeasure::getOverUnderFringeRC(extMetRCTable* rcModel)
{
  //	uint n= getOverUnderIndex();
  uint maxCnt = _currentModel->getMaxCnt(_met);
  uint n = getMetIndexOverUnder(
      _met, _underMet, _overMet, _layerCnt, maxCnt, logger_);

  if (rcModel == NULL || rcModel->_capOverUnder[_met] == NULL)
    return NULL;

  extDistRC* rc = rcModel->_capOverUnder[_met]->getFringeRC(n, _width);

  return rc;
}
extDistRC* extMeasure::getOverUnderRC(extMetRCTable* rcModel)
{
  //	uint n= getOverUnderIndex();
  uint maxCnt = _currentModel->getMaxCnt(_met);
  uint n = getMetIndexOverUnder(
      _met, _underMet, _overMet, _layerCnt, maxCnt, logger_);

  extDistRC* rc = NULL;
  if (_dist < 0)
    rc = rcModel->_capOverUnder[_met]->getFringeRC(n, _width);
  else
    rc = rcModel->_capOverUnder[_met]->getRC(n, _width, _dist);

  return rc;
}
extDistRC* extMeasure::getOverRC(extMetRCTable* rcModel)
{
  if (_met >= (int) _layerCnt)
    return NULL;

  extDistRC* rc = NULL;
  if (_dist < 0)
    rc = rcModel->_capOver[_met]->getFringeRC(_underMet, _width);
  else
    rc = rcModel->_capOver[_met]->getRC(_underMet, _width, _dist);

  return rc;
}
uint extMeasure::getUnderIndex(uint overMet)
{
  return overMet - _met - 1;
}
uint extMeasure::getUnderIndex()
{
  return _overMet - _met - 1;
}
extDistRC* extMeasure::getUnderLastWidthDistRC(extMetRCTable* rcModel,
                                               uint overMet)
{
  if (rcModel->_capUnder[_met] == NULL)
    return NULL;

  uint n = getUnderIndex(overMet);

  return rcModel->_capUnder[_met]->getLastWidthFringeRC(n);
}

extDistRC* extMeasure::getUnderRC(extMetRCTable* rcModel)
{
  if (rcModel->_capUnder[_met] == NULL)
    return NULL;

  uint n = getUnderIndex();

  extDistRC* rc = NULL;
  if (_dist < 0)
    rc = rcModel->_capUnder[_met]->getFringeRC(n, _width);
  else
    rc = rcModel->_capUnder[_met]->getRC(n, _width, _dist);

  return rc;
}
extDistRC* extMeasure::getVerticalUnderRC(extMetRCTable* rcModel,
                                          uint diagDist,
                                          uint tgtWidth,
                                          uint overMet)
{
  if (rcModel->_capDiagUnder[_met] == NULL) {
    return getUnderRC(rcModel);  // DELETE
    return NULL;
  }

  uint n = getUnderIndex(overMet);

  //	uint couplingDist= 99000;
  extDistRC* rc
      = rcModel->_capDiagUnder[_met]->getRC_99(n, _width, tgtWidth, diagDist);

  return rc;
}
double extMeasure::getDiagUnderCC(extMetRCTable* rcModel,
                                  uint dist,
                                  uint overMet)
{
  if (rcModel->_capDiagUnder[_met] == NULL)
    return 0.0;

  uint n = getUnderIndex(overMet);

  extDistRC* rc = rcModel->_capDiagUnder[_met]->getRC(n, _width, dist);

  if (rc != NULL) {
    if (IsDebugNet()) {
      int dbUnit = _extMain->_block->getDbUnitsPerMicron();
      rc->printDebugRC_diag(_met, overMet, 0, _width, dist, dbUnit, logger_);
    }
    return rc->_fringe;  // TODO 620
  } else
    return 0.0;
}
double extMeasure::getDiagUnderCC(extMetRCTable* rcModel,
                                  uint diagWidth,
                                  uint diagDist,
                                  uint overMet)
{
  if (rcModel->_capDiagUnder[_met] == NULL)
    return 0.0;

  uint n = getUnderIndex(overMet);

  extDistRC* rc = rcModel->_capDiagUnder[_met]->getRC(
      n, _width, diagWidth, diagDist, _dist);

  if (rc != NULL)
    return rc->_fringe;
  else
    return 0.0;
}
extDistRC* extMeasure::getDiagUnderCC2(extMetRCTable* rcModel,
                                       uint diagWidth,
                                       uint diagDist,
                                       uint overMet)
{
  if (rcModel->_capDiagUnder[_met] == NULL)
    return NULL;

  uint n = getUnderIndex(overMet);

  extDistRC* rc = rcModel->_capDiagUnder[_met]->getRC(
      n, _width, diagWidth, diagDist, _dist);

  if (rc == NULL)
    return NULL;
  return rc;
}
double extRCModel::getRes(uint met)
{
  if (met > 13)
    return 0;

  extDistRC* rc = _capOver->getCapOver(met, 0);
  if (rc == NULL)
    return 0;

  return rc->getRes();
}
uint extRCTable::addCapOver(uint met, uint metUnder, extDistRC* rc)
{
  return _inTable[met][metUnder]->add(rc);
}
extRCModel::extRCModel(uint layerCnt, const char* name, Logger* logger)
{
  logger_ = logger;
  _layerCnt = layerCnt;
  strcpy(_name, name);
  _resOver = new extRCTable(true, layerCnt);
  _capOver = new extRCTable(true, layerCnt);
  _capUnder = new extRCTable(false, layerCnt);
  _capDiagUnder = new extRCTable(false, layerCnt);
  _rcPoolPtr = new AthPool<extDistRC>(false, 1024);
  _process = NULL;
  _maxMinFlag = false;

  _wireDirName = new char[2048];
  _topDir = new char[1024];
  _patternName = new char[1024];
  _parser = new Ath__parser();
  _solverFileName = new char[1024];
  _wireFileName = new char[1024];
  _capLogFP = NULL;
  _logFP = NULL;

  _readCapLog = false;
  _commentFlag = false;

  _modelCnt = 0;
  _dataRateTable = NULL;
  _modelTable = NULL;
  _tmpDataRate = 0;
  _extMain = NULL;
  _ruleFileName = NULL;
  _diagModel = 0;
  _verticalDiag = false;
  _keepFile = false;
  _metLevel = 0;
}
extRCModel::extRCModel(const char* name, Logger* logger)
{
  logger = logger;
  _layerCnt = 0;
  strcpy(_name, name);
  _resOver = NULL;
  _capOver = NULL;
  _capUnder = NULL;
  _capDiagUnder = NULL;
  _rcPoolPtr = new AthPool<extDistRC>(false, 1024);
  _process = NULL;
  _maxMinFlag = false;

  _wireDirName = new char[2048];
  _topDir = new char[1024];
  _patternName = new char[1024];
  _parser = new Ath__parser();
  _solverFileName = new char[1024];
  _wireFileName = new char[1024];
  _capLogFP = NULL;
  _logFP = NULL;

  _readCapLog = false;
  _commentFlag = false;

  _modelCnt = 0;
  _modelTable = NULL;
  _tmpDataRate = 0;

  _noVariationIndex = -1;

  _extMain = NULL;
  _ruleFileName = NULL;
  _diagModel = 0;
  _verticalDiag = false;
  _keepFile = false;
  _metLevel = 0;
}

extRCModel::~extRCModel()
{
  delete _resOver;
  delete _capOver;
  delete _capUnder;
  delete _capDiagUnder;
  delete _rcPoolPtr;

  delete[] _wireDirName;
  delete[] _topDir;
  delete[] _patternName;
  delete _parser;
  delete[] _solverFileName;
  delete[] _wireFileName;

  if (_modelCnt > 0) {
    for (uint ii = 0; ii < _modelCnt; ii++)
      delete _modelTable[ii];

    delete[] _modelTable;
    delete _dataRateTable;
  }
}
void extRCModel::setExtMain(extMain* x)
{
  _extMain = x;
}
extProcess* extRCModel::getProcess()
{
  return _process;
}
void extRCModel::setProcess(extProcess* p)
{
  _process = p;
}
void extRCModel::createModelTable(uint n, uint layerCnt)
{
  _layerCnt = layerCnt;
  _modelCnt = n;

  _dataRateTable = new Ath__array1D<double>(_modelCnt);
  _modelTable = new extMetRCTable*[_modelCnt];
  for (uint jj = 0; jj < _modelCnt; jj++)
    _modelTable[jj] = new extMetRCTable(_layerCnt, _rcPoolPtr, logger_);
}
void extRCModel::setDataRateTable(uint met)
{
  if (_process == NULL)
    return;
  /*
          FILE *fp= openFile("./", "ou", NULL, "w");
          for (uint m= 1; m<_layerCnt; m++) {
                  for (uint mUnder= 1; mUnder<m; mUnder++) {
                          for (uint mOver= m+1; mOver<_layerCnt; mOver++) {
                                  fprintf(fp, "met= %d   over %d   under %d
     index= %d\n", m, mUnder, mOver, getMetIndexOverUnder(m, mUnder, mOver,
     _layerCnt));

                          }
                  }
                  fprintf(fp, "\n");
          }
          fclose(fp);
  */
  _maxMinFlag = _process->getMaxMinFlag();
  bool thickVarFlag = _process->getThickVarFlag();
  extVariation* xvar = _process->getVariation(met);

  if (xvar != NULL) {
    Ath__array1D<double>* dTable = xvar->getDataRateTable();

    createModelTable(dTable->getCnt() + 1, _layerCnt);

    _dataRateTable->add(0.0);
    for (uint ii = 0; ii < dTable->getCnt(); ii++)
      _dataRateTable->add(dTable->get(ii));

  } else if (_maxMinFlag) {
    createModelTable(3, _layerCnt);
    for (uint i = 0; i < 3; i++)
      _dataRateTable->add(i);
  } else if (thickVarFlag) {
    Ath__array1D<double>* dTable = _process->getDataRateTable(1);
    createModelTable(dTable->getCnt(), _layerCnt);
    for (uint ii = 0; ii < dTable->getCnt(); ii++)
      _dataRateTable->add(dTable->get(ii));
  } else {
    createModelTable(1, _layerCnt);
    //		_dataRateTable->set(0, 0.0);
    _dataRateTable->add(0.0);
  }
  _tmpDataRate = 0;
}
uint extRCModel::addLefTotRC(uint met, uint underMet, double fr, double r)
{
  extDistRC* rc = _rcPoolPtr->alloc();
  rc->set(0, 0.0, fr, 0.0, r);

  uint n = _capOver->addCapOver(met, underMet, rc);
  return n;
}

uint extRCModel::addCapOver(uint met,
                            uint underMet,
                            uint d,
                            double cc,
                            double fr,
                            double a,
                            double r)
{
  extDistRC* rc = _rcPoolPtr->alloc();
  rc->set(d, cc, fr, a, r);

  uint n = _capOver->addCapOver(met, underMet, rc);
  return n;
}
/*
uint extRCModel::addCapUnder(uint met, uint overMet, uint d, double cc, double
fr, double a, double r)
{
        extDistRC *rc= _rcPoolPtr->alloc();
        rc->set(d, cc, fr, a, r);

        uint n= _capOver->addCapUnder(met, overMet, rc);
        return n;
}
*/
extMeasure::extMeasure()
{
  _met = -1;
  _underMet = -1;
  _overMet = -1;

  _w_m = 0.0;
  _s_m = 0.0;
  _w_nm = 0;
  _s_nm = 0;
  _r = 0.0;
  _t = 0.0;
  _h = 0.0;
  _w2_m = 0.0;
  _s2_m = 0.0;
  _w2_nm = 0;
  _s2_nm = 0;

  _topWidth = 0.0;
  _botWidth = 0.0;
  _teff = 0.0;
  _heff = 0.0;
  _seff = 0.0;

  _topWidthR = 0.0;
  _botWidthR = 0.0;
  _teffR = 0.0;

  _varFlag = false;
  _3dFlag = false;
  _rcValid = false;
  _benchFlag = false;
  _diag = false;
  _verticalDiag = false;
  _plate = false;
  _metExtFlag = false;

  for (uint ii = 0; ii < 20; ii++) {
    _rc[ii] = new extDistRC();
    _rc[ii]->setLogger(logger_);
  }

  _tmpRC = _rc[0];

  _capTable = NULL;
  _ll[0] = 0;
  _ll[1] = 0;
  _ur[0] = 0;
  _ur[1] = 0;

  _maxCapNodeCnt = 100;
  for (int n = 0; n < (int) _maxCapNodeCnt; n++) {
    for (int k = 0; k < (int) _maxCapNodeCnt; k++) {
      _capMatrix[n][k] = 0.0;
    }
  }
  _extMain = NULL;

  _2dBoxPool = new AthPool<ext2dBox>(false, 1024);

  _lenOUPool = NULL;
  _lenOUtable = NULL;

  _totCCcnt = 0;
  _totSmallCCcnt = 0;
  _totBigCCcnt = 0;
  _totSignalSegCnt = 0;
  _totSegCnt = 0;

  _tmpDstTable = new Ath__array1D<SEQ*>(32);
  _tmpSrcTable = new Ath__array1D<SEQ*>(32);
  _diagTable = new Ath__array1D<SEQ*>(32);
  _tmpTable = new Ath__array1D<SEQ*>(32);
  _ouTable = new Ath__array1D<SEQ*>(32);
  _overTable = new Ath__array1D<SEQ*>(32);
  _underTable = new Ath__array1D<SEQ*>(32);

  _seqPool = new AthPool<SEQ>(false, 1024);

  _dgContextFile = NULL;
  _diagFlow = false;
  _btermThreshold = false;
  _rotatedGs = false;
  _sameNetFlag = false;
}
void extMeasure::allocOUpool()
{
  _lenOUPool = new AthPool<extLenOU>(false, 128);
  _lenOUtable = new Ath__array1D<extLenOU*>(128);
}
extMeasure::~extMeasure()
{
  for (uint ii = 0; ii < 20; ii++)
    delete _rc[ii];

  delete _tmpDstTable;
  delete _tmpSrcTable;
  delete _diagTable;
  delete _tmpTable;
  delete _ouTable;
  delete _overTable;
  delete _underTable;

  delete _seqPool;

  delete _2dBoxPool;
  if (_lenOUPool != NULL) {
    delete _lenOUPool;
    delete _lenOUtable;
  }
}

void extMeasure::setMets(int m, int u, int o)
{
  _met = m;
  _underMet = u;
  _overMet = o;
  _over = false;
  _overUnder = false;
  if ((u > 0) && (o > 0))
    _overUnder = true;
  else if ((u >= 0) && (o < 0))
    _over = true;
}
void extMeasure::setTargetParams(double w,
                                 double s,
                                 double r,
                                 double t,
                                 double h,
                                 double w2,
                                 double s2)
{
  _w_m = w;
  _s_m = s;
  _w_nm = Ath__double2int(1000 * w);
  _s_nm = Ath__double2int(1000 * s);
  _r = r;
  _t = t;
  _h = h;
  if (w2 > 0.0) {
    _w2_m = w2;
    _w2_nm = Ath__double2int(1000 * w2);
  } else {
    _w2_m = _w_m;
    _w2_nm = _w_nm;
  }
  //	if (s2>0.0 || (s2==0.0 && _diag && _benchFlag)) {
  if (s2 > 0.0 || (s2 == 0.0 && _diag)) {
    _s2_m = s2;
    _s2_nm = Ath__double2int(1000 * s2);
  } else {
    _s2_m = _s_m;
    _s2_nm = _s_nm;
  }
}
void extMeasure::setEffParams(double wTop, double wBot, double teff)
{
  _topWidth = wTop;
  _botWidth = wBot;
  _teff = teff;
  //_heff= _h+_t-teff;
  _heff = _h;
  /*
          if (_diag)
                  _seff= _s_m;
          else
  */
  if (!_metExtFlag && _s_m != 99)
    _seff = _w_m + _s_m - wTop;
  else
    _seff = _s_m;
}
extDistRC* extMeasure::addRC(extDistRC* rcUnit, uint len, uint jj)
{
  if (rcUnit == NULL)
    return NULL;
  int dbUnit = _extMain->_block->getDbUnitsPerMicron();
  if (IsDebugNet())
    rcUnit->printDebugRC(
        _met, _overMet, _underMet, _width, _dist, dbUnit, logger_);

  if (_sameNetFlag) {  // TO OPTIMIZE
    _rc[jj]->_fringe += 0.5 * rcUnit->_fringe * len;
  } else {
    _rc[jj]->_fringe += rcUnit->_fringe * len;

    if (_dist > 0)  // dist based
      _rc[jj]->_coupling += rcUnit->_coupling * len;
  }

  _rc[jj]->_res += rcUnit->_res * len;
  if (IsDebugNet()) {
    _rc[jj]->printDebugRC_sum(len, dbUnit, logger_);
  }
  return rcUnit;
}
extDistRC* extMeasure::computeOverUnderRC(uint len)
{
  extDistRC* rcUnit = NULL;

  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    rcUnit = getOverUnderRC(rcModel);

    addRC(rcUnit, len, ii);
  }
  return rcUnit;
}
extDistRC* extMeasure::computeOverRC(uint len)
{
  extDistRC* rcUnit = NULL;

  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    rcUnit = getOverRC(rcModel);

    addRC(rcUnit, len, ii);
  }
  return rcUnit;
}
extDistRC* extMeasure::computeR(uint len, double* valTable)
{
  extDistRC* rcUnit = NULL;

  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    rcUnit = getOverRC(rcModel);
    if (rcUnit != NULL)
      _rc[ii]->_res += rcUnit->_res * len;
  }
  return rcUnit;
}
extDistRC* extMeasure::computeUnderRC(uint len)
{
  extDistRC* rcUnit = NULL;

  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    rcUnit = getUnderRC(rcModel);

    addRC(rcUnit, len, ii);
  }
  return rcUnit;
}
/*
void extMeasure::addCap()
{
        return;
        if (! _rcValid)
                return;

        if (_underMet>0)
                _capTable->addCapOver(_met, _underMet, _rc);

        else if (_overMet>0)
                _capTable->addCapOver(_met, _overMet, _rc);
}
*/
void extMeasure::printMets(FILE* fp)
{
  if (_overUnder)
    fprintf(fp, "M%d over M%d under M%d ", _met, _underMet, _overMet);
  else if (_over) {
    if (_diag)
      fprintf(fp, "M%d over diag M%d ", _met, _underMet);
    else
      fprintf(fp, "M%d over M%d ", _met, _underMet);
  } else {
    if (_diag)
      fprintf(fp, "M%d under diag M%d ", _met, _overMet);
    else
      fprintf(fp, "M%d under M%d ", _met, _overMet);
  }
}
void extMeasure::printStats(FILE* fp)
{
  fprintf(fp,
          "<==> w= %g[%g %g]  s= %g[%g]  th= %g[%g]  h= %g[%g]  r= %g",
          _w_m,
          _topWidth,
          _botWidth,
          _s_m,
          _seff,
          _t,
          _teff,
          _h,
          _heff,
          _r);
}

FILE* extRCModel::openFile(const char* topDir,
                           const char* name,
                           const char* suffix,
                           const char* permissions)
{
  char filename[2048];

  if (topDir != NULL)
    sprintf(filename, "%s", topDir);

  if (suffix == NULL)
    sprintf(filename, "%s/%s", filename, name);
  else
    sprintf(filename, "%s/%s%s", filename, name, suffix);

  FILE* fp = fopen(filename, permissions);
  if (fp == NULL) {
    logger_->info(RCX,
                  159,
                  "Cannot open file {} with permissions {}",
                  filename,
                  permissions);
    return NULL;
  }
  return fp;
}

uint extRCModel::getCapValues(uint lastNode,
                              double& cc1,
                              double& cc2,
                              double& fr,
                              double& tot,
                              extMeasure* m)
{
  double totCap = 0.0;
  uint wireNum = m->_wireCnt / 2 + 1;  // assume odd numebr
  if (m->_diag && _diagModel == 2) {
    fr = m->_capMatrix[1][0];   // diag
    cc1 = m->_capMatrix[1][1];  // left cc in diag side
    cc2 = m->_capMatrix[1][2];  // right cc
    fprintf(_capLogFP, "\n");
    m->printMets(_capLogFP);
    fprintf(_capLogFP, " diag= %g, cc1=%g, cc2=%g  ", fr, cc1, cc2);
    m->printStats(_capLogFP);
    fprintf(_capLogFP, "\n\nEND\n\n");
    return lastNode;
  }
  if (m->_diag && _diagModel == 1) {
    cc1 = m->_capMatrix[1][0];
    fprintf(_capLogFP, "\n");
    m->printMets(_capLogFP);
    fprintf(_capLogFP, " diag= %g  ", cc1);
    m->printStats(_capLogFP);
    fprintf(_capLogFP, "\n\nEND\n\n");
    return lastNode;
  }

  if (lastNode == 1) {
    totCap = m->_capMatrix[1][0];
    fr = totCap;
    if (m->_plate)
      fr /= 100000;
    fprintf(_capLogFP, "\n");
    m->printMets(_capLogFP);
    tot = cc1 + cc2 + fr;

    fprintf(
        _capLogFP, " cc1= %g  cc2= %g  fr= %g  tot= %g  ", cc1, cc2, fr, tot);

    m->printStats(_capLogFP);
    fprintf(_capLogFP, "\n\nEND\n\n");

    return lastNode;
  } else if (lastNode > 1) {
    totCap = m->_capMatrix[1][0];
    cc1 = m->_capMatrix[1][1];  // left cc
    cc2 = m->_capMatrix[1][2];
  }

  if (lastNode != m->_wireCnt) {
    //		return 0;
    logger_->warn(
        RCX, 209, "Reads only {} nodes from {}", lastNode, _wireDirName);
  }

  fprintf(_capLogFP, "\n");
  m->printMets(_capLogFP);
  fr = totCap - cc1 - cc2;

  fprintf(
      _capLogFP, " cc1= %g  cc2= %g  fr= %g  tot= %g  ", cc1, cc2, fr, totCap);

  m->printStats(_capLogFP);
  fprintf(_capLogFP, "\n\nEND\n\n");

  return lastNode;
}

uint extRCModel::getCapValues3D(uint lastNode,
                                double& cc1,
                                double& cc2,
                                double& fr,
                                double& tot,
                                extMeasure* m)
{
  double totCap = 0.0;
  uint wireNum = m->_wireCnt / 2 + 1;  // assume odd numebr
  if (m->_diag) {
    cc1 = m->_capMatrix[1][0];
    fprintf(_capLogFP, "\n");
    m->printMets(_capLogFP);
    fprintf(_capLogFP, " diag= %g  ", cc1);
    m->printStats(_capLogFP);
    fprintf(_capLogFP, "\n\nEND\n\n");
    return lastNode;
  }

  uint n = 1;
  for (; n <= lastNode; n++) {
    if (n != wireNum)
      continue;
    totCap = m->_capMatrix[1][0];

    cc1 = m->_capMatrix[1][n - 1];
    cc2 = m->_capMatrix[1][n + 1];
    break;
  }
  totCap += m->getCCfringe3D(lastNode, n, 2, 3);

  fr = totCap;

  if (lastNode != m->_wireCnt) {
    //                return 0;
    logger_->info(
        RCX, 209, "Reads only {} nodes from {}", lastNode, _wireDirName);
  }

  fprintf(_capLogFP, "\n");
  m->printMets(_capLogFP);
  tot = cc1 + cc2 + fr;

  fprintf(_capLogFP, " cc1= %g  cc2= %g  fr= %g  tot= %g  ", cc1, cc2, fr, tot);

  m->printStats(_capLogFP);
  fprintf(_capLogFP, "\n\nEND\n\n");

  return lastNode;
}

uint extRCModel::getCapMatrixValues(uint lastNode, extMeasure* m)
{
  uint ccCnt = 0;
  for (uint n = 1; n <= lastNode; n++) {
    double frCap = m->_capMatrix[n][0] * m->_len;
    m->_capMatrix[n][0] = 0.0;

    logger_->info(RCX,
                  210,
                  "FrCap for netId {} (nodeId= {})  {}",
                  m->_idTable[n],
                  n,
                  frCap);

    //		uint w= 0;
    double res = _extMain->getLefResistance(
        m->_met, m->_w_nm, m->_len, m->_rIndex);  // TO_TEST

    dbRSeg* rseg1 = m->getFirstDbRseg(m->_idTable[n]);
    rseg1->setResistance(res);

    uint k = n + 1;
    if (k <= lastNode) {
      double cc1 = m->_capMatrix[n][k] * m->_len;
      m->_capMatrix[n][k] = 0.0;

      logger_->info(RCX,
                    211,
                    "\tccCap for netIds {}({}), {}({}) {}",
                    m->_idTable[n],
                    n,
                    m->_idTable[k],
                    k,
                    cc1);
      ccCnt++;

      dbRSeg* rseg2 = m->getFirstDbRseg(m->_idTable[k]);
      m->_extMain->updateCCCap(rseg1, rseg2, cc1);
    }
    double ccFr = m->getCCfringe(lastNode, n, 2, 3) * m->_len;

    frCap += ccFr;
    rseg1->setCapacitance(frCap);

    logger_->info(RCX,
                  212,
                  "\tfrCap from CC for netId {}({}) {}",
                  m->_idTable[n],
                  n,
                  ccFr);
    logger_->info(
        RCX, 213, "\ttotFrCap for netId {}({}) {}", m->_idTable[n], n, frCap);
  }
  m->printStats(_capLogFP);
  fprintf(_capLogFP, "\n\nEND\n\n");

  for (uint ii = 1; ii <= lastNode; ii++) {
    for (uint jj = ii + 1; jj <= lastNode; jj++) {
      m->_capMatrix[ii][jj] = 0.0;
    }
  }
  return ccCnt;
}
uint extRCModel::getCapMatrixValues3D(uint lastNode, extMeasure* m)
{
  uint wireNum = m->_wireCnt / 2 + 1;  // assume odd numebr
  uint n;
  if (lastNode == 1) {
    n = 1;
    double frCap = m->_capMatrix[1][0];
    m->_capMatrix[1][0] = 0.0;

    logger_->info(RCX,
                  210,
                  "FrCap for netId {} (nodeId= {})  {}",
                  m->_idTable[n],
                  n,
                  frCap);

    //                uint w= 0;
    dbRSeg* rseg1 = m->getFirstDbRseg(m->_idTable[n]);
    rseg1->setCapacitance(frCap);
    logger_->info(RCX,
                  213,
                  "\ttotFrCap for netId {}({}) {}",
                  m->_idTable[n],
                  n,
                  m->_capMatrix[1][n]);
    m->printStats(_capLogFP);
    fprintf(_capLogFP, "\n\nEND\n\n");
    return 0;
  }

  for (n = 1; n <= lastNode; n++) {
    if (n != wireNum)
      continue;
    double frCap = m->_capMatrix[1][0];
    m->_capMatrix[1][0] = 0.0;

    logger_->info(RCX,
                  213,
                  "FrCap for netId {} (nodeId= {})  {}",
                  m->_idTable[n],
                  n,
                  frCap);

    //                uint w= 0;
    //                double res= _extMain->getLefResistance(m->_met,
    //                1000*m->_w, m->_len);

    dbRSeg* rseg1 = m->getFirstDbRseg(m->_idTable[n]);
    //                rseg1->setResistance(res);

    double cc1 = m->_capMatrix[1][n - 1];
    m->_capMatrix[1][n - 1] = 0.0;
    logger_->info(RCX,
                  211,
                  "\tccCap for netIds {}({}), {}({}) {}",
                  m->_idTable[n],
                  n,
                  m->_idTable[n - 1],
                  n - 1,
                  cc1);
    dbRSeg* rseg2 = m->getFirstDbRseg(m->_idTable[n - 1]);
    m->_extMain->updateCCCap(rseg1, rseg2, cc1);
    double cc2 = m->_capMatrix[1][n + 1];
    m->_capMatrix[1][n + 1] = 0.0;
    logger_->info(RCX,
                  211,
                  "\tccCap for netIds {}({}), {}({}) {}",
                  m->_idTable[n],
                  n,
                  m->_idTable[n + 1],
                  n + 1,
                  cc2);
    uint netId3 = m->_idTable[n + 1];
    dbRSeg* rseg3 = m->getFirstDbRseg(netId3);
    m->_extMain->updateCCCap(rseg1, rseg3, cc2);

    double ccFr = m->getCCfringe3D(lastNode, n, 2, 3);
    frCap += ccFr;
    rseg1->setCapacitance(frCap);

    logger_->info(RCX,
                  212,
                  "\tfrCap from CC for netId {}({}) {}",
                  m->_idTable[n],
                  n,
                  ccFr);
    //                logger_->info(RCX, 0, "\ttotFrCap for netId {}({}) {}",
    //                m->_idTable[n], n, frCap);
    logger_->info(RCX,
                  213,
                  "\ttotFrCap for netId {}({}) {}",
                  m->_idTable[n],
                  n,
                  m->_capMatrix[1][n]);
  }
  m->printStats(_capLogFP);
  fprintf(_capLogFP, "\n\nEND\n\n");

  for (uint ii = 1; ii <= lastNode; ii++) {
    m->_idTable[ii] = 0;
    for (uint jj = ii + 1; jj <= lastNode; jj++) {
      m->_capMatrix[ii][jj] = 0.0;
    }
  }
  return 0;
}

uint extRCModel::readCapacitanceBench(bool readCapLog, extMeasure* m)
{
  double units = 1.0e+12;

  FILE* solverFP = NULL;
  if (!readCapLog) {
    solverFP = openSolverFile();
    if (solverFP == NULL) {
      return 0;
    }
    _parser->setInputFP(solverFP);
  }

  Ath__parser wParser;

  bool matrixFlag = false;
  /*
   C_1_2 M1_w1 M1_w2 6.367907e-17
   C_1_3 M1_w1 M1_w3 4.394765e-18
   C_1_0 M1_w1 GROUND_RC2 4.842417e-17
   C_2_3 M1_w2 M1_w3 6.367920e-17
   C_2_0 M1_w2 GROUND_RC2 2.877861e-17
   C_3_0 M1_w3 GROUND_RC2 4.842436e-17
  */

  uint n = m->_wireCnt / 2 + 1;
  uint cnt = 0;
  m->_capMatrix[1][0] = 0.0;
  m->_capMatrix[1][1] = 0.0;
  m->_capMatrix[1][2] = 0.0;
  //	while (_parser->parseNextLine()>0) {
  while (1) {
    if (!_parser->isKeyword(0, "BEGIN") || matrixFlag)
      if (!(_parser->parseNextLine() > 0))
        break;
    if (matrixFlag) {
      if (_parser->isKeyword(0, "END"))
        break;

      if (!_parser->isKeyword(0, "Charge"))
        continue;

      //_parser->printWords(stdout);
      _parser->printWords(_capLogFP);

      double cap = _parser->getDouble(4);
      if (cap < 0.0)
        cap = -cap;
      cap *= units;

      wParser.mkWords(_parser->get(2), "M");
      if (!m->_benchFlag && wParser.getInt(0) != 0) {
        if (wParser.mkWords(_parser->get(2), "w") < 2)
          continue;
        uint n1 = wParser.getInt(1);
        if (n1 == n - 1) {
          m->_capMatrix[1][1] = cap;  // left cc
          cnt++;
        } else if (n1 == n) {
          m->_capMatrix[1][0] = cap;
          m->_idTable[cnt] = n1;
          cnt++;
        } else if (n1 == n + 1) {
          m->_capMatrix[1][2] = cap;  // right cc
          cnt++;
        }
      }
      continue;
    }

    if (_parser->isKeyword(0, "***") && _parser->isKeyword(1, "POTENTIAL")) {
      matrixFlag = true;

      //			if (_parser->isKeyword(4, "Total")) {

      fprintf(_capLogFP, "BEGIN %s\n", _wireDirName);
      fprintf(_capLogFP, "%s\n", _commentLine);
      if (_keepFile && m != NULL) {
        if (m->_benchFlag)
          writeWires2(_capLogFP, m, m->_wireCnt);
        else
          writeRuleWires(_capLogFP, m, m->_wireCnt);
      }
      //			}
      continue;
    } else if (_parser->isKeyword(0, "BEGIN")
               && (strcmp(_parser->get(1), _wireDirName) == 0)) {
      matrixFlag = true;

      fprintf(_capLogFP, "BEGIN %s\n", _wireDirName);

      continue;
    } else if (_parser->isKeyword(0, "BEGIN")
               && (strcmp(_parser->get(1), _wireDirName) != 0)) {
      matrixFlag = false;
      break;
    }
  }
  if (solverFP != NULL)
    fclose(solverFP);

  return cnt;
}
uint extRCModel::readCapacitanceBenchDiag(bool readCapLog, extMeasure* m)
{
  int met;
  if (m->_overMet > 0)
    met = m->_overMet;
  else if (m->_underMet > 0)
    met = m->_underMet;
  uint n = m->_wireCnt / 2 + 1;

  double units = 1.0e+12;

  FILE* solverFP = NULL;
  if (!readCapLog) {
    solverFP = openSolverFile();
    if (solverFP == NULL) {
      return 0;
    }
    _parser->setInputFP(solverFP);
  }

  Ath__parser wParser;

  bool matrixFlag = false;
  uint cnt = 0;
  m->_capMatrix[1][0] = 0.0;
  m->_capMatrix[1][1] = 0.0;
  m->_capMatrix[1][2] = 0.0;
  //        while (_parser->parseNextLine()>0) {
  while (1) {
    if (!_parser->isKeyword(0, "BEGIN") || matrixFlag)
      if (!(_parser->parseNextLine() > 0))
        break;
    if (matrixFlag) {
      if (_parser->isKeyword(0, "END"))
        break;

      if (!_parser->isKeyword(0, "Charge"))
        continue;

      //_parser->printWords(stdout);
      _parser->printWords(_capLogFP);
      double cap = _parser->getDouble(4);
      if (cap < 0.0)
        cap = -cap;
      cap *= units;

      wParser.mkWords(_parser->get(2), "M");
      if (_diagModel == 1) {
        if (wParser.getInt(0) != met)
          continue;
        wParser.mkWords(_parser->get(2), "w");
        uint n1 = wParser.getInt(1);
        if (n1 != n)
          continue;
        m->_capMatrix[1][0] = cap;
        m->_idTable[cnt] = n1;
        cnt++;
      }
      if (_diagModel == 2) {
        if (wParser.getInt(0) == met) {
          wParser.mkWords(_parser->get(2), "w");
          uint n1 = wParser.getInt(1);
          if (n1 != n)
            continue;
          m->_capMatrix[1][0] = cap;  // diag
          m->_idTable[cnt] = n1;
          cnt++;
        } else if (!m->_benchFlag && wParser.getInt(0) != 0) {
          wParser.mkWords(_parser->get(2), "w");
          uint n1 = wParser.getInt(1);
          if (n1 == n - 1) {
            m->_capMatrix[1][1] = cap;  // left cc in diag side
            cnt++;
          }
          if (n1 == n + 1) {
            m->_capMatrix[1][2] = cap;  // right cc
            cnt++;
          }
        }
      }
      continue;
    }

    if (_parser->isKeyword(0, "***") && _parser->isKeyword(1, "POTENTIAL")) {
      matrixFlag = true;

      fprintf(_capLogFP, "BEGIN %s\n", _wireDirName);
      fprintf(_capLogFP, "%s\n", _commentLine);
      if (_keepFile && m != NULL) {
        if (m->_benchFlag)
          writeWires2(_capLogFP, m, m->_wireCnt);
        else
          writeRuleWires(_capLogFP, m, m->_wireCnt);
      }
      continue;
    } else if (_parser->isKeyword(0, "BEGIN")
               && (strcmp(_parser->get(1), _wireDirName) == 0)) {
      matrixFlag = true;

      fprintf(_capLogFP, "BEGIN %s\n", _wireDirName);
      continue;
    } else if (_parser->isKeyword(0, "BEGIN")
               && (strcmp(_parser->get(1), _wireDirName) != 0)) {
      matrixFlag = false;
      break;
    }
  }

  if (solverFP != NULL)
    fclose(solverFP);

  return cnt;
}
// void extRCModel::mkFileNames(uint met, const char* ou, uint ouMet, double w,
// double s, double r)
void extRCModel::mkFileNames(extMeasure* m, char* wiresNameSuffix)
{
  char overUnder[128];

  if ((m->_overMet > 0) && (m->_underMet > 0))
    sprintf(overUnder, "M%doM%duM%d", m->_met, m->_underMet, m->_overMet);

  else if (m->_overMet > 0)
    if (m->_diag)
      sprintf(overUnder, "M%dduM%d", m->_met, m->_overMet);
    else
      sprintf(overUnder, "M%duM%d", m->_met, m->_overMet);

  else if (m->_underMet >= 0)
    sprintf(overUnder, "M%doM%d", m->_met, m->_underMet);

  else
    sprintf(overUnder, "Uknown");

  double w = m->_w_m;
  double s = m->_s_m;
  double r = m->_r;
  double w2 = m->_w2_m;
  double s2 = m->_s2_m;

  /*
          if ((r!=0)&&(s>0))
                  sprintf(_wireDirName, "%s/%s/%s/W%g/S%g__D%g", _topDir,
     _patternName, overUnder, w, s, r); else if (s>0) sprintf(_wireDirName,
     "%s/%s/%s/W%g/S%g", _topDir, _patternName, overUnder, w, s); else
                  sprintf(_wireDirName, "%s/%s/%s/W%g", _topDir, _patternName,
     overUnder, w);
  */
  if (!m->_benchFlag) {
    if (m->_diag) {
      if (_diagModel == 2)
        sprintf(_wireDirName,
                "%s/%s/%s/D%g/W%g/DW%g/DS%g/S%g",
                _topDir,
                _patternName,
                overUnder,
                r,
                w,
                w2,
                s2,
                s);
      if (_diagModel == 1)
        sprintf(_wireDirName,
                "%s/%s/%s/D%g/W%g/S%g",
                _topDir,
                _patternName,
                overUnder,
                r,
                w,
                s);
    } else
      sprintf(_wireDirName,
              "%s/%s/%s/D%g/W%g/S%g",
              _topDir,
              _patternName,
              overUnder,
              r,
              w,
              s);
  } else {
    sprintf(_wireDirName,
            "%s/%s/%s/W%g_W%g/S%g_S%g",
            _topDir,
            _patternName,
            overUnder,
            w,
            w2,
            s,
            s2);
  }

  if (wiresNameSuffix != NULL)
    sprintf(_wireFileName, "%s.%s", "wires", wiresNameSuffix);
  else
    sprintf(_wireFileName, "%s", "wires");

  fprintf(_logFP, "pattern Dir %s\n\n", _wireDirName);
  fflush(_logFP);
}
double get_nm(extMeasure* m, double n)
{
  if (n == 0)
    return 0;
  double a = 1000 * 1000.0 * n / m->_dbunit;
  return a;
}
int get_nm(int n, int units )
{
  if (n == 0)
    return 0;
  int a = (1000 * n) / units;
  return a;
}
void extRCModel::mkNet_prefix(extMeasure* m, const char* wiresNameSuffix)
{
  char overUnder[128];

  if ((m->_overMet > 0) && (m->_underMet > 0))
    sprintf(overUnder, "M%doM%duM%d", m->_met, m->_underMet, m->_overMet);

  else if (m->_overMet > 0)
    if (m->_diag)
      sprintf(overUnder, "M%duuM%d", m->_met, m->_overMet);
    else
      sprintf(overUnder, "M%duM%d", m->_met, m->_overMet);

  else if (m->_underMet >= 0) {
    if (m->_diag)
      sprintf(overUnder, "M%duuM%d", m->_underMet, m->_met);
    else
      sprintf(overUnder, "M%doM%d", m->_met, m->_underMet);
  } else
    sprintf(overUnder, "Unknown");

  double w = m->_w_m;
  double s = m->_s_m;
  double r = m->_r;
  double w2 = m->_w2_m;
  double s2 = m->_s2_m;
  
  int n= get_nm(m, m->_s_m);
  sprintf(_wireDirName,
          "%s_%s_W%gW%g_S%gS%g",
          _patternName,
          overUnder,
          get_nm(m, m->_w_m),
          get_nm(m, m->_w2_m),
          get_nm(m, m->_s_m),
          get_nm(m, m->_s2_m));
  sprintf(_wireDirName,
          "%s_%s_W%gW%g_S%05dS%05d",
          _patternName,
          overUnder,
          get_nm(m, m->_w_m),
          get_nm(m, m->_w2_m),
          get_nm(m->_s_nm, m->_dbunit),
          get_nm(m->_s2_nm, m->_dbunit));

  if (wiresNameSuffix != NULL)
    sprintf(_wireFileName, "%s.%s", "wires", wiresNameSuffix);
  else
    sprintf(_wireFileName, "%s", "wires");

  fprintf(_logFP, "pattern Dir %s\n\n", _wireDirName);
  fflush(_logFP);
}

FILE* extRCModel::mkPatternFile()
{
  _parser->mkDirTree(_wireDirName, "/");

  FILE* fp = openFile(_wireDirName, _wireFileName, NULL, "w");
  if (fp == NULL)
    return NULL;

  fprintf(fp, "$pattern %s\n\n", _wireDirName);

  return fp;
}
FILE* extRCModel::openSolverFile()
{
  FILE* fp = openFile(_wireDirName, _wireFileName, ".out", "r");
  if (fp != NULL)
    _parser->setInputFP(fp);

  return fp;
}
bool extRCModel::openCapLogFile()
{
  _readCapLog = false;
  if (_readSolver && !_runSolver)
    _readCapLog = true;

  const char* capLog = "caps.log";

  char buff[1024];
  sprintf(buff, "%s/%s", _topDir, _patternName);
  _parser->mkDirTree(buff, "/");

  FILE* fp = openFile(buff, capLog, NULL, "r");

  if (fp == NULL) {  // no previous run
    _capLogFP = openFile(buff, capLog, NULL, "w");
    _parser->setInputFP(_capLogFP);
    return false;
  }
  fclose(fp);

  char cmd[4000];

  FILE* fp1 = NULL;
  if (_readCapLog) {
    fp1 = openFile(buff, capLog, NULL, "r");
    _capLogFP = openFile(buff, capLog, "out", "a");
  } else if (_metLevel > 0) {
    _capLogFP = openFile(buff, capLog, NULL, "a");
  } else {
    sprintf(cmd,
            "mv %s/%s/%s %s/%s/%s.in",
            _topDir,
            _patternName,
            capLog,
            _topDir,
            _patternName,
            capLog);
    system(cmd);

    _capLogFP = openFile(buff, capLog, NULL, "w");

    fp1 = openFile(buff, capLog, ".in", "r");
  }
  if (fp1 == NULL)
    return false;

  _parser->setInputFP(fp1);

  _readCapLog = true;
  return true;
}
void extRCModel::closeCapLogFile()
{
  fclose(_capLogFP);
}
void extRCModel::writeRuleWires(FILE* fp, extMeasure* measure, uint wireCnt)
{
  extMasterConductor* m = _process->getMasterConductor(measure->_met);
  double minWidth = _process->getConductor(measure->_met)->_min_width;
  double minSpace = _process->getConductor(measure->_met)->_min_spacing;
  double top_ext = _process->getConductor(measure->_met)->_top_ext;
  double pitch;
  if (measure->_diag && _diagModel == 1) {
    //		pitch= measure->_topWidth + 2*minSpace;
    if (measure->_metExtFlag)
      pitch = measure->_topWidth - 2 * top_ext + minSpace;
    else
      pitch = measure->_topWidth + minSpace;
  } else {
    if (measure->_metExtFlag)
      pitch = measure->_topWidth - 2 * top_ext + measure->_seff;
    else
      pitch = measure->_topWidth + measure->_seff;
  }
  double min_pitch = minWidth + minSpace;

  uint n = wireCnt / 2;  // ASSUME odd number of wires, 2 will also work
  double orig = 0.0;
  double x;
  uint ii;

  if (!measure->_plate) {
    // assume origin = (0,0)
    x = -min_pitch * (n - 1) - pitch - 0.5 * measure->_topWidth - top_ext
        + orig;
    for (ii = 0; ii < n - 1; ii++) {
      m->writeRaphaelConformalPoly(fp, minWidth, x, _process);
      m->writeRaphaelPoly(fp, ii + 1, minWidth, x, 0.0, _process);
      x += min_pitch;
    }
    x += 0.5 * measure->_topWidth;
    m->writeRaphaelConformalPoly(fp, 0.0, x, _process);
    m->writeRaphaelPoly(fp, n, x, 0.0);

    m->writeRaphaelConformalPoly(fp, 0.0, orig, _process);
    m->writeRaphaelPoly(fp, n + 1, orig, 1.0);
    x = orig + pitch;
    m->writeRaphaelConformalPoly(fp, 0.0, x, _process);
    m->writeRaphaelPoly(fp, n + 2, x, 0.0);

    x += 0.5 * measure->_topWidth - top_ext + minSpace;
    for (uint jj = n + 2; jj < wireCnt; jj++) {
      m->writeRaphaelConformalPoly(fp, minWidth, x, _process);
      m->writeRaphaelPoly(fp, jj + 1, minWidth, x, 0.0, _process);
      x += min_pitch;
    }
  } else {
    m->writeRaphaelConformalPoly(fp, 100, -50, _process);
    m->writeRaphaelPoly(fp, n + 1, 100, -50, 1.0, _process);
  }

  if (!measure->_diag)
    return;
  int met;
  if (measure->_overMet > 0)
    met = measure->_overMet;
  else if (measure->_underMet > 0)
    met = measure->_underMet;
  else
    return;

  m = _process->getMasterConductor(met);
  minWidth = _process->getConductor(met)->_min_width;
  minSpace = _process->getConductor(met)->_min_spacing;
  min_pitch = minWidth + minSpace;
  pitch = measure->_w2_m + measure->_s2_m;

  if (_diagModel == 2) {
    if (measure->_seff != 99) {
      x = orig - measure->_s2_m - min_pitch * n - 0.5 * measure->_w2_m;
      for (ii = 0; ii < n; ii++) {
        m->writeRaphaelConformalPoly(fp, minWidth, x, _process);
        m->writeRaphaelPoly(fp, ii + 1, minWidth, x, 0.0, _process);

        x += min_pitch;
      }
      m->writeRaphaelConformalPoly(fp, measure->_w2_m, x, _process);
      m->writeRaphaelPoly(fp, ii + 1, measure->_w2_m, x, 0.0, _process);
    } else {
      x = orig - measure->_s2_m - 0.5 * measure->_w2_m;
      m->writeRaphaelConformalPoly(fp, measure->_w2_m, x, _process);
      m->writeRaphaelPoly(fp, n + 1, measure->_w2_m, x, 0.0, _process);
    }
  }
  if (_diagModel == 1) {
    x = orig - measure->_seff - min_pitch * n - 0.5 * minWidth;
    for (ii = 0; ii < n + 1; ii++) {
      m->writeRaphaelConformalPoly(fp, minWidth, x, _process);
      m->writeRaphaelPoly(fp, ii + 1, minWidth, x, 0.0, _process);
      x += min_pitch;
    }
  }
}
void extRCModel::writeWires2(FILE* fp, extMeasure* measure, uint wireCnt)
{
  extMasterConductor* m = _process->getMasterConductor(measure->_met);
  double pitch = measure->_topWidth + measure->_seff;
  double min_pitch = 0.001 * (measure->_minWidth + measure->_minSpace);

  uint n = wireCnt / 2;  // ASSUME odd number of wires, 2 will also work
  double orig = 0.0;

  // assume origin = (0,0)
  double x = -min_pitch * (n - 1) - pitch - 0.5 * measure->_topWidth + orig;
  for (uint ii = 0; ii < n - 1; ii++) {
    m->writeRaphaelPoly(fp, ii + 1, 0.001 * measure->_minWidth, x, 0.0);
    x += min_pitch;
  }
  x += 0.5 * measure->_topWidth;
  m->writeRaphaelPoly(fp, n, x, 0.0);

  m->writeRaphaelPoly(fp, n + 1, orig, 1.0);
  x = 0.5 * measure->_w_m + measure->_s2_m;
  m->writeRaphaelPoly(fp, n + 2, measure->_w2_m, x, 0.0);

  //	x= orig+pitch;
  x = orig + 0.5 * measure->_topWidth + measure->_w2_m + measure->_s2_m
      + 0.001 * measure->_minSpace;
  for (uint jj = n + 2; jj < wireCnt; jj++) {
    m->writeRaphaelPoly(fp, jj + 1, 0.001 * measure->_minWidth, x, 0.0);
    x += min_pitch;
  }
}
void extRCModel::writeRuleWires_3D(FILE* fp, extMeasure* measure, uint wireCnt)
{
  extMasterConductor* m = _process->getMasterConductor(measure->_met);
  double pitch = measure->_topWidth + measure->_seff;
  double minWidth = _process->getConductor(measure->_met)->_min_width;
  double minSpace = _process->getConductor(measure->_met)->_min_spacing;
  double min_pitch = minWidth + minSpace;

  uint n = wireCnt / 2;  // ASSUME odd number of wires, 2 will also work
  double orig = 0.0;

  // assume origin = (0,0)
  double x = -min_pitch * (n - 1) - pitch - 0.5 * measure->_topWidth + orig;
  for (uint ii = 0; ii < n - 1; ii++) {
    m->writeRaphaelPoly3D(fp, ii + 1, minWidth, measure->_len * 0.001, 0.0);
    x += min_pitch;
  }
  x += 0.5 * measure->_topWidth;
  m->writeRaphaelPoly(fp, n, x, measure->_len * 0.001, 0.0);

  m->writeRaphaelPoly3D(fp, n + 1, orig, measure->_len * 0.001, 1.0);
  x = orig + pitch;
  m->writeRaphaelPoly3D(fp, n + 2, measure->_len * 0.001, x, 0.0);

  x += 0.5 * measure->_topWidth + minSpace;
  for (uint jj = n + 2; jj < wireCnt; jj++) {
    m->writeRaphaelPoly3D(fp, jj + 1, minWidth, measure->_len * 0.001, x, 0.0);
    x += min_pitch;
  }
}

void extRCModel::writeWires2_3D(FILE* fp, extMeasure* measure, uint wireCnt)
{
  extMasterConductor* m = _process->getMasterConductor(measure->_met);
  double pitch = measure->_topWidth + measure->_seff;
  double min_pitch = 0.001 * (measure->_minWidth + measure->_minSpace);

  uint n = wireCnt / 2;  // ASSUME odd number of wires, 2 will also work
  double orig = 0.0;

  // assume origin = (0,0)
  double x = -min_pitch * (n - 1) - pitch - 0.5 * measure->_topWidth + orig;
  for (uint ii = 0; ii < n - 1; ii++) {
    m->writeRaphaelPoly3D(
        fp, ii + 1, 0.001 * measure->_minWidth, measure->_len * 0.001, 0.0);
    x += min_pitch;
  }
  x += 0.5 * measure->_topWidth;
  m->writeRaphaelPoly(fp, n, x, measure->_len * 0.001, 0.0);

  m->writeRaphaelPoly3D(fp, n + 1, orig, measure->_len * 0.001, 1.0);
  x = 0.5 * measure->_w_m + measure->_s2_m;
  m->writeRaphaelPoly3D(
      fp, n + 2, measure->_w2_m, measure->_len * 0.001, x, 0.0);

  x = orig + 0.5 * measure->_topWidth + measure->_w2_m + measure->_s2_m
      + 0.001 * measure->_minSpace;
  for (uint jj = n + 2; jj < wireCnt; jj++) {
    m->writeRaphaelPoly3D(
        fp, jj + 1, 0.001 * measure->_minWidth, measure->_len * 0.001, x, 0.0);
    x += min_pitch;
  }
}
int extRCModel::writeBenchWires(FILE* fp, extMeasure* measure)
{
  uint grid_gap_cnt = 20;

  int bboxLL[2];
  bboxLL[measure->_dir] = measure->_ur[measure->_dir];
  bboxLL[!measure->_dir] = measure->_ll[!measure->_dir];

  extMasterConductor* m = _process->getMasterConductor(measure->_met);

  int n
      = measure->_wireCnt / 2;  // ASSUME odd number of wires, 2 will also work

  double pitchUp_print;
  /*
          if (measure->_diag)
                  pitchUp_print= measure->_topWidth +
     0.001*2*measure->_minSpace; else
  */
  pitchUp_print = measure->_topWidth + measure->_seff;
  //	double pitchDown_print= measure->_topWidth + measure->_seff;
  double pitch_print = 0.001 * (measure->_minWidth + measure->_minSpace);

  uint w_layout = measure->_minWidth;
  uint s_layout = measure->_minSpace;

  double x = -(measure->_topWidth * 0.5 + pitchUp_print + pitch_print);

  measure->clean2dBoxTable(measure->_met, false);

  double x_tmp[50];
  uint netIdTable[50];
  uint idCnt = 1;
  int ii;
  for (ii = 0; ii < n - 1; ii++) {
    netIdTable[idCnt]
        = measure->createNetSingleWire(_wireDirName, idCnt, w_layout, s_layout);
    idCnt++;
    x_tmp[ii] = x;
    x -= pitch_print;
  }

  double X[50];
  ii--;
  int cnt = 0;
  for (; ii >= 0; ii--)
    X[cnt++] = x_tmp[ii];

  uint WW = measure->_w_nm;
  uint SS1;
  //	if (measure->_diag)
  //		SS1= 2*measure->_minSpace;
  //	else
  SS1 = measure->_s_nm;
  uint WW2 = measure->_w2_nm;
  uint SS2 = measure->_s2_nm;

  X[cnt++] = -pitchUp_print;
  int mid = cnt;
  netIdTable[idCnt]
      = measure->createNetSingleWire(_wireDirName, idCnt, WW, s_layout);
  idCnt++;

  X[cnt++] = 0.0;
  netIdTable[idCnt]
      = measure->createNetSingleWire(_wireDirName, idCnt, WW, SS1);
  idCnt++;
  uint base = measure->_ll[measure->_dir] + WW / 2;

  X[cnt++] = (SS2 + WW * 0.5) * 0.001;
  netIdTable[idCnt]
      = measure->createNetSingleWire(_wireDirName, idCnt, WW2, SS2);
  idCnt++;

  //	x= measure->_topWidth*0.5+pitchUp_print+0.001*measure->_minSpace;
  x = measure->_topWidth * 0.5 + 0.001 * (WW2 + SS2 + measure->_minSpace);
  for (int jj = 0; jj < n - 1; jj++) {
    X[cnt++] = x;
    x += pitch_print;
    netIdTable[idCnt]
        = measure->createNetSingleWire(_wireDirName, idCnt, w_layout, s_layout);
    idCnt++;
  }

  for (ii = 0; ii < cnt; ii++) {
    uint length = measure->getBoxLength(ii, measure->_met, false);
    // layout;
    if (ii == mid) {
      if (measure->_3dFlag)
        //				m->writeRaphaelPoly3D(fp,
        // netIdTable[ii+1], X[ii], measure->_len*0.001, 1.0);
        m->writeRaphaelPoly3D(
            fp, netIdTable[ii + 1], X[ii], length * 0.001, 1.0);
      else
        m->writeRaphaelPoly(fp, netIdTable[ii + 1], X[ii], 1.0);
    } else if (ii == mid - 1) {
      if (measure->_3dFlag)
        //				m->writeRaphaelPoly3D(fp,
        // netIdTable[ii+1], X[ii], measure->_len*0.001, 0.0);
        m->writeRaphaelPoly3D(
            fp, netIdTable[ii + 1], X[ii], length * 0.001, 0.0);
      else
        m->writeRaphaelPoly(fp, netIdTable[ii + 1], X[ii], 0.0);
    } else if (ii == mid + 1) {
      if (measure->_3dFlag)
        m->writeRaphaelPoly3D(
            fp, netIdTable[ii + 1], 0.001 * WW2, length * 0.001, X[ii], 0.0);
      else
        m->writeRaphaelPoly(fp, netIdTable[ii + 1], 0.001 * WW2, X[ii], 0.0);
    } else {
      if (measure->_3dFlag)
        //				m->writeRaphaelPoly3D(fp,
        // netIdTable[ii+1], 0.001*measure->_minWidth, measure->_len*0.001,
        // X[ii], 0.0);
        m->writeRaphaelPoly3D(fp,
                              netIdTable[ii + 1],
                              0.001 * measure->_minWidth,
                              length * 0.001,
                              X[ii],
                              0.0);
      else
        m->writeRaphaelPoly(
            fp, netIdTable[ii + 1], 0.001 * measure->_minWidth, X[ii], 0.0);
    }
  }

  if (measure->_diag) {
    int met;
    if (measure->_overMet > 0)
      met = measure->_overMet;
    else if (measure->_underMet > 0)
      met = measure->_underMet;

    m = _process->getMasterConductor(met);
    double minWidth = _process->getConductor(met)->_min_width;
    double minSpace = _process->getConductor(met)->_min_spacing;
    double min_pitch = minWidth + minSpace;
    measure->clean2dBoxTable(met, false);
    int i;
    uint begin = base - Ath__double2int(measure->_seff * 1000)
                 + Ath__double2int(minWidth * 1000) / 2;
    for (i = 0; i < n + 1; i++) {
      netIdTable[idCnt]
          = measure->createDiagNetSingleWire(_wireDirName,
                                             idCnt,
                                             begin,
                                             Ath__double2int(1000 * minWidth),
                                             Ath__double2int(1000 * minSpace),
                                             measure->_dir);
      begin -= Ath__double2int(min_pitch * 1000);
      idCnt++;
    }

    double h = _process->getConductor(met)->_height;
    double t = _process->getConductor(met)->_thickness;
    measure->writeDiagRaphael3D(fp, met, false, base, h, t);

    measure->_ur[measure->_dir] += grid_gap_cnt * (w_layout + s_layout);

    if (measure->_3dFlag) {
      fprintf(fp, "\nOPTIONS SET_GRID=1000000;\n\n");
      fprintf(fp, "POTENTIAL\n");
    } else {
      fprintf(fp, "\nOPTIONS SET_GRID=10000;\n\n");
      fprintf(fp, "POTENTIAL");
      /*
                              for (ii= 0; ii<cnt; ii++)
                                      m->writeBoxName(fp, netIdTable[ii+1]);
      */
      fprintf(fp, " \n");
    }

    return cnt;
  }

  int bboxUR[2] = {measure->_ur[0], measure->_ur[1]};

  double pitchMult = 1.0;

  measure->clean2dBoxTable(measure->_underMet, true);
  measure->createContextNets(
      _wireDirName, bboxLL, bboxUR, measure->_underMet, pitchMult);

  measure->clean2dBoxTable(measure->_overMet, true);
  measure->createContextNets(
      _wireDirName, bboxLL, bboxUR, measure->_overMet, pitchMult);

  //	double mainNetStart= X[0];
  int main_xlo, main_ylo, main_xhi, main_yhi, low;
  measure->getBox(measure->_met, false, main_xlo, main_ylo, main_xhi, main_yhi);
  if (!measure->_dir)
    low = main_ylo - measure->_minWidth / 2;
  else
    low = main_xlo - measure->_minWidth / 2;
  uint contextRaphaelCnt = 0;
  if (measure->_underMet > 0) {
    double h = _process->getConductor(measure->_underMet)->_height;
    double t = _process->getConductor(measure->_underMet)->_thickness;
    /*
                    dbTechLayer
       *layer=measure->_tech->findRoutingLayer(_underMet); uint minWidth=
       layer->getWidth(); uint minSpace= layer->getSpacing(); uint pitch=
       1000*((minWidth+minSpace)*pitchMult)/1000; uint offset=
       2*(minWidth+minSpace); int start= mainNetStart+offset; contextRaphaelCnt=
       measure->writeRaphael3D(fp, measure->_underMet, true, mainNetStart, h,
       t);
    */
    contextRaphaelCnt
        = measure->writeRaphael3D(fp, measure->_underMet, true, low, h, t);
  }

  if (measure->_overMet > 0) {
    double h = _process->getConductor(measure->_overMet)->_height;
    double t = _process->getConductor(measure->_overMet)->_thickness;
    /*
                    dbTechLayer
       *layer=measure->_tech->findRoutingLayer(_overMet); uint minWidth=
       layer->getWidth(); uint minSpace= layer->getSpacing(); uint pitch=
       1000*((minWidth+minSpace)*pitchMult)/1000; uint offset=
       2*(minWidth+minSpace); int start= mainNetStart+offset; contextRaphaelCnt
       += measure->writeRaphael3D(fp, measure->_overMet, true, mainNetStart, h,
       t);
    */
    contextRaphaelCnt
        += measure->writeRaphael3D(fp, measure->_overMet, true, low, h, t);
  }

  measure->_ur[measure->_dir] += grid_gap_cnt * (w_layout + s_layout);

  if (measure->_3dFlag) {
    fprintf(fp, "\nOPTIONS SET_GRID=1000000;\n\n");
    fprintf(fp, "POTENTIAL\n");
  } else {
    fprintf(fp, "\nOPTIONS SET_GRID=10000;\n\n");
    fprintf(fp, "POTENTIAL");
    /*
                    for (ii= 0; ii<cnt; ii++)
                            m->writeBoxName(fp, netIdTable[ii+1]);
    */
    fprintf(fp, " \n");
  }

  return cnt;
}
void extRCModel::writeRaphaelCaps(FILE* fp, extMeasure* measure, uint wireCnt)
{
  extMasterConductor* m = _process->getMasterConductor(measure->_met);

  fprintf(fp, "\nOPTIONS SET_GRID=10000;\n\n");
  fprintf(fp, "POTENTIAL\n");
  /*
          if (measure->_diag)
                  fprintf(fp, "POTENTIAL\n");
          else {
                  fprintf(fp, "POTENTIAL");
                  if (!measure->_plate) {
                          for (uint kk= 1; kk<=wireCnt; kk++)
                                  m->writeBoxName(fp, kk);
                  } else  {
                          m->writeBoxName(fp, wireCnt/2+1);
                          if (measure->_overMet>0)
                                  fprintf(fp, "M%d__%s;",
     measure->_overMet,"GND");
                  }
                  fprintf(fp, " \n");
          }
  */
}
void extRCModel::writeRaphaelCaps3D(FILE* fp, extMeasure* measure, uint wireCnt)
{
  //        extMasterConductor *m= _process->getMasterConductor(measure->_met);

  fprintf(fp, "\nOPTIONS SET_GRID=1000000;\n\n");
  fprintf(fp, "POTENTIAL\n");
}
void extRCModel::writeWires(FILE* fp, extMeasure* measure, uint wireCnt)
{
  extMasterConductor* m = _process->getMasterConductor(measure->_met);
  double pitch = measure->_topWidth + measure->_seff;

  uint n = wireCnt / 2;  // ASSUME odd number of wires, 2 will also work

  if (_commentFlag)
    fprintf(fp, "%s\n", _commentLine);

  // assume origin = (0,0)
  double x = -pitch * n;
  m->writeRaphaelPoly(fp, 1, x, 1.0);

  for (uint ii = 1; ii < wireCnt; ii++) {
    x += pitch;
    m->writeRaphaelPoly(fp, ii + 1, x, 0.0);
  }

  fprintf(fp, "\nOPTIONS SET_GRID=10000;\n\n");
  fprintf(fp, "POTENTIAL\n");
  /*
          fprintf(fp, "CAPACITANCE ");
          for (uint kk= 1; kk<=wireCnt; kk++)
                  m->writeBoxName(fp, kk);
          fprintf(fp, " \n");
  */
}
void extRCModel::writeWires(FILE* fp,
                            extMasterConductor* m,
                            uint wireCnt,
                            double X,
                            double width,
                            double space)
{
  uint n = wireCnt / 2;  // ASSUME odd number of wires, 2 will also work
  double x = X - (width + space) * n;

  x = m->writeRaphaelBox(fp, 1, width, x, 1.0);
  for (uint ii = 1; ii < wireCnt; ii++)
    x = m->writeRaphaelBox(fp, ii + 1, width, x + space, 0.0);

  fprintf(fp, "\nOPTIONS SET_GRID=10000;\n\n");
  fprintf(fp, "POTENTIAL\n");
  /*
          fprintf(fp, "CAPACITANCE ");
          for (uint kk= 1; kk<=wireCnt; kk++)
                  m->writeBoxName(fp, kk);
          fprintf(fp, " \n");
  */
}
void extRCModel::setOptions(const char* topDir,
                            const char* pattern,
                            bool writeFiles,
                            bool readFiles,
                            bool runSolver,
                            bool keepFile,
                            uint metLevel)
{
  _logFP = openFile("./", "rulesGen", ".log", "w");
  strcpy(_topDir, topDir);
  strcpy(_patternName, pattern);

  _writeFiles = true;
  _readSolver = true;
  _runSolver = true;

  if (writeFiles) {
    _writeFiles = true;
    _readSolver = false;
    _runSolver = false;
  } else if (readFiles) {
    _writeFiles = false;
    _readSolver = true;
    _runSolver = false;
  } else if (runSolver) {
    _writeFiles = false;
    _readSolver = false;
    _runSolver = true;
  }
  if (keepFile)
    _keepFile = true;
  if (metLevel)
    _metLevel = metLevel;
#ifdef _WIN32
  _runSolver = false;
#endif
}
void extRCModel::setOptions(const char* topDir,
                            const char* pattern,
                            bool writeFiles,
                            bool readFiles,
                            bool runSolver)
{
  _logFP = openFile("./", "rulesGen", ".log", "w");
  strcpy(_topDir, topDir);
  strcpy(_patternName, pattern);

  _writeFiles = true;
  _readSolver = true;
  _runSolver = true;
  _keepFile = true;

  if (writeFiles) {
    _writeFiles = true;
    _readSolver = false;
    _runSolver = false;
  } else if (readFiles) {
    _writeFiles = false;
    _readSolver = true;
    _runSolver = false;
  } else if (runSolver) {
    _writeFiles = false;
    _readSolver = false;
    _runSolver = true;
  }
#ifdef _WIN32
  _runSolver = false;
#endif
}
void extRCModel::closeFiles()
{
  fflush(_logFP);

  if (_logFP != NULL)
    fclose(_logFP);
}
void extRCModel::runSolver(const char* solverOption)
{
  char cmd[4000];
#ifndef _WIN32
  //	sprintf(cmd, "cd %s ; /opt/ads/bin/casyn raphael %s %s ; cd
  //../../../../../../ ", _wireDirName, solverOption, _wireFileName);
  // this is for check in
  if (_diagModel == 2)
    sprintf(cmd,
            "ca raphael %s %s/%s -o %s/%s.out",
            solverOption,
            _wireDirName,
            _wireFileName,
            _wireDirName,
            _wireFileName);
  else
    sprintf(cmd,
            "ca raphael %s %s/%s -o %s/%s.out",
            solverOption,
            _wireDirName,
            _wireFileName,
            _wireDirName,
            _wireFileName);

  // this is for local run
  /*	if (_diagModel==2)
                  sprintf(cmd, "/opt/ads/bin/casyn raphael %s %s/%s -o
     %s/%s.out", solverOption, _wireDirName, _wireFileName, _wireDirName,
     _wireFileName); else sprintf(cmd, "/opt/ads/bin/casyn raphael %s %s/%s -o
     %s/%s.out", solverOption, _wireDirName, _wireFileName, _wireDirName,
     _wireFileName);
  */

  //	sprintf(cmd, "cd %s ; ca raphael %s %s ; cd ../../../../../../ ",
  //_wireDirName, solverOption, _wireFileName);
  logger_->info(RCX, 214, "{}", cmd);
#endif
#ifdef _WIN32
  if (_diagModel == 2)
    sprintf(cmd, "cd %s ; dir ; cd ../../../../../../../../ ", _wireDirName);
  else
    sprintf(cmd, "cd %s ; dir ; cd ../../../../../../ ", _wireDirName);
  logger_->info(RCX, 214, "{}", cmd);
#endif
  system(cmd);
}
void extRCModel::cleanFiles()
{
  char cmd[4000];
  sprintf(cmd, "rm -rf %s ", _wireDirName);
  system(cmd);
}
uint extRCModel::getOverUnderIndex(extMeasure* m, uint maxCnt)
{
  return getMetIndexOverUnder(
      m->_met, m->_underMet, m->_overMet, _layerCnt, maxCnt, logger_);
}
uint extRCModel::getUnderIndex(extMeasure* m)
{
  return m->_overMet - m->_met - 1;
}
void extDistWidthRCTable::addRCw(uint n, uint w, extDistRC* rc)
{
  int wIndex = _widthTable->findIndex(w);
  if (wIndex < 0)
    wIndex = _widthTable->add(w);

  _rcDistTable[n][wIndex]->addMeasureRC(rc);
}

void extMetRCTable::addRCw(extMeasure* m)
{
  extDistWidthRCTable* table = NULL;
  int n;
  if (m->_overUnder) {
    n = getMetIndexOverUnder(m->_met,
                             m->_underMet,
                             m->_overMet,
                             _layerCnt,
                             _capOverUnder[m->_met]->_metCnt,
                             logger_);
    assert(n >= 0);
    table = _capOverUnder[m->_met];
  } else if (m->_over) {
    n = m->_underMet;
    if (m->_res) {
        table = _resOver[m->_met];
    } else {
    table = _capOver[m->_met];
    }
  } else if (m->_diag) {
    n = m->getUnderIndex();
    if (m->_diagModel == 1) {  // TODO 0620 : diagModel=2
      table = _capDiagUnder[m->_met];
    }
  } else {
    n = m->getUnderIndex();
    table = _capUnder[m->_met];
  }
  if (table != NULL)
    table->addRCw(n, m->_w_nm, m->_tmpRC);
}
void extRCModel::addRC(extMeasure* m)
{
  if (m->_overUnder) {
    uint maxCnt = _modelTable[m->_rIndex]->_capOverUnder[m->_met]->_metCnt;
    uint n = getOverUnderIndex(m, maxCnt);
    _modelTable[m->_rIndex]
        ->_capOverUnder[m->_met]
        ->_rcDistTable[n][m->_wIndex]
        ->addMeasureRC(m->_tmpRC);
  } else if (m->_over) {
    if (m->_res) {
      _modelTable[m->_rIndex]
        ->_resOver[m->_met]
        ->_rcDistTable[m->_underMet][m->_wIndex]
        ->addMeasureRC(m->_tmpRC);
    } else {
    _modelTable[m->_rIndex]
        ->_capOver[m->_met]
        ->_rcDistTable[m->_underMet][m->_wIndex]
        ->addMeasureRC(m->_tmpRC);
    }
  } else if (m->_diag) {
    uint n = getUnderIndex(m);
    if (_diagModel == 2)
      _modelTable[m->_rIndex]
          ->_capDiagUnder[m->_met]
          ->_rcDiagDistTable[n][m->_wIndex][m->_dwIndex][m->_dsIndex]
          ->addMeasureRC(m->_tmpRC);
    else
      _modelTable[m->_rIndex]
          ->_capDiagUnder[m->_met]
          ->_rcDistTable[n][m->_wIndex]
          ->addMeasureRC(m->_tmpRC);
  } else {
    uint n = getUnderIndex(m);
    _modelTable[m->_rIndex]
        ->_capUnder[m->_met]
        ->_rcDistTable[n][m->_wIndex]
        ->addMeasureRC(m->_tmpRC);
  }
}
void extMetRCTable::mkWidthAndSpaceMappings()
{
  for (uint ii = 1; ii < _layerCnt; ii++) {
    if (_capOver[ii] != NULL)
      _capOver[ii]->makeWSmapping();
    else
      logger_->info(RCX, 215, "Can't find <OVER> rules for {}", ii);

    if (_resOver[ii] != NULL)
      _resOver[ii]->makeWSmapping();
    else
      logger_->info(RCX, 215, "Can't find <RESOVER> Res rules for {}", ii);

    if (_capUnder[ii] != NULL)
      _capUnder[ii]->makeWSmapping();
    else
      logger_->info(RCX, 216, "Can't find <UNDER> rules for {}", ii);

    if (_capOverUnder[ii] != NULL)
      _capOverUnder[ii]->makeWSmapping();
    else if ((ii > 1) && (ii < _layerCnt - 1))
      logger_->info(RCX, 217, "Can't find <OVERUNDER> rules for {}", ii);
  }
}
void extRCModel::writeRules(char* name, bool binary)
{
  bool writeRes= false;
  //	FILE *fp= openFile("./", name, NULL, "w");
  FILE* fp = fopen(name, "w");

  uint cnt = 0;
  fprintf(fp, "Extraction Rules for OpenRCX\n\n");
  if (_diag || _diagModel > 0) {
    if (_diagModel == 1)
      fprintf(fp, "DIAGMODEL ON\n\n");
    else if (_diagModel == 2)
      fprintf(fp, "DIAGMODEL TRUE\n\n");
  }

  fprintf(fp, "LayerCount %d\n", _layerCnt - 1);
  fprintf(fp, "DensityRate %d ", _modelCnt);
  for (uint kk = 0; kk < _modelCnt; kk++)
    fprintf(fp, " %g", _dataRateTable->get(kk));
  fprintf(fp, "\n");

  for (uint m = 0; m < _modelCnt; m++) {
    fprintf(fp, "\nDensityModel %d\n", m);

    for (uint ii = 1; ii < _layerCnt; ii++) {

      if (writeRes) {
      if (_modelTable[m]->_resOver[ii] != NULL)
        cnt += _modelTable[m]->_resOver[ii]->writeRulesOver_res(fp, binary);
      else if ((m > 0) && (_modelTable[0]->_resOver[ii] != NULL))
        cnt += _modelTable[0]->_resOver[ii]->writeRulesOver_res(fp, binary);
      else if (m == 0) {
        logger_->info(
            RCX,
            218,
            "Cannot write <OVER> Res rules for <DensityModel> {} and layer {}",
            m,
            ii);
      }
      }
      if (_modelTable[m]->_capOver[ii] != NULL)
        cnt += _modelTable[m]->_capOver[ii]->writeRulesOver(fp, binary);
      else if ((m > 0) && (_modelTable[0]->_capOver[ii] != NULL))
        cnt += _modelTable[0]->_capOver[ii]->writeRulesOver(fp, binary);
      else if (m == 0) {
        logger_->info(
            RCX,
            218,
            "Cannot write <OVER> rules for <DensityModel> {} and layer {}",
            m,
            ii);
      }
      
      if (_modelTable[m]->_capUnder[ii] != NULL)
        cnt += _modelTable[m]->_capUnder[ii]->writeRulesUnder(fp, binary);
      else if ((m > 0) && (_modelTable[0]->_capUnder[ii] != NULL))
        cnt += _modelTable[0]->_capUnder[ii]->writeRulesUnder(fp, binary);
      else if (m == 0) {
        logger_->info(
            RCX,
            219,
            "Cannot write <UNDER> rules for <DensityModel> {} and layer {}",
            m,
            ii);
      }

      if (_modelTable[m]->_capDiagUnder[ii] != NULL) {
        if (_diagModel == 1)
          cnt += _modelTable[m]->_capDiagUnder[ii]->writeRulesDiagUnder(fp,
                                                                        binary);
        if (_diagModel == 2)
          cnt += _modelTable[m]->_capDiagUnder[ii]->writeRulesDiagUnder2(
              fp, binary);
      } else if ((m > 0) && (_modelTable[0]->_capDiagUnder[ii] != NULL)) {
        if (_diagModel == 1)
          cnt += _modelTable[0]->_capDiagUnder[ii]->writeRulesDiagUnder(fp,
                                                                        binary);
        if (_diagModel == 2)
          cnt += _modelTable[0]->_capDiagUnder[ii]->writeRulesDiagUnder2(
              fp, binary);
      } else if (m == 0) {
        logger_->info(
            RCX,
            220,
            "Cannot write <DIAGUNDER> rules for <DensityModel> {} and layer {}",
            m,
            ii);
      }

      if (_modelTable[m]->_capOverUnder[ii] != NULL)
        cnt += _modelTable[m]->_capOverUnder[ii]->writeRulesOverUnder(fp,
                                                                      binary);
      else if ((m > 0) && (_modelTable[0]->_capOverUnder[ii] != NULL))
        cnt += _modelTable[0]->_capOverUnder[ii]->writeRulesOverUnder(fp,
                                                                      binary);
      else if ((m == 0) && (ii > 1) && (ii < _layerCnt - 1)) {
        logger_->info(
            RCX,
            221,
            "Cannot write <OVERUNDER> rules for <DensityModel> {} and layer {}",
            m,
            ii);
      }
    }
    fprintf(fp, "END DensityModel %d\n", m);
  }
  fclose(fp);
}
uint extRCModel::readMetalHeader(Ath__parser* parser,
                                 uint& met,
                                 const char* keyword,
                                 bool bin,
                                 bool ignore)
{
  if (parser->isKeyword(0, "END")
      && (strcmp(parser->get(1), "DensityModel") == 0))
    return 0;

  if (!(parser->parseNextLine() > 0)) {
    return 0;
  }

  //	uint cnt= 0;
  if (parser->isKeyword(0, "Metal") && (strcmp(parser->get(2), keyword) == 0)) {
    met = parser->getInt(1);
    return 1;
  }

  return 0;
}
void extMetRCTable::allocateInitialTables(uint layerCnt,
                                          uint widthCnt,
                                          bool over,
                                          bool under,
                                          bool diag)
{
  for (uint met = 1; met < _layerCnt; met++) {
    if (over && under && (met > 1) && (met < _layerCnt - 1)) {
      // uint n= getMetIndexOverUnder(met, met-1, _layerCnt-1, layerCnt,
      // logger_);
      uint n = getMaxMetIndexOverUnder(met, layerCnt, logger_);
      _capOverUnder[met] = new extDistWidthRCTable(
          false, met, layerCnt, n + 1, widthCnt, _rcPoolPtr, logger_);
    }
    if (over) {
      _capOver[met] = new extDistWidthRCTable(
          true, met, layerCnt, met, widthCnt, _rcPoolPtr, logger_);
      _resOver[met] = new extDistWidthRCTable(
          true, met, layerCnt, met, widthCnt, _rcPoolPtr, logger_);
    }
    if (under) {
      _capUnder[met] = new extDistWidthRCTable(false,
                                               met,
                                               layerCnt,
                                               _layerCnt - met - 1,
                                               widthCnt,
                                               _rcPoolPtr,
                                               logger_);
    }
    if (diag) {
      _capDiagUnder[met] = new extDistWidthRCTable(false,
                                                   met,
                                                   layerCnt,
                                                   _layerCnt - met - 1,
                                                   widthCnt,
                                                   _rcPoolPtr,
                                                   logger_);
    }
  }
}
Ath__array1D<double>* extRCModel::readHeaderAndWidth(Ath__parser* parser,
                                                     uint& met,
                                                     const char* ouKey,
                                                     const char* wKey,
                                                     bool bin,
                                                     bool ignore)
{
  if (readMetalHeader(parser, met, ouKey, bin, ignore) <= 0)
    return NULL;

  if (!(parser->parseNextLine() > 0))
    return NULL;

  return parser->readDoubleArray("WIDTH", 4);
}
uint extRCModel::readRules(Ath__parser* parser,
                           uint m,
                           uint ii,
                           const char* ouKey,
                           const char* wKey,
                           bool over,
                           bool under,
                           bool bin,
                           bool diag,
                           bool ignore,
                           double dbFactor)
{
  uint cnt = 0;
  uint met = 0;
  Ath__array1D<double>* wTable
      = readHeaderAndWidth(parser, met, ouKey, wKey, bin, false);

  if (wTable == NULL)
    return 0;

  uint widthCnt = wTable->getCnt();

  extDistWidthRCTable* dummy = NULL;
  if (ignore)
    dummy = new extDistWidthRCTable(true, met, _layerCnt, widthCnt, logger_);

  uint diagWidthCnt = 0;
  uint diagDistCnt = 0;

  if (diag && strcmp(ouKey, "DIAGUNDER") == 0 && _diagModel == 2) {
    parser->parseNextLine();
    if (parser->isKeyword(0, "DIAG_WIDTH"))
      diagWidthCnt = parser->getInt(3);
    parser->parseNextLine();
    if (parser->isKeyword(0, "DIAG_DIST"))
      diagDistCnt = parser->getInt(3);
  }

  if (over && under && (met > 1)) {
    if (!ignore) {
      _modelTable[m]->allocOverUnderTable(met, wTable, dbFactor);
      _modelTable[m]->_capOverUnder[met]->readRulesOverUnder(
          parser, widthCnt, bin, ignore, dbFactor);
    } else
      dummy->readRulesOverUnder(parser, widthCnt, bin, ignore, dbFactor);
  } else if (over) {
    if (strcmp(ouKey, "OVER")==0) {
    if (!ignore) {
      _modelTable[m]->allocOverTable(met, wTable, dbFactor);
      _modelTable[m]->_capOver[met]->readRulesOver(
          parser, widthCnt, bin, ignore, "OVER", dbFactor);
    } else
      dummy->readRulesOver(parser, widthCnt, bin, ignore, "OVER", dbFactor);
    } else { // RESOVER
      if (!ignore) {
          // ASSUME: HAVE READ NORMAL RULES FIRST
          _modelTable[m]->allocOverTable(met, wTable, dbFactor);
          _modelTable[m]->_capOver[met]->readRulesOver(
              parser, widthCnt, bin, ignore, "RESOVER", dbFactor);
        } else
          dummy->readRulesOver(parser, widthCnt, bin, ignore, "RESOVER", dbFactor);     
    }
  } else if (under) {
    if (!ignore) {
      _modelTable[m]->allocUnderTable(met, wTable, dbFactor);
      _modelTable[m]->_capUnder[met]->readRulesUnder(
          parser, widthCnt, bin, ignore, dbFactor);
    } else
      dummy->readRulesUnder(parser, widthCnt, bin, ignore, dbFactor);
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
      if (_diagModel == 2)
        dummy->readRulesDiagUnder(
            parser, widthCnt, diagWidthCnt, diagDistCnt, bin, ignore, dbFactor);
      else if (_diagModel == 1)
        dummy->readRulesDiagUnder(parser, widthCnt, bin, ignore, dbFactor);
    }
  }
  if (ignore)
    delete dummy;

  if (wTable != NULL)
    delete wTable;

  return cnt;
}

bool extRCModel::readRules(char* name,
                           bool bin,
                           bool over,
                           bool under,
                           bool overUnder,
                           bool diag,
                           uint cornerCnt,
                           uint* cornerTable,
                           double dbFactor)
{
  OUREVERSEORDER = false;
  diag = false;
  uint cnt = 0;
  _ruleFileName = strdup(name);
  Ath__parser parser;
  // parser.setDbg(1);
  parser.addSeparator("\r");
  parser.openFile(name);
  while (parser.parseNextLine() > 0) {
    if (parser.isKeyword(0, "OUREVERSEORDER")) {
      if (strcmp(parser.get(1), "ON") == 0) {
        OUREVERSEORDER = true;
      }
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

    if (parser.isKeyword(0, "rcStats")) {  // TO_TEST
      _layerCnt = parser.getInt(2);
      createModelTable(1, _layerCnt);
      for (uint kk = 0; kk < _modelCnt; kk++)
        _dataRateTable->add(0.0);

      _modelTable[0]->allocateInitialTables(_layerCnt, 10, true, true, true);

      _modelTable[0]->readRCstats(&parser);

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
      uint rulesFileModelCnt = parser.getInt(1);
      if (cornerCnt > 0) {
        if ((rulesFileModelCnt > 0) && (rulesFileModelCnt < cornerCnt)) {
          logger_->warn(
              RCX,
              222,
              "There were {} extraction models defined but only {} exists "
              "in the extraction rules file {}",
              cornerCnt,
              rulesFileModelCnt,
              name);
          return false;
        }
        createModelTable(cornerCnt, _layerCnt);

        for (uint jj = 0; jj < cornerCnt; jj++) {
          uint modelIndex = cornerTable[jj];

          uint kk;
          for (kk = 0; kk < rulesFileModelCnt; kk++) {
            if (modelIndex != kk)
              continue;
            _dataRateTable->add(parser.getDouble(kk + 2));
            break;
          }
          if (kk == rulesFileModelCnt) {
            logger_->warn(RCX,
                          223,
                          "Cannot find model index {} in extRules file {}",
                          modelIndex,
                          name);
            return false;
          }
        }
      } else {  // old behavior;
                // david 7.20
                // createModelTable(rulesFileModelCnt,
        // _layerCnt);
        createModelTable(1, _layerCnt);

        for (uint kk = 0; kk < _modelCnt; kk++) {
          _dataRateTable->add(parser.getDouble(kk + 2));
        }
        for (uint ii = 0; ii < _modelCnt; ii++) {
          _modelTable[ii]->_rate = _dataRateTable->get(ii);
        }
      }
      continue;
    }
    // parser.setDbg(1);

    if (parser.isKeyword(0, "DensityModel")) {
      uint m = parser.getInt(1);
      uint modelIndex = m;
      bool skipModel = false;
      if (cornerCnt > 0) {
        uint jj = 0;
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
// skipModel= true;
bool res_skipModel= true;

      for (uint ii = 1; ii < _layerCnt; ii++) {
        if (!res_skipModel) {
            cnt += readRules(&parser,
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
        cnt += readRules(&parser,
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
        if (ii < _layerCnt - 1) {
          cnt += readRules(&parser,
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
          if (diag)
            cnt += readRules(&parser,
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

        if ((ii > 1) && (ii < _layerCnt - 1))
          cnt += readRules(&parser,
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
      }
      // break;
      parser.parseNextLine();
    }
  }
  return true;
}
double extRCModel::measureResistance(extMeasure* m,
                                     double ro,
                                     double top_widthR,
                                     double bot_widthR,
                                     double thicknessR)
{
  double r = ro / ((top_widthR + bot_widthR) * thicknessR * 0.5);
  return r;
}
bool extRCModel::solverStep(extMeasure* m)
{
#ifndef SKIP_SOLVER
  if (_runSolver) {
    if (m->_3dFlag)
      runSolver("rc3 -n -x -z");
    else
      runSolver("rc2 -n -x -z");
    return true;
  }
#endif

  return false;
}
bool extRCModel::measurePatternVar(extMeasure* m,
                                   double top_width,
                                   double bot_width,
                                   double thickness,
                                   uint wireCnt,
                                   char* wiresNameSuffix,
                                   double res)
{
  m->setEffParams(top_width, bot_width, thickness);
  double thicknessChange
      = _process->adjustMasterLayersForHeight(m->_met, thickness);

  // _process->getMasterConductor(m->_met)->reset(m->_heff, top_width,
  // bot_width, thickness);
  _process->getMasterConductor(m->_met)->resetWidth(top_width, bot_width);

  mkFileNames(m, wiresNameSuffix);

  printCommentLine('$', m);
  fprintf(_logFP, "%s\n", _commentLine);
  fprintf(_logFP, "%c %g thicknessChange\n", '$', thicknessChange);
  fflush(_logFP);

  if (_writeFiles) {
    FILE* wfp = mkPatternFile();

    if (wfp == NULL)
      return false;  // should be an exception!! and return!

    double maxHeight
        = _process->adjustMasterDielectricsForHeight(m->_met, thicknessChange);
    maxHeight *= 1.2;

    if (m->_3dFlag) {
      //			double W = (m->_ur[m->_dir] -
      // m->_ll[m->_dir])*10;
      double W = 40;
      // DKF  _process->writeProcessAndGround3D(wfp, "GND", m->_underMet,
      // m->_overMet, -30.0, 60.0, m->_len*0.001, maxHeight, W);
      _process->writeProcessAndGround3D(wfp,
                                        "GND",
                                        -1,
                                        -1,
                                        -30.0,
                                        60.0,
                                        m->_len * 0.001,
                                        maxHeight,
                                        W,
                                        m->_diag);
    } else {
      if (m->_diag) {
        int met = -1;
        if (m->_overMet + 1 < (int) _layerCnt)
          met = m->_overMet + 1;
        _process->writeProcessAndGround(
            wfp, "GND", m->_met - 1, met, -50.0, 100.0, maxHeight, m->_diag);
      } else
        _process->writeProcessAndGround(wfp,
                                        "GND",
                                        m->_underMet,
                                        m->_overMet,
                                        -50.0,
                                        100.0,
                                        maxHeight,
                                        m->_diag);
    }

    if (_commentFlag)
      fprintf(wfp, "%s\n", _commentLine);

    if (m->_benchFlag) {
      writeBenchWires(wfp, m);
    } else {
      if (m->_3dFlag) {  // 3d based extraction rules
                         //				writeWires2_3D(wfp, m,
                         // wireCnt);
        writeRuleWires_3D(wfp, m, wireCnt);
        writeRaphaelCaps3D(wfp, m, wireCnt);
      } else {
        //				writeWires2(wfp, m, wireCnt);
        writeRuleWires(wfp, m, wireCnt);
        writeRaphaelCaps(wfp, m, wireCnt);
      }
    }

    fclose(wfp);
  }
  solverStep(m);

  // if (!m->_benchFlag && _readSolver) {
  if (_readSolver) {
    uint lineCnt = 0;

    if (m->_3dFlag)
      lineCnt = readCapacitanceBench3D(_readCapLog, m, true);
    else {
      if (m->_diag)
        lineCnt = readCapacitanceBenchDiag(_readCapLog, m);
      else
        lineCnt = readCapacitanceBench(_readCapLog, m);
    }
    if (lineCnt <= 0 && _keepFile) {
      _readCapLog = false;

      solverStep(m);

      if (m->_3dFlag)
        lineCnt = readCapacitanceBench3D(_readCapLog, m, true);
      else {
        if (m->_diag)
          lineCnt = readCapacitanceBenchDiag(_readCapLog, m);
        else
          lineCnt = readCapacitanceBench(_readCapLog, m);
      }
    }
    if (!m->_benchFlag && (lineCnt > 0)) {
      double cc1 = 0.0, cc2 = 0.0, fr = 0.0, tot = 0.0;
      if (m->_3dFlag)
        getCapValues3D(lineCnt, cc1, cc2, fr, tot, m);
      else
        getCapValues(lineCnt, cc1, cc2, fr, tot, m);

      m->_rcValid = false;

      if (wiresNameSuffix != NULL)
        return true;

      if (lineCnt <= 0) {
        logger_->info(
            RCX, 224, "Not valid cap values from solver dir {}", _wireDirName);
        return true;
      }

      extDistRC* rc = _rcPoolPtr->alloc();

      if (m->_diag && _diagModel == 2)
        rc->set(m->_s_nm, (cc1 + cc2) * 0.5, fr, 0.0, 0.001 * res);
      else
        rc->set(m->_s_nm, cc1, 0.5 * fr, 0.0, 0.001 * res);
      m->_tmpRC = rc;

      m->_rcValid = true;

      // m->addCap();
      addRC(m);
    }
    if (m->_benchFlag && (lineCnt > 0)) {
      if (m->_3dFlag)
        getCapMatrixValues3D(lineCnt, m);
      else
        getCapMatrixValues(lineCnt, m);
    }
  }
  if (!_keepFile)
    cleanFiles();
  return true;
}
uint extRCModel::readCapacitanceBench3D(bool readCapLog,
                                        extMeasure* m,
                                        bool skipPrintWires)
{
  double units = 1.0e+15;

  FILE* solverFP = NULL;
  if (!readCapLog) {
    solverFP = openSolverFile();
    if (solverFP == NULL) {
      return 0;
    }
    _parser->setInputFP(solverFP);
  }

  Ath__parser wParser;

  bool matrixFlag = false;
  uint cnt = 0;
  m->_capMatrix[1][0] = 0.0;
  while (_parser->parseNextLine() > 0) {
    if (matrixFlag) {
      if (_parser->isKeyword(0, "END"))
        break;

      if (!_parser->isKeyword(0, "Charge"))
        continue;

      //_parser->printWords(stdout);
      _parser->printWords(_capLogFP);

      double cap = _parser->getDouble(4);
      if (cap < 0.0)
        cap = -cap;
      cap *= units;

      wParser.mkWords(_parser->get(2), "w");
      uint n1 = wParser.getInt(1);
      if (!n1) {
        m->_capMatrix[1][0] += cap;
        continue;
      }
      m->_capMatrix[1][cnt + 1] = cap;
      m->_idTable[cnt + 1] = n1;
      cnt++;
      continue;
    }

    if (_parser->isKeyword(0, "***") && _parser->isKeyword(1, "POTENTIAL")) {
      matrixFlag = true;

      fprintf(_capLogFP, "BEGIN %s\n", _wireDirName);
      fprintf(_capLogFP, "%s\n", _commentLine);
      if (!skipPrintWires) {
        if (m != NULL) {
          if (m->_benchFlag)
            writeWires2_3D(_capLogFP, m, m->_wireCnt);
          else
            writeRuleWires_3D(_capLogFP, m, m->_wireCnt);
        }
      }
      continue;
    } else if (_parser->isKeyword(0, "BEGIN")
               && (strcmp(_parser->get(1), _wireDirName) == 0)) {
      matrixFlag = true;

      fprintf(_capLogFP, "BEGIN %s\n", _wireDirName);

      continue;
    }
  }

  for (uint i = 1; i < cnt; i++) {
    for (uint j = i + 1; j < cnt + 1; j++) {
      if (m->_idTable[j] < m->_idTable[i]) {
        uint t = m->_idTable[i];
        double tt = m->_capMatrix[1][i];
        m->_idTable[i] = m->_idTable[j];
        m->_capMatrix[1][i] = m->_capMatrix[1][j];
        m->_idTable[j] = t;
        m->_capMatrix[1][j] = tt;
      }
    }
  }

  if (solverFP != NULL)
    fclose(solverFP);

  return cnt;
}
void extRCModel::printCommentLine(char commentChar, extMeasure* m)
{
  sprintf(_commentLine,
          "%c %s w= %g s= %g r= %g\n\n%c %s %6.3f %6.3f top_width\n%c %s %6.3f "
          "%g bot_width\n%c %s %6.3f %6.3f spacing\n%c %s %6.3f %6.3f height "
          "\n%c %s %6.3f %6.3f thicknes\n",
          commentChar,
          "Layout params",
          m->_w_m,
          m->_s_m,
          m->_r,
          commentChar,
          "Layout/Eff",
          m->_w_m,
          m->_topWidth,
          commentChar,
          "Layout/Eff",
          m->_w_m,
          m->_botWidth,
          commentChar,
          "Layout/Eff",
          m->_s_m,
          m->_seff,
          commentChar,
          "Layout/Eff",
          m->_h,
          m->_heff,
          commentChar,
          "Layout/Eff",
          m->_t,
          m->_teff);
  _commentFlag = true;
}
void extRCModel::getDiagTables(extMeasure* m, uint widthCnt, uint spaceCnt)
{
  Ath__array1D<double>* diagSTable0 = NULL;
  Ath__array1D<double>* diagWTable0 = NULL;
  diagSTable0 = _process->getDiagSpaceTable(m->_overMet);
  diagWTable0 = _process->getWidthTable(m->_overMet);
  m->_diagWidthTable0.resetCnt();
  if (diagWTable0) {
    for (uint wIndex = 0;
         (wIndex < diagWTable0->getCnt()) && (wIndex < widthCnt);
         wIndex++) {
      double w = diagWTable0->get(wIndex);
      m->_diagWidthTable0.add(w);
    }
  }
  m->_diagSpaceTable0.resetCnt();
  if (diagSTable0) {
    for (uint dsIndex = 0;
         (dsIndex < diagSTable0->getCnt()) && (dsIndex < spaceCnt);
         dsIndex++) {
      double ds = diagSTable0->get(dsIndex);
      m->_diagSpaceTable0.add(ds);
    }
  }
}
void extRCModel::computeTables(extMeasure* m,
                               uint wireCnt,
                               uint widthCnt,
                               uint spaceCnt,
                               uint dCnt)
{
  //	extVariation *xvar= _process->getVariation(m->_met);
  extVariation* xvar = NULL;
  if (!_maxMinFlag)
    xvar = _process->getVariation(m->_met);

  m->_thickVarFlag = _process->getThickVarFlag();

  Ath__array1D<double>* wTable = NULL;
  Ath__array1D<double>* sTable = NULL;
  Ath__array1D<double>* dTable = NULL;
  Ath__array1D<double>* pTable = NULL;
  Ath__array1D<double>* wTable0 = NULL;
  Ath__array1D<double>* sTable0 = NULL;
  Ath__array1D<double>* diagSTable0 = NULL;
  Ath__array1D<double>* diagWTable0 = NULL;
  if (xvar != NULL) {
    wTable = xvar->getWidthTable();
    sTable = xvar->getSpaceTable();
    dTable = xvar->getDataRateTable();
    pTable = xvar->getPTable();
    wTable0 = _process->getWidthTable(m->_met);
    sTable0 = _process->getSpaceTable(m->_met);
    if (_diagModel == 2 && m->_overMet < (int) _layerCnt) {
      diagSTable0 = _process->getDiagSpaceTable(m->_overMet);
      diagWTable0 = _process->getWidthTable(m->_overMet);
    } else {
      diagSTable0 = _process->getDiagSpaceTable(m->_met);
    }
  } else {  // no variation
    wTable = _process->getWidthTable(m->_met);
    sTable = _process->getSpaceTable(m->_met);
    dTable = _process->getDataRateTable(m->_met);
    wTable0 = _process->getWidthTable(m->_met);
    sTable0 = _process->getSpaceTable(m->_met);
    if (_diagModel == 2 && m->_overMet < (int) _layerCnt) {
      diagSTable0 = _process->getDiagSpaceTable(m->_overMet);
      diagWTable0 = _process->getWidthTable(m->_overMet);
    } else {
      diagSTable0 = _process->getDiagSpaceTable(m->_met);
    }
    if (_maxMinFlag)
      for (uint i = 1; i < 3; i++) {
        dTable->add(i);
      }
  }
  m->_widthTable.resetCnt();
  for (uint wIndex = 0; (wIndex < wTable->getCnt()) && (wIndex < widthCnt);
       wIndex++) {
    double w = wTable->get(wIndex);  // layout
    m->_widthTable.add(w);
  }
  if (_diagModel == 2 && m->_overMet < (int) _layerCnt) {
    m->_diagWidthTable0.resetCnt();
    if (diagWTable0) {
      for (uint wIndex = 0;
           (wIndex < diagWTable0->getCnt()) && (wIndex < widthCnt);
           wIndex++) {
        double w = diagWTable0->get(wIndex);
        m->_diagWidthTable0.add(w);
      }
    }
  }
  m->_spaceTable.resetCnt();
  if (m->_diagModel == 1) {
    m->_spaceTable.add(0.0);
  }
  for (uint sIndex = 0; (sIndex < sTable->getCnt()) && (sIndex < spaceCnt);
       sIndex++) {
    double s = sTable->get(sIndex);  // layout
    m->_spaceTable.add(s);
  }
  if (m->_diagModel == 2)
    m->_spaceTable.add(99);
  m->_spaceTable.add(100);
  m->_diagSpaceTable0.resetCnt();
  if (diagSTable0) {
    for (uint dsIndex = 0;
         (dsIndex < diagSTable0->getCnt()) && (dsIndex < spaceCnt);
         dsIndex++) {
      double ds = diagSTable0->get(dsIndex);
      m->_diagSpaceTable0.add(ds);
    }
  }
  m->_dataTable.resetCnt();
  //	if (xvar!=NULL || _maxMinFlag) {
  if (!_maxMinFlag && xvar != NULL)
    m->_dataTable.add(0.0);
  m->_widthTable0.resetCnt();
  for (uint wIndex1 = 0; (wIndex1 < wTable0->getCnt()) && (wIndex1 < widthCnt);
       wIndex1++) {
    double w = wTable0->get(wIndex1);
    m->_widthTable0.add(w);
  }
  m->_spaceTable0.resetCnt();
  if (m->_diagModel == 1) {
    m->_spaceTable0.add(0.0);
  }
  for (uint sIndex1 = 0; (sIndex1 < sTable0->getCnt()) && (sIndex1 < spaceCnt);
       sIndex1++) {
    double s = sTable0->get(sIndex1);
    m->_spaceTable0.add(s);
  }
  if (m->_diagModel == 2)
    m->_spaceTable0.add(99);
  m->_spaceTable0.add(100);
  //	}

  for (uint dIndex = 0; (dIndex < dTable->getCnt()) && (dIndex < dCnt);
       dIndex++) {
    double r = dTable->get(dIndex);  // layout
    m->_dataTable.add(r);
  }
  if (pTable != NULL) {
    m->_pTable.resetCnt();
    for (uint pIndex = 0; pIndex < pTable->getCnt(); pIndex++) {
      double p = pTable->get(pIndex);
      m->_pTable.add(p);
    }
  }
}
uint extRCModel::measureDiagWithVar(extMeasure* measure)
{
  uint cnt = 0;
  int met = measure->_met;
  extVariation* xvar = _process->getVariation(measure->_met);

  extConductor* cond = _process->getConductor(met);
  double t = cond->_thickness;
  double h = cond->_height;
  double ro = cond->_p;
  double res = 0.0;
  double top_ext = cond->_top_ext;
  double bot_ext = cond->_bot_ext;

  //        extMasterConductor* m= _process->getMasterConductor(met);
  if (top_ext != 0.0 || bot_ext != 0.0) {
    xvar = NULL;
    measure->_metExtFlag = true;
  }

  for (uint dIndex = 0; dIndex < measure->_dataTable.getCnt(); dIndex++) {
    double r = measure->_dataTable.get(dIndex);  // layout
    measure->_rIndex = dIndex;

    uint wcnt;
    uint scnt;
    uint dwcnt;
    uint dscnt;
    if (dIndex) {
      wcnt = measure->_widthTable.getCnt();
      scnt = measure->_spaceTable.getCnt();
      dwcnt = measure->_diagWidthTable0.getCnt();
      dscnt = measure->_diagSpaceTable0.getCnt();
    } else {
      wcnt = measure->_widthTable0.getCnt();
      scnt = measure->_spaceTable0.getCnt();
      dwcnt = measure->_diagWidthTable0.getCnt();
      dscnt = measure->_diagSpaceTable0.getCnt();
    }
    for (uint wIndex = 0; wIndex < wcnt; wIndex++) {
      double w;
      if (!dIndex)
        w = measure->_widthTable0.get(wIndex);
      else
        w = measure->_widthTable.get(wIndex);  // layout
      measure->_wIndex = wIndex;
      for (uint dwIndex = 0; dwIndex < dwcnt; dwIndex++) {
        double dw;
        dw = measure->_diagWidthTable0.get(dwIndex);
        measure->_dwIndex = dwIndex;
        for (uint dsIndex = 0; dsIndex < dscnt; dsIndex++) {
          double ds;
          ds = measure->_diagSpaceTable0.get(dsIndex);
          measure->_dsIndex = dsIndex;
          for (uint sIndex = 0; sIndex < scnt; sIndex++) {
            double s;
            if (!dIndex)
              s = measure->_spaceTable0.get(sIndex);
            else
              s = measure->_spaceTable.get(sIndex);
            double top_width = w + 2 * top_ext;
            double top_widthR = w + 2 * top_ext;
            double bot_width = w + 2 * bot_ext;
            double thickness = t;
            double bot_widthR = w + 2 * bot_ext;
            double thicknessR = t;

            if (r == 0.0) {
              //                                        		top_width=
              //                                        w;
              //                                        top_widthR= w;
              if (xvar != NULL) {
                double a = xvar->getP(w);
                if (a != 0.0)
                  ro = a;
              }
              res = measureResistance(
                  measure, ro, top_widthR, bot_widthR, thicknessR);
            } else if (xvar != NULL && !_maxMinFlag) {
              uint ss;
              //							if
              //(sIndex < scnt-1)
              if (sIndex < scnt - 2)
                ss = sIndex;
              else
                //								ss
                //= scnt-2;
                ss = scnt - 3;
              top_width = xvar->getTopWidth(wIndex, ss);
              top_widthR = xvar->getTopWidthR(wIndex, ss);
              bot_width = xvar->getBottomWidth(w, dIndex - 1);
              bot_width = top_width - bot_width;
              thickness = xvar->getThickness(w, dIndex - 1);
              bot_widthR = xvar->getBottomWidthR(w, dIndex - 1);
              bot_widthR = top_widthR - bot_widthR;
              thicknessR = xvar->getThicknessR(w, dIndex - 1);
              double a = xvar->getP(w);
              if (a != 0.0)
                ro = a;
              res = measureResistance(
                  measure, ro, top_widthR, bot_widthR, thicknessR);
            } else if (_maxMinFlag && r == 1.0) {
              top_width = w - 2 * cond->_min_cw_del;
              thickness = t - cond->_min_ct_del;
              bot_width = top_width - 2 * thickness * cond->_min_ca;
              if (bot_width > w)
                bot_width = w;
              res = measureResistance(
                  measure, ro, top_widthR, bot_widthR, thicknessR);
            } else if (_maxMinFlag && r == 2.0) {
              top_width = w + 2 * cond->_max_cw_del;
              thickness = t + cond->_max_ct_del;
              bot_width = top_width - 2 * thickness * cond->_max_ca;
              if (bot_width < w)
                bot_width = w;
              res = measureResistance(
                  measure, ro, top_widthR, bot_widthR, thicknessR);
            } else if (measure->_thickVarFlag) {
              thickness *= 1 + r;
              thicknessR *= 1 + r;
            } else {
              continue;
            }
            measure->setTargetParams(w, s, r, t, h, dw, ds);
            measurePatternVar(measure,
                              top_width,
                              bot_width,
                              thickness,
                              measure->_wireCnt,
                              NULL,
                              res * 0.5);

            cnt++;
          }
        }
      }
    }
  }
  return cnt;
}
uint extRCModel::measureWithVar(extMeasure* measure)
{
  uint cnt = 0;
  int met = measure->_met;
  extVariation* xvar = _process->getVariation(measure->_met);

  extConductor* cond = _process->getConductor(met);
  double t = cond->_thickness;
  double h = cond->_height;
  double ro = cond->_p;
  double res = 0.0;
  double top_ext = cond->_top_ext;
  double bot_ext = cond->_bot_ext;

  //	extMasterConductor* m= _process->getMasterConductor(met);
  if (top_ext != 0.0 || bot_ext != 0.0) {
    xvar = NULL;
    measure->_metExtFlag = true;
  }

  for (uint dIndex = 0; dIndex < measure->_dataTable.getCnt(); dIndex++) {
    double r = measure->_dataTable.get(dIndex);  // layout
    measure->_rIndex = dIndex;

    uint wcnt;
    uint scnt;
    if (dIndex) {
      wcnt = measure->_widthTable.getCnt();
      if (!measure->_diag)
        scnt = measure->_spaceTable.getCnt();
      else
        //      scnt=measure->_spaceTable0.getCnt();
        scnt = measure->_diagSpaceTable0.getCnt();
    } else {
      wcnt = measure->_widthTable0.getCnt();
      if (!measure->_diag)
        scnt = measure->_spaceTable0.getCnt();
      else
        scnt = measure->_diagSpaceTable0.getCnt();
    }
    for (uint wIndex = 0; wIndex < wcnt; wIndex++) {
      double w;
      if (!dIndex)
        w = measure->_widthTable0.get(wIndex);
      else
        w = measure->_widthTable.get(wIndex);  // layout
      measure->_wIndex = wIndex;

      for (uint sIndex = 0; sIndex < scnt; sIndex++) {
        double s;
        if (!measure->_diag) {
          if (!dIndex)
            s = measure->_spaceTable0.get(sIndex);
          else
            s = measure->_spaceTable.get(sIndex);  // layout
          if (sIndex == scnt - 1 && wIndex == wcnt - 1 && !measure->_overUnder)
            measure->_plate = true;
          else
            measure->_plate = false;
        } else
          s = measure->_diagSpaceTable0.get(sIndex);

        double top_width = w + 2 * top_ext;
        double top_widthR = w + 2 * top_ext;
        double bot_width = w + 2 * bot_ext;
        double thickness = t;
        double bot_widthR = w + 2 * bot_ext;
        double thicknessR = t;

        if (r == 0.0) {
          //					top_width= w;
          //					top_widthR= w;
          if (xvar != NULL) {
            double a = xvar->getP(w);
            if (a != 0.0)
              ro = a;
          }
          res = measureResistance(
              measure, ro, top_widthR, bot_widthR, thicknessR);
        } else if (xvar != NULL && !_maxMinFlag) {
          uint ss;
          if (measure->_diag)
            ss = 5;
          else {
            if (sIndex < scnt - 1)
              ss = sIndex;
            else
              ss = scnt - 2;
          }
          /*
                                                  top_width=
             xvar->getTopWidth(wIndex, sIndex); top_widthR=
             xvar->getTopWidthR(wIndex, sIndex);
          */
          top_width = xvar->getTopWidth(wIndex, ss);
          top_widthR = xvar->getTopWidthR(wIndex, ss);

          bot_width = xvar->getBottomWidth(w, dIndex - 1);
          bot_width = top_width - bot_width;

          thickness = xvar->getThickness(w, dIndex - 1);

          bot_widthR = xvar->getBottomWidthR(w, dIndex - 1);
          bot_widthR = top_widthR - bot_widthR;

          thicknessR = xvar->getThicknessR(w, dIndex - 1);
          double a = xvar->getP(w);
          if (a != 0.0)
            ro = a;
          res = measureResistance(
              measure, ro, top_widthR, bot_widthR, thicknessR);
        } else if (_maxMinFlag && r == 1.0) {
          top_width = w - 2 * cond->_min_cw_del;
          thickness = t - cond->_min_ct_del;
          bot_width = top_width - 2 * thickness * cond->_min_ca;
          if (bot_width > w)
            bot_width = w;
          res = measureResistance(
              measure, ro, top_widthR, bot_widthR, thicknessR);
        } else if (_maxMinFlag && r == 2.0) {
          top_width = w + 2 * cond->_max_cw_del;
          thickness = t + cond->_max_ct_del;
          bot_width = top_width - 2 * thickness * cond->_max_ca;
          if (bot_width < w)
            bot_width = w;
          res = measureResistance(
              measure, ro, top_widthR, bot_widthR, thicknessR);
        } else if (measure->_thickVarFlag) {
          thickness *= 1 + r;
          thicknessR *= 1 + r;
        } else {
          continue;
        }

        measure->setTargetParams(w, s, r, t, h);
        measurePatternVar(measure,
                          top_width,
                          bot_width,
                          thickness,
                          measure->_wireCnt,
                          NULL,
                          res * 0.5);

        //				measure->setTargetParams(w, s, 0.0, t,
        // h); 				measurePatternVar(measure, w, w, t,
        // measure->_wireCnt, "2");

        cnt++;
      }
    }
  }

  return cnt;
}
void extRCModel::allocOverTable(extMeasure* measure)
{
  for (uint ii = 0; ii < measure->_dataTable.getCnt(); ii++) {
    if (!ii)
      _modelTable[ii]->allocOverTable(measure->_met, &measure->_widthTable0);
    else
      _modelTable[ii]->allocOverTable(measure->_met, &measure->_widthTable);
  }
}
void extRCModel::allocDiagUnderTable(extMeasure* measure)
{
  for (uint ii = 0; ii < measure->_dataTable.getCnt(); ii++) {
    if (!ii) {
      if (_diagModel == 2)
        _modelTable[ii]->allocDiagUnderTable(
            measure->_met,
            &measure->_widthTable0,
            measure->_diagWidthTable0.getCnt(),
            measure->_diagSpaceTable0.getCnt());
      else if (_diagModel == 1)
        _modelTable[ii]->allocDiagUnderTable(measure->_met,
                                             &measure->_widthTable0);
    } else {
      if (_diagModel == 2)
        _modelTable[ii]->allocDiagUnderTable(
            measure->_met,
            &measure->_widthTable,
            measure->_diagWidthTable0.getCnt(),
            measure->_diagSpaceTable0.getCnt());
      else if (_diagModel == 1)
        _modelTable[ii]->allocDiagUnderTable(measure->_met,
                                             &measure->_widthTable);
    }
  }
}
void extRCModel::setDiagUnderTables(extMeasure* measure)
{
  for (uint ii = 0; ii < measure->_dataTable.getCnt(); ii++)
    _modelTable[ii]->setDiagUnderTables(measure->_met,
                                        measure->_overMet,
                                        &measure->_diagWidthTable0,
                                        &measure->_diagSpaceTable0);
}
void extRCModel::allocUnderTable(extMeasure* measure)
{
  for (uint ii = 0; ii < measure->_dataTable.getCnt(); ii++) {
    if (!ii)
      _modelTable[ii]->allocUnderTable(measure->_met, &measure->_widthTable0);
    else
      _modelTable[ii]->allocUnderTable(measure->_met, &measure->_widthTable);
  }
}
void extRCModel::allocOverUnderTable(extMeasure* measure)
{
  for (uint ii = 0; ii < measure->_dataTable.getCnt(); ii++) {
    if (!ii)
      _modelTable[ii]->allocOverUnderTable(measure->_met,
                                           &measure->_widthTable0);
    else
      _modelTable[ii]->allocOverUnderTable(measure->_met,
                                           &measure->_widthTable);
  }
}
uint extRCModel::linesOver(uint wireCnt,
                           uint widthCnt,
                           uint spaceCnt,
                           uint dCnt,
                           uint metLevel)
{
  sprintf(_patternName, "Over%d", wireCnt);

  openCapLogFile();
  uint cnt = 0;

  extMeasure measure;
  measure._wireCnt = wireCnt;

  for (uint met = 1; met < _layerCnt; met++) {
    if (metLevel > 0 && met != metLevel)
      continue;

    measure._met = met;
    computeTables(&measure, wireCnt, widthCnt, spaceCnt, dCnt);

    allocOverTable(&measure);

    for (uint underMet = 0; underMet < met; underMet++) {
      measure.setMets(met, underMet, -1);
      measure._capTable = _capOver;

      uint cnt1 = measureWithVar(&measure);

      logger_->info(RCX,
                    225,
                    "Finished {} measurements for pattern M{}_over_M{}",
                    cnt1,
                    met,
                    underMet);

      cnt += cnt1;
    }
  }
  if (metLevel < 0)
    logger_->info(
        RCX, 226, "Finished {} measurements for pattern MET_OVER_MET", cnt);

  closeCapLogFile();
  return cnt;
}
uint extRCModel::linesDiagUnder(uint wireCnt,
                                uint widthCnt,
                                uint spaceCnt,
                                uint dCnt,
                                uint metLevel)
{
  _diag = true;
  sprintf(_patternName, "DiagUnder%d", wireCnt);
  openCapLogFile();
  uint cnt = 0;

  extMeasure measure;
  measure._wireCnt = wireCnt;
  measure._diag = true;
  measure._diagModel = _diagModel;

  for (uint met = 1; met < _layerCnt - 1; met++) {
    if (metLevel > 0 && met != metLevel)
      continue;

    measure._met = met;
    measure._overMet = met;
    computeTables(&measure, wireCnt, widthCnt, spaceCnt, dCnt);

    allocDiagUnderTable(&measure);

    for (uint overMet = met + 1; overMet < met + 5 && overMet < _layerCnt;
         overMet++) {  // the max overMet need to be the same as in functions
                       // writeRulesDiagUnder readRulesDiagUnder
      measure.setMets(met, 0, overMet);
      uint cnt1;
      if (_diagModel == 2) {
        getDiagTables(&measure, widthCnt, spaceCnt);
        setDiagUnderTables(&measure);
        measure._capTable = _capDiagUnder;
        cnt1 = measureDiagWithVar(&measure);
      } else if (_diagModel == 1) {
        measure._capTable = _capDiagUnder;
        cnt1 = measureWithVar(&measure);
      }

      logger_->info(RCX,
                    227,
                    "Finished {} measurements for pattern M{}_diagUnder_M{}",
                    cnt1,
                    met,
                    overMet);

      cnt += cnt1;
    }
  }
  if (metLevel < 0)
    logger_->info(RCX,
                  228,
                  "Finished {} measurements for pattern MET_DIAGUNDER_MET",
                  cnt);

  closeCapLogFile();
  return cnt;
}
uint extRCModel::linesUnder(uint wireCnt,
                            uint widthCnt,
                            uint spaceCnt,
                            uint dCnt,
                            uint metLevel)
{
  sprintf(_patternName, "Under%d", wireCnt);
  openCapLogFile();
  uint cnt = 0;

  extMeasure measure;
  measure._wireCnt = wireCnt;

  for (uint met = 1; met < _layerCnt; met++) {
    if (metLevel > 0 && met != metLevel)
      continue;

    measure._met = met;
    computeTables(&measure, wireCnt, widthCnt, spaceCnt, dCnt);

    allocUnderTable(&measure);

    for (uint overMet = met + 1; overMet < _layerCnt; overMet++) {
      measure.setMets(met, 0, overMet);
      measure._capTable = _capUnder;

      uint cnt1 = measureWithVar(&measure);

      logger_->info(RCX,
                    229,
                    "Finished {} measurements for pattern M{}_under_M{}",
                    cnt1,
                    met,
                    overMet);

      cnt += cnt1;
    }
  }
  if (metLevel < 0)
    logger_->info(
        RCX, 230, "Finished {} measurements for pattern MET_UNDER_MET", cnt);

  closeCapLogFile();
  return cnt;
}
uint extRCModel::linesOverUnder(uint wireCnt,
                                uint widthCnt,
                                uint spaceCnt,
                                uint dCnt,
                                uint metLevel)
{
  sprintf(_patternName, "OverUnder%d", wireCnt);
  openCapLogFile();
  uint cnt = 0;

  extMeasure measure;
  measure._wireCnt = wireCnt;
  for (uint met = 1; met < _layerCnt - 1; met++) {
    if (metLevel > 0 && met != metLevel)
      continue;
    measure._met = met;
    computeTables(&measure, wireCnt, widthCnt, spaceCnt, dCnt);

    allocOverUnderTable(&measure);

    for (uint underMet = 1; underMet < met; underMet++) {
      for (uint overMet = met + 1; overMet < _layerCnt; overMet++) {
        measure.setMets(met, underMet, overMet);
        measure._capTable = _capUnder;

        uint cnt1 = measureWithVar(&measure);

        logger_->info(
            RCX,
            231,
            "\nFinished {} measurements for pattern M{}_over_M{}_under_M{}",
            cnt1,
            met,
            underMet,
            overMet);

        cnt += cnt1;
      }
    }
  }
  if (metLevel < 0)
    logger_->info(
        RCX, 230, "Finished {} measurements for pattern MET_UNDER_MET", cnt);

  closeCapLogFile();
  return cnt;
}

uint extMain::metRulesGen(const char* name,
                          const char* topDir,
                          const char* rulesFile,
                          int pattern,
                          bool writeFiles,
                          bool readFiles,
                          bool runSolver,
                          bool keepFile,
                          uint met)
{
  extRCModel* m = _modelTable->get(0);

  m->setOptions(topDir, name, writeFiles, readFiles, runSolver, keepFile, met);
  if ((pattern > 0) && (pattern <= 9))
    m->linesOver(pattern, 20, 20, 20, met);
  else if ((pattern > 10) && (pattern <= 19))
    m->linesUnder(pattern - 10, 20, 20, 20, met);
  else if ((pattern > 20) && (pattern <= 29))
    m->linesOverUnder(pattern - 20, 20, 20, 20, met);
  else if ((pattern > 30) && (pattern <= 39)) {
    m->setDiagModel(1);
    m->linesDiagUnder(pattern - 30, 20, 20, 20, met);
  } else if ((pattern > 40) && (pattern <= 49)) {
    m->setDiagModel(2);
    m->linesDiagUnder(pattern - 40, 20, 20, 20, met);
  }
  m->closeFiles();
  return 0;
}
uint extMain::writeRules(const char* name,
                         const char* topDir,
                         const char* rulesFile,
                         int pattern,
                         bool readDb,
                         bool readFiles)
{
  if (readDb) {
    GenExtRules(rulesFile, pattern);
    return 0;
  }

  if (!readFiles) {
    extRCModel* m = _modelTable->get(0);

    m->setOptions(topDir, name, false, true, false, false);
    m->writeRules((char*) rulesFile, false);
    return 0;
  }

  if (pattern > 0)
    rulesGen(name, topDir, rulesFile, pattern, false, true, false, false);
  else
    rulesGen(name, topDir, rulesFile, 205, false, true, false, false);
  return 0;
}
uint extMain::rulesGen(const char* name,
                       const char* topDir,
                       const char* rulesFile,
                       int pattern,
                       bool writeFiles,
                       bool readFiles,
                       bool runSolver,
                       bool keepFile)
{
  extRCModel* m = _modelTable->get(0);

  m->setOptions(topDir, name, writeFiles, readFiles, runSolver, keepFile);

  if ((pattern > 0) && (pattern <= 9))
    m->linesOver(pattern, 20, 20, 20);
  else if ((pattern > 10) && (pattern <= 19))
    m->linesUnder(pattern - 10, 20, 20, 20);
  else if ((pattern > 20) && (pattern <= 29))
    m->linesOverUnder(pattern - 20, 20, 20, 20);
  else if ((pattern > 30) && (pattern <= 39)) {
    m->setDiagModel(1);
    m->linesDiagUnder(pattern - 30, 20, 20, 20);
  } else if ((pattern > 40) && (pattern <= 49)) {
    m->setDiagModel(2);
    m->linesDiagUnder(pattern - 40, 20, 20, 20);
  } else if (pattern > 100) {
    m->linesOver(pattern % 10, 20, 20, 20);
    m->linesUnder(pattern % 10, 20, 20, 20);
    m->linesOverUnder(pattern % 10, 20, 20, 20);
    if (pattern > 200)
      m->setDiagModel(2);
    else
      m->setDiagModel(1);
    m->linesDiagUnder(pattern % 10, 20, 20, 20);
  }
  m->closeFiles();

  m->writeRules((char*) rulesFile, false);
  return 0;
}
uint extMain::readProcess(const char* name, const char* filename)
{
  /** for testing OverUnderIndex
  uint mCnt= atoi(filename);

  for (uint ii= 2; ii<mCnt; ii++) {
          fprintf(stdout, "\n");
          for (uint u= ii-1; u>0; u--) {
                  for (uint o=  ii+1; o<mCnt; o++) {
                          uint metIndex= getMetIndexOverUnder(ii, u, o, mCnt,
  logger_);

                          fprintf(stdout, "%d  m= %d  %d  = %d\n", u,ii,o,
  metIndex);
                  }
          }
          uint maxIndex= getMaxMetIndexOverUnder(ii, mCnt, logger_);
          fprintf(stdout, "\nm= %d  maxIndex= %d\n\n", ii, maxIndex);
  }
  return 1;
  */

  extProcess* p = new extProcess(32, 32, logger_);

  p->readProcess(name, (char*) filename);
  p->writeProcess("process.out");

  // create rc model

  uint layerCnt = p->getConductorCnt();
  extRCModel* m = new extRCModel(layerCnt, (char*) name, logger_);
  _modelTable->add(m);

  m->setProcess(p);
  m->setDataRateTable(1);

  return 0;
}
uint extMain::readExtRules(const char* name,
                           const char* filename,
                           int min,
                           int typ,
                           int max)
{
  //	extProcess *p= new extProcess(32, 32, logger_);

  //	p->readProcess(name, (char *) filename);

  // create rc model

  extRCModel* m = new extRCModel((char*) name, logger_);
  _modelTable->add(m);

  uint cornerTable[10];
  uint cornerCnt = 0;
  int dbunit = _block->getDbUnitsPerMicron();
  double dbFactor = 1;
  if (dbunit > 1000)
    dbFactor = dbunit * 0.001;

  logger_->info(RCX, 36, "Database dbFactor= {}  dbunit= {}", dbFactor, dbunit);

  _minModelIndex = 0;
  _maxModelIndex = 0;
  _typModelIndex = 0;
  if ((min >= 0) || (max >= 0)) {
    if ((min >= 0) && (max >= 0)) {
      _minModelIndex = min;
      _maxModelIndex = max;
      // ypModelIndex= (min+max)/2;

      cornerTable[cornerCnt++] = min;
      cornerTable[cornerCnt++] = max;
    } else if (min >= 0) {
      _minModelIndex = min;
      cornerTable[cornerCnt++] = min;
    } else if (max >= 0) {
      _maxModelIndex = max;
      cornerTable[cornerCnt++] = max;
    }
    m->readRules((char*) filename,
                 false,
                 true,
                 true,
                 true,
                 true,
                 cornerCnt,
                 cornerTable,
                 dbFactor);
    //		int modelCnt= getRCmodel(0)->getModelCnt();
  } else {
    m->readRules((char*) filename,
                 false,
                 true,
                 true,
                 true,
                 true,
                 0,
                 cornerTable,
                 dbFactor);
    int modelCnt = getRCmodel(0)->getModelCnt();
    _minModelIndex = 0;
    _maxModelIndex = modelCnt - 1;
    _typModelIndex = (modelCnt - 1) / 2;
  }

  //	m->setProcess(p);
  //	m->setDataRateTable(1);

  return 0;
}

void extMain::setLogger(Logger* logger)
{
  if (!logger_)
    logger_ = logger;
}

uint extRCModel::findBiggestDatarateIndex(double d)
{
  return _dataRateTable->findNextBiggestIndex(d, 1);
}
int extRCModel::findDatarateIndex(double d)
{
  for (uint ii = 0; ii < _modelCnt; ii++) {
    if (d == _dataRateTable->get(ii))
      return ii;
    else if (d > _dataRateTable->get(ii))
      return ii - 1;
  }
  return -1;
}
int extRCModel::findClosestDataRate(uint n, double diff)
{
  if (n == _modelCnt - 1)
    return n;
  if (n == 0)
    return n;

  double down = _dataRateTable->get(n);
  double down_dist = diff - down;

  double up = _dataRateTable->get(n + 1);
  double up_dist = up - diff;

  if (down_dist > up_dist)
    return n++;

  return n;
}
int extRCModel::findVariationZero(double d)
{
  _noVariationIndex = _dataRateTable->findIndex(d);
  return _noVariationIndex;
}
extDistWidthRCTable* extRCModel::getWidthDistRCtable(uint met,
                                                     int mUnder,
                                                     int mOver,
                                                     int& n,
                                                     double dRate)
{
  int rIndex = 0;
  if (dRate > 0) {
    rIndex = findDatarateIndex(dRate);
    if (rIndex < 0)
      return NULL;
  }
  if ((mUnder > 0) && (mOver > 0)) {
    n = getMetIndexOverUnder(met,
                             mUnder,
                             mOver,
                             _layerCnt,
                             _modelTable[rIndex]->_capOverUnder[met]->_metCnt,
                             logger_);
    assert(n >= 0);
    return _modelTable[rIndex]->_capOverUnder[met];
  } else if (mOver) {
    n = mUnder;
    return _modelTable[rIndex]->_capOver[met];
  } else {
    n = mOver - met - 1;
    return _modelTable[rIndex]->_capUnder[met];
  }
}
/*
void extRCModel::getRCtable(Ath__array1D<int> *sTable, Ath__array1D<double>
*rcTable, uint valType, uint met, int mUnder, int mOver, int width, double
dRate)
{
        int n= 0;
        extDistWidthRCTable *capTable= getWidthDistRCtable(met, mUnder, mOver,
n, dRate); if (valType==0) // coupling ;
//_modelTable[rIndex]->_capOverUnder[met]->getFringeTable(n, width, sTable,
rcTable, false); else if (valType==1) // fringe capTable->getFringeTable(n,
width, sTable, rcTable, false); else if (valType==2) // res ;
//_modelTable[rIndex]->_capOverUnder[met]->getFringeTable(n, width, sTable,
rcTable, false); else // total ;
//_modelTable[rIndex]->_capOverUnder[met]->getFringeTable(n, width, sTable,
rcTable, false);
}
*/
}  // namespace rcx
