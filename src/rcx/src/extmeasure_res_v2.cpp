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

#include "gseq.h"
#include "rcx/dbUtil.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;
using namespace odb;

class compareSEQ_H
{
 public:
  bool operator()(SEQ* wire1, SEQ* wire2)
  {
    return (wire1->_ll[0] > wire2->_ll[0] ? true : false);
  }
};
class compare_int
{
 public:
  bool operator()(int x, int y) { return (x <= y); }
};
class compareSEQ_V
{
 public:
  bool operator()(SEQ* wire1, SEQ* wire2)
  {
    return (wire1->_ll[1] > wire2->_ll[1] ? true : false);
  }
};
int extMeasure::DebugPrint(SEQ* s,
                           Ath__array1D<SEQ*>* dgContext,
                           int trackNum,
                           int plane)
{
  int scnt = (int) dgContext->getCnt();
  fprintf(
      stdout, "cnt %d  plane %d  track %d  ------- \n", scnt, plane, trackNum);
  fprintf(stdout,
          "     target: %d  %d  %d  %d id %d\n",
          s->_ll[0],
          s->_ll[1],
          s->_ur[0],
          s->_ur[1],
          s->type);
  uint idx = 0;  // at idx=0, index info
  for (; idx < scnt; idx++) {
    SEQ* tseq = dgContext->get(idx);

    char same_net = ' ';
    if (tseq->type == _rsegSrcId)
      same_net = 'S';

    int dd = s->_ll[_dir] - tseq->_ur[_dir];  // look below
    fprintf(stdout,
            "%d  %d  %d  %d id %d  DIST= %d %c\n",
            tseq->_ll[0],
            tseq->_ll[1],
            tseq->_ur[0],
            tseq->_ur[1],
            tseq->type,
            dd,
            same_net);
  }
  return scnt;
}
int binarySearch(Ath__array1D<SEQ*>* arr,
                 int target,
                 int begin,
                 int end,
                 int dir)
{
  int left = begin;
  int right = end;

  while (left <= right) {
    int mid = left + (right - left) / 2;
    if (mid == end)
      return mid;
    int xy1 = arr->get(mid)->_ll[dir];
    if (xy1 < target) {
      left = mid + 1;  // Target is in the right half
    } else {
      right = mid - 1;  // Target is in the left half
    }
  }
  return left;
}

int extMeasure::SingleDiagTrackDist_opt(SEQ* s,
                                        Ath__array1D<SEQ*>* dgContext,
                                        bool skipZeroDist,
                                        bool skipNegativeDist,
                                        Ath__array1D<int>* sortedDistTable,
                                        Ath__array1D<SEQ*>* segFilteredTable)
{
  if (dgContext->getCnt() == 0)
    return 0;

  SEQ* first = dgContext->get(1);
  if (s->_ur[!_dir] <= first->_ll[!_dir])
    return 0;
  SEQ* last = dgContext->get(dgContext->getCnt() - 1);
  if (last->type == _rsegSrcId) {  // same net
    if (dgContext->getCnt() < 2)
      return 0;
    last = dgContext->get(dgContext->getCnt() - 2);
  }
  if (s->_ll[!_dir] >= last->_ur[!_dir])
    return 0;

  sortedDistTable->resetCnt();
  std::vector<int> distTable;

  // find first overlap
  SEQ* indexInfo = dgContext->get(0);
  // DELETE int init_index= indexInfo->type;
  int ii = indexInfo->type;
  int back_cnt = 0;
  for (; ii > 0; ii--) {
    SEQ* tseq = dgContext->get(ii);
    back_cnt++;
    int xy1 = s->_ll[!_dir];
    int xy2 = tseq->_ur[!_dir];
    if (xy1 > xy2)
      // if (s->_ll[!_dir] < tseq->_ll[!_dir])
      break;
  }
  int start
      = indexInfo->type == 0 ? 1 : ii - 1;  // -1 means 2 segments with same lo
                                            // xy on different tracks
  if (start < 0)
    start = 0;

  bool same_net = false;
  int min_dist = 100000000;
  int max_dist = 0;
  uint cnt = 0;
  uint tot_cnt = 0;

  int scnt = (int) dgContext->getCnt();
  int search_start = start;
  search_start = binarySearch(dgContext, s->_ll[!_dir], start, scnt, !_dir);
  uint idx = search_start;
  for (; idx < scnt; idx++) {
    SEQ* tseq = dgContext->get(idx);
    tot_cnt++;

    // ---------   ----------------------       ---------  ------      --------
    // s segment:                    ___________________

    if (s->_ur[!_dir] <= tseq->_ll[!_dir]) {
      indexInfo->type = idx;  // starting point next segment
      break;
    }
    if (s->_ll[!_dir] >= tseq->_ur[!_dir]) {
      continue;  // on  the right
    }
    if (tseq->type == _rsegSrcId) {
      same_net = true;
      continue;  // same net
    }
    int dd = s->_ll[_dir] - tseq->_ur[_dir];  // look below

    if (skipZeroDist && dd == 0)
      continue;
    if (skipNegativeDist && dd < 0)
      continue;
    cnt++;

    if (min_dist > dd)
      min_dist = dd;
    if (max_dist < dd)
      max_dist = dd;

    segFilteredTable->add(tseq);
  }
  // fprintf(stdout, "%5d     track_cnt %5d    overlapped %5d    init_index %5d
  // back_cnt %5d   start %5d    search_start %5d  end_index %5d\n", tot_cnt,
  // scnt, cnt, init_index, back_cnt, start, search_start, indexInfo->type); if
  // (tot_cnt==74 && cnt==0) fprintf(stdout, "%5d     track_cnt %5d overlapped
  // %5d    init_index %5d    back_cnt %5d   start %5d    search_start %5d
  // end_index %5d\n", tot_cnt, scnt, cnt, init_index, back_cnt, start,
  // search_start, indexInfo->type);
  if (cnt == 0)
    return -1;
  if (same_net) {
    if (cnt == 0)  // only same net wire on the table
      return -1;
    if (min_dist == max_dist && same_net
        && cnt == scnt - 1)  // only one track, filter same_net, zero, negative
                             // dist
      return 1;
  }
  if (min_dist == max_dist && !same_net && cnt == scnt - 1) {  // normal!
    if (segFilteredTable->getCnt() == 0)
      return 0;
  }
  if (segFilteredTable->getCnt() == 1) {
    sortedDistTable->add(min_dist);
    return 1;
  }
  if (segFilteredTable->getCnt() == 2) {
    sortedDistTable->add(min_dist);
    sortedDistTable->add(max_dist);
    return 2;
  }
  int prev_dist = -100000;
  for (uint ii = 0; ii < cnt; ii++) {
    SEQ* tseq = segFilteredTable->get(ii);
    int dd = s->_ll[_dir] - tseq->_ur[_dir];  // look below
    if (dd != prev_dist) {
      distTable.push_back(dd);
      prev_dist = dd;
    }
  }
  if (distTable.size() > 1)
    std::sort(distTable.begin(), distTable.end(), compare_int());
  for (uint k = 0; k < distTable.size(); k++) {
    int dist = distTable[k];
    sortedDistTable->add(dist);
  }
  distTable.clear();

  return sortedDistTable->getCnt();
}
int extMeasure::SingleDiagTrackDist(SEQ* s,
                                    Ath__array1D<SEQ*>* dgContext,
                                    bool skipZeroDist,
                                    bool skipNegativeDist,
                                    std::vector<int>& distTable,
                                    Ath__array1D<SEQ*>* segFilteredTable)
{
  bool same_net = false;
  int min_dist = 100000000;
  int max_dist = 0;
  uint cnt = 0;

  int scnt = (int) dgContext->getCnt();
  uint idx = 1;  // at idx=0, index info
  for (; idx < scnt; idx++) {
    SEQ* tseq = dgContext->get(idx);

    if (tseq->type == _rsegSrcId) {
      same_net = true;
      continue;  // same net
    }
    int dd = s->_ll[_dir] - tseq->_ur[_dir];  // look below

    if (skipZeroDist && dd == 0)
      continue;
    if (skipNegativeDist && dd < 0)
      continue;
    cnt++;

    if (min_dist > dd)
      min_dist = dd;
    if (max_dist < dd)
      max_dist = dd;

    segFilteredTable->add(tseq);
  }
  if (cnt == 0)
    return -1;
  if (same_net) {
    if (cnt == 0)  // only same net wire on the table
      return -1;
    if (min_dist == max_dist && same_net
        && cnt == scnt - 1)  // only one track, filter same_net, zero, negative
                             // dist
      return 1;
  }
  if (min_dist == max_dist && !same_net && cnt == scnt - 1)  // normal!
    return 0;

  int prev_dist = -100000;
  for (uint ii = 0; ii < cnt; ii++) {
    SEQ* tseq = segFilteredTable->get(ii);
    int dd = s->_ll[_dir] - tseq->_ur[_dir];  // look below
    if (dd != prev_dist) {
      distTable.push_back(dd);
      prev_dist = dd;
    }
  }
  return distTable.size();
}
int extMeasure::computeResDist(SEQ* s,
                               uint trackMin,
                               uint trackMax,
                               uint targetMet,
                               Ath__array1D<SEQ*>* diagTable)
{
  // return 0;
  int trackDist = 20;
  int loTrack;
  int hiTrack;
  int planeIndex
      = getDgPlaneAndTrackIndex(targetMet, trackDist, loTrack, hiTrack);
  if (planeIndex < 0)
    return 0;

  uint len = 0;

  Ath__array1D<SEQ*>* tmpTable = new Ath__array1D<SEQ*>(16);
  copySeqUsingPool(s, tmpTable);

  Ath__array1D<SEQ*>* residueTable = new Ath__array1D<SEQ*>(16);
  Ath__array1D<SEQ*>* dgTable = new Ath__array1D<SEQ*>(16);
  Ath__array1D<SEQ*>* seqTable = new Ath__array1D<SEQ*>(16);
  Ath__array1D<int>* distTable = new Ath__array1D<int>(4);

  int trackTable[200];
  uint cnt = 0;
  for (int kk = (int) trackMin; kk <= (int) trackMax;
       kk++)  // skip overlapping track
  {
    if (-kk >= _dgContextLowTrack[planeIndex])
      trackTable[cnt++] = *_dgContextTracks / 2 - kk;
  }

  for (uint ii = 0; ii < cnt; ii++) {
    int trackn = trackTable[ii];

    if (_dgContextArray[planeIndex][trackn]->getCnt() <= 1)
      continue;

    // Check for same track
    // DELETE bool same_track = false;
    Ath__array1D<SEQ*>* dTable = _dgContextArray[planeIndex][trackn];
    // DELETE int cnt = (int) dTable->getCnt();

    bool dbg1 = IsDebugNet();
    bool filteringRequired = true;
    if (IsDebugNet())
      DebugPrint(s, dTable, trackn, planeIndex);

    seqTable->resetCnt();
    // int res= SingleDiagTrackDist(s, dTable, true, true, distTable, seqTable);
    int res
        = SingleDiagTrackDist_opt(s, dTable, true, true, distTable, seqTable);
    if (res < 0) {
      if (dbg1)
        fprintf(stdout, "res= %d -- skip diag overlap\n", res);
      continue;
    } else if (res == 0) {
      filteringRequired = true;
      // NEED TO EXCLUDE idx=0; keep as reference

      if (dbg1)
        fprintf(stdout, "Normal\n");
    } else if (res == 1 && distTable->getCnt() == 0) {
      if (dbg1)
        fprintf(stdout, "single dist same net\n");
    } else if (res == 1 && distTable->getCnt() > 0) {
      if (dbg1)
        fprintf(stdout, "dist %d\n", distTable->get(0));
    } else {
      if (dbg1)
        for (uint ii = 0; ii < res; ii++)
          fprintf(stdout, " dist %d\n", distTable->get(ii));
    }
    bool covered = false;
    // if (filteringRequired && res>1) {
    if (filteringRequired) {
      if (IsDebugNet()) {
        fprintf(stdout, "---- FILTERED ----\n");
        DebugPrint(s, seqTable, trackn, planeIndex);
      }
      for (uint k = 0; k < distTable->getCnt(); k++) {
        dgTable->resetCnt();
        int dist = distTable->get(k);
        for (uint n = 0; n < seqTable->getCnt(); n++) {
          SEQ* t = seqTable->get(n);
          int dd = s->_ll[_dir] - t->_ur[_dir];  // look below
          if (dd == dist)
            dgTable->add(t);
        }
        if (IsDebugNet()) {
          fprintf(stdout, "---- dist target %d ----\n", dist);
          DebugPrint(s, dgTable, trackn, planeIndex);
        }
        len += computeResLoop(tmpTable,
                              dgTable,
                              targetMet,
                              _dir,
                              planeIndex,
                              trackn,
                              residueTable);
        if (tmpTable->getCnt() == 0) {
          covered = true;
          break;
        }
      }
      if (covered)
        break;

      continue;
    }
    /*
    bool add_all_diag = false;
    if (!add_all_diag) {
      for (uint jj = 0; jj < tmpTable.getCnt(); jj++)
        len += computeRes(tmpTable.get(jj),
                          NULL,
                          targetMet,
                          _dir,
                          planeIndex,
                          trackn,
                          &residueTable);
    } else {
      len += computeRes(s, NULL, targetMet, _dir, planeIndex, trackn,
    &residueTable);
    }
    seq_release(&tmpTable);
    tableCopyP(&residueTable, &tmpTable);
    residueTable.resetCnt();
    */
  }
  if (diagTable != NULL)
    tableCopyP(tmpTable, diagTable);
  else
    seq_release(tmpTable);
  delete tmpTable;
  delete distTable;
  delete residueTable;
  delete dgTable;

  return len;
}
uint extMeasure::computeResLoop(Ath__array1D<SEQ*>* tmpTable,
                                Ath__array1D<SEQ*>* dgTable,
                                uint targetMet,
                                uint dir,
                                uint planeIndex,
                                uint trackn,
                                Ath__array1D<SEQ*>* residueTable)
{
  uint len = 0;
  // bool add_all_diag = false;
  //  if (!add_all_diag) {
  for (uint jj = 0; jj < tmpTable->getCnt(); jj++) {
    SEQ* tseq = tmpTable->get(jj);
    len += computeRes(
        tseq, dgTable, targetMet, dir, planeIndex, trackn, residueTable);
    if (residueTable->getCnt() == 0) {
      seq_release(tmpTable);
      tmpTable->resetCnt(0);
      if (IsDebugNet()) {
        fprintf(stdout,
                "COVERED: %d  %d  %d  %d id %d \n",
                tseq->_ll[0],
                tseq->_ll[1],
                tseq->_ur[0],
                tseq->_ur[1],
                tseq->type);
      }
      return len;
    }
  }
  // }
  // TODO else {
  // TODO  len += computeRes(s, targetMet, _dir, planeIndex, trackn,
  // &residueTable);
  // TODO }
  seq_release(tmpTable);
  tableCopyP(residueTable, tmpTable);
  residueTable->resetCnt();
  return len;
}

uint extMeasure::computeRes(SEQ* s,
                            Ath__array1D<SEQ*>* dgContext,
                            uint targetMet,
                            uint dir,
                            uint planeIndex,
                            uint trackn,
                            Ath__array1D<SEQ*>* residueSeq)
{
  if (IsDebugNet())
    fprintf(stdout, "track=%d =================\n", trackn);

  Ath__array1D<SEQ*> overlapSeq(16);
  if (dgContext != NULL) {
    if (dgContext->getCnt() == 0)
      return 0;
    getDgOverlap_res(s, _dir, dgContext, &overlapSeq, residueSeq);
  } else {
    dgContext = _dgContextArray[planeIndex][trackn];
    if (dgContext->getCnt() <= 1)
      return 0;
    getDgOverlap_res(s,
                     _dir,
                     dgContext,
                     &overlapSeq,
                     residueSeq);  // TODO: replace with old one
  }
  uint len = 0;
  for (uint jj = 0; jj < overlapSeq.getCnt(); jj++) {
    SEQ* tgt = overlapSeq.get(jj);
    uint diagDist = calcDist(tgt->_ll, tgt->_ur);
    uint len1 = getLength(tgt, !_dir);

    len += len1;
    calcRes(_rsegSrcId, len1, _dist, diagDist, _met);

    _diagTable->add(tgt);
  }
  // seq_release(&overlapSeq);
  overlapSeq.resetCnt();
  return len;
}

int extDistRCTable::getComputeRC_maxDist()
{
  return _maxDist;
}

extDistRC* extDistRCTable::getComputeRC_res(uint dist1, uint dist2)
{
  int min_dist = 0;
  if (dist1 > dist2) {
    min_dist = dist1;
    dist1 = dist2;
    dist2 = min_dist;
  }
  if (_measureTable == NULL)
    return NULL;

  if (_measureTable->getCnt() <= 0)
    return NULL;

  extDistRC* rc1 = _measureTableR[0]->geti(0);
  rc1->_diag = 0.0;
  if (rc1 == NULL)
    return NULL;

  if (dist1 + dist2 == 0) {  // ASSUMPTION: 0 dist exists as first
    return rc1;
  }
  if (dist1 >= _maxDist && dist2 >= _maxDist) {
    return NULL;
  }
  if (dist2 > _maxDist) {
    dist2 = dist1;
    dist1 = 0;
  }

  uint index_dist = 0;
  bool found = false;
  extDistRC* rc2 = _measureTableR[1]->geti(0);
  if (rc2 == NULL)
    return rc1;

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
  extDistRC* rc = NULL;
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
    if (rc != NULL && dist1 < rc->_sep) {
      _measureTable = _measureTableR[index_dist - 1];
      _computeTable = _computeTableR[index_dist - 1];
      extDistRC* res1 = findIndexed_res(dist1, dist2);
      double R1 = res->interpolate_res(dist1, res1);
      res1->_diag = R1;
      return res1;
    }
    return res;
  }
  return NULL;
}

void extMeasure::getDgOverlap_res(SEQ* sseq,
                                  uint dir,
                                  Ath__array1D<SEQ*>* dgContext,
                                  Ath__array1D<SEQ*>* overlapSeq,
                                  Ath__array1D<SEQ*>* residueSeq)
{
  uint lp = dir ? 0 : 1;
  uint wp = dir ? 1 : 0;
  SEQ* rseq;
  SEQ* tseq;
  SEQ* wseq;
  int covered = sseq->_ll[lp];
  int min_dist = 1000000000;
  int max_dist = -100000000;

  if (dgContext->getCnt() == 0) {
    rseq = _seqPool->alloc();
    rseq->_ll[wp] = sseq->_ll[wp];
    rseq->_ur[wp] = sseq->_ur[wp];
    rseq->_ll[lp] = sseq->_ll[lp];
    rseq->_ur[lp] = sseq->_ur[lp];
    residueSeq->add(rseq);
    return;
  }

  std::vector<SEQ*> segVector;
  std::vector<SEQ*> segVector1;
  std::vector<SEQ*> segVector2;
  std::vector<SEQ*> segVectorDist[10000];
  int idx = 0;
  int scnt = (int) dgContext->getCnt();

  for (; idx < scnt; idx++) {
    tseq = dgContext->get(idx);
    if (tseq->type == _rsegSrcId)
      continue;  // same net
    if (tseq->_ur[0] == 0 && tseq->_ur[0] == 0 && tseq->_ll[0] == 0
        && tseq->_ll[1] == 0)
      continue;

    int dd = sseq->_ll[_dir] - tseq->_ur[_dir];  // look below

    if (dd <= 0)
      continue;

    if (min_dist > dd)
      min_dist = dd;
    if (max_dist < dd)
      max_dist = dd;
    if (IsDebugNet()) {
      int dd = sseq->_ll[_dir] - tseq->_ur[_dir];
      fprintf(stdout,
              "%d  %d  %d  %d   DIST= %d\n",
              tseq->_ll[0],
              tseq->_ll[1],
              tseq->_ur[0],
              tseq->_ur[1],
              dd);
    }
  }
  if (IsDebugNet()) {
    fprintf(stdout, "minDist= %d ________________\n", min_dist);
    fprintf(stdout, "maxDist= %d ________________\n", max_dist);
  }

  bool skipSorting = true;
  if (segVector.size() > 1 && !skipSorting) {
    if (_dir)
      std::sort(segVector.begin(), segVector.end(), compareSEQ_V());
    else
      std::sort(segVector.begin(), segVector.end(), compareSEQ_H());
  }
  idx = 0;
  for (; idx < dgContext->getCnt(); idx++) {
    tseq = dgContext->get(idx);

    if (tseq->_ur[lp] <= covered)
      continue;

    if (tseq->_ur[0] == 0 && tseq->_ur[0] == 0 && tseq->_ll[0] == 0
        && tseq->_ll[1] == 0)
      continue;

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
    if (tseq->_ur[lp] <= sseq->_ur[lp])
      wseq->_ur[lp] = tseq->_ur[lp];
    else
      wseq->_ur[lp] = sseq->_ur[lp];
    if (tseq->_ll[lp] <= covered)
      wseq->_ll[lp] = covered;
    else {
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
    if (tseq->_ur[lp] >= sseq->_ur[lp])
      break;
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

  // TODO dgContext->get(0)->_ll[0] = idx;
  if (residueSeq->getCnt() == 0 && overlapSeq->getCnt() == 0) {
    rseq = _seqPool->alloc();
    rseq->_ll[wp] = sseq->_ll[wp];
    rseq->_ur[wp] = sseq->_ur[wp];
    rseq->_ll[lp] = sseq->_ll[lp];
    rseq->_ur[lp] = sseq->_ur[lp];
    residueSeq->add(rseq);
  }
}

void extMeasure::calcRes(int rsegId1,
                         uint len,
                         int dist1,
                         int dist2,
                         int tgtMet)
{
  if (dist1 == -1)
    dist1 = 0;
  if (dist2 == -1)
    dist1 = 0;
  int min_dist = 0;
  if (dist1 > dist2) {
    min_dist = dist1;
    dist1 = dist2;
    dist2 = min_dist;
  }
  uint modelCnt = _metRCTable.getCnt();
  for (uint ii = 0; ii < modelCnt; ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    if (rcModel->_resOver[tgtMet] == NULL)
      continue;

    extDistRC* rc = rcModel->_resOver[tgtMet]->getRes(0, _width, dist1, dist2);
    if (rc != NULL) {
      double R1 = rc->_diag > 0 ? rc->_diag : rc->_res;
      double R = len * R1;
      double prev = _rc[ii]->_res;
      _rc[ii]->_res += R;

      if (IsDebugNet()) {
        DebugRes_calc(_debugFP,
                      "calcRes",
                      rsegId1,
                      "len_down ",
                      len,
                      dist1,
                      dist2,
                      tgtMet,
                      _rc[ii]->_res,
                      R,
                      R1,
                      prev);
        DebugRes_calc(stdout,
                      "calcRes",
                      rsegId1,
                      "len_down ",
                      len,
                      dist1,
                      dist2,
                      tgtMet,
                      _rc[ii]->_res,
                      R,
                      R1,
                      prev);
      }
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
    if (rcModel->_resOver[tgtMet] == NULL)
      continue;

    extDistRC* rc = rcModel->_resOver[tgtMet]->getRes(0, _width, dist1, dist2);
    double R = len * rc->_res;
    deltaRes[ii] = R;

    if (IsDebugNet()) {
      DebugRes_calc(_debugFP,
                    "calcRes0",
                    0,
                    "len_delta ",
                    len,
                    dist1,
                    dist2,
                    tgtMet,
                    _rc[ii]->_res,
                    R,
                    rc->_res,
                    0);
      DebugRes_calc(_debugFP,
                    "calcRes0",
                    0,
                    "len_delta ",
                    len,
                    dist1,
                    dist2,
                    tgtMet,
                    _rc[ii]->_res,
                    R,
                    rc->_res,
                    0);
    }
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

    if (rcModel->_resOver[tgtMet] == NULL)
      continue;

    extDistRC* rc = rcModel->_resOver[tgtMet]->getRes(0, width, dist1, dist2);
    if (rc == NULL)
      continue;
    double R = len * rc->_res;
    deltaRes[jj] = R;
  }
}
extDistRC* extDistRCTable::findRes(int dist1, int dist2, bool compute)
{
  Ath__array1D<extDistRC*>* table = _computeTable;
  if (!compute)
    table = _measureTable;

  if (table->getCnt() == 0)
    return NULL;

  int target_dist_index = -1;
  extDistRC* rc = NULL;
  uint ii = 0;
  for (; ii < table->getCnt(); ii++) {
    rc = table->get(ii);
    if (dist1 == rc->_coupling) {
      target_dist_index = ii;
      break;
    }
    if (dist1 < rc->_coupling) {
      target_dist_index = ii;
      break;
    }
  }
  if (target_dist_index < 0) {
    rc = table->get(0);  // assume first rec is 0,0
    return rc;
  }
  extDistRC* last_rc = NULL;
  for (uint ii = target_dist_index; ii < table->getCnt(); ii++) {
    extDistRC* rc1 = table->get(ii);
    if (rc->_coupling != rc1->_coupling) {
      return last_rc;
    }
    if (dist2 == rc1->_sep) {
      return rc1;
    }
    if (dist2 < rc1->_sep) {
      if (last_rc != NULL)
        return last_rc;
      return rc1;
    }
    last_rc = rc1;
  }
  if (table->getCnt() > 0) {
    rc = table->get(0);  // assume first rec is 0,0
    return rc;
  }
  return NULL;
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
  if (dist2 >= resLast->_sep)
    return resLast;

  uint n = dist2 / _unit;
  extDistRC* res = _computeTable->geti(n);
  return res;
}

extDistRC* extDistWidthRCTable::getRes(uint mou, uint w, int dist1, int dist2)
{
  int wIndex = getWidthIndex(w);
  if (wIndex < 0)
    return NULL;

  // extDistRC *rc= _rcDistTable[mou][wIndex]->findRes(dist1, dist2, false);
  extDistRC* rc = _rcDistTable[mou][wIndex]->getComputeRC_res(dist1, dist2);

  return rc;
}
extDistRCTable* extDistWidthRCTable::getRuleTable(uint mou, uint w)
{
  int wIndex = getWidthIndex(w);
  if (wIndex < 0)
    return NULL;

  return _rcDistTable[mou][wIndex];
}

}  // namespace rcx
