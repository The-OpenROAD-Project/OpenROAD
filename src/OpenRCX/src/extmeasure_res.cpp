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
#include <dbRtTree.h>

#include "dbUtil.h"
#include "OpenRCX/extRCap.h"
#include "utility/Logger.h"

#ifdef HI_ACC_1
#define FRINGE_UP_DOWN
#endif
//#define CHECK_SAME_NET
//#define DEBUG_NET 208091
//#define MIN_FOR_LOOPS

namespace rcx {

using utl::RCX;

using odb::dbBlock;
using odb::dbBox;
using odb::dbBTerm;
using odb::dbCapNode;
using odb::dbCCSeg;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbNet;
using odb::dbRSeg;
using odb::dbRtTree;
using odb::dbSet;
using odb::dbShape;
using odb::dbSigType;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbTechLayerType;
using odb::dbWire;
using odb::dbWirePath;
using odb::dbWirePathItr;
using odb::dbWirePathShape;
using odb::gs;
using odb::Rect;
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
  if (planeIndex < 0)
    return 0;

  uint len = 0;

  Ath__array1D<SEQ*> tmpTable(16);
  copySeqUsingPool(s, &tmpTable);

  Ath__array1D<SEQ*> residueTable(16);

  int trackTable[200];
  uint cnt = 0;
  for (int kk = (int) trackMin; kk <= (int) trackMax; kk++)  // skip overlapping track
  {
    if (!kk)
      continue;
    /*
#ifdef HI_ACC_1
    if (kk <= _dgContextHiTrack[planeIndex])
#else
    if (kk < _dgContextHiTrack[planeIndex])
#endif
      trackTable[cnt++] = *_dgContextTracks / 2 + kk;
      */
   if (-kk >= _dgContextLowTrack[planeIndex])
      trackTable[cnt++] = *_dgContextTracks / 2 - kk;
  }

  for (uint ii = 1; ii < cnt; ii++) {
    int trackn = trackTable[ii];

#ifdef HI_ACC_1
    if (_dgContextArray[planeIndex][trackn]->getCnt() <= 1)
      continue;
#endif
    bool add_all_diag = false;
    if (!add_all_diag) {
      for (uint jj = 0; jj < tmpTable.getCnt(); jj++)
        len += computeRes(tmpTable.get(jj),
                           targetMet,
                           _dir,
                           planeIndex,
                           trackn,
                           &residueTable);
    } else {
      len += computeRes(s, targetMet, _dir, planeIndex, trackn, &residueTable);
    }

    seq_release(&tmpTable);
    tableCopyP(&residueTable, &tmpTable);
    residueTable.resetCnt();
  }
  // seq_release(&tmpTable);
  if (diagTable != NULL)
    tableCopyP(&tmpTable, diagTable);
  else
    seq_release(&tmpTable);
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
  if (dgContext->getCnt() <= 1)
    return 0;

  Ath__array1D<SEQ*> overlapSeq(16);
  getDgOverlap(s, _dir, dgContext, &overlapSeq, residueSeq);

  uint len = 0;
  for (uint jj = 0; jj < overlapSeq.getCnt(); jj++) {
    SEQ* tgt = overlapSeq.get(jj);
    uint diagDist = calcDist(tgt->_ll, tgt->_ur);
    uint tgWidth = tgt->_ur[_dir] - tgt->_ll[_dir];
    uint len1 = getLength(tgt, !_dir);

    DebugDiagCoords(_met, targetMet, len1, diagDist, tgt->_ll, tgt->_ur);
    int d2= _dist>0 ? diagDist-_dist - (_ur[_dir] - _ll[_dir]): diagDist;
    int w2= tgt->_ur[_dir] - tgt->_ll[_dir];
    const char *x = _dir ? "x" : "y";
    const char *y = _dir ? "y" : "x";
    fprintf(stdout, "   %s %7d %7d  %s=%d  W%d  L%d D%d   d2=%d\n", x, tgt->_ll[!_dir], tgt->_ur[!_dir], y, tgt->_ll[_dir],  w2,  len1, diagDist, d2);
    len += len1;
  }
  seq_release(&overlapSeq);
  return len;
}
}  // namespace rcx

