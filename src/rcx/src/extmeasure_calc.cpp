
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
#include "rcx/extSegment.h"
#include "utl/Logger.h"

#ifdef HI_ACC_1
#define FRINGE_UP_DOWN
#endif
// #define CHECK_SAME_NET
// #define MIN_FOR_LOOPS

namespace rcx {
using utl::RCX;

void extMeasureRC::VerticalCap(Ath__array1D<extSegment*>* segTable,
                               bool look_up)
{
  for (uint ii = 0; ii < segTable->getCnt(); ii++) {
    extSegment* s = segTable->get(0);
    Wire* w2 = look_up ? s->_up : s->_down;

    if (w2 == NULL)
      continue;
    uint rsegId1 = s->_wire->getRsegId();
    uint rsegId2 = w2->getRsegId();

    uint met = s->_wire->getLevel();
    uint tgtMet = w2->getLevel();
    uint tgtWidth = w2->getWidth();

    uint width = s->_width;
    uint diagDist = 0;

    if (VerticalCap(met,
                    tgtMet,
                    rsegId2,
                    rsegId1,
                    s->_len,
                    width,
                    tgtWidth,
                    diagDist)) {
      // print
    }
  }
}
bool extMeasureRC::DiagCap(FILE* fp,
                           Wire* w,
                           bool lookUp,
                           uint maxDist,
                           uint trackLimitCnt,
                           Ath__array1D<extSegment*>* segTable,
                           bool PowerOnly)
{
  bool no_dist_limit = false;
  if (PowerOnly)
    no_dist_limit = true;

  uint met = w->getLevel();
  _width = w->getWidth();
  uint rsegId1 = w->getRsegId();

  uint pitch = w->getPitch();
  maxDist = trackLimitCnt * pitch;
  for (uint ii = 0; ii < segTable->getCnt(); ii++) {
    extSegment* s = segTable->get(0);
    Wire* w2 = lookUp ? s->_up : s->_down;
    if (w2 == NULL)
      continue;
    if (PowerOnly && !w2->isPower())
      continue;
    int dist = lookUp ? s->_dist : s->_dist_down;

    uint tgtMet = w2->getLevel();

    if (!no_dist_limit) {
      if (tgtMet - met < 2) {
        if (dist > pitch)
          continue;
      } else {
        if (dist > pitch)
          continue;
      }
    }
    uint tgtWidth = w2->getWidth();
    uint rsegId2 = w2->getRsegId();

    if (DiagCouplingCap(
            met, tgtMet, rsegId2, rsegId1, s->_len, _width, tgtWidth, dist)) {
      // print
    }
  }
  return true;
}
dbRSeg* extMeasureRC::GetRseg(int id)
{
  dbRSeg* rseg1 = id > 0 ? dbRSeg::getRSeg(_block, id) : NULL;
  return rseg1;
}
bool extMeasureRC::VerticalCap(uint met,
                               uint tgtMet,
                               int rsegId1,
                               uint rsegId2,
                               uint len,
                               uint width,
                               uint tgtWidth,
                               uint diagDist)
{
  // assumption: signal-2-signal, no power
  // dbRSeg* rseg2 = GetRseg(rsegId2);
  // dbRSeg* rseg1 = GetRseg(rsegId1);

  _width = width;
  double capTable[10];
  uint modelCnt = _metRCTable.getCnt();
  for (uint ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    // NOT working extDistRC* rc = getVerticalUnderRC(rcModel, diagDist,
    // tgtWidth, tgtMet);
    extDistRC* rc = getDiagUnderCC(rcModel, diagDist, tgtMet);
    if (rc == NULL)
      return false;

    capTable[ii] = len * rc->_fringe;
  }
  createCap(rsegId1, rsegId2, capTable);
  return true;
}
bool extMeasureRC::DiagCouplingCap(uint met,
                                   uint tgtMet,
                                   int rsegId1,
                                   uint rsegId2,
                                   uint len,
                                   uint width,
                                   uint tgtWidth,
                                   uint diagDist)
{
  // assumption: signal-2-signal, no power
  // dbRSeg* rseg2 = GetRseg(rsegId2);
  // dbRSeg* rseg1 = GetRseg(rsegId1);

  _width = width;
  double capTable[10];
  uint modelCnt = _metRCTable.getCnt();
  for (uint ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    // NOT working extDistRC* rc = getVerticalUnderRC(rcModel, diagDist,
    // tgtWidth, tgtMet);
    if (tgtMet < met) {
      _met = tgtMet;
      tgtMet = met;
    }
    extDistRC* rc = getDiagUnderCC(rcModel, diagDist, tgtMet);
    if (rc == NULL)
      return false;

    capTable[ii]
        = len * rc->_fringe;  // OVERLOADED value from model - dkf 110424
  }
  createCap(rsegId1, rsegId2, capTable);
  return true;
}

}  // namespace rcx
