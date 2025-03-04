
///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, IC BENCH, Dimitris Fotakis
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

// DELETE #include <dbRtTree.h>

#include "dbUtil.h"
#include "rcx/extMeasureRC.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

#ifdef HI_ACC_1
#define FRINGE_UP_DOWN
#endif
// #define CHECK_SAME_NET
// #define MIN_FOR_LOOPS

namespace rcx {

using utl::RCX;
using namespace odb;

void extMeasureRC::ResetRCs()
{
  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    _rc[ii]->Reset();
  }
}
void extMeasureRC::measureRC_ids_flags(CoupleOptions& options)
{
  int rsegId1 = options[1];  // dbRSeg id for SRC segment
  int rsegId2 = options[2];  // dbRSeg id for Target segment

  _rsegSrcId = rsegId1;
  _rsegTgtId = rsegId2;

  defineBox(options);

  dbRSeg* rseg1 = NULL;
  dbNet* srcNet = NULL;
  uint netId1 = 0;
  if (rsegId1 > 0) {
    rseg1 = dbRSeg::getRSeg(_block, rsegId1);
    srcNet = rseg1->getNet();
    netId1 = srcNet->getId();
  }
  _netSrcId = netId1;

  dbRSeg* rseg2 = NULL;
  dbNet* tgtNet = NULL;
  uint netId2 = 0;
  if (rsegId2 > 0) {
    rseg2 = dbRSeg::getRSeg(_block, rsegId2);
    tgtNet = rseg2->getNet();
    netId2 = tgtNet->getId();
  }
  _sameNetFlag = (srcNet == tgtNet);
  _netTgtId = netId2;

  _verticalDiag = _currentModel->getVerticalDiagFlag();
  // DELETE int prevCovered = options[20];
  // DELETE prevCovered = 0;

  _netId = _extMain->_debug_net_id;
  if (IsDebugNet()) {
    OpenDebugFile();
    if (_debugFP != NULL)
      fprintf(_debugFP,
              "init_measureRC %d met= %d  len= %d  dist= %d r1= %d r2= %d\n",
              _totSignalSegCnt,
              _met,
              _len,
              _dist,
              rsegId1,
              rsegId2);
  }
}
bool extMeasureRC::updateCoupCap(dbRSeg* rseg1,
                                 dbRSeg* rseg2,
                                 int jj,
                                 double v,
                                 const char* dbg_msg)
{
  if (rseg1 != NULL && rseg2 != NULL) {
    dbCCSeg* ccap
        = dbCCSeg::create(dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
                          dbCapNode::getCapNode(_block, rseg2->getTargetNode()),
                          true);
    ccap->addCapacitance(v, jj);
    if (IsDebugNet()) {
      DebugUpdateCC(stdout,
                    dbg_msg,
                    rseg1->getId(),
                    rseg2->getId(),
                    _rc[jj]->_coupling,
                    ccap->getCapacitance());
      DebugUpdateCC(_debugFP,
                    dbg_msg,
                    rseg1->getId(),
                    rseg2->getId(),
                    _rc[jj]->_coupling,
                    ccap->getCapacitance());
    }
    return true;
  }
  if (rseg1 != NULL) {
    double tot = _extMain->updateTotalCap(rseg1, v, jj);
    if (IsDebugNet()) {
      DebugUpdateValue(stdout, dbg_msg, "CC-FR", rseg1->getId(), v, tot);
      DebugUpdateValue(_debugFP, dbg_msg, "CC-FR", rseg1->getId(), v, tot);
    }
  }
  if (rseg2 != NULL) {
    double tot = _extMain->updateTotalCap(rseg2, v, jj);
    if (IsDebugNet()) {
      DebugUpdateValue(stdout, dbg_msg, "CC-FR", rseg2->getId(), v, tot);
      DebugUpdateValue(_debugFP, dbg_msg, "CC-FR", rseg2->getId(), v, tot);
    }
  }

  return false;
}

extDistRC* extMeasureRC::getOverOpenRC_Dist(extMetRCTable* rcModel,
                                            uint width,
                                            int met,
                                            int metUnder,
                                            int dist)
{
  if (met >= (int) _layerCnt)
    return NULL;

  extDistRC* rc = NULL;
  if (dist < 0)  // TODO: single wire
    rc = rcModel->_capOver[met]->getFringeRC(metUnder, width);
  else
    rc = rcModel->_capOver_open[met][0]->getRC(metUnder, width, dist);

  return rc;
}
extDistRC* extMeasureRC::getOverRC_Dist(extMetRCTable* rcModel,
                                        uint width,
                                        int met,
                                        int metUnder,
                                        int dist,
                                        int open)
{
  if (met >= (int) _layerCnt)
    return NULL;

  extDistRC* rc = NULL;
  if (dist < 0)
    rc = rcModel->_capOver[met]->getFringeRC(metUnder, width);
  else if (open < 0)
    rc = rcModel->_capOver[met]->getRC(metUnder, width, dist);
  else if (open < 2)
    rc = rcModel->_capOver_open[met][open]->getRC(metUnder, width, dist);

  return rc;
}
float extMeasureRC::getOver_over1(extMetRCTable* rcModel,
                                  uint width,
                                  int met,
                                  int metUnder,
                                  int dist1,
                                  int dist2,
                                  int lenOverSub)
{
  _tmpRC->Reset();
  // lenOverSub not required.
  // for now dist1==diagResDist==200
  extDistRC* rc0 = getOverRC_Dist(rcModel, width, met, metUnder, dist1);
  extDistRC* rc = rcModel->_capOver_open[met][1]->getRC(metUnder, width, dist2);
  extDistRC* rc1 = getOverRC_Dist(rcModel, width, met, metUnder, dist2);

  float fr0 = 2 * rc->_fringe;

  // DBG    float delta_fr= (2*rc->_fringe - (rc0->_fringe + rc1->_fringe));
  // DBG    float fr1= rc1->_fringe;
  // wire is next to openended and no fringe was added -- no full fringe
  _tmpRC->_fringeW = 2 * rc->_fringe;
  _tmpRC->_fringe = rc1->_fringe;
  float cc = rc->_coupling * lenOverSub;
  // DBG  float cc0 = rc0->_coupling * lenOverSub;
  // DBG  float cc1 = rc1->_coupling * lenOverSub;
  float delta_cc
      = (2 * rc->_coupling - (rc0->_coupling + rc1->_coupling)) * lenOverSub;
  cc += delta_cc;
  _tmpRC->_coupling = rc1->_coupling;
  return fr0;
}
float extMeasureRC::getOU_over1(extMetRCTable* rcModel,
                                int lenOverSub,
                                int dist1,
                                int dist2)
{
  _tmpRC->Reset();
  // lenOverSub not required.
  // for now dist1==diagResDist==200
  extDistRC* rc0 = NULL;
  extDistRC* rc = NULL;
  extDistRC* rc1 = NULL;
  if (_overMet <= 0) {  // Over
    rc0 = getOverRC_Dist(rcModel, _width, _met, _underMet, dist1);
    rc = rcModel->_capOver_open[_met][1]->getRC(_underMet, _width, dist2);
    rc1 = getOverRC_Dist(rcModel, _width, _met, _underMet, dist2);
  } else if (_underMet <= 0) {  // Under
    rc0 = getUnderRC_Dist(rcModel, _width, _met, _overMet, dist1);
    rc = getUnderRC_Dist(rcModel, _width, _met, _overMet, dist2, 1);
    rc1 = getUnderRC_Dist(rcModel, _width, _met, _overMet, dist2);
  } else {  // Over Under
    rc0 = getOverUnderRC_Dist(
        rcModel, _width, _met, _underMet, _overMet, dist1);
    rc = getOverUnderRC_Dist(
        rcModel, _width, _met, _underMet, _overMet, dist2, 1);
    rc1 = getOverUnderRC_Dist(
        rcModel, _width, _met, _underMet, _overMet, dist2);
  }
  float fr0 = 2 * rc->_fringe;

  // fr= (2*rc->_fringe - rc0->_fringe) * lenOverSub;
  // DBG float delta_fr= (2*rc->_fringe - (rc0->_fringe + rc1->_fringe));
  // DBG float fr1= rc1->_fringe;
  // wire is next to openended and no fringe was added -- no full fringe
  _tmpRC->_fringeW = 2 * rc->_fringe;
  _tmpRC->_fringe = rc1->_fringe;
  float cc = rc->_coupling * lenOverSub;
  // DBG float cc0 = rc0->_coupling * lenOverSub;
  // DBG float cc1 = rc1->_coupling * lenOverSub;
  float delta_cc
      = (2 * rc->_coupling - (rc0->_coupling + rc1->_coupling)) * lenOverSub;
  cc += delta_cc;
  _tmpRC->_coupling = rc1->_coupling;
  return fr0;
}

float extMeasureRC::getOverR_weightedFringe(extMetRCTable* rcModel,
                                            uint width,
                                            int met,
                                            int metUnder,
                                            int dist1,
                                            int dist2)
{
  if (dist1 < 0 || dist2 < 0) {
    float openFr
        = getOverRC_Open(rcModel, _width, _met, metUnder, _dist, _diagResDist);
    return openFr;
  }
  /*
  if ((dist1>0 && dist2>0) && (dist1!=dist2) && (dist1==200)) {
    extDistRC* rc = rcModel->_capOver_open[met][1]->getRC(metUnder, width,
  _dist); float openFr= getOverRC_Open(rcModel, _width, _met, metUnder, _dist,
  _diagResDist); return openFr;
  }
*/
  bool useWeights = useWeightedAvg(dist1, dist2, metUnder);

  extDistRC* rc1 = getOverRC_Dist(rcModel, width, met, metUnder, dist1);
  extDistRC* rc2 = getOverRC_Dist(rcModel, width, met, metUnder, dist2);

  float fr = weightedFringe(rc1, rc2, useWeights);

  return fr;
}
void extDistRC::Reset()
{
  _fringe = 0;
  _fringeW = 0;
  _coupling = 0;
  _res = 0;
  _diag = 0;
  _sep = 0;
}
float extMeasureRC::getOverRC_Open(extMetRCTable* rcModel,
                                   uint width,
                                   int met,
                                   int metUnder,
                                   int dist1,
                                   int dist2)
{
  _tmpRC->Reset();
  if (dist1 < 0 && dist2 < 0) {
    return 0;  // TODO
  }
  if (dist2 < 0) {
    dist2 = dist1;
    dist1 = -1;
  }
  _tmpRC->Reset();
  extDistRC* rc1 = getOverOpenRC_Dist(rcModel, width, met, metUnder, dist2);
  if (rc1 == NULL)
    return 0;
  // TODO extDistRC* rc2= getOverRC_Dist(rcModel, width, met, metUnder, dist2);
  _tmpRC->_fringe = 2 * rc1->getFringe();
  _tmpRC->_coupling = 2 * rc1->getCoupling();
  float fr1 = 2 * rc1->getFringe();
  // TODO float fr2= rc2->getFringe();
  // TODO float fr= fr1 - fr2;

  return fr1;
}
float extMeasureRC::getOURC_Open(extMetRCTable* rcModel,
                                 uint width,
                                 int met,
                                 int metUnder,
                                 int metOver,
                                 int dist1,
                                 int dist2)
{
  _tmpRC->Reset();
  if (dist1 < 0 && dist2 < 0) {
    return 0;  // TODO
  }
  if (dist2 < 0) {
    dist2 = dist1;
    dist1 = -1;
  }
  _tmpRC->Reset();
  extDistRC* rc1 = NULL;
  // TODO extDistRC* rc2= NULL;
  if (metUnder <= 0) {  // Under pattern
    rc1 = getUnderRC_Dist(rcModel, width, met, metOver, dist2, 0);
    // TODO ?? rc2= getUnderRC_Dist(rcModel, width, met, metOver, dist2);
  } else {
    rc1 = getOverUnderRC_Dist(rcModel, width, met, metUnder, metOver, dist2, 0);
    // TODO ?? rc2= getOverUnderRC_Dist(rcModel, width, met, metUnder, metOver,
    // dist2);
  }
  if (rc1 == NULL)
    return 0;

  _tmpRC->_fringe = 2 * rc1->getFringe();
  _tmpRC->_coupling = 2 * rc1->getCoupling();
  float fr1 = 2 * rc1->getFringe();
  // TODO ?? float fr2= rc2->getFringe();
  // TODO ?? float fr= fr1 - fr2;

  return fr1;
}
extDistRC* extMeasureRC::getUnderRC_Dist(extMetRCTable* rcModel,
                                         uint width,
                                         int met,
                                         int overMet,
                                         int dist,
                                         int open)
{
  if (rcModel->_capUnder[met] == NULL)
    return NULL;

  uint n = overMet - met - 1;

  extDistRC* rc = NULL;
  if (dist < 0)
    rc = rcModel->_capUnder[met]->getFringeRC(n, width);
  else if (open < 0)
    rc = rcModel->_capUnder[met]->getRC(n, width, dist);
  else if (open < 2 && rcModel->_capUnder_open != NULL
           && rcModel->_capUnder_open[met][open] != NULL)
    rc = rcModel->_capUnder_open[met][open]->getRC(n, width, dist);
  return rc;
}
float extMeasureRC::getUnderRC_weightedFringe(extMetRCTable* rcModel,
                                              uint width,
                                              int met,
                                              int metOver,
                                              int dist1,
                                              int dist2)
{
  if (dist1 < 0 || dist2 < 0) {
    float openFr = getOURC_Open(rcModel, width, met, 0, metOver, dist1, dist2);
    return openFr;
  }
  int minDist = dist1;
  int maxDist = dist2;
  if (dist1 > dist2) {
    minDist = dist2;
    maxDist = dist1;
  }
  extDistRC* rc1 = getUnderRC_Dist(rcModel, width, met, metOver, minDist);
  extDistRC* rc2 = getUnderRC_Dist(rcModel, width, met, metOver, maxDist);

  float fr = weightedFringe(rc1, rc2);

  return fr;
}
int extMeasureRC::getMetIndexOverUnder(int met,
                                       int mUnder,
                                       int mOver,
                                       int layerCnt,
                                       int maxCnt)
{
  int n = layerCnt - met - 1;
  n *= mUnder - 1;
  n += mOver - met - 1;

  if ((n < 0) || (n >= maxCnt)) {
    return -1;
  }

  return n;
}
extDistRC* extMeasureRC::getOverUnderRC_Dist(extMetRCTable* rcModel,
                                             int width,
                                             int met,
                                             int underMet,
                                             int overMet,
                                             int dist,
                                             int open)
{
  uint maxCnt = _currentModel->getMaxCnt(met);
  int n = extMeasureRC::getMetIndexOverUnder(
      met, underMet, overMet, _layerCnt, maxCnt);

  extDistRC* rc = NULL;
  if (dist < 0)
    rc = rcModel->_capOverUnder[met]->getFringeRC(n, width);
  else if (open < 0)
    rc = rcModel->_capOverUnder[met]->getRC(n, width, dist);
  else if ((open < 2) && (rcModel->_capOverUnder_open != NULL)
           && rcModel->_capOverUnder_open[met][open] != NULL)
    rc = rcModel->_capOverUnder_open[met][open]->getRC(n, width, dist);

  return rc;
}
/* TODO
extDistRC* extMeasure::computeOverUnderRC(extMetRCTable* rcModel, uint len)
{
  if (_dist<0 || _diagResDist<0) { // Model0 - openended
    float openFr= getOURC_Open(rcModel, _width, _met, _underMet, _overMet,
_dist, _diagResDist); return _tmpRC; } else if ((_dist>0 && _diagResDist>0) &&
_diagResDist==200 && _dist!=_diagResDist) { // Model1 float FR=
getOver_over1(rcModel, _width, _met, _underMet, _diagResDist, _dist, len);
      addRC_new(_tmpRC, len, ii, true);
      _rc[ii]->_fringeW += _tmpRC->_fringeW * len;
  } else if ((_dist>0 && _diagResDist>0) && _dist!=_diagResDist) { // add
weighted avg frw= getOverUnderRC_weightedFringe(rcModel, _width, _met,
_underMet, _overMet, _dist, _diagResDist);
  }
  else {
    extDistRC* rcUnit= getOverUnderRC(rcModel);
  }


    double frw= 0;
  //  if (_diagResDist>0)
      frw= getOverUnderRC_weightedFringe(rcModel, _width, _met, _underMet,
_overMet, _dist, _diagResDist); if (_dist<0 || _diagResDist<0) addRC_new(_tmpRC,
len, ii, _dist>0 || _diagResDist>0); else if ((_dist>0 && _diagResDist>0) &&
_diagResDist==200 && _dist!=_diagResDist) { float FR= getOver_over1(rcModel,
_width, _met, _underMet, _diagResDist, _dist, len); addRC_new(_tmpRC, len, ii,
true); _rc[ii]->_fringeW += _tmpRC->_fringeW * len; } else { addRC(rcUnit, len,
ii, frw);
    }

  return rcUnit;
}
*/
float extMeasureRC::getOverUnderRC_weightedFringe(extMetRCTable* rcModel,
                                                  uint width,
                                                  int met,
                                                  int underMet,
                                                  int metOver,
                                                  int dist1,
                                                  int dist2)
{
  if (dist1 < 0 || dist2 < 0) {
    float openFr
        = getOURC_Open(rcModel, width, met, underMet, metOver, dist1, dist2);
    return openFr;
  }
  int minDist = dist1;
  int maxDist = dist2;
  if (dist1 > dist2) {
    minDist = dist2;
    maxDist = dist1;
  }
  extDistRC* rc1
      = getOverUnderRC_Dist(rcModel, width, met, underMet, metOver, minDist);
  extDistRC* rc2
      = getOverUnderRC_Dist(rcModel, width, met, underMet, metOver, maxDist);

  float fr = weightedFringe(rc1, rc2);
  return fr;
}
float extMeasureRC::weightedFringe(extDistRC* rc1,
                                   extDistRC* rc2,
                                   bool use_weighted)
{
  // if (!_useWeighted)
  //  return 0;

  if (rc1 == NULL || rc2 == NULL)
    return 0;

  float fr1 = rc1->getFringe();
  float fr2 = rc2->getFringe();

  float fr = fr1 + fr2;
  if (use_weighted) {
    int ratio = (int) ceil(fr2 / fr1);
    float wfr = (fr2 * ratio + fr1) / (ratio + 1);
    if (fr1 > fr2) {
      ratio = (int) ceil(fr1 / fr2);
      wfr = (fr1 * ratio + fr2) / (ratio + 1);
    }
    wfr *= 2;
    if (_useWeighted)
      return wfr;
    float delta = wfr - fr;
    if (delta > 0) {
      return delta;
      int dist_ratio = rc2->_sep / rc1->_sep;
      if (dist_ratio > 3)
        fr = delta;
      else
        fr = delta / 2;

      // fr= 0.75 * delta;
      // fr= 0.25 * delta;
      return fr;
      // _useWeighted= true;
    } else {
      return 0;
    }
  } else {
    return 0;
  }

  return fr;
}
bool extMeasureRC::useWeightedAvg(int& dist1, int& dist2, int underMet)
{
  // only used by Over pattern
  if (dist1 > 0 && dist2 == dist1)
    return false;

  int minDist = dist1;
  int maxDist = dist2;
  if (dist1 > dist2) {
    minDist = dist2;
    maxDist = dist1;
  }
  dist1 = minDist;
  dist2 = maxDist;

  // if ((dist1<0 || dist2<0) && underMet>0)
  if (dist1 < 0 || dist2 < 0)
    return false;

  return true;
}
}  // namespace rcx
