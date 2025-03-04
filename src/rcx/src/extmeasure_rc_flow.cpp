
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

#include "dbUtil.h"
#include "rcx/extMeasureRC.h"
#include "rcx/extRCap.h"
#include "rcx/extSegment.h"
#include "utl/Logger.h"

#ifdef HI_ACC_1
#define FRINGE_UP_DOWN
#endif
// #define CHECK_SAME_NET
// #define MIN_FOR_LOOPS

namespace rcx {

using utl::RCX;

using namespace odb;

bool extMeasureRC::measure_RC_new(extSegment* s, bool skip_res_calc)
{
  if (!measure_init(s))  // power net/via
    return false;

  // Resistance -------------------------------------------------
  dbRSeg* rseg1 = dbRSeg::getRSeg(_block, _rsegSrcId);
  if (!skip_res_calc) {
    calcRes(_rsegSrcId, s->_len, s->_dist, s->_dist_down, _met);
    for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
      double totR1 = _rc[jj]->_res;
      _extMain->updateRes(rseg1, totR1, jj);
    }
  }
  if (s->_down == NULL && s->_up == NULL) {
    _dist = -1;
    _rsegSrcId = s->_wire->getRsegId();
    _rsegTgtId = -1;

    dbRSeg* rseg1 = dbRSeg::getRSeg(_block, _rsegSrcId);
    computeAndStoreRC_new(rseg1, NULL, 0);
  }
  if (s->_down != NULL && s->_down->isPower()) {
    _netTgtId = s->_wire->getNet()->getId();
    _rsegTgtId = s->_wire->getRsegId();
    dbRSeg* rseg2 = dbRSeg::getRSeg(_block, _rsegTgtId);

    dbRSeg* rseg1 = NULL;
    if (s->_down->isPower())
      _rsegSrcId = -1;
    else {
      _rsegSrcId = s->_down->getRsegId();
      rseg1 = dbRSeg::getRSeg(_block, _rsegSrcId);
    }
    _dist = s->_dist_down;
    _diagResDist = s->_up != NULL ? s->_dist : -1;
    _diagResLen = s->_up != NULL ? s->_len : 0;

    computeAndStoreRC_new(rseg1, rseg2, 0);
  }
  if (s->_up != NULL) {
    // measure_init_cap(s, true);
    _netSrcId = s->_wire->getNet()->getId();
    _rsegSrcId = s->_wire->getRsegId();
    dbRSeg* rseg1 = dbRSeg::getRSeg(_block, _rsegSrcId);

    _dist = s->_dist;
    _diagResDist = s->_down != NULL ? s->_dist_down : -1;
    _diagResLen = s->_down != NULL ? s->_len : 0;

    _netTgtId = s->_up->getNet()->getId();
    dbRSeg* rseg2 = NULL;
    if (s->_up->isPower())
      _rsegTgtId = -1;
    else {
      _rsegTgtId = s->_up->getRsegId();
      if (_rsegTgtId > 0)
        rseg2 = dbRSeg::getRSeg(_block, _rsegTgtId);
    }
    computeAndStoreRC_new(rseg1, rseg2, 0);
  } else if (s->_down != NULL) {
    // Symmetric of looking up!!
    //  assume up is NULL
    // measure_init_cap(s, true);

    _netSrcId = s->_wire->getNet()->getId();
    _rsegSrcId = s->_wire->getRsegId();
    dbRSeg* rseg1 = dbRSeg::getRSeg(_block, _rsegSrcId);

    _dist = -1;
    _diagResDist = s->_dist_down;
    _diagResLen = _len;

    _netTgtId = -1;
    _rsegTgtId = -1;
    // dbRSeg *rseg2 = dbRSeg::getRSeg(_block, _rsegTgtId);
    dbRSeg* rseg2 = NULL;

    computeAndStoreRC_new(rseg1, rseg2, 0);
  }
  return true;
}
bool extMeasureRC::measure_init(extSegment* s)
{
  _currentSeg = s;
  _newDiagFlow = true;
  for (uint ii = 0; ii < 2; ii++) {
    _ll[ii] = s->_ll[ii];
    _ur[ii] = s->_ur[ii];
  }
  _len = s->_len;
  _dist = s->_up != NULL ? s->_dist : -1;
  _width = s->_width;
  _netSrcId = s->_wire->getNet()->getId();
  _rsegSrcId = s->_wire->getRsegId();

  _netTgtId = 0;
  if (s->_up != NULL) {
    _netTgtId = s->_up->getNet()->getId();
    _rsegTgtId = s->_up->getRsegId();
  }
  if (_rsegSrcId <= 0)  // cannot happen
    return false;

  _diagResDist = -1;
  _diagResLen = 0;
  if (s->_down != NULL) {
    _diagResDist = s->_dist_down;
    _diagResLen = s->_len;
  }

  _netId = _extMain->_debug_net_id;
  if (IsDebugNet()) {
    OpenDebugFile();
  }
  return true;
}
bool extMeasureRC::measure_init_cap(extSegment* s, bool up)
{
  for (uint ii = 0; ii < 2; ii++) {
    _ll[ii] = s->_ll[ii];
    _ur[ii] = s->_ur[ii];
  }
  _len = s->_len;
  _width = s->_width;

  if (up) {
    _rsegSrcId = s->_wire->getRsegId();
    if (s->_up != NULL) {
      _rsegTgtId = s->_up->getRsegId();
      // _r = dbRSeg::getRSeg(_block, _rsegSrcId);
      _dist = s->_dist;
    } else {
      _rsegTgtId = -1;
      _dist = -1;
    }
  } else {
    _rsegTgtId = s->_wire->getRsegId();
    if (s->_down != NULL) {
      _rsegSrcId = s->_down->getRsegId();
      _dist = s->_dist_down;
    } else {
      _rsegSrcId = -1;
      _dist = -1;
    }
  }
  return true;
}
bool extMeasureRC::measureRC_res_init(uint rsegId)
{
  if (IsDebugNet())  // TODO: debug
  {
    DebugStart_res(_debugFP);
    DebugStart_res(stdout);
  }
  if (rsegId == 0)
    return false;

  _rsegSrcId = rsegId;
  dbRSeg* rseg1 = dbRSeg::getRSeg(_block, _rsegSrcId);

  bool rvia1 = rseg1 != NULL && isVia(rseg1->getId());

  if (!(!rvia1 && rseg1 != NULL))
    return false;

  for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
    double totR1 = rseg1->getResistance(jj);
    if (totR1 > 0) {
      _extMain->updateRes(rseg1, -totR1, jj);
    }
  }
  return true;
}

uint extMeasureRC::CalcDiagBelow(extSegment* s, Wire* dw)
{
  if (dw == NULL)
    return 0;
  if (!dw->isPower())
    return 0;
  uint overMet = _met;
  // int rsegId=  dw->getRsegId();
  int rsegId = -1;

  uint below = 0;
  Ath__array1D<extSegment*> table(4);

  FindSegments(
      true, _dir, 1000 /* TODO */, s->_wire, s->_xy, s->_len, dw, &table);

  for (uint ii = 0; ii < table.getCnt(); ii++) {
    extSegment* s1 = table.get(ii);
    int diagDist = GetDistance(s->_wire, dw);  // TODO: might more
    _met = dw->getLevel();

    CalcDiag(overMet, diagDist, s1->_wire->getWidth(), s1->_len, s1, rsegId);
  }
  _met = overMet;
  return below;
}
uint extMeasureRC::CalcDiag(uint targetMet,
                            uint diagDist,
                            uint tgWidth,
                            uint len1,
                            extSegment* s,
                            int rsegId)
{
  DebugDiagCoords(_met, targetMet, len1, diagDist, s->_ll, s->_ur);
  if (IsDebugNet()) {
    DebugCoords(stdout, _rsegSrcId, _ll, _ur, "\t\tCalcDiag-new-flow");
    DebugCoords(_debugFP, _rsegSrcId, _ll, _ur, "\t\tCalcDiag-new-flow");
    DebugDiagCoords(stdout,
                    _met,
                    targetMet,
                    len1,
                    diagDist,
                    s->_ll,
                    s->_ur,
                    "\t\tCalcDiag-new-flow");
    DebugDiagCoords(_debugFP,
                    _met,
                    targetMet,
                    len1,
                    diagDist,
                    s->_ll,
                    s->_ur,
                    "\t\tCalcDiag-new-flow");
  }
  bool skip_high_acc = true;
#ifdef HI_ACC_1
  if (_dist < 0 && !skip_high_acc) {
    if (diagDist <= _width && diagDist >= 0 && (int) _width < 10 * _minWidth
        && _verticalDiag) {
      verticalCap(_rsegSrcId, rsegId, len1, tgWidth, diagDist, targetMet);
    } else if (((int) tgWidth > 10 * _minWidth)
               && ((int) _width > 10 * _minWidth)
               && (tgWidth >= 2 * diagDist)) {
      areaCap(_rsegSrcId, rsegId, len1, targetMet);
    } else if ((int) tgWidth > 2 * _minWidth && tgWidth >= 2 * diagDist) {
      calcDiagRC(_rsegSrcId, rsegId, len1, 1000000, targetMet);
    }
    // TO_OPTIMIZE
    else if (_diagModel == 2) {
      if (_verticalDiag)
        verticalCap(_rsegSrcId, rsegId, len1, tgWidth, diagDist, targetMet);
      else
        calcDiagRC(_rsegSrcId, rsegId, len1, tgWidth, diagDist, targetMet);
    }
    return len1;
  }
#endif
  int DIAG_MAX_DIST_FORCE = 10000000;
  // int DIAG_MAX_DIST_FORCE= 500;
  if (_diagModel == 2)
    calcDiagRC(_rsegSrcId, rsegId, len1, tgWidth, diagDist, targetMet);
  if (_diagModel == 1 && diagDist < DIAG_MAX_DIST_FORCE) {
    if (rsegId < 0)
      calcDiagRC(rsegId, _rsegSrcId, len1, diagDist, targetMet);
    else
      calcDiagRC(_rsegSrcId, rsegId, len1, diagDist, targetMet);
  }
  return len1;
}
}  // namespace rcx
