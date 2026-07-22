// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <cstdint>
#include <cstdio>

#include "odb/db.h"
#include "rcx/array1.h"
#include "rcx/dbUtil.h"
#include "rcx/extMeasureRC.h"
#include "rcx/extRCap.h"
#include "rcx/extSegment.h"
#include "utl/Logger.h"

#ifdef HI_ACC_1
#define FRINGE_UP_DOWN
#endif

namespace rcx {

void extMeasureRC::VerticalCap(Array1D<extSegment*>* segTable, bool look_up)
{
  for (uint32_t ii = 0; ii < segTable->getCnt(); ii++) {
    extSegment* s = segTable->get(0);
    Wire* w2 = look_up ? s->_up : s->_down;

    if (w2 == nullptr) {
      continue;
    }
    uint32_t rsegId1 = s->_wire->getRsegId();
    uint32_t rsegId2 = w2->getRsegId();

    uint32_t met = s->_wire->getLevel();
    uint32_t tgtMet = w2->getLevel();
    uint32_t tgtWidth = w2->getWidth();

    uint32_t width = s->_width;
    uint32_t diagDist = 0;

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
                           uint32_t maxDist,
                           uint32_t trackLimitCnt,
                           Array1D<extSegment*>* segTable,
                           bool PowerOnly)
{
  bool no_dist_limit = false;
  if (PowerOnly) {
    no_dist_limit = true;
  }

  uint32_t met = w->getLevel();
  _width = w->getWidth();
  uint32_t rsegId1 = w->getRsegId();

  uint32_t pitch = w->getPitch();
  for (uint32_t ii = 0; ii < segTable->getCnt(); ii++) {
    extSegment* s = segTable->get(0);
    Wire* w2 = lookUp ? s->_up : s->_down;
    if (w2 == nullptr) {
      continue;
    }
    if (PowerOnly && !w2->isPower()) {
      continue;
    }
    int dist = lookUp ? s->_dist : s->_dist_down;

    uint32_t tgtMet = w2->getLevel();

    if (!no_dist_limit) {
      if (tgtMet - met < 2) {
        if (dist > pitch) {
          continue;
        }
      } else {
        if (dist > pitch) {
          continue;
        }
      }
    }
    uint32_t tgtWidth = w2->getWidth();
    uint32_t rsegId2 = w2->getRsegId();

    if (DiagCouplingCap(
            met, tgtMet, rsegId2, rsegId1, s->_len, _width, tgtWidth, dist)) {
      // print
    }
  }
  return true;
}
odb::dbRSeg* extMeasureRC::GetRseg(int id)
{
  odb::dbRSeg* rseg1 = id > 0 ? odb::dbRSeg::getRSeg(_block, id) : nullptr;
  return rseg1;
}
bool extMeasureRC::VerticalCap(uint32_t met,
                               uint32_t tgtMet,
                               int rsegId1,
                               uint32_t rsegId2,
                               uint32_t len,
                               uint32_t width,
                               uint32_t tgtWidth,
                               uint32_t diagDist)
{
  // assumption: signal-2-signal, no power
  // dbRSeg* rseg2 = GetRseg(rsegId2);
  // dbRSeg* rseg1 = GetRseg(rsegId1);

  _width = width;
  double capTable[10];
  uint32_t modelCnt = _metRCTable.getCnt();
  for (uint32_t ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    // NOT working extDistRC* rc = getVerticalUnderRC(rcModel, diagDist,
    // tgtWidth, tgtMet);
    extDistRC* rc = getDiagUnderCC(rcModel, diagDist, tgtMet);
    if (rc == nullptr) {
      return false;
    }

    capTable[ii] = len * rc->getFringe();
  }
  createCap(rsegId1, rsegId2, capTable);
  return true;
}
bool extMeasureRC::DiagCouplingCap(uint32_t met,
                                   uint32_t tgtMet,
                                   int rsegId1,
                                   uint32_t rsegId2,
                                   uint32_t len,
                                   uint32_t width,
                                   uint32_t tgtWidth,
                                   uint32_t diagDist)
{
  // assumption: signal-2-signal, no power
  // dbRSeg* rseg2 = GetRseg(rsegId2);
  // dbRSeg* rseg1 = GetRseg(rsegId1);

  _width = width;
  double capTable[10];
  uint32_t modelCnt = _metRCTable.getCnt();
  for (uint32_t ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);

    // NOT working extDistRC* rc = getVerticalUnderRC(rcModel, diagDist,
    // tgtWidth, tgtMet);
    if (tgtMet < met) {
      _met = tgtMet;
      tgtMet = met;
    }
    extDistRC* rc = getDiagUnderCC(rcModel, diagDist, tgtMet);
    if (rc == nullptr) {
      return false;
    }

    capTable[ii]
        = len * rc->getFringe();  // OVERLOADED value from model - dkf 110424
  }
  createCap(rsegId1, rsegId2, capTable);
  return true;
}

}  // namespace rcx
