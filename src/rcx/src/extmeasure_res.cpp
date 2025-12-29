// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cassert>
#include <cstdint>

#include "gseq.h"
#include "rcx/array1.h"
#include "rcx/dbUtil.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

int extMeasure::computeResDist(SEQ* s,
                               uint32_t trackMin,
                               uint32_t trackMax,
                               uint32_t targetMet,
                               Array1D<SEQ*>* diagTable)
{
  int trackDist = 4;
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
    if (-kk >= _dgContextLowTrack[planeIndex]) {
      trackTable[cnt++] = *_dgContextTracks / 2 - kk;
    }
  }

  for (uint32_t ii = 0; ii < cnt; ii++) {
    int trackn = trackTable[ii];

    if (_dgContextArray[planeIndex][trackn]->getCnt() <= 1) {
      continue;
    }
    // Check for same track
    bool same_track = false;
    Array1D<SEQ*>* dTable = _dgContextArray[planeIndex][trackn];
    int cnt = (int) dTable->getCnt();
    for (uint32_t idx = 1; idx < (int) cnt; idx++) {
      SEQ* tseq = dTable->get(idx);
      if (tseq->type == _rsegSrcId) {
        same_track = true;
        break;
      }
    }
    if (same_track) {
      continue;
    }

    bool add_all_diag = false;
    if (!add_all_diag) {
      for (uint32_t jj = 0; jj < tmpTable.getCnt(); jj++) {
        len += computeRes(tmpTable.get(jj),
                          targetMet,
                          _dir,
                          planeIndex,
                          trackn,
                          &residueTable);
      }
    } else {
      len += computeRes(s, targetMet, _dir, planeIndex, trackn, &residueTable);
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

uint32_t extMeasure::computeRes(SEQ* s,
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
  getDgOverlap_res(s, _dir, dgContext, &overlapSeq, residueSeq);

  uint32_t len = 0;
  for (uint32_t jj = 0; jj < overlapSeq.getCnt(); jj++) {
    SEQ* tgt = overlapSeq.get(jj);
    uint32_t diagDist = calcDist(tgt->_ll, tgt->_ur);
    uint32_t len1 = getLength(tgt, !_dir);

    len += len1;
    calcRes(_rsegSrcId, len1, _dist, diagDist, _met);
  }
  seq_release(&overlapSeq);
  return len;
}

int extMeasure::getMaxDist(int tgtMet, uint32_t modelIndex)
{
  uint32_t modelCnt = _metRCTable.getCnt();
  if (modelCnt == 0) {
    return -1;
  }

  extMetRCTable* rcModel = _metRCTable.get(modelIndex);
  if (rcModel->_resOver[tgtMet] == nullptr) {
    return -1;
  }

  extDistRCTable* distTable
      = rcModel->_resOver[tgtMet]->getRuleTable(0, _width);

  int maxDist = distTable->getComputeRC_maxDist();
  return maxDist;
}

int extDistRCTable::getComputeRC_maxDist()
{
  return maxDist_;
}

void extMeasure::calcRes(int rsegId1,
                         uint32_t len,
                         int dist1,
                         int dist2,
                         int tgtMet)
{
  if (dist1 == -1) {
    dist1 = 0;
  }
  if (dist2 == -1) {
    dist1 = 0;
  }
  int min_dist = 0;
  if (dist1 > dist2) {
    min_dist = dist1;
    dist1 = dist2;
    dist2 = min_dist;
  }
  uint32_t modelCnt = _metRCTable.getCnt();
  for (uint32_t ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    if (rcModel->_resOver[tgtMet] == nullptr) {
      continue;
    }

    extDistRC* rc = rcModel->_resOver[tgtMet]->getRes(0, _width, dist1, dist2);
    if (rc != nullptr) {
      double R1 = rc->diag_ > 0 ? rc->diag_ : rc->res_;
      double R = len * R1;
      _rc[ii]->res_ += R;
    }
  }
}

void extMeasure::calcRes0(double* deltaRes,
                          uint32_t tgtMet,
                          uint32_t len,
                          int dist1,
                          int dist2)
{
  uint32_t modelCnt = _metRCTable.getCnt();
  for (uint32_t ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    if (rcModel->_resOver[tgtMet] == nullptr) {
      continue;
    }

    extDistRC* rc = rcModel->_resOver[tgtMet]->getRes(0, _width, dist1, dist2);
    double R = len * rc->res_;
    deltaRes[ii] = R;
  }
}

void extMain::calcRes0(double* deltaRes,
                       uint32_t tgtMet,
                       uint32_t width,
                       uint32_t len,
                       int dist1,
                       int dist2)
{
  for (uint32_t jj = 0; jj < _modelMap.getCnt(); jj++) {
    uint32_t modelIndex = _modelMap.get(jj);
    extMetRCTable* rcModel = _currentModel->getMetRCTable(modelIndex);

    if (rcModel->_resOver[tgtMet] == nullptr) {
      continue;
    }

    extDistRC* rc = rcModel->_resOver[tgtMet]->getRes(0, width, dist1, dist2);
    if (rc == nullptr) {
      continue;
    }
    double R = len * rc->res_;
    deltaRes[jj] = R;
  }
}

extDistRC* extDistRCTable::getComputeRC_res(uint32_t dist1, uint32_t dist2)
{
  int min_dist = 0;
  if (dist1 > dist2) {
    min_dist = dist1;
    dist1 = dist2;
    dist2 = min_dist;
  }
  if (measureTable_ == nullptr) {
    return nullptr;
  }

  if (measureTable_->getCnt() <= 0) {
    return nullptr;
  }

  extDistRC* rc1 = measureTableR_[0]->geti(0);
  rc1->diag_ = 0.0;
  if (rc1 == nullptr) {
    return nullptr;
  }

  if (dist1 + dist2 == 0) {  // ASSUMPTION: 0 dist exists as first
    return rc1;
  }
  if (dist1 >= maxDist_ && dist2 >= maxDist_) {
    return nullptr;
  }
  if (dist2 > maxDist_) {
    dist2 = dist1;
    dist1 = 0;
  }

  uint32_t index_dist = 0;
  bool found = false;
  extDistRC* rc2 = measureTableR_[1]->geti(0);
  if (rc2 == nullptr) {
    return rc1;
  }

  if (dist1 <= rc1->sep_) {
    index_dist = 0;
    found = true;
  } else if (dist1 < rc2->sep_) {
    index_dist = 0;
    found = true;
  } else if (dist1 == rc2->sep_) {
    index_dist = 1;
    found = true;
  } else {
    index_dist = 2;
  }
  extDistRC* rc = nullptr;
  if (!found) {
    // find first dist

    uint32_t ii;
    for (ii = index_dist; ii < distCnt_; ii++) {
      rc = measureTableR_[ii]->geti(0);
      if (dist1 == rc->sep_) {
        found = true;
        index_dist = ii;
        break;
      }
      if (dist1 < rc->sep_) {
        found = true;
        index_dist = ii;
        break;
      }
    }
    if (!found && ii == distCnt_) {
      index_dist = ii - 1;
      found = true;
    }
  }
  if (found) {
    if (!measureInR_) {
      delete measureTable_;
    }
    measureInR_ = true;
    measureTable_ = measureTableR_[index_dist];
    computeTable_ = computeTableR_[index_dist];
    extDistRC* res = findIndexed_res(dist1, dist2);
    res->diag_ = 0;
    if (rc != nullptr && dist1 < rc->sep_) {
      measureTable_ = measureTableR_[index_dist - 1];
      computeTable_ = computeTableR_[index_dist - 1];
      extDistRC* res1 = findIndexed_res(dist1, dist2);
      double R1 = res->interpolate_res(dist1, res1);
      res1->diag_ = R1;
      return res1;
    }
    return res;
  }
  return nullptr;
}

extDistRC* extDistRCTable::findIndexed_res(uint32_t dist1, uint32_t dist2)
{
  extDistRC* firstRC = measureTable_->get(0);
  uint32_t firstDist = firstRC->sep_;
  if (dist2 <= firstDist) {
    return firstRC;
  }
  if (measureTable_->getCnt() == 1) {
    return firstRC;
  }
  extDistRC* resLast = measureTable_->getLast();
  if (dist2 >= resLast->sep_) {
    return resLast;
  }

  uint32_t n = dist2 / unit_;
  extDistRC* res = computeTable_->geti(n);
  return res;
}

extDistRC* extDistWidthRCTable::getRes(uint32_t mou,
                                       uint32_t w,
                                       int dist1,
                                       int dist2)
{
  int wIndex = getWidthIndex(w);
  if (wIndex < 0) {
    return nullptr;
  }

  extDistRC* rc = _rcDistTable[mou][wIndex]->getComputeRC_res(dist1, dist2);

  return rc;
}

extDistRCTable* extDistWidthRCTable::getRuleTable(uint32_t mou, uint32_t w)
{
  int wIndex = getWidthIndex(w);
  if (wIndex < 0) {
    return nullptr;
  }

  return _rcDistTable[mou][wIndex];
}

void extMeasure::getDgOverlap_res(SEQ* sseq,
                                  uint32_t dir,
                                  Array1D<SEQ*>* dgContext,
                                  Array1D<SEQ*>* overlapSeq,
                                  Array1D<SEQ*>* residueSeq)
{
  int idx = 1;
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

  for (; idx < (int) dgContext->getCnt(); idx++) {
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
    if (idx == (int) dgContext->getCnt() - 1 && covered < sseq->_ur[lp]) {
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
  if (idx == dgContext->getCnt() && residueSeq->getCnt() == 0) {
    rseq = _seqPool->alloc();
    rseq->_ll[wp] = sseq->_ll[wp];
    rseq->_ur[wp] = sseq->_ur[wp];
    rseq->_ll[lp] = sseq->_ll[lp];
    rseq->_ur[lp] = sseq->_ur[lp];
    residueSeq->add(rseq);
  }
}

}  // namespace rcx
