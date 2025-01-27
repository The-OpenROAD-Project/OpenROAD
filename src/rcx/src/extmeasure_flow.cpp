
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
// #include <dbRtTree.h>

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

extSegment* extMeasureRC::CreateUpDownSegment(
    bool lookUp,
    Ath__wire* w,
    int xy1,
    int len1,
    Ath__wire* w2,
    Ath__array1D<extSegment*>* segTable)
{
  if (len1 == 0)
    return NULL;

  extSegment* s = _seqmentPool->alloc();

  s->set(_dir, w, xy1, len1, NULL, NULL);
  segTable->add(s);
  s->setUpDown(lookUp, w2);
  return s;
}
void extMeasureRC::FindSegmentsTrack(Ath__wire* w1,
                                     int xy1,
                                     int len1,
                                     Ath__wire* w2_next,
                                     uint ii,
                                     Ath__array1D<Ath__wire*>* trackTable,
                                     bool lookUp,
                                     uint dir,
                                     int maxDist,
                                     Ath__array1D<extSegment*>* segTable)
{
  if (w2_next == NULL) {
    if (ii >= trackTable->getCnt())
      return;
    w2_next = trackTable->get(ii);
  }
  // DELETE uint d = !dir;
  int dist = GetDistance(w1, w2_next);
  if (dist > maxDist) {
    // extSegment *s= CreateUpDownSegment(lookUp, w1, xy1, len1, NULL,
    // segTable);
    CreateUpDownSegment(lookUp, w1, xy1, len1, NULL, segTable);
    return;
  }
  if (xy1 + len1 < w2_next->getXY())  // no overlap and on the left
  {
    FindSegmentsTrack(w1,
                      xy1,
                      len1,
                      NULL,
                      ii + 1,
                      trackTable,
                      lookUp,
                      dir,
                      maxDist,
                      segTable);
    return;
  }
  Ath__wire* prev = NULL;
  Ath__wire* w2 = w2_next;
  for (; w2 != NULL; w2 = w2->getNext()) {
    // TODO out of range on the right
    if (OverlapOnly(xy1, len1, w2->getXY(), w2->getLen()))
      break;

    if (prev != NULL
        && Enclosed(
            xy1, xy1 + len1, prev->getXY() + prev->getLen(), w2->getXY())) {
      FindSegmentsTrack(w1,
                        xy1,
                        len1,
                        NULL,
                        ii + 1,
                        trackTable,
                        lookUp,
                        dir,
                        maxDist,
                        segTable);
      return;
    }
    prev = w2;
  }
  if (w2 == NULL)  // end or track
  {
    FindSegmentsTrack(w1,
                      xy1,
                      len1,
                      NULL,
                      ii + 1,
                      trackTable,
                      lookUp,
                      dir,
                      maxDist,
                      segTable);
    return;
  }
  int xy2 = w2->getXY() + w2->getLen();
  int dx2;
  int dx1 = GetDx1Dx2(xy1, len1, w2, dx2);

  if (dx1 <= 0) {    // Covered Left
    if (dx2 >= 0) {  // covered Right
      // extSegment *s= CreateUpDownSegment(lookUp, w1, xy1, len1, w2,
      // segTable);
      CreateUpDownSegment(lookUp, w1, xy1, len1, w2, segTable);
    } else {  // not covered right
      // extSegment *s = new extSegment(dir, w1, xy1, xy2 - xy1, NULL, NULL);
      CreateUpDownSegment(lookUp, w1, xy1, xy2 - xy1, w2, segTable);

      Ath__wire* next = w2->getNext();
      uint jj = next == NULL ? ii + 1 : ii;
      FindSegmentsTrack(
          w1, xy2, -dx2, next, jj, trackTable, lookUp, dir, maxDist, segTable);
    }
  } else {  // Open Left
    FindSegmentsTrack(
        w1, xy1, dx1, NULL, ii + 1, trackTable, lookUp, dir, maxDist, segTable);

    if (dx2 >= 0) {  // covered Right
      // extSegment *s = new extSegment(dir, w1, w2->getXY(), xy1 + len1 -
      // w2->getXY(), NULL, NULL); extSegment *s = CreateUpDownSegment(lookUp,
      // w1, w2->getXY(), xy1 + len1 - w2->getXY(), w2, segTable);
      CreateUpDownSegment(
          lookUp, w1, w2->getXY(), xy1 + len1 - w2->getXY(), w2, segTable);
    } else {  // not covered right
      //  extSegment *s = CreateUpDownSegment(lookUp, w1, w2->getXY(),
      //  w2->getLen(), w2, segTable);
      CreateUpDownSegment(lookUp, w1, w2->getXY(), w2->getLen(), w2, segTable);

      Ath__wire* next = w2->getNext();
      uint jj = next == NULL ? ii + 1 : ii;
      FindSegmentsTrack(
          w1, xy2, -dx2, next, jj, trackTable, lookUp, dir, maxDist, segTable);
    }
  }
}

extSegment* extMeasureRC::CreateUpDownSegment(
    Ath__wire* w,
    Ath__wire* up,
    int xy1,
    int len1,
    Ath__wire* down,
    Ath__array1D<extSegment*>* segTable,
    int metOver,
    int metUnder)
{
  if (len1 == 0)
    return NULL;

  extSegment* s = _seqmentPool->alloc();
  s->set(_dir, w, xy1, len1, up, down, metOver, metUnder);

  segTable->add(s);
  return s;
}
extSegment* extMeasureRC::GetNext(uint ii,
                                  int& xy1,
                                  int& len1,
                                  Ath__array1D<extSegment*>* segTable)
{
  if (ii < segTable->getCnt()) {
    extSegment* s = segTable->get(ii);
    xy1 = s->_xy;
    len1 = s->_len;
    return s;
  }
  return NULL;
}
extSegment* extMeasureRC::GetNextSegment(uint ii,
                                         Ath__array1D<extSegment*>* segTable)
{
  if (ii < segTable->getCnt()) {
    extSegment* s = segTable->get(ii);
    return s;
  }
  return NULL;
}
uint extMeasureRC::FindUpDownSegments(Ath__array1D<extSegment*>* upTable,
                                      Ath__array1D<extSegment*>* downTable,
                                      Ath__array1D<extSegment*>* segTable,
                                      int metOver,
                                      int metUnder)
{
  // metOver, metUnder is used in cross overlap
  uint cnt = 0;
  uint jj = 0;
  uint ii = 0;
  extSegment* up = upTable->get(ii);
  extSegment* down = downTable->get(jj);
  int xy1 = up->_xy;
  int len1 = up->_len;

  while (up != NULL && down != NULL) {
    int dx2;
    int dx1 = GetDx1Dx2(xy1, len1, down, dx2);

    if (dx1 < 0 && down->_xy + down->_len < xy1)  // down on left side
    {
      CreateUpDownSegment(down->_wire,
                          NULL,
                          down->_xy,
                          down->_len,
                          down->_down,
                          segTable,
                          -1,
                          metUnder);
      down = GetNextSegment(++jj, downTable);
      continue;
    } else if (xy1 + len1
               < down->_xy)  // no overlap and w2 too far on the right
    {
      CreateUpDownSegment(
          up->_wire, up->_up, xy1, len1, NULL, segTable, metOver, -1);
      up = GetNext(++ii, xy1, len1, upTable);
      continue;
    }
    if (dx1 <= 0) {    // Covered Left
      if (dx2 >= 0) {  // covered Right
        CreateUpDownSegment(up->_wire,
                            up->_up,
                            xy1,
                            len1,
                            down->_down,
                            segTable,
                            metOver,
                            metUnder);
        if (dx2 > 0) {
          down->_xy = xy1 + len1;
          down->_len = dx2;
        }
        up = GetNext(++ii, xy1, len1, upTable);
        if (up == NULL) {
          if (dx1 < 0)
            CreateUpDownSegment(down->_wire,
                                NULL,
                                down->_xy,
                                -dx1,
                                down->_down,
                                segTable,
                                -1,
                                metUnder);
          if (dx2 > 0)
            CreateUpDownSegment(down->_wire,
                                NULL,
                                down->_xy + down->_len - dx2,
                                dx2,
                                down->_down,
                                segTable,
                                -1,
                                metUnder);
          jj++;
          break;
        }
      } else {  // not covered right
        int len2 = down->_xy + down->_len - xy1;
        CreateUpDownSegment(up->_wire,
                            up->_up,
                            xy1,
                            len2,
                            down->_down,
                            segTable,
                            metOver,
                            metUnder);
        xy1 = down->_xy + down->_len;
        len1 = -dx2;
        up->_xy = xy1;
        up->_len = len1;
        down = GetNextSegment(++jj, downTable);
        if (down == NULL) {
          CreateUpDownSegment(
              up->_wire, up->_up, xy1, len1, NULL, segTable, metOver, -1);
          ii++;
          break;
        }
      }
    } else {  // Open Left
      CreateUpDownSegment(
          up->_wire, up->_up, xy1, dx1, NULL, segTable, metOver, -1);

      if (dx2 >= 0) {  // covered Right
        CreateUpDownSegment(up->_wire,
                            up->_up,
                            down->_xy,
                            len1 - dx1,
                            down->_down,
                            segTable,
                            metOver,
                            metUnder);
        up = GetNext(++ii, xy1, len1, upTable);
        if (up == NULL) {
          if (dx2 > 0)
            CreateUpDownSegment(down->_wire,
                                NULL,
                                down->_xy + down->_len - dx2,
                                dx2,
                                down->_down,
                                segTable,
                                -1,
                                metUnder);
          jj++;
          break;
        }
      } else {  // not covered right
        CreateUpDownSegment(up->_wire,
                            up->_up,
                            down->_xy,
                            down->_len,
                            down->_down,
                            segTable,
                            metOver,
                            metUnder);
        xy1 = down->_xy + down->_len;
        len1 = -dx2;
        down = GetNextSegment(++jj, downTable);
        if (down == NULL) {
          CreateUpDownSegment(
              up->_wire, up->_up, xy1, len1, NULL, segTable, metOver, -1);
          ii++;
          break;
        }
      }
    }
  }
  cnt += extMeasureRC::CopySegments(
      true, upTable, ii, upTable->getCnt(), segTable, metOver, -1);
  cnt += extMeasureRC::CopySegments(
      false, downTable, jj, downTable->getCnt(), segTable, -1, metUnder);

  return cnt;
}
uint extMeasureRC::CopySegments(bool up,
                                Ath__array1D<extSegment*>* upTable,
                                uint start,
                                uint end,
                                Ath__array1D<extSegment*>* segTable,
                                int maxDist,
                                int metOver,
                                int metUnder)
{
  for (uint kk = start; kk < end; kk++) {
    extSegment* s = upTable->get(kk);
    if (up)
      CreateUpDownSegment(s->_wire, s->_up, s->_xy, s->_len, NULL, segTable);
    else
      CreateUpDownSegment(s->_wire, NULL, s->_xy, s->_len, s->_down, segTable);
    /*
    extSegment *s = upTable->get(kk);
    if (up)
    {
        if (s->_dist>maxDist)
            continue;
        CreateUpDownSegment(s->_wire, s->_up, s->_xy, s->_len, NULL, segTable);
    }
    else
    {
         if (s->_dist_down>maxDist)
            continue;
        CreateUpDownSegment(s->_wire, NULL, s->_xy, s->_len, s->_down,
    segTable);
    }
    */
  }
  return 0;
}
void extMeasureRC::Print(FILE* fp,
                         Ath__array1D<Ath__wire*>* segTable,
                         const char* msg)
{
  // fprintf(fp, "%s\n", msg);
  for (uint ii = 0; ii < segTable->getCnt(); ii++) {
    Ath__wire* w = segTable->get(ii);
    PrintWire(fp, w, 0);
  }
}
void extMeasureRC::Print(FILE* fp,
                         Ath__array1D<extSegment*>* segTable,
                         uint d,
                         bool lookUp)
{
  for (uint ii = 0; ii < segTable->getCnt(); ii++) {
    extSegment* s = segTable->get(ii);
    Print(fp, s, d, lookUp);
    // print coupled net
  }
}
void extMeasureRC::Print(FILE* fp, extSegment* s, uint d, bool lookUp)
{
  int dist = lookUp ? s->_dist : s->_dist_down;
  Ath__wire* w1 = s->_up;
  // DELETE int base1 = s->_ll[1];
  if (lookUp && s->_up != NULL)
    w1 = s->_up;
  else if (!lookUp && s->_down != NULL)
    w1 = s->_down;

  if (w1 == NULL) {
    fprintf(
        fp, "%7.3f %7.3f -1\n", GetDBcoords(s->_ll[0]), GetDBcoords(s->_ll[1]));
  } else {
    dbNet* net = w1->getNet();
    fprintf(fp,
            "%7.3f %7.3f %7.3fcc M%d %5dD %8dL WL%d s%d r%d N%d %s\n",
            GetDBcoords(s->_ll[0]),
            GetDBcoords(s->_ll[1]),
            GetDBcoords(w1->getBase()),
            w1->getLevel(),
            dist,
            s->_len,
            s->_wire->getLen(),
            w1->getOtherId(),
            w1->getRsegId(),
            net->getId(),
            net->getConstName());
  }
}
void extMeasureRC::PrintUpDownNet(FILE* fp,
                                  Ath__wire* s,
                                  int dist,
                                  const char* prefix)
{
  if (fp == NULL)
    return;
  fprintf(fp, "%s  ", prefix);
  if (s == NULL)
    fprintf(fp, "D%d\n", -1);
  else {
    PrintWire(fp, s, s->getLevel());
  }
}
void extMeasureRC::PrintUpDown(FILE* fp, Ath__array1D<extSegment*>* segTable)
{
  if (fp == NULL)
    return;
  fprintf(fp, "Full Coupling Segments ---- \n");
  for (uint ii = 0; ii < segTable->getCnt(); ii++) {
    extSegment* s = segTable->get(ii);
    PrintUpDown(fp, s);
    PrintUpDownNet(fp, s->_up, s->_dist, "\t");
    PrintUpDownNet(fp, s->_down, s->_dist_down, "\t");
  }
}
void extMeasureRC::PrintUpDown(FILE* fp, extSegment* s)
{
  if (fp == NULL)
    return;
  dbNet* net = s->_wire->getNet();
  fprintf(fp,
          "%7.3f %7.3f %7.3fb %5dup %5ddn  %7dL WL%d s%d r%d N%d %s\n",
          GetDBcoords(s->_xy),
          GetDBcoords(s->_xy + s->_len),
          GetDBcoords(s->_base),
          s->_dist,
          s->_dist_down,
          s->_len,
          s->_wire->getLen(),
          s->_wire->getOtherId(),
          s->_wire->getRsegId(),
          net->getId(),
          net->getConstName());
}
/*
void extMeasureRC::PrintUpDown_save(FILE *fp, extSegment *s)
{
    dbNet *net = s->_wire->getNet();
    fprintf(fp, "%7.3f %7.3f %5dup %5ddn  %7dL WL%d s%d r%d N%d %s\n",
            GetDBcoords(s->_ll[0]), GetDBcoords(s->_ll[1]), s->_dist,
s->_dist_down, s->_len, s->_wire->getLen(), s->_wire->getOtherId(),
s->_wire->getRsegId(), net->getId(), net->getConstName());
}*/
bool extMeasureRC::CheckOrdered(Ath__array1D<extSegment*>* segTable)
{
  if (segTable->getCnt() <= 1)
    return true;

  for (uint ii = 0; ii < segTable->getCnt() - 1; ii++) {
    extSegment* s1 = segTable->get(ii);
    extSegment* s2 = segTable->get(ii + 1);
    if (s1->_xy > s2->_xy)
      return false;
  }
  return true;
}
int extMeasureRC::CouplingFlow_new(uint dir,
                                   uint couplingDist,
                                   uint diag_met_limit)
{
  uint notOrderCnt = 0;
  uint oneEmptyTable = 0;
  uint oneCntTable = 0;
  uint wireCnt = 0;
  bool dbgOverlaps = true;
  FILE* fp = OpenPrintFile(dir, "Segments");

  uint limitTrackNum = 10;
  Ath__array1D<extSegment*> upTable;
  Ath__array1D<extSegment*> downTable;
  Ath__array1D<extSegment*> verticalUpTable;
  Ath__array1D<extSegment*> verticalDownTable;
  Ath__array1D<extSegment*> segTable;
  Ath__array1D<extSegment*> aboveTable;
  Ath__array1D<extSegment*> belowTable;
  Ath__array1D<extSegment*> whiteTable;

  Ath__array1D<Ath__wire*> UpTable;

  uint colCnt = _search->getColCnt();
  Ath__array1D<Ath__wire*>** firstWireTable = allocMarkTable(colCnt);

  // TODO need to add in constructor/destructor
  _verticalPowerTable = new Ath__array1D<Ath__wire*>*[colCnt];
  for (uint ii = 0; ii < colCnt; ii++)
    _verticalPowerTable[ii] = new Ath__array1D<Ath__wire*>(4);

  for (uint level = 1; level < colCnt; level++) {
    Ath__grid* netGrid = _search->getGrid(dir, level);
    upTable.resetCnt();

    uint maxDist = 10 * netGrid->_pitch;
    for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++) {
      Ath__track* track = netGrid->getTrackPtr(tr);
      if (track == NULL)
        continue;

      ResetFirstWires(level, level + 1, dir, firstWireTable);
      for (Ath__wire* w = track->getNextWire(NULL); w != NULL;
           w = w->getNext()) {
        if (w->isPower())
          continue;

        uint rsegId = w->getRsegId();
        if (rsegId == 0)
          continue;

        if (dbgOverlaps) {
          fprintf(fp, "\n");
          Print5wires(fp, w, level);
        }
        UpTable.resetCnt();
        uint upWireCnt = FindAllNeigbors_up(w,
                                            tr + 1,
                                            dir,
                                            level,
                                            couplingDist,
                                            limitTrackNum,
                                            firstWireTable,
                                            &UpTable);

        FILE* fp1 = stdout;
        fprintf(fp1, "\nFindAllNeigbors_up: %d\n", upWireCnt);
        Print5wires(fp1, w, w->getLevel());
        fprintf(fp1, "-----------------------------\n");
        Print(fp1, &UpTable, "");
        fprintf(fp1, "===================================================\n");

        Ath__array1D<extSegment*> UpSegTable;
        bool lookUp = true;
        _dir = dir;
        FindSegmentsTrack(w,
                          w->getXY(),
                          w->getLen(),
                          NULL,
                          0,
                          &UpTable,
                          lookUp,
                          dir,
                          maxDist,
                          &UpSegTable);
        fprintf(fp1, "Up Segments %d\n", UpSegTable.getCnt());
        Print(fp1, &UpSegTable, !dir, true);
        fprintf(fp1, "===================================================\n");

        if (FindDiagonalNeighbors_vertical_power(
                dir, w, 10000, 100, 3, _verticalPowerTable)
            > 0)  // power
        {
          fprintf(fp, "Power Net Overlaps:\n");
          for (uint ii = 1; ii < colCnt; ii++)
            Print(fp, _verticalPowerTable[ii], "");
        }
        _met = level;
        _dir = dir;
        measureRC_res_init(rsegId);  // reset res value
        if (IsDebugNet())            // TODO: debug
        {
          DebugStart_res(_debugFP);
          DebugStart_res(stdout);
        }
        Release(&upTable);
        Release(&downTable);
        // DELETE uint len = FindSegments(true, dir, maxDist, w, w->getXY(),
        // w->getLen(), w->_upNext, &upTable);
        FindSegments(true,
                     dir,
                     maxDist,
                     w,
                     w->getXY(),
                     w->getLen(),
                     w->_upNext,
                     &upTable);
        BubbleSort(&upTable);
        if (dbgOverlaps) {
          fprintf(fp, "Up Segments %d\n", upTable.getCnt());
          Print(fp, &upTable, !dir, true);
        }
        FindSegments(false,
                     dir,
                     maxDist,
                     w,
                     w->getXY(),
                     w->getLen(),
                     w->_downNext,
                     &downTable);
        if (dbgOverlaps) {
          fprintf(fp, "Down Segments %d\n", downTable.getCnt());
          Print(fp, &downTable, !dir, false);
        }
        FindSegments(true,
                     dir,
                     maxDist,
                     w,
                     w->getXY(),
                     w->getLen(),
                     w->_aboveNext,
                     &aboveTable);
        if (dbgOverlaps) {
          fprintf(fp, "Above Diag Segments %d\n", aboveTable.getCnt());
          Print(fp, &aboveTable, dir, true);
        }
        FindSegments(false,
                     dir,
                     maxDist,
                     w,
                     w->getXY(),
                     w->getLen(),
                     w->_belowNext,
                     &belowTable);
        if (dbgOverlaps) {
          fprintf(fp, "Below Diag Segments %d\n", belowTable.getCnt());
          Print(fp, &belowTable, dir, false);
        }

        // TODO: debugStart
        uint cnt1 = 0;
        wireCnt++;
        if (upTable.getCnt() == 0 || downTable.getCnt() == 0) {
          oneEmptyTable++;
          // CreateUpDownSegment(w, NULL, w->getXY(), w->getLen(), NULL,
          // &segTable);
        }
        if (upTable.getCnt() == 1 || downTable.getCnt() == 1)
          oneCntTable++;
        if (!CheckOrdered(&upTable) || !CheckOrdered(&downTable)) {
          notOrderCnt++;
          BubbleSort(&upTable);
          BubbleSort(&downTable);
          if (!CheckOrdered(&upTable) || !CheckOrdered(&downTable))
            fprintf(stdout, "======> NOT SORTED after Buggble\n");
        }
        if (upTable.getCnt() == 0 && downTable.getCnt() > 0)  // OpenEnded Down
          cnt1 += CopySegments(
              false, &downTable, 0, downTable.getCnt(), &segTable);
        else if (upTable.getCnt() > 0
                 && downTable.getCnt() == 0)  // OpenEnded Up
          cnt1 += CopySegments(true, &upTable, 0, upTable.getCnt(), &segTable);
        else if (upTable.getCnt() == 1 && downTable.getCnt() > 0)  // 1 up,
          cnt1 += FindUpDownSegments(&upTable, &downTable, &segTable);
        else if (upTable.getCnt() > 0 && downTable.getCnt() == 1)  // 1 up,
          cnt1 += FindUpDownSegments(&upTable, &downTable, &segTable);
        else if (upTable.getCnt() > 0 && downTable.getCnt() > 0)
          cnt1 += FindUpDownSegments(&upTable, &downTable, &segTable);

        if (dbgOverlaps) {
          PrintUpDown(fp, &segTable);
          // PrintUpDown(stdout, &segTable);
        }
        BubbleSort(&segTable);
        if (!CheckOrdered(&segTable))
          fprintf(stdout, "======> segTable NOT SORTED after Buggble\n");
        /*
                            if (w->_verticalNext != NULL)
                            {
                                uint len = CalcDiagBelow(s,
           s->_wire->_belowNext); Ath__wire *below = s->_wire->_belowNext; if
           (below != NULL && below->_belowNext != NULL && _met <= 3) len =
           CalcDiagBelow(s, below->_belowNext);
                            }
        */
        for (uint ii = 0; ii < segTable.getCnt(); ii++) {
          extSegment* s = segTable.get(ii);
          measure_RC_new(s);
        }
        Release(&upTable);
        Release(&downTable);
        Release(&segTable);
        Release(&aboveTable);
        Release(&belowTable);
      }
    }
    fprintf(stdout,
            "\nDir=%d  wireCnt=%d  NotOrderedCnt=%d  oneEmptyTable=%d  "
            "oneCntTable=%d\n",
            dir,
            wireCnt,
            notOrderCnt,
            oneEmptyTable,
            oneCntTable);
  }
  return 0;
}
void extMeasureRC::BubbleSort(Ath__array1D<extSegment*>* segTable)
{
  int n = segTable->getCnt();
  if (n < 2)
    return;

  for (uint ii = 0; ii < n - 1; ii++) {
    bool swap = false;
    for (uint jj = 0; jj < n - ii - 1; jj++) {
      extSegment* s1 = segTable->get(jj);
      extSegment* s2 = segTable->get(jj + 1);
      if (s1->_xy > s2->_xy) {
        segTable->set(jj, s2);
        segTable->set(jj + 1, s1);
        swap = true;
      }
    }
    if (!swap)
      break;
  }
}
void extMeasureRC::OverSubRC_dist_new(dbRSeg* rseg1,
                                      dbRSeg* rseg2,
                                      int ouCovered,
                                      int diagCovered,
                                      int srcCovered)
{
  int lenOverSub = _len - ouCovered;
  if (!(lenOverSub > 0))
    return;

  bool over1 = false;
  bool openEnded = false;
  // note: _dist>0
  if (_diagResDist < 0)
    openEnded = true;
  else
    over1 = true;

  over1 = over1 && ((_dist != _diagResDist) && (_diagResDist == 200));
  _underMet = 0;
  for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
    extMetRCTable* rcModel = _metRCTable.get(jj);
    double fr = 0;
    double cc = 0;
    double tot = 0;
    /*
        if (isolated) {
          extDistRC* rc_last = rcModel->getOverFringeRC_last(_met, _width);
          if (rc_last == NULL)
            continue;
          fr = 2 * rc_last->_fringe * lenOverSub;
          tot = _extMain->updateTotalCap(rseg1, fr, jj);
        }
        else */
    if (openEnded) {
      getOverRC_Open(rcModel, _width, _met, 0, _dist, _diagResDist);
      fr = _tmpRC->_fringe * lenOverSub;
      if (rseg1 != NULL) {
        tot = _extMain->updateTotalCap(rseg1, fr, jj);
        if (IsDebugNet()) {
          DebugUpdateValue(stdout,
                           "OverSubRC_dist_new-Openend",
                           "FR",
                           rseg1->getId(),
                           fr,
                           tot);
          DebugUpdateValue(_debugFP,
                           "OverSubRC_dist_new-Openend",
                           "FR",
                           rseg1->getId(),
                           fr,
                           tot);
        }
      }
      // if (!_sameNetFlag && !_useWeighted)
      //    _extMain->updateTotalCap(rseg2, fr/2, jj);

      cc = _tmpRC->_coupling * lenOverSub;
      updateCoupCap(rseg1, rseg2, jj, cc, "OverSubRC_dist_new-openEnded");
      tot += cc;
    }
    // else if ((_dist!=_diagResDist) && (_diagResDist==200)){
    else if (over1) {
      extDistRC* rc0 = rcModel->_capOver[_met]->getRC(0, _width, _diagResDist);
      extDistRC* rc = rcModel->_capOver_open[_met][1]->getRC(0, _width, _dist);
      extDistRC* rc1 = rcModel->_capOver[_met]->getRC(0, _width, _dist);

      // float delta_fr = (2 * rc->_fringe - (rc0->_fringe + rc1->_fringe)) *
      // lenOverSub; float fr1 = rc1->_fringe * lenOverSub;

      // float cc = rc->_coupling * lenOverSub;
      // float cc0 = rc0->_coupling * lenOverSub;
      // float cc1 = rc1->_coupling * lenOverSub;
      float delta_cc = (2 * rc->_coupling - (rc0->_coupling + rc1->_coupling))
                       * lenOverSub;
      cc += delta_cc;

      // float FR = getOver_over1(rcModel, _width, _met, 0, _diagResDist, _dist,
      // lenOverSub);

      float FR0 = _tmpRC->_fringeW * lenOverSub;
      float FR1 = _tmpRC->_fringe * lenOverSub;
      float CC = _tmpRC->_coupling * lenOverSub;

      // if (FR0!=fr0 || FR1!=fr1) {
      //  fprintf(stdout, "ERROR getOver_over1\n");
      // }

      tot = _extMain->updateTotalCap(rseg1, FR0, jj);
      if (IsDebugNet()) {
        DebugUpdateValue(
            stdout, "OverSubRC_dist_new-Over1", "FR", rseg1->getId(), FR0, tot);
        DebugUpdateValue(_debugFP,
                         "OverSubRC_dist_new-Over1",
                         "FR",
                         rseg1->getId(),
                         FR0,
                         tot);
      }
      if (!_sameNetFlag) {
        double tot1 = _extMain->updateTotalCap(rseg2, FR1, jj);
        if (IsDebugNet()) {
          DebugUpdateValue(stdout,
                           "OverSubRC_dist_new-Over1",
                           "FR",
                           rseg2->getId(),
                           FR1,
                           tot1);
          DebugUpdateValue(_debugFP,
                           "OverSubRC_dist_new-Over1",
                           "FR",
                           rseg2->getId(),
                           FR1,
                           tot1);
        }
      }

      updateCoupCap(rseg1, rseg2, jj, CC, "OverSubRC_dist_new-Over1");
      tot += cc;
    } else {
      extDistRC* rc = getOverRC(rcModel);
      fr = rc->_fringe * lenOverSub;
      tot = _extMain->updateTotalCap(rseg1, fr, jj);
      if (IsDebugNet() && rseg1 != NULL) {
        DebugUpdateValue(
            stdout, "OverSubRC_dist_new-else", "FR", rseg1->getId(), fr, tot);
        DebugUpdateValue(
            _debugFP, "OverSubRC_dist_new-else", "FR", rseg1->getId(), fr, tot);
      }
      if (!_sameNetFlag) {
        _extMain->updateTotalCap(rseg2, fr, jj);
        if (IsDebugNet()) {
          DebugUpdateValue(
              stdout, "OverSubRC_dist_new-else", "FR", rseg2->getId(), fr, tot);
          DebugUpdateValue(_debugFP,
                           "OverSubRC_dist_new-else",
                           "FR",
                           rseg2->getId(),
                           fr,
                           tot);
        }
      }
      cc = rc->_coupling * lenOverSub;
      updateCoupCap(rseg1, rseg2, jj, cc, "OverSubRC_dist_new-else");
      tot += cc;

      bool useWeighFr = true;
      if (useWeighFr && rseg1 != NULL) {
        float frw = getOverR_weightedFringe(
            rcModel, _width, _met, 0, _dist, _diagResDist);
        float total_weighted_fringe = frw * lenOverSub;
        double tot2
            = _extMain->updateTotalCap(rseg1, total_weighted_fringe, jj);
        tot += tot2;
        if (IsDebugNet()) {
          DebugUpdateValue(stdout,
                           "OverSubRC_dist_new-else-weight",
                           "FR",
                           rseg1->getId(),
                           total_weighted_fringe,
                           tot2);
          DebugUpdateValue(_debugFP,
                           "OverSubRC_dist_new-else-weight",
                           "FR",
                           rseg1->getId(),
                           total_weighted_fringe,
                           tot2);
        }
      }
    }
  }
}
int extMeasureRC::computeAndStoreRC_new(dbRSeg* rseg1,
                                        dbRSeg* rseg2,
                                        int srcCovered)
{
  ResetRCs();
  _tmpRC->Reset();
  dbRSeg* RSEG = rseg1 != NULL ? rseg1 : rseg2;
  bool DEBUG1 = false;
  if (DEBUG1) {
    extMeasure::segInfo("SRC", _netSrcId, _rsegSrcId);
    extMeasure::segInfo("DST", _netTgtId, _rsegTgtId);
  }
  // Copy from computeAndStore om 09212023
  bool SUBTRACT_DIAG = false;
  bool USE_DB_UBITS = false;
  bool COMPUTE_OVER_SUB = true;

  rcSegInfo();

  if (IsDebugNet())
    debugPrint(_extMain->getLogger(),
               RCX,
               "debug_net",
               1,
               "measureRC:"
               "C"
               "\t[BEGIN-OUD] ----- OverUnder/Diagonal RC ----- BEGIN");

  //_diagLen = 0;
  int totLenCovered = 0;
  _lenOUtable->resetCnt();
  _diagFlow = true;
  if (_extMain->_ccContextDepth > 0) {
    if (!_diagFlow)
      totLenCovered = measureOverUnderCap();
    else
      totLenCovered = measureDiagOU(1, 2);
  }
  ouCovered_debug(totLenCovered);
  if (IsDebugNet()) {
    dbRSeg* rseg = rseg1 != NULL ? rseg1 : rseg2;
    segInfo(_debugFP, "\t\tUPDATE_after_OU ", _netId, rseg->getId());
    segInfo(stdout, "\t\tUPDATE_after_OU ", _netId, rseg->getId());
  }
  if (USE_DB_UBITS) {
    totLenCovered = _extMain->GetDBcoords2(totLenCovered);
    _len = _extMain->GetDBcoords2(_len);
    _diagLen = _extMain->GetDBcoords2(_diagLen);
  }
  int lenOverSub = _len - totLenCovered;

  if (_diagLen > 0 && SUBTRACT_DIAG)
    lenOverSub -= _diagLen;

  if (lenOverSub < 0)
    lenOverSub = 0;

  // Case where the geometric search returns no neighbor found
  // _dist is infinit
  if (_dist < 0) {
    if (totLenCovered < 0)
      totLenCovered = 0;

    _underMet = 0;

    _no_debug = true;
    _no_debug = false;

    for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
      _rc[jj]->_res = 0;  // DF 022821 : Res non context based

      if (_rc[jj]->_fringe > 0) {
        double fr = _rc[jj]->_fringe;
        fr += _rc[jj]->_fringeW;
        _extMain->updateTotalCap(rseg1, fr, jj);
        if (IsDebugNet()) {
          printDebugRC(_debugFP, _rc[jj], "\tOU_TOTAL_INF", "\n");
          printDebugRC(stdout, _rc[jj], "\tOU_TOTAL_INF", "\n");

          DebugUpdateValue(stdout,
                           "OVER_SUB_INF",
                           "FR",
                           rseg1->getId(),
                           fr,
                           rseg1->getCapacitance());
          DebugUpdateValue(_debugFP,
                           "OVER_SUB_INF",
                           "FR",
                           rseg1->getId(),
                           fr,
                           rseg1->getCapacitance());

          segInfo(_debugFP,
                  "\t\tOU_TOTAL_UPDATE ",
                  RSEG->getNet()->getId(),
                  RSEG->getId());
          segInfo(stdout,
                  "\t\tOU_TOTAL_UPDATE ",
                  RSEG->getNet()->getId(),
                  RSEG->getId());
        }
      }
    }
    if (COMPUTE_OVER_SUB) {
      OverSubRC(rseg1, NULL, totLenCovered, _diagLen, _len);
      return totLenCovered;
    }
  } else {  // dist based

    bool OpenEnded = _dist < 0 || _diagResDist < 0;
    bool over1
        = !OpenEnded && ((_dist != _diagResDist) && (_diagResDist == 200));

    _underMet = 0;
    for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
      if (IsDebugNet()) {
        printDebugRC(_debugFP, _rc[jj], "\tOU_TOTAL_dist", "\n");
        printDebugRC(stdout, _rc[jj], "\tOU_TOTAL_dist", "\n");
      }
      double tot1 = 0;
      double tot2 = 0;
      if (OpenEnded) {
        double tot_fr = _rc[jj]->_fringe;
        if (rseg1 != NULL) {
          tot1 = _extMain->updateTotalCap(rseg1, tot_fr, jj);
          if (IsDebugNet()) {
            DebugUpdateValue(stdout,
                             "OVER_SUB_DIST_OpenEnded",
                             "FR",
                             rseg1->getId(),
                             _rc[jj]->_fringe,
                             tot1);
            DebugUpdateValue(_debugFP,
                             "OVER_SUB_DIST_OpenEnded",
                             "FR",
                             rseg1->getId(),
                             _rc[jj]->_fringe,
                             tot1);
          }
        }
        updateCoupCap(
            rseg1, rseg2, jj, _rc[jj]->_coupling, "OVER_SUB_DIST_OpenEnded");
      } else if (over1) {
        tot1 = _extMain->updateTotalCap(rseg1, _rc[jj]->_fringeW, jj);
        if (IsDebugNet()) {
          DebugUpdateValue(stdout,
                           "OVER_SUB_DIST_Over1",
                           "FR",
                           rseg1->getId(),
                           _rc[jj]->_fringe,
                           tot1);
          DebugUpdateValue(_debugFP,
                           "OVER_SUB_DIST_Over1",
                           "FR",
                           rseg1->getId(),
                           _rc[jj]->_fringe,
                           tot1);
        }

        if (!_sameNetFlag) {
          double tot1 = _extMain->updateTotalCap(rseg2, _rc[jj]->_fringe, jj);
          if (IsDebugNet()) {
            DebugUpdateValue(stdout,
                             "OVER_SUB_DIST_Over1",
                             "FR",
                             rseg2->getId(),
                             _rc[jj]->_fringe,
                             tot1);
            DebugUpdateValue(_debugFP,
                             "OVER_SUB_DIST_Over1",
                             "FR",
                             rseg2->getId(),
                             _rc[jj]->_fringe,
                             tot1);
          }
        }
        updateCoupCap(
            rseg1, rseg2, jj, _rc[jj]->_coupling, "OVER_SUB_DIST_Over1");
      } else {
        double tot_fr = _rc[jj]->_fringe + _rc[jj]->_fringeW;
        tot1 = _extMain->updateTotalCap(rseg1, tot_fr, jj);
        if (IsDebugNet() && rseg1 != NULL) {
          DebugUpdateValue(
              stdout, "OVER_SUB_DIST_else", "FR", rseg1->getId(), tot_fr, tot1);
          DebugUpdateValue(_debugFP,
                           "OVER_SUB_DIST_else",
                           "FR",
                           rseg1->getId(),
                           tot_fr,
                           tot1);
        }
        tot2 = _extMain->updateTotalCap(rseg2, _rc[jj]->_fringe, jj);
        if (IsDebugNet() && rseg2 != NULL) {
          DebugUpdateValue(stdout,
                           "OVER_SUB_DIST_else",
                           "FR",
                           rseg2->getId(),
                           _rc[jj]->_fringe,
                           tot2);
          DebugUpdateValue(_debugFP,
                           "OVER_SUB_DIST_else",
                           "FR",
                           rseg2->getId(),
                           _rc[jj]->_fringe,
                           tot2);
        }
        updateCoupCap(
            rseg1, rseg2, jj, _rc[jj]->_coupling, "OVER_SUB_DIST_else");
      }
      tot1 += _rc[jj]->_coupling;
      tot2 += _rc[jj]->_coupling;
    }
    rcSegInfo();
    if (IsDebugNet())
      debugPrint(_extMain->getLogger(),
                 RCX,
                 "debug_net",
                 1,
                 "measureRC:"
                 "C"
                 "\t[END-OUD] ------ OverUnder/Diagonal RC ------ END");
    OverSubRC_dist_new(rseg1, rseg2, totLenCovered, _diagLen, _len);
  }
  if (IsDebugNet()) {
    segInfo(_debugFP,
            "\t\tOU_TOTAL_UPDATE_dist ",
            RSEG->getNet()->getId(),
            RSEG->getId());
    segInfo(stdout,
            "\t\tOU_TOTAL_UPDATE_dist ",
            RSEG->getNet()->getId(),
            RSEG->getId());
    /*
     if (rseg2!=NULL) {
      segInfo(_debugFP, "\t\tOU_TOTAL_UPDATE_dist ", rseg2->getNet()->getId(),
    rseg2->getId()); segInfo(stdout, "\t\tOU_TOTAL_UPDATE_dist ",
    rseg2->getNet()->getId(), rseg2->getId());
    } */
  }

  return totLenCovered;
}

}  // namespace rcx
