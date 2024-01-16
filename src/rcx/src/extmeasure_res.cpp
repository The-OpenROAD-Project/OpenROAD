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
#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

using odb::SEQ;

int extMeasure::computeResDist(SEQ* s,
                               uint trackMin,
                               uint trackMax,
                               uint targetMet,
                               Ath__array1D<SEQ*>* diagTable)
{
  int trackDist = 4;
  int loTrack;
  int hiTrack;
  int planeIndex
      = getDgPlaneAndTrackIndex(targetMet, trackDist, loTrack, hiTrack);
  if (planeIndex < 0) {
    return 0;
  }

  uint len = 0;

  Ath__array1D<SEQ*> tmpTable(16);
  copySeqUsingPool(s, &tmpTable);

  Ath__array1D<SEQ*> residueTable(16);

  int trackTable[200];
  uint cnt = 0;
  for (int kk = (int) trackMin; kk <= (int) trackMax;
       kk++)  // skip overlapping track
  {
    if (-kk >= _dgContextLowTrack[planeIndex]) {
      trackTable[cnt++] = *_dgContextTracks / 2 - kk;
    }
  }

  for (uint ii = 0; ii < cnt; ii++) {
    int trackn = trackTable[ii];

    if (_dgContextArray[planeIndex][trackn]->getCnt() <= 1) {
      continue;
    }
    // Check for same track
    bool same_track = false;
    Ath__array1D<SEQ*>* dTable = _dgContextArray[planeIndex][trackn];
    int cnt = (int) dTable->getCnt();
    for (uint idx = 1; idx < (int) cnt; idx++) {
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
      for (uint jj = 0; jj < tmpTable.getCnt(); jj++) {
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

uint extMeasure::computeRes(SEQ* s,
                            uint targetMet,
                            uint dir,
                            uint planeIndex,
                            uint trackn,
                            Ath__array1D<SEQ*>* residueSeq)
{
  Ath__array1D<SEQ*>* dgContext = _dgContextArray[planeIndex][trackn];
  if (dgContext->getCnt() <= 1) {
    return 0;
  }

  Ath__array1D<SEQ*> overlapSeq(16);
  getDgOverlap_res(s, _dir, dgContext, &overlapSeq, residueSeq);

  uint len = 0;
  for (uint jj = 0; jj < overlapSeq.getCnt(); jj++) {
    SEQ* tgt = overlapSeq.get(jj);
    uint diagDist = calcDist(tgt->_ll, tgt->_ur);
    uint len1 = getLength(tgt, !_dir);

    DebugDiagCoords(_met, targetMet, len1, diagDist, tgt->_ll, tgt->_ur);
    len += len1;
    calcRes(_rsegSrcId, len1, _dist, diagDist, _met);
  }
  seq_release(&overlapSeq);
  return len;
}

int extMeasure::getMaxDist(int tgtMet, uint modelIndex)
{
  uint modelCnt = _metRCTable.getCnt();
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
  return _maxDist;
}

void extMeasure::calcRes(int rsegId1,
                         uint len,
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
  uint modelCnt = _metRCTable.getCnt();
  for (uint ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    if (rcModel->_resOver[tgtMet] == nullptr) {
      continue;
    }

    extDistRC* rc = rcModel->_resOver[tgtMet]->getRes(0, _width, dist1, dist2);
    if (rc != nullptr) {
      double R1 = rc->_diag > 0 ? rc->_diag : rc->_res;
      double R = len * R1;
      _rc[ii]->_res += R;
    }
  }
}

void extMeasure::calcRes0(double* deltaRes,
                          uint tgtMet,
                          uint len,
                          int dist1,
                          int dist2)
{
  uint modelCnt = _metRCTable.getCnt();
  for (uint ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    if (rcModel->_resOver[tgtMet] == nullptr) {
      continue;
    }

    extDistRC* rc = rcModel->_resOver[tgtMet]->getRes(0, _width, dist1, dist2);
    double R = len * rc->_res;
    deltaRes[ii] = R;
  }
}

void extMain::calcRes0(double* deltaRes,
                       uint tgtMet,
                       uint width,
                       uint len,
                       int dist1,
                       int dist2)
{
  for (uint jj = 0; jj < _modelMap.getCnt(); jj++) {
    uint modelIndex = _modelMap.get(jj);
    extMetRCTable* rcModel = _currentModel->getMetRCTable(modelIndex);

    if (rcModel->_resOver[tgtMet] == nullptr) {
      continue;
    }

    extDistRC* rc = rcModel->_resOver[tgtMet]->getRes(0, width, dist1, dist2);
    if (rc == nullptr) {
      continue;
    }
    double R = len * rc->_res;
    deltaRes[jj] = R;
  }
}

extDistRC* extDistRCTable::getComputeRC_res(uint dist1, uint dist2)
{
  int min_dist = 0;
  if (dist1 > dist2) {
    min_dist = dist1;
    dist1 = dist2;
    dist2 = min_dist;
  }
  if (_measureTable == nullptr) {
    return nullptr;
  }

  if (_measureTable->getCnt() <= 0) {
    return nullptr;
  }

  extDistRC* rc1 = _measureTableR[0]->geti(0);
  rc1->_diag = 0.0;
  if (rc1 == nullptr) {
    return nullptr;
  }

  if (dist1 + dist2 == 0) {  // ASSUMPTION: 0 dist exists as first
    return rc1;
  }
  if (dist1 >= _maxDist && dist2 >= _maxDist) {
    return nullptr;
  }
  if (dist2 > _maxDist) {
    dist2 = dist1;
    dist1 = 0;
  }

  uint index_dist = 0;
  bool found = false;
  extDistRC* rc2 = _measureTableR[1]->geti(0);
  if (rc2 == nullptr) {
    return rc1;
  }

  if (dist1 <= rc1->_sep) {
    index_dist = 0;
    found = true;
  } else if (dist1 < rc2->_sep) {
    index_dist = 0;
    found = true;
  } else if (dist1 == rc2->_sep) {
    index_dist = 1;
    found = true;
  } else {
    index_dist = 2;
  }
  extDistRC* rc = nullptr;
  if (!found) {
    // find first dist

    uint ii;
    for (ii = index_dist; ii < _distCnt; ii++) {
      rc = _measureTableR[ii]->geti(0);
      if (dist1 == rc->_sep) {
        found = true;
        index_dist = ii;
        break;
      }
      if (dist1 < rc->_sep) {
        found = true;
        index_dist = ii;
        break;
      }
    }
    if (!found && ii == _distCnt) {
      index_dist = ii - 1;
      found = true;
    }
  }
  if (found) {
    _measureTable = _measureTableR[index_dist];
    _computeTable = _computeTableR[index_dist];
    extDistRC* res = findIndexed_res(dist1, dist2);
    res->_diag = 0;
    if (rc != nullptr && dist1 < rc->_sep) {
      _measureTable = _measureTableR[index_dist - 1];
      _computeTable = _computeTableR[index_dist - 1];
      extDistRC* res1 = findIndexed_res(dist1, dist2);
      double R1 = res->interpolate_res(dist1, res1);
      res1->_diag = R1;
      return res1;
    }
    return res;
  }
  return nullptr;
}

extDistRC* extDistRCTable::findIndexed_res(uint dist1, uint dist2)
{
  extDistRC* firstRC = _measureTable->get(0);
  uint firstDist = firstRC->_sep;
  if (dist2 <= firstDist) {
    return firstRC;
  }
  if (_measureTable->getCnt() == 1) {
    return firstRC;
  }
  extDistRC* resLast = _measureTable->getLast();
  if (dist2 >= resLast->_sep) {
    return resLast;
  }

  uint n = dist2 / _unit;
  extDistRC* res = _computeTable->geti(n);
  return res;
}

extDistRC* extDistWidthRCTable::getRes(uint mou, uint w, int dist1, int dist2)
{
  int wIndex = getWidthIndex(w);
  if (wIndex < 0) {
    return nullptr;
  }

  extDistRC* rc = _rcDistTable[mou][wIndex]->getComputeRC_res(dist1, dist2);

  return rc;
}

extDistRCTable* extDistWidthRCTable::getRuleTable(uint mou, uint w)
{
  int wIndex = getWidthIndex(w);
  if (wIndex < 0) {
    return nullptr;
  }

  return _rcDistTable[mou][wIndex];
}

void extMeasure::getDgOverlap_res(SEQ* sseq,
                                  uint dir,
                                  Ath__array1D<SEQ*>* dgContext,
                                  Ath__array1D<SEQ*>* overlapSeq,
                                  Ath__array1D<SEQ*>* residueSeq)
{
  int idx = 1;
  uint lp = dir ? 0 : 1;
  uint wp = dir ? 1 : 0;
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
