// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "odb/db.h"
#include "rcx/array1.h"
#include "rcx/dbUtil.h"
#include "rcx/extMeasureRC.h"
#include "rcx/extRCap.h"
#include "rcx/grids.h"
#include "utl/Logger.h"

#ifdef HI_ACC_1
#define FRINGE_UP_DOWN
#endif
// #define CHECK_SAME_NET
// #define MIN_FOR_LOOPS

using odb::dbNet;

namespace rcx {

// Find immediate coupling neighbor wires in all directions and levels for every
// Wire
int extMeasureRC::FindCouplingNeighbors(uint32_t dir,
                                        uint32_t couplingDist,
                                        uint32_t diag_met_limit)
{
  uint32_t limitTrackNum = 10;
  Array1D<Wire*> firstWireTable;

  uint32_t colCnt = _search->getColCnt();
  for (uint32_t jj = 1; jj < colCnt; jj++) {
    Grid* netGrid = _search->getGrid(dir, jj);
    firstWireTable.resetCnt();

    for (uint32_t tr = 0; tr < netGrid->getTrackCnt(); tr++) {
      Track* track = netGrid->getTrackPtr(tr);
      if (track == nullptr) {
        continue;
      }
      ResetFirstWires(
          netGrid, &firstWireTable, tr, netGrid->getTrackCnt(), limitTrackNum);

      for (Wire* w = track->getNextWire(nullptr); w != nullptr;
           w = w->getNext()) {
        bool found = false;
        uint32_t start_next_track
            = tr;  // in case that track holds wires with different base
        for (uint32_t next_tr = start_next_track;
             !found && next_tr - tr < limitTrackNum
             && next_tr < netGrid->getTrackCnt();
             next_tr++) {
          Wire* first_wire;
          if (next_tr == tr) {
            first_wire = w->getNext();
          } else {
            first_wire = GetNextWire(netGrid, next_tr, &firstWireTable);
          }

          // PrintWire(stdout, first_wire, jj);
          Wire* w2 = FindOverlap_found(w, first_wire, found);
          if (w2 != nullptr && found) {
            firstWireTable.set(next_tr, w2);

            w->setUpNext(w2);
            break;
          }
          if (w2 != nullptr && !found) {
            firstWireTable.set(next_tr, w2);
            w->setUpNext(w2);
            break;
          }
          if (first_wire != nullptr) {
            firstWireTable.set(next_tr, first_wire);
            break;
          }
        }
      }
    }
  }
  FindCouplingNeighbors_down(dir, couplingDist, diag_met_limit);
  if (_extMain->_dbgOption > 1) {
    PrintAllGrids(dir, OpenPrintFile(dir, "couple"), 1);
  }

  uint32_t cnt1 = FindDiagonalNeighbors_vertical_up(
      dir, couplingDist, diag_met_limit, 2, 0, false);
  if (_extMain->_dbgOption > 1) {
    fprintf(stdout,
            "%d 1st Pass: FindDiagonalNeighbors_vertical_up met_limit=2 "
            "trackLimit=0",
            cnt1);
    PrintAllGrids(dir, OpenPrintFile(dir, "vertical_up.pass1"), 2);
  }
  uint32_t cnt2 = FindDiagonalNeighbors_vertical_down(
      dir, couplingDist, diag_met_limit, 2, 0, false);
  if (_extMain->_dbgOption > 1) {
    fprintf(stdout,
            "%d 1st Pass: FindDiagonalNeighbors_vertical_down met_limit=2 "
            "trackLimit=0",
            cnt2);
    PrintAllGrids(dir, OpenPrintFile(dir, "vertical_down.pass1"), 2);
  }
  return 0;
}
Wire* extMeasureRC::SetUpDown(Wire* w2,
                              int next_tr,
                              bool found,
                              Wire* first_wire,
                              Array1D<Wire*>* firstWireTable)
{
  if (w2 != nullptr && found) {
    firstWireTable->set(next_tr, w2);
    return w2;
  }
  if (w2 != nullptr && !found) {
    firstWireTable->set(next_tr, w2);
  }
  if (first_wire != nullptr) {
    firstWireTable->set(next_tr, first_wire);
  }
  return nullptr;
}
int extMeasureRC::FindCouplingNeighbors_down(uint32_t dir,
                                             uint32_t couplingDist,
                                             uint32_t diag_met_limit)
{
  uint32_t limitTrackNum = 10;
  Array1D<Wire*> firstWireTable;
  uint32_t colCnt = _search->getColCnt();
  for (uint32_t jj = 1; jj < colCnt; jj++) {
    Grid* netGrid = _search->getGrid(dir, jj);
    firstWireTable.resetCnt();

    int tr = netGrid->getTrackCnt() - 1;
    for (; tr >= 0; tr--) {
      Track* track = netGrid->getTrackPtr((uint32_t) tr);
      if (track == nullptr) {
        continue;
      }

      int start_track_index = tr - 10 >= 0 ? tr - 10 : 0;
      ResetFirstWires(
          netGrid, &firstWireTable, start_track_index, tr, limitTrackNum);

      Wire* first_wire1 = track->getNextWire(nullptr);
      for (Wire* w = first_wire1; w != nullptr; w = w->getNext()) {
        bool found = false;
        for (int next_tr = tr - 1;
             !found && tr - next_tr < limitTrackNum && next_tr >= 0;
             next_tr--) {
          Wire* first_wire = GetNextWire(netGrid, next_tr, &firstWireTable);
          Wire* w2 = FindOverlap_found(w, first_wire, found);
          if (w2 != nullptr && found) {
            firstWireTable.set(next_tr, w2);
            w->setDownNext(w2);
            break;
          }
          if (w2 != nullptr && !found) {
            firstWireTable.set(next_tr, w2);
            break;
          }
          if (first_wire != nullptr) {
            firstWireTable.set(next_tr, first_wire);
            break;
          }
        }
      }
    }
  }
  return 0;
}
void extMeasureRC::ResetFirstWires(Grid* netGrid,
                                   Array1D<Wire*>* firstWireTable,
                                   int tr1,
                                   int trCnt,
                                   uint32_t limitTrackNum)
{
  for (uint32_t ii = tr1; ii - tr1 < limitTrackNum && ii < trCnt; ii++) {
    Track* track = netGrid->getTrackPtr(ii);
    if (track == nullptr) {
      firstWireTable->set(ii, nullptr);
      continue;
    }
    Wire* w1 = track->getNextWire(nullptr);
    firstWireTable->set(ii, w1);
  }
}
void extMeasureRC::ResetFirstWires(uint32_t m1,
                                   uint32_t m2,
                                   uint32_t dir,
                                   Array1D<Wire*>** firstWireTable)
{
  for (uint32_t level = m1; level < m2; level++)  // for layers above
  {
    Grid* upGrid = _search->getGrid(dir, level);
    uint32_t n = upGrid->getTrackCnt();
    ResetFirstWires(upGrid, firstWireTable[level], 0, n, n);
  }
}

Wire* extMeasureRC::FindOverlap(Wire* w, Array1D<Wire*>* firstWireTable, int tr)
{
  Wire* first_wire = firstWireTable->geti(tr);
  if (first_wire == nullptr) {
    return nullptr;
  }

  int xy1 = w->getXY();
  int len1 = w->getLen();

  Wire* w2 = first_wire;
  for (; w2 != nullptr; w2 = w2->getNext()) {
    int xy2 = w2->getXY();
    int len2 = w2->getLen();

    Wire* w3 = w2->getNext();
    if (w3 != nullptr && xy1 >= xy2 + len2
        && xy1 + len1 <= w3->getXY())  // in between!
    {
      firstWireTable->set(tr, w3);
      return nullptr;
    }
    firstWireTable->set(tr, w3);
    if (xy1 + len1 <= xy2) {  // on the left
      continue;
    }
    if (xy1 >= xy2 + len2) {  // on the right
      continue;
    }

    if (xy1 >= xy2 && xy1 <= xy2 + len2) {
      return w2;
    }
    if (xy2 >= xy1 && xy2 <= xy1 + len1) {
      return w2;
    }
    if (xy1 + len1 >= xy2 && xy1 + len1 <= xy2 + len2) {
      return w2;
    }
    if (xy2 + len2 >= xy1 && xy2 + len2 <= xy1 + len1) {
      return w2;
    }
  }
  return w2;
}
Wire* extMeasureRC::FindOverlapWire(Wire* w, Wire* first_wire)
{
  bool white_overlap_check = true;
  if (first_wire == nullptr) {
    return nullptr;
  }

  // int xy1 = w->getXY()+w->getWidth();
  // int len1 = w->getLen()-2*w->getWidth();
  int xy1 = w->getXY();
  int len1 = w->getLen();

  Wire* prev = nullptr;
  for (Wire* w2 = first_wire; w2 != nullptr; w2 = w2->getNext()) {
    int xy2 = w2->getXY();
    int len2 = w2->getLen();

    if (xy1 + len1 <= xy2)  // on the left
    {
      prev = w2;
      continue;
    }
    if (white_overlap_check && prev != nullptr
        && Enclosed(xy1, xy1 + len1, prev->getXY(), w2->getXY())) {
      return prev;
    }

    if (xy1 >= xy2 + len2)  // on the right
    {
      prev = w2;
      continue;
    }
    if (OverlapOnly(xy1, len1, xy2, len2)) {
      return w2;
    }
    prev = w2;
  }
  return nullptr;
}
Wire* extMeasureRC::FindOverlap(Wire* w, Wire* first_wire)
{
  bool white_overlap_check = true;
  if (first_wire == nullptr) {
    return nullptr;
  }

  // int xy1 = w->getXY()+w->getWidth();
  // int len1 = w->getLen()-2*w->getWidth();
  int xy1 = w->getXY();
  int len1 = w->getLen();

  Wire* prev = nullptr;
  for (Wire* w2 = first_wire; w2 != nullptr; w2 = w2->getNext()) {
    int xy2 = w2->getXY();
    int len2 = w2->getLen();

    if (xy1 + len1 <= xy2)  // on the left
    {
      prev = w2;
      continue;
    }
    if (xy1 >= xy2 + len2)  // on the right
    {
      prev = w2;
      continue;
    }
    if (OverlapOnly(xy1, len1, xy2, len2)) {
      if (xy1 >= xy2) {
        return w2;
      }
      if (prev != nullptr) {
        return prev;
      }
      return w2;
    }
    if (white_overlap_check && prev != nullptr
        && Enclosed(xy1, xy1 + len1, prev->getXY(), w2->getXY())) {
      return prev;
    }
    prev = w2;
  }
  return first_wire;
}
Wire* extMeasureRC::FindOverlap_found(Wire* w, Wire* first_wire, bool& found)
{
  bool white_overlap_check = true;
  if (first_wire == nullptr) {
    return nullptr;
  }

  // int xy1 = w->getXY()+w->getWidth();
  // int len1 = w->getLen()-2*w->getWidth();
  int xy1 = w->getXY();
  int len1 = w->getLen();

  Wire* prev = nullptr;
  for (Wire* w2 = first_wire; w2 != nullptr; w2 = w2->getNext()) {
    int xy2 = w2->getXY();
    int len2 = w2->getLen();

    if (xy1 + len1 <= xy2)  // on the left
    {
      prev = w2;
      return prev;
    }
    if (xy1 >= xy2 + len2)  // on the right
    {
      prev = w2;
      continue;
    }
    if (OverlapOnly(xy1, len1, xy2, len2)) {
      found = true;
      if (xy1 >= xy2) {
        return w2;
      }
      if (prev != nullptr) {
        return prev;
      }
      return w2;
    }
    if (white_overlap_check && prev != nullptr
        && Enclosed(xy1, xy1 + len1, prev->getXY(), w2->getXY())) {
      return prev;
    }
    prev = w2;
  }
  return first_wire;
}
bool extMeasureRC::IsSegmentOverlap(int x1, int len1, int x2, int len2)
{
  if (x1 + len1 <= x2) {  // on the left
    return false;
  }
  if (x1 >= x2 + len2) {  // on the right
    return false;
  }
  if (x1 + len1 <= x2) {  // on the left
    return false;
  }

  if (x1 >= x2 && x1 <= x2 + len2) {
    return true;
  }
  if (x2 >= x1 && x2 <= x1 + len1) {
    return true;
  }
  if (x1 + len1 >= x2 && x1 + len1 <= x2 + len2) {
    return true;
  }
  if (x2 + len2 >= x1 && x2 + len2 <= x1 + len1) {
    return true;
  }
  return false;
}
bool extMeasureRC::IsOverlap(Wire* w, Wire* w2)
{
  if (w2 == nullptr) {
    return false;
  }

  if (!IsSegmentOverlap(w->getXY(), w->getLen(), w2->getXY(), w2->getLen())) {
    return false;
  }

  bool overlap = IsSegmentOverlap(
      w->getBase(), w->getWidth(), w2->getBase(), w2->getWidth());
  return overlap;
}
void extMeasureRC::PrintWire(FILE* fp,
                             Wire* w,
                             int level,
                             const char* prefix,
                             const char* postfix)
{
  if (w == nullptr) {
    return;
  }
  uint32_t met = w->getLevel();
  uint32_t dir;
  int x1, y1, x2, y2;
  w->getCoords(&x1, &y1, &x2, &y2, &dir);

  dbNet* net = w->getNet();
  fprintf(fp,
          "%s%7.3f %7.3f   %7.3f %7.3f  %6dL M%d D%d W%d s%d r%d N%d %s%s\n",
          prefix,
          GetDBcoords(x1),
          GetDBcoords(y1),
          GetDBcoords(x2),
          GetDBcoords(y2),
          w->getLen(),
          met,
          dir,
          w->getWidth(),
          w->getOtherId(),
          w->getRsegId(),
          net->getId(),
          net->getConstName(),
          postfix);
}
void extMeasureRC::Print5wires(FILE* fp, Wire* w, uint32_t level)
{
  fprintf(fp, "---------------------------------- \n");
  int jj = level;
  if (w->getUpNext()) {
    if (w->getUpNext()->getUpNext()) {
      PrintWire(fp, w->getUpNext()->getUpNext(), jj);
    }
    PrintWire(fp, w->getUpNext(), 0);
  }
  fprintf(fp, "\n");
  PrintWire(fp, w, jj, "", " _____ ");
  fprintf(fp, "\n");
  if (w->getDownNext()) {
    PrintWire(fp, w->getDownNext(), jj);
    if (w->getDownNext()->getDownNext()) {
      PrintWire(fp, w->getDownNext()->getDownNext(), jj);
    }
  }
  fprintf(fp, "---------------------------------- \n");
}
void extMeasureRC::PrintDiagwires(FILE* fp, Wire* w, uint32_t level)
{
  if (w->getAboveNext()) {
    fprintf(fp, "Vertical up::\n");
    if (w->getAboveNext()->getAboveNext()) {
      PrintWire(fp,
                w->getAboveNext()->getAboveNext(),
                w->getAboveNext()->getAboveNext()->getLevel());
    }
    PrintWire(fp, w->getAboveNext(), w->getAboveNext()->getLevel());
  }
  fprintf(fp, "-------------------------------------------------------\n");
  PrintWire(fp, w, w->getLen());
  fprintf(fp, "-------------------------------------------------------\n");
  if (w->getBelowNext() != nullptr) {
    fprintf(fp, "Vertical Down:\n");
    PrintWire(fp, w->getBelowNext(), w->getBelowNext()->getLevel());
    if (w->getBelowNext()->getBelowNext() != nullptr) {
      PrintWire(fp,
                w->getBelowNext()->getBelowNext(),
                w->getBelowNext()->getBelowNext()->getLevel());
    }
  }
  fprintf(fp, "\n");
}
Wire* extMeasureRC::GetNextWire(Grid* netGrid,
                                uint32_t tr,
                                Array1D<Wire*>* firstWireTable)
{
  Track* next_track = netGrid->getTrackPtr(tr);
  if (next_track == nullptr) {
    return nullptr;
  }

  Wire* first_wire = nullptr;
  if (tr < firstWireTable->getSize()) {
    first_wire = firstWireTable->geti(tr);
  }
  if (first_wire == nullptr) {
    first_wire = next_track->getNextWire(nullptr);
  }

  return first_wire;
}
Wire* extMeasureRC::FindOverlap(Wire* w,
                                Grid* netGrid,
                                uint32_t tr,
                                Array1D<Wire*>* firstWireTable)
{
  Wire* first_wire = GetNextWire(netGrid, tr, firstWireTable);
  if (first_wire == nullptr) {
    return nullptr;
  }

  Wire* w2 = FindOverlap(w, first_wire);
  return w2;
}
bool extMeasureRC::CheckWithNeighbors(Wire* w, Wire* prev)
{
  if (w->getAboveNext()) {
    return true;
  }

  if (prev != nullptr && IsOverlap(w, prev->getAboveNext())) {
    w->setAboveNext(prev->getAboveNext());
    prev = w;
    return true;
  }
  if (w->getDownNext() != nullptr
      && IsOverlap(w, w->getDownNext()->getAboveNext())) {
    w->setAboveNext(w->getDownNext()->getAboveNext());
    return true;
  }
  return false;
}
bool extMeasureRC::CheckWithNeighbors_below(Wire* w, Wire* prev)
{
  if (w->getBelowNext() != nullptr) {
    return true;
  }

  if (prev != nullptr && IsOverlap(w, prev->getBelowNext())) {
    w->setBelowNext(prev->getBelowNext());
    prev = w;
    return true;
  }
  if (w->getDownNext() != nullptr
      && IsOverlap(w, w->getDownNext()->getBelowNext())) {
    w->setBelowNext(w->getDownNext()->getBelowNext());
    return true;
  }
  return false;
}
Array1D<Wire*>** extMeasureRC::allocMarkTable(uint32_t n)
{
  Array1D<Wire*>** tbl = new Array1D<Wire*>*[n];
  for (uint32_t ii = 0; ii < n; ii++) {
    tbl[ii] = new Array1D<Wire*>(128);
  }
  return tbl;
}
void extMeasureRC::DeleteMarkTable(Array1D<Wire*>** tbl, uint32_t n)
{
  for (uint32_t ii = 0; ii < n; ii++) {
    delete tbl[ii];
  }
  delete[] tbl;
}
FILE* extMeasureRC::OpenFile(const char* name, const char* perms)
{
  FILE* fp = fopen(name, perms);
  if (fp == nullptr) {
    fprintf(stderr, "Cannot open file %s with %s\n", name, perms);
    exit(1);
  }
  return fp;
}
FILE* extMeasureRC::OpenPrintFile(uint32_t dir, const char* name)
{
  char buf[100];
  sprintf(buf, "%s.%d", name, dir);
  return OpenFile(buf, "w");
}
int extMeasureRC::PrintAllGrids(uint32_t dir, FILE* fp, uint32_t mode)
{
  uint32_t colCnt = _search->getColCnt();
  for (uint32_t jj = 1; jj < colCnt; jj++)  // For all Layers
  {
    const char* vert = dir == 1 ? "H" : "V";
    Grid* netGrid = _search->getGrid(dir, jj);
    fprintf(fp,
            "Metal %d  Dir %d %s Tracks=%d "
            "----------------------------------------- \n",
            jj,
            dir,
            vert,
            netGrid->getTrackCnt());
    for (uint32_t tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Track* track = netGrid->getTrackPtr(tr);
      if (track == nullptr) {
        continue;
      }

      fprintf(fp, "Track %d M%d %s %d \n", tr, jj, vert, track->getBase());
      for (Wire* w = track->getNextWire(nullptr); w != nullptr;
           w = w->getNext()) {
        if (mode == 0) {
          PrintWire(fp, w, jj);
        } else if (mode == 1) {  // coupling neighbors
          Print5wires(fp, w, jj);
        } else if (mode == 2) {  // diagonal coupling neighbors
          PrintDiagwires(fp, w, jj);
        }
      }
    }
  }
  return 0;
}

int extMeasureRC::ConnectWires(uint32_t dir)
{
  if (_extMain->_dbgOption > 1) {
    FILE* file = OpenPrintFile(dir, "wires.org");
    PrintAllGrids(dir, file, 0);
    fclose(file);
  }

  uint32_t cnt = 0;

  uint32_t colCnt = _search->getColCnt();
  for (uint32_t jj = 1; jj < colCnt; jj++)  // For all Layers
  {
    Grid* netGrid = _search->getGrid(dir, jj);
    for (uint32_t tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Track* track = netGrid->getTrackPtr(tr);
      if (track == nullptr) {
        continue;
      }

      ConnectAllWires(track);
      /* DEBUG
    if (ConnectAllWires(track)) {
      if (_extMain->_dbgOption > 1) {
        for (Wire* w = track->getNextWire(nullptr); w != nullptr;
             w = w->getNext())
          PrintWire(stdout, w, jj);
      }
    }
    */
    }
  }
  if (_extMain->_dbgOption > 1) {
    FILE* file = OpenPrintFile(dir, "wires");
    PrintAllGrids(dir, file, 0);
    fclose(file);
  }
  return cnt;
}
uint32_t extMeasureRC::ConnectAllWires(Track* track)
{
  Array1D<Wire*> tbl(128);
  uint32_t ii = track->getGrid()->searchLowMarker();
  int first_marker_index = -1;
  for (; ii <= track->getGrid()->searchHiMarker(); ii++) {
    for (Wire* w = track->getMarker(ii); w != nullptr; w = w->getNext()) {
      tbl.add(w);
      if (first_marker_index < 0) {
        first_marker_index = ii;
      }
    }
  }
  if (tbl.getCnt() == 0) {
    return 0;
  }

  for (uint32_t ii = 0; ii < tbl.getCnt() - 1; ii++) {
    Wire* w = tbl.get(ii);
    if (w->getNext() == nullptr) {
      Wire* w1 = tbl.get(ii + 1);
      w->setNext(w1);
    }
  }
  bool swap = false;
  if (tbl.getCnt() <= 1) {
    return false;
  }
  for (uint32_t ii = 0; ii < tbl.getCnt() - 1; ii++) {
    Wire* prev = nullptr;
    if (ii > 0) {
      prev = tbl.get(ii - 1);
    }
    Wire* w1 = tbl.get(ii);
    Wire* w2 = tbl.get(ii + 1);
    if (w1->getXY() == w2->getXY()) {
      if (w1->getBase() > w2->getBase()) {
        tbl.set(ii, w2);
        tbl.set(ii + 1, w1);
        w1->setNext(w2->getNext());
        w2->setNext(w1);
        if (ii > 0) {
          prev->setNext(w2);
        }
        swap = true;
      }
    }
  }
  Wire* w0 = tbl.get(0);
  track->setMarker(first_marker_index, w0);

  return swap;
}

int extMeasureRC::FindDiagonalNeighbors(uint32_t dir,
                                        uint32_t couplingDist,
                                        uint32_t diag_met_limit,
                                        uint32_t lookUpLevel,
                                        uint32_t limitTrackNum)
{
  uint32_t cnt = 0;

  uint32_t levelCnt = _search->getColCnt();
  Array1D<Wire*>** firstWireTable = allocMarkTable(levelCnt);

  for (uint32_t jj = 1; jj < levelCnt; jj++)  // For all Layers
  {
    Grid* netGrid = _search->getGrid(dir, jj);
    for (uint32_t tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Track* track = netGrid->getTrackPtr(tr);
      if (track == nullptr) {
        continue;
      }

      uint32_t m1 = jj + 1;
      uint32_t m2 = jj + 1 + lookUpLevel;
      m2 = std::min(m2, levelCnt);

      ResetFirstWires(m1, m2, dir, firstWireTable);

      Wire* prev = nullptr;
      Wire* first_wire1 = track->getNextWire(nullptr);
      for (Wire* w = first_wire1; w != nullptr;
           w = w->getNext())  // for all wires in the track
      {
        if (CheckWithNeighbors(w, prev)) {
          prev = w;
          continue;
        }
        prev = w;
        for (uint32_t level = m1; level < m2; level++)  // for layers above
        {
          Grid* upGrid = _search->getGrid(dir, level);
          int up_track_num = upGrid->getTrackNum1(w->getBase());
          int start_track = up_track_num - limitTrackNum >= 0
                                ? up_track_num - limitTrackNum
                                : 0;
          int end_track = up_track_num + limitTrackNum + 1;

          bool found = false;
          for (uint32_t next_tr = start_track;
               next_tr < end_track && next_tr < netGrid->getTrackCnt();
               next_tr++)  // for tracks overlapping wire
          {
            Wire* w2 = FindOverlap(w, netGrid, next_tr, firstWireTable[level]);
            if (w2 == nullptr) {
              continue;
            }

            firstWireTable[level]->set(next_tr, w2);
            w->setAboveNext(w2);
            found = true;
            cnt++;
            break;
          }
          if (found) {
            break;
          }
        }
      }
    }
  }
  DeleteMarkTable(firstWireTable, levelCnt);
  return cnt;
}
int extMeasureRC::FindDiagonalNeighbors_vertical_power(
    uint32_t dir,
    Wire* w,
    uint32_t couplingDist,
    uint32_t diag_met_limit,
    uint32_t limitTrackNum,
    Array1D<Wire*>** upWireTable)
{
  uint32_t cnt = 0;
  uint32_t current_met = w->getLevel();
  for (uint32_t level = 1; level < _search->getColCnt();
       level++)  // For all Layers
  {
    if (level == current_met) {
      continue;
    }
    upWireTable[level]->resetCnt();

    Grid* grid = _search->getGrid(dir, level);
    int up_track_num = grid->getTrackNum1(w->getBase());
    int start_track
        = up_track_num - limitTrackNum >= 0 ? up_track_num - limitTrackNum : 0;
    int end_track = up_track_num + limitTrackNum + 1;

    for (uint32_t next_tr = start_track;
         next_tr < end_track && next_tr < grid->getTrackCnt();
         next_tr++)  // for tracks overlapping wire
    {
      Track* next_track = grid->getTrackPtr(next_tr);
      if (next_track == nullptr) {
        continue;
      }
      Wire* first = next_track->getNextWire(nullptr);
      if (first == nullptr) {
        continue;
      }
      if (!first->isPower()) {  // assume power nets occupy entire track TODO:
                                // optimize
        continue;
      }
      Wire* w2 = FindOverlap(w, first);
      if (w2 == nullptr) {
        continue;
      }
      if (GetDistance(w, w2) > 0) {
        continue;
      }

      upWireTable[level]->add(w2);
      cnt++;
      break;
    }
  }
  return cnt;
}
Wire* extMeasureRC::FindDiagonalNeighbors_vertical_up_down(
    Wire* w,
    bool& found,
    uint32_t dir,
    uint32_t level,
    uint32_t couplingDist,
    uint32_t limitTrackNum,
    Array1D<Wire*>** firstWireTable)
{
  Grid* upGrid = _search->getGrid(dir, level);
  int up_track_num = upGrid->getTrackNum1(w->getBase());
  int start_track
      = up_track_num - limitTrackNum >= 0 ? up_track_num - limitTrackNum : 0;
  int end_track = up_track_num + limitTrackNum + 1;
  //  if (up_track_num==3329 && level==4 && dir==1 && w->getXY()==1093580 &&
  //  w->getBase()==1331980)
  //    PrintWire(stdout, w, level);

  found = false;
  for (uint32_t next_tr = start_track;
       next_tr < end_track && next_tr < upGrid->getTrackCnt();
       next_tr++)  // for tracks overlapping wire
  {
    Wire* first = GetNextWire(upGrid, next_tr, firstWireTable[level]);
    if (first != nullptr && first->isPower())  // assume power nets occupy
                                               // entire track TODO: optimize
    {
      found = true;
      return nullptr;
    }
    Wire* w2 = FindOverlap(w, first);
    if (w2 == nullptr) {
      continue;
    }

    firstWireTable[level]->set(next_tr, w2);
    found = true;
    return w2;
  }
  return nullptr;
}
int extMeasureRC::FindDiagonalNeighbors_vertical_up(uint32_t dir,
                                                    uint32_t couplingDist,
                                                    uint32_t diag_met_limit,
                                                    uint32_t lookUpLevel,
                                                    uint32_t limitTrackNum,
                                                    bool skipCheckNeighbors)
{
  uint32_t cnt = 0;
  int levelCnt = _search->getColCnt();
  Array1D<Wire*>** firstWireTable = allocMarkTable(levelCnt);

  for (uint32_t jj = 1; jj < levelCnt - 1; jj++)  // For all Layers
  {
    Grid* netGrid = _search->getGrid(dir, jj);
    for (uint32_t tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Track* track = netGrid->getTrackPtr(tr);
      if (track == nullptr) {
        continue;
      }

      uint32_t m1 = jj + 1;
      uint32_t m2 = jj + 1 + lookUpLevel;
      m2 = std::min<uint32_t>(m2, levelCnt);

      ResetFirstWires(m1, m2, dir, firstWireTable);

      Wire* prev = nullptr;
      Wire* first_wire1 = track->getNextWire(nullptr);
      for (Wire* w = first_wire1; w != nullptr;
           w = w->getNext())  // for all wires in the track
      {
        if (w->isPower() || w->getAboveNext() != nullptr) {
          continue;
        }

        prev = w;
        if (!skipCheckNeighbors && CheckWithNeighbors(w, prev)) {
          continue;
        }

        for (uint32_t level = m1; level < m2; level++)  // for layers above
        {
          bool found = false;
          Wire* w2 = FindDiagonalNeighbors_vertical_up_down(w,
                                                            found,
                                                            dir,
                                                            level,
                                                            couplingDist,
                                                            limitTrackNum,
                                                            firstWireTable);
          if (w2 != nullptr) {
            w->setAboveNext(w2);
            break;
          }
          if (found) {
            break;
          }
        }
      }
    }
  }
  DeleteMarkTable(firstWireTable, levelCnt);
  return cnt;
}
int extMeasureRC::FindDiagonalNeighbors_vertical_down(uint32_t dir,
                                                      uint32_t couplingDist,
                                                      uint32_t diag_met_limit,
                                                      uint32_t lookUpLevel,
                                                      uint32_t limitTrackNum,
                                                      bool skipCheckNeighbors)
{
  uint32_t cnt = 0;
  int levelCnt = _search->getColCnt();
  Array1D<Wire*>** firstWireTable = allocMarkTable(levelCnt);

  for (int jj = levelCnt - 1; jj > 1; jj--)  // For all Layers going down
  {
    Grid* netGrid = _search->getGrid(dir, jj);
    for (uint32_t tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Track* track = netGrid->getTrackPtr(tr);
      if (track == nullptr) {
        continue;
      }

      int m1 = jj - 1;
      int m2 = jj - 1 - lookUpLevel;
      m2 = std::max(m2, 1);

      ResetFirstWires(m2, m1, dir, firstWireTable);

      Wire* prev = nullptr;
      Wire* first_wire1 = track->getNextWire(nullptr);
      for (Wire* w = first_wire1; w != nullptr;
           w = w->getNext())  // for all wires in the track
      {
        if (w->isPower() || w->getAboveNext() != nullptr) {
          continue;
        }

        prev = w;
        if (!skipCheckNeighbors && CheckWithNeighbors(w, prev)) {
          continue;
        }

        for (int level = m1; level > m2; level--)  // for layers above
        {
          bool found = false;
          Wire* w2 = FindDiagonalNeighbors_vertical_up_down(w,
                                                            found,
                                                            dir,
                                                            level,
                                                            couplingDist,
                                                            limitTrackNum,
                                                            firstWireTable);
          if (w2 != nullptr) {
            w->setBelowNext(w2);
            break;
          }
          if (found) {
            break;
          }
        }
      }
    }
  }
  DeleteMarkTable(firstWireTable, levelCnt);
  return cnt;
}
int extMeasureRC::FindDiagonalNeighbors_down(uint32_t dir,
                                             uint32_t couplingDist,
                                             uint32_t diag_met_limit,
                                             uint32_t lookUpLevel,
                                             uint32_t limitTrackNum)
{
  uint32_t cnt = 0;

  uint32_t levelCnt = _search->getColCnt();
  Array1D<Wire*>** firstWireTable = allocMarkTable(levelCnt);

  for (int jj = 1; jj < 4 && jj < levelCnt; jj++)  // For all Layers
  {
    int m1 = jj - 1;
    if (m1 <= 0) {
      continue;
    }
    int m2 = jj - 1 - lookUpLevel;
    m2 = std::max(m2, 1);

    Grid* netGrid = _search->getGrid(dir, jj);
    for (uint32_t tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Track* track = netGrid->getTrackPtr(tr);
      if (track == nullptr) {
        continue;
      }

      ResetFirstWires(m2, jj, dir, firstWireTable);

      Wire* prev = nullptr;
      Wire* first_wire1 = track->getNextWire(nullptr);
      for (Wire* w = first_wire1; w != nullptr;
           w = w->getNext())  // for all wires in the track
      {
        if (w->getXY() == 20500 && w->getBase() == 6500) {
          Print5wires(stdout, w, w->getLevel());
        }
        if (CheckWithNeighbors_below(w, prev)) {
          prev = w;
          continue;
        }
        prev = w;
        for (int level = m1; level >= m2; level--)  // for layers above
        {
          Grid* upGrid = _search->getGrid(dir, level);
          int up_track_num = upGrid->getTrackNum1(w->getBase());
          int start_track = up_track_num - limitTrackNum >= 0
                                ? up_track_num - limitTrackNum
                                : 0;
          int end_track = up_track_num + limitTrackNum + 1;

          bool found = false;
          for (uint32_t next_tr = start_track;
               next_tr < end_track && next_tr < netGrid->getTrackCnt();
               next_tr++)  // for tracks overlapping wire
          {
            Wire* w2 = FindOverlap(w, netGrid, next_tr, firstWireTable[level]);
            if (w2 == nullptr) {
              continue;
            }

            firstWireTable[level]->set(next_tr, w2);
            w->setBelowNext(w2);
            found = true;
            cnt++;
            break;
          }
          if (found) {
            break;
          }
        }
      }
    }
  }
  DeleteMarkTable(firstWireTable, levelCnt);
  return cnt;
}
}  // namespace rcx
