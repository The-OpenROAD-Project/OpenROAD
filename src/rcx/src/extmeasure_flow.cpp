// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "odb/db.h"
#include "rcx/array1.h"
#include "rcx/dbUtil.h"
#include "rcx/extMeasureRC.h"
#include "rcx/extRCap.h"
#include "rcx/extSegment.h"
#include "rcx/grids.h"
#include "utl/Logger.h"

#ifdef HI_ACC_1
#define FRINGE_UP_DOWN
#endif

using odb::dbNet;
using odb::dbRSeg;
using utl::RCX;

namespace rcx {

extSegment* extMeasureRC::CreateUpDownSegment(bool lookUp,
                                              Wire* w,
                                              int xy1,
                                              int len1,
                                              Wire* w2,
                                              Array1D<extSegment*>* segTable)
{
  if (len1 == 0) {
    return nullptr;
  }

  extSegment* s = _seqmentPool->alloc();

  s->set(_dir, w, xy1, len1, nullptr, nullptr);
  segTable->add(s);
  s->setUpDown(lookUp, w2);
  return s;
}
void extMeasureRC::FindSegmentsTrack(Wire* w1,
                                     int xy1,
                                     int len1,
                                     Wire* w2_next,
                                     uint32_t ii,
                                     Array1D<Wire*>* trackTable,
                                     bool lookUp,
                                     uint32_t dir,
                                     int maxDist,
                                     Array1D<extSegment*>* segTable)
{
  if (w2_next == nullptr) {
    if (ii >= trackTable->getCnt()) {
      return;
    }
    w2_next = trackTable->get(ii);
  }
  // DELETE uint32_t d = !dir;
  int dist = GetDistance(w1, w2_next);
  if (dist > maxDist) {
    // extSegment *s= CreateUpDownSegment(lookUp, w1, xy1, len1, nullptr,
    // segTable);
    CreateUpDownSegment(lookUp, w1, xy1, len1, nullptr, segTable);
    return;
  }
  if (xy1 + len1 < w2_next->getXY())  // no overlap and on the left
  {
    FindSegmentsTrack(w1,
                      xy1,
                      len1,
                      nullptr,
                      ii + 1,
                      trackTable,
                      lookUp,
                      dir,
                      maxDist,
                      segTable);
    return;
  }
  Wire* prev = nullptr;
  Wire* w2 = w2_next;
  for (; w2 != nullptr; w2 = w2->getNext()) {
    // TODO out of range on the right
    if (OverlapOnly(xy1, len1, w2->getXY(), w2->getLen())) {
      break;
    }

    if (prev != nullptr
        && Enclosed(
            xy1, xy1 + len1, prev->getXY() + prev->getLen(), w2->getXY())) {
      FindSegmentsTrack(w1,
                        xy1,
                        len1,
                        nullptr,
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
  if (w2 == nullptr)  // end or track
  {
    FindSegmentsTrack(w1,
                      xy1,
                      len1,
                      nullptr,
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
      // extSegment *s = new extSegment(dir, w1, xy1, xy2 - xy1, nullptr,
      // nullptr);
      CreateUpDownSegment(lookUp, w1, xy1, xy2 - xy1, w2, segTable);

      Wire* next = w2->getNext();
      uint32_t jj = next == nullptr ? ii + 1 : ii;
      FindSegmentsTrack(
          w1, xy2, -dx2, next, jj, trackTable, lookUp, dir, maxDist, segTable);
    }
  } else {  // Open Left
    FindSegmentsTrack(w1,
                      xy1,
                      dx1,
                      nullptr,
                      ii + 1,
                      trackTable,
                      lookUp,
                      dir,
                      maxDist,
                      segTable);

    if (dx2 >= 0) {  // covered Right
      // extSegment *s = new extSegment(dir, w1, w2->getXY(), xy1 + len1 -
      // w2->getXY(), nullptr, nullptr); extSegment *s =
      // CreateUpDownSegment(lookUp, w1, w2->getXY(), xy1 + len1 - w2->getXY(),
      // w2, segTable);
      CreateUpDownSegment(
          lookUp, w1, w2->getXY(), xy1 + len1 - w2->getXY(), w2, segTable);
    } else {  // not covered right
      //  extSegment *s = CreateUpDownSegment(lookUp, w1, w2->getXY(),
      //  w2->getLen(), w2, segTable);
      CreateUpDownSegment(lookUp, w1, w2->getXY(), w2->getLen(), w2, segTable);

      Wire* next = w2->getNext();
      uint32_t jj = next == nullptr ? ii + 1 : ii;
      FindSegmentsTrack(
          w1, xy2, -dx2, next, jj, trackTable, lookUp, dir, maxDist, segTable);
    }
  }
}

extSegment* extMeasureRC::CreateUpDownSegment(Wire* w,
                                              Wire* up,
                                              int xy1,
                                              int len1,
                                              Wire* down,
                                              Array1D<extSegment*>* segTable,
                                              int metOver,
                                              int metUnder)
{
  if (len1 == 0) {
    return nullptr;
  }

  extSegment* s = _seqmentPool->alloc();
  s->set(_dir, w, xy1, len1, up, down, metOver, metUnder);

  segTable->add(s);
  return s;
}
extSegment* extMeasureRC::GetNext(uint32_t ii,
                                  int& xy1,
                                  int& len1,
                                  Array1D<extSegment*>* segTable)
{
  if (ii < segTable->getCnt()) {
    extSegment* s = segTable->get(ii);
    xy1 = s->_xy;
    len1 = s->_len;
    return s;
  }
  return nullptr;
}
extSegment* extMeasureRC::GetNextSegment(uint32_t ii,
                                         Array1D<extSegment*>* segTable)
{
  if (ii < segTable->getCnt()) {
    extSegment* s = segTable->get(ii);
    return s;
  }
  return nullptr;
}
uint32_t extMeasureRC::FindUpDownSegments(Array1D<extSegment*>* upTable,
                                          Array1D<extSegment*>* downTable,
                                          Array1D<extSegment*>* segTable,
                                          int metOver,
                                          int metUnder)
{
  // metOver, metUnder is used in cross overlap
  uint32_t cnt = 0;
  uint32_t jj = 0;
  uint32_t ii = 0;
  extSegment* up = upTable->get(ii);
  extSegment* down = downTable->get(jj);
  int xy1 = up->_xy;
  int len1 = up->_len;

  while (up != nullptr && down != nullptr) {
    int dx2;
    int dx1 = GetDx1Dx2(xy1, len1, down, dx2);

    if (dx1 < 0 && down->_xy + down->_len < xy1)  // down on left side
    {
      CreateUpDownSegment(down->_wire,
                          nullptr,
                          down->_xy,
                          down->_len,
                          down->_down,
                          segTable,
                          -1,
                          metUnder);
      down = GetNextSegment(++jj, downTable);
      continue;
    }
    if (xy1 + len1 < down->_xy) {
      // no overlap and w2 too far on the right
      CreateUpDownSegment(
          up->_wire, up->_up, xy1, len1, nullptr, segTable, metOver, -1);
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
        if (up == nullptr) {
          if (dx1 < 0) {
            CreateUpDownSegment(down->_wire,
                                nullptr,
                                down->_xy,
                                -dx1,
                                down->_down,
                                segTable,
                                -1,
                                metUnder);
          }
          if (dx2 > 0) {
            CreateUpDownSegment(down->_wire,
                                nullptr,
                                down->_xy + down->_len - dx2,
                                dx2,
                                down->_down,
                                segTable,
                                -1,
                                metUnder);
          }
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
        if (down == nullptr) {
          CreateUpDownSegment(
              up->_wire, up->_up, xy1, len1, nullptr, segTable, metOver, -1);
          ii++;
          break;
        }
      }
    } else {  // Open Left
      CreateUpDownSegment(
          up->_wire, up->_up, xy1, dx1, nullptr, segTable, metOver, -1);

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
        if (up == nullptr) {
          if (dx2 > 0) {
            CreateUpDownSegment(down->_wire,
                                nullptr,
                                down->_xy + down->_len - dx2,
                                dx2,
                                down->_down,
                                segTable,
                                -1,
                                metUnder);
          }
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
        if (down == nullptr) {
          CreateUpDownSegment(
              up->_wire, up->_up, xy1, len1, nullptr, segTable, metOver, -1);
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
uint32_t extMeasureRC::CopySegments(bool up,
                                    Array1D<extSegment*>* upTable,
                                    uint32_t start,
                                    uint32_t end,
                                    Array1D<extSegment*>* segTable,
                                    int maxDist,
                                    int metOver,
                                    int metUnder)
{
  for (uint32_t kk = start; kk < end; kk++) {
    extSegment* s = upTable->get(kk);
    if (up) {
      CreateUpDownSegment(s->_wire, s->_up, s->_xy, s->_len, nullptr, segTable);
    } else {
      CreateUpDownSegment(
          s->_wire, nullptr, s->_xy, s->_len, s->_down, segTable);
    }
    /*
    extSegment *s = upTable->get(kk);
    if (up)
    {
        if (s->_dist>maxDist)
            continue;
        CreateUpDownSegment(s->_wire, s->_up, s->_xy, s->_len, nullptr,
    segTable);
    }
    else
    {
         if (s->_dist_down>maxDist)
            continue;
        CreateUpDownSegment(s->_wire, nullptr, s->_xy, s->_len, s->_down,
    segTable);
    }
    */
  }
  return 0;
}
void extMeasureRC::Print(FILE* fp, Array1D<Wire*>* segTable, const char* msg)
{
  // fprintf(fp, "%s\n", msg);
  for (uint32_t ii = 0; ii < segTable->getCnt(); ii++) {
    Wire* w = segTable->get(ii);
    PrintWire(fp, w, 0);
  }
}
void extMeasureRC::Print(FILE* fp,
                         Array1D<extSegment*>* segTable,
                         uint32_t d,
                         bool lookUp)
{
  for (uint32_t ii = 0; ii < segTable->getCnt(); ii++) {
    extSegment* s = segTable->get(ii);
    Print(fp, s, d, lookUp);
    // print coupled net
  }
}
void extMeasureRC::Print(FILE* fp, extSegment* s, uint32_t d, bool lookUp)
{
  int dist = lookUp ? s->_dist : s->_dist_down;
  Wire* w1 = s->_up;
  // DELETE int base1 = s->_ll[1];
  if (lookUp && s->_up != nullptr) {
    w1 = s->_up;
  } else if (!lookUp && s->_down != nullptr) {
    w1 = s->_down;
  }

  if (w1 == nullptr) {
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
                                  Wire* s,
                                  int dist,
                                  const char* prefix)
{
  if (fp == nullptr) {
    return;
  }
  fprintf(fp, "%s  ", prefix);
  if (s == nullptr) {
    fprintf(fp, "D%d\n", -1);
  } else {
    PrintWire(fp, s, s->getLevel());
  }
}
void extMeasureRC::PrintUpDown(FILE* fp, Array1D<extSegment*>* segTable)
{
  if (fp == nullptr) {
    return;
  }
  fprintf(fp, "Full Coupling Segments ---- \n");
  for (uint32_t ii = 0; ii < segTable->getCnt(); ii++) {
    extSegment* s = segTable->get(ii);
    PrintUpDown(fp, s);
    PrintUpDownNet(fp, s->_up, s->_dist, "\t");
    PrintUpDownNet(fp, s->_down, s->_dist_down, "\t");
  }
}
void extMeasureRC::PrintUpDown(FILE* fp, extSegment* s)
{
  if (fp == nullptr) {
    return;
  }
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
bool extMeasureRC::CheckOrdered(Array1D<extSegment*>* segTable)
{
  if (segTable->getCnt() <= 1) {
    return true;
  }

  for (uint32_t ii = 0; ii < segTable->getCnt() - 1; ii++) {
    extSegment* s1 = segTable->get(ii);
    extSegment* s2 = segTable->get(ii + 1);
    if (s1->_xy > s2->_xy) {
      return false;
    }
  }
  return true;
}
int extMeasureRC::CouplingFlow_new(uint32_t dir,
                                   uint32_t couplingDist,
                                   uint32_t diag_met_limit)
{
  uint32_t notOrderCnt = 0;
  uint32_t oneEmptyTable = 0;
  uint32_t oneCntTable = 0;
  uint32_t wireCnt = 0;
  bool dbgOverlaps = true;
  FILE* fp = OpenPrintFile(dir, "Segments");

  uint32_t limitTrackNum = 10;
  Array1D<extSegment*> upTable;
  Array1D<extSegment*> downTable;
  Array1D<extSegment*> verticalUpTable;
  Array1D<extSegment*> verticalDownTable;
  Array1D<extSegment*> segTable;
  Array1D<extSegment*> aboveTable;
  Array1D<extSegment*> belowTable;
  Array1D<extSegment*> whiteTable;

  Array1D<Wire*> UpTable;

  uint32_t colCnt = _search->getColCnt();
  Array1D<Wire*>** firstWireTable = allocMarkTable(colCnt);

  // TODO need to add in constructor/destructor
  _verticalPowerTable = new Array1D<Wire*>*[colCnt];
  for (uint32_t ii = 0; ii < colCnt; ii++) {
    _verticalPowerTable[ii] = new Array1D<Wire*>(4);
  }

  for (uint32_t level = 1; level < colCnt; level++) {
    Grid* netGrid = _search->getGrid(dir, level);
    upTable.resetCnt();

    uint32_t maxDist = 10 * netGrid->getPitch();
    for (uint32_t tr = 0; tr < netGrid->getTrackCnt(); tr++) {
      Track* track = netGrid->getTrackPtr(tr);
      if (track == nullptr) {
        continue;
      }

      ResetFirstWires(level, level + 1, dir, firstWireTable);
      for (Wire* w = track->getNextWire(nullptr); w != nullptr;
           w = w->getNext()) {
        if (w->isPower()) {
          continue;
        }

        uint32_t rsegId = w->getRsegId();
        if (rsegId == 0) {
          continue;
        }

        if (dbgOverlaps) {
          fprintf(fp, "\n");
          Print5wires(fp, w, level);
        }
        UpTable.resetCnt();
        uint32_t upWireCnt = FindAllNeigbors_up(w,
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

        Array1D<extSegment*> UpSegTable;
        bool lookUp = true;
        _dir = dir;
        FindSegmentsTrack(w,
                          w->getXY(),
                          w->getLen(),
                          nullptr,
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
          for (uint32_t ii = 1; ii < colCnt; ii++) {
            Print(fp, _verticalPowerTable[ii], "");
          }
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
        // DELETE uint32_t len = FindSegments(true, dir, maxDist, w, w->getXY(),
        // w->getLen(), w->_upNext, &upTable);
        FindSegments(true,
                     dir,
                     maxDist,
                     w,
                     w->getXY(),
                     w->getLen(),
                     w->getUpNext(),
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
                     w->getDownNext(),
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
                     w->getAboveNext(),
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
                     w->getBelowNext(),
                     &belowTable);
        if (dbgOverlaps) {
          fprintf(fp, "Below Diag Segments %d\n", belowTable.getCnt());
          Print(fp, &belowTable, dir, false);
        }

        // TODO: debugStart
        wireCnt++;
        if (upTable.getCnt() == 0 || downTable.getCnt() == 0) {
          oneEmptyTable++;
          // CreateUpDownSegment(w, nullptr, w->getXY(), w->getLen(), nullptr,
          // &segTable);
        }
        if (upTable.getCnt() == 1 || downTable.getCnt() == 1) {
          oneCntTable++;
        }
        if (!CheckOrdered(&upTable) || !CheckOrdered(&downTable)) {
          notOrderCnt++;
          BubbleSort(&upTable);
          BubbleSort(&downTable);
          if (!CheckOrdered(&upTable) || !CheckOrdered(&downTable)) {
            fprintf(stdout, "======> NOT SORTED after Buggble\n");
          }
        }
        if (upTable.getCnt() == 0
            && downTable.getCnt() > 0) {  // OpenEnded Down
          CopySegments(false, &downTable, 0, downTable.getCnt(), &segTable);
        } else if (upTable.getCnt() > 0
                   && downTable.getCnt() == 0) {  // OpenEnded Up
          CopySegments(true, &upTable, 0, upTable.getCnt(), &segTable);
        } else if (upTable.getCnt() == 1 && downTable.getCnt() > 0) {  // 1 up,
          FindUpDownSegments(&upTable, &downTable, &segTable);
        } else if (upTable.getCnt() > 0 && downTable.getCnt() == 1) {  // 1 up,
          FindUpDownSegments(&upTable, &downTable, &segTable);
        } else if (upTable.getCnt() > 0 && downTable.getCnt() > 0) {
          FindUpDownSegments(&upTable, &downTable, &segTable);
        }

        if (dbgOverlaps) {
          PrintUpDown(fp, &segTable);
          // PrintUpDown(stdout, &segTable);
        }
        BubbleSort(&segTable);
        if (!CheckOrdered(&segTable)) {
          fprintf(stdout, "======> segTable NOT SORTED after Buggble\n");
        }
        for (uint32_t ii = 0; ii < segTable.getCnt(); ii++) {
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
void extMeasureRC::BubbleSort(Array1D<extSegment*>* segTable)
{
  int n = segTable->getCnt();
  if (n < 2) {
    return;
  }

  for (uint32_t ii = 0; ii < n - 1; ii++) {
    bool swap = false;
    for (uint32_t jj = 0; jj < n - ii - 1; jj++) {
      extSegment* s1 = segTable->get(jj);
      extSegment* s2 = segTable->get(jj + 1);
      if (s1->_xy > s2->_xy) {
        segTable->set(jj, s2);
        segTable->set(jj + 1, s1);
        swap = true;
      }
    }
    if (!swap) {
      break;
    }
  }
}
void extMeasureRC::OverSubRC_dist_new(dbRSeg* rseg1,
                                      dbRSeg* rseg2,
                                      int ouCovered,
                                      int diagCovered,
                                      int srcCovered)
{
  int lenOverSub = _len - ouCovered;
  if (!(lenOverSub > 0)) {
    return;
  }

  bool over1 = false;
  bool openEnded = false;
  // note: _dist>0
  if (_diagResDist < 0) {
    openEnded = true;
  } else {
    over1 = true;
  }

  over1 = over1 && ((_dist != _diagResDist) && (_diagResDist == 200));
  _underMet = 0;
  for (uint32_t jj = 0; jj < _metRCTable.getCnt(); jj++) {
    extMetRCTable* rcModel = _metRCTable.get(jj);
    double fr = 0;
    double cc = 0;
    double tot = 0;
    /*
        if (isolated) {
          extDistRC* rc_last = rcModel->getOverFringeRC_last(_met, _width);
          if (rc_last == nullptr)
            continue;
          fr = 2 * rc_last->_fringe * lenOverSub;
          tot = _extMain->updateTotalCap(rseg1, fr, jj);
        }
        else */
    if (openEnded) {
      getOverRC_Open(rcModel, _width, _met, 0, _dist, _diagResDist);
      fr = _tmpRC->getFringe() * lenOverSub;
      if (rseg1 != nullptr) {
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

      cc = _tmpRC->getCoupling() * lenOverSub;
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
      float delta_cc
          = (2 * rc->getCoupling() - (rc0->getCoupling() + rc1->getCoupling()))
            * lenOverSub;
      cc += delta_cc;

      // float FR = getOver_over1(rcModel, _width, _met, 0, _diagResDist, _dist,
      // lenOverSub);

      float FR0 = _tmpRC->getFringeW() * lenOverSub;
      float FR1 = _tmpRC->getFringe() * lenOverSub;
      float CC = _tmpRC->getCoupling() * lenOverSub;

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
      fr = rc->getFringe() * lenOverSub;
      tot = _extMain->updateTotalCap(rseg1, fr, jj);
      if (IsDebugNet() && rseg1 != nullptr) {
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
      cc = rc->getCoupling() * lenOverSub;
      updateCoupCap(rseg1, rseg2, jj, cc, "OverSubRC_dist_new-else");
      tot += cc;

      bool useWeighFr = true;
      if (useWeighFr && rseg1 != nullptr) {
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
  dbRSeg* RSEG = rseg1 != nullptr ? rseg1 : rseg2;
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

  if (IsDebugNet()) {
    debugPrint(_extMain->getLogger(),
               RCX,
               "debug_net",
               1,
               "measureRC:"
               "C"
               "\t[BEGIN-OUD] ----- OverUnder/Diagonal RC ----- BEGIN");
  }

  //_diagLen = 0;
  int totLenCovered = 0;
  _lenOUtable->resetCnt();
  _diagFlow = true;
  if (_extMain->_ccContextDepth > 0) {
    if (!_diagFlow) {
      totLenCovered = measureOverUnderCap();
    } else {
      totLenCovered = measureDiagOU(1, 2);
    }
  }
  ouCovered_debug(totLenCovered);
  if (IsDebugNet()) {
    dbRSeg* rseg = rseg1 != nullptr ? rseg1 : rseg2;
    segInfo(_debugFP, "\t\tUPDATE_after_OU ", _netId, rseg->getId());
    segInfo(stdout, "\t\tUPDATE_after_OU ", _netId, rseg->getId());
  }
  if (USE_DB_UBITS) {
    totLenCovered = _extMain->GetDBcoords2(totLenCovered);
    _len = _extMain->GetDBcoords2(_len);
    _diagLen = _extMain->GetDBcoords2(_diagLen);
  }
  int lenOverSub = _len - totLenCovered;

  if (_diagLen > 0 && SUBTRACT_DIAG) {
    lenOverSub -= _diagLen;
  }

  lenOverSub = std::max(lenOverSub, 0);

  // Case where the geometric search returns no neighbor found
  // _dist is infinit
  if (_dist < 0) {
    totLenCovered = std::max(totLenCovered, 0);

    _underMet = 0;

    _no_debug = true;
    _no_debug = false;

    for (uint32_t jj = 0; jj < _metRCTable.getCnt(); jj++) {
      _rc[jj]->setRes(0);  // DF 022821 : Res non context based

      if (_rc[jj]->getFringe() > 0) {
        double fr = _rc[jj]->getFringe();
        fr += _rc[jj]->getFringeW();
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
      OverSubRC(rseg1, nullptr, totLenCovered, _diagLen, _len);
      return totLenCovered;
    }
  } else {  // dist based

    bool OpenEnded = _dist < 0 || _diagResDist < 0;
    bool over1
        = !OpenEnded && ((_dist != _diagResDist) && (_diagResDist == 200));

    _underMet = 0;
    for (uint32_t jj = 0; jj < _metRCTable.getCnt(); jj++) {
      if (IsDebugNet()) {
        printDebugRC(_debugFP, _rc[jj], "\tOU_TOTAL_dist", "\n");
        printDebugRC(stdout, _rc[jj], "\tOU_TOTAL_dist", "\n");
      }
      double tot1 = 0;
      double tot2 = 0;
      if (OpenEnded) {
        double tot_fr = _rc[jj]->getFringe();
        if (rseg1 != nullptr) {
          tot1 = _extMain->updateTotalCap(rseg1, tot_fr, jj);
          if (IsDebugNet()) {
            DebugUpdateValue(stdout,
                             "OVER_SUB_DIST_OpenEnded",
                             "FR",
                             rseg1->getId(),
                             _rc[jj]->getFringe(),
                             tot1);
            DebugUpdateValue(_debugFP,
                             "OVER_SUB_DIST_OpenEnded",
                             "FR",
                             rseg1->getId(),
                             _rc[jj]->getFringe(),
                             tot1);
          }
        }
        updateCoupCap(rseg1,
                      rseg2,
                      jj,
                      _rc[jj]->getCoupling(),
                      "OVER_SUB_DIST_OpenEnded");
      } else if (over1) {
        tot1 = _extMain->updateTotalCap(rseg1, _rc[jj]->getFringeW(), jj);
        if (IsDebugNet()) {
          DebugUpdateValue(stdout,
                           "OVER_SUB_DIST_Over1",
                           "FR",
                           rseg1->getId(),
                           _rc[jj]->getFringe(),
                           tot1);
          DebugUpdateValue(_debugFP,
                           "OVER_SUB_DIST_Over1",
                           "FR",
                           rseg1->getId(),
                           _rc[jj]->getFringe(),
                           tot1);
        }

        if (!_sameNetFlag) {
          double tot1
              = _extMain->updateTotalCap(rseg2, _rc[jj]->getFringe(), jj);
          if (IsDebugNet()) {
            DebugUpdateValue(stdout,
                             "OVER_SUB_DIST_Over1",
                             "FR",
                             rseg2->getId(),
                             _rc[jj]->getFringe(),
                             tot1);
            DebugUpdateValue(_debugFP,
                             "OVER_SUB_DIST_Over1",
                             "FR",
                             rseg2->getId(),
                             _rc[jj]->getFringe(),
                             tot1);
          }
        }
        updateCoupCap(
            rseg1, rseg2, jj, _rc[jj]->getCoupling(), "OVER_SUB_DIST_Over1");
      } else {
        double tot_fr = _rc[jj]->getFringe() + _rc[jj]->getFringeW();
        tot1 = _extMain->updateTotalCap(rseg1, tot_fr, jj);
        if (IsDebugNet() && rseg1 != nullptr) {
          DebugUpdateValue(
              stdout, "OVER_SUB_DIST_else", "FR", rseg1->getId(), tot_fr, tot1);
          DebugUpdateValue(_debugFP,
                           "OVER_SUB_DIST_else",
                           "FR",
                           rseg1->getId(),
                           tot_fr,
                           tot1);
        }
        tot2 = _extMain->updateTotalCap(rseg2, _rc[jj]->getFringe(), jj);
        if (IsDebugNet() && rseg2 != nullptr) {
          DebugUpdateValue(stdout,
                           "OVER_SUB_DIST_else",
                           "FR",
                           rseg2->getId(),
                           _rc[jj]->getFringe(),
                           tot2);
          DebugUpdateValue(_debugFP,
                           "OVER_SUB_DIST_else",
                           "FR",
                           rseg2->getId(),
                           _rc[jj]->getFringe(),
                           tot2);
        }
        updateCoupCap(
            rseg1, rseg2, jj, _rc[jj]->getCoupling(), "OVER_SUB_DIST_else");
      }
      tot1 += _rc[jj]->getCoupling();
      tot2 += _rc[jj]->getCoupling();
    }
    rcSegInfo();
    if (IsDebugNet()) {
      debugPrint(_extMain->getLogger(),
                 RCX,
                 "debug_net",
                 1,
                 "measureRC:"
                 "C"
                 "\t[END-OUD] ------ OverUnder/Diagonal RC ------ END");
    }
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
     if (rseg2!=nullptr) {
      segInfo(_debugFP, "\t\tOU_TOTAL_UPDATE_dist ", rseg2->getNet()->getId(),
    rseg2->getId()); segInfo(stdout, "\t\tOU_TOTAL_UPDATE_dist ",
    rseg2->getNet()->getId(), rseg2->getId());
    } */
  }

  return totLenCovered;
}

}  // namespace rcx
