
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

// Find immediate coupling neighbor wires in all directions and levels for every
// Ath__wire
int extMeasureRC::FindCouplingNeighbors(uint dir,
                                        uint couplingDist,
                                        uint diag_met_limit)
{
  uint limitTrackNum = 10;
  Ath__array1D<Ath__wire*> firstWireTable;

  uint colCnt = _search->getColCnt();
  for (uint jj = 1; jj < colCnt; jj++) {
    Ath__grid* netGrid = _search->getGrid(dir, jj);
    firstWireTable.resetCnt();

    for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++) {
      Ath__track* track = netGrid->getTrackPtr(tr);
      if (track == NULL)
        continue;
      ResetFirstWires(
          netGrid, &firstWireTable, tr, netGrid->getTrackCnt(), limitTrackNum);

      for (Ath__wire* w = track->getNextWire(NULL); w != NULL;
           w = w->getNext()) {
        bool found = false;
        uint start_next_track
            = tr;  // in case that track holds wires with different base
        for (uint next_tr = start_next_track;
             !found && next_tr - tr < limitTrackNum
             && next_tr < netGrid->getTrackCnt();
             next_tr++) {
          Ath__wire* first_wire;
          if (next_tr == tr)
            first_wire = w->getNext();
          else
            first_wire = GetNextWire(netGrid, next_tr, &firstWireTable);

          // PrintWire(stdout, first_wire, jj);
          Ath__wire* w2 = FindOverlap_found(w, first_wire, found);
          if (w2 != NULL && found) {
            firstWireTable.set(next_tr, w2);

            w->_upNext = w2;
            break;
          } else if (w2 != NULL && !found) {
            firstWireTable.set(next_tr, w2);
            w->_upNext = w2;
            break;
          } else if (first_wire != NULL) {
            firstWireTable.set(next_tr, first_wire);
            break;
          }
        }
      }
    }
  }
  FindCouplingNeighbors_down(dir, couplingDist, diag_met_limit);
  if (_extMain->_dbgOption > 1)
    PrintAllGrids(dir, OpenPrintFile(dir, "couple"), 1);

  uint cnt1 = FindDiagonalNeighbors_vertical_up(
      dir, couplingDist, diag_met_limit, 2, 0, false);
  if (_extMain->_dbgOption > 1) {
    fprintf(stdout,
            "%d 1st Pass: FindDiagonalNeighbors_vertical_up met_limit=2 "
            "trackLimit=0",
            cnt1);
    PrintAllGrids(dir, OpenPrintFile(dir, "vertical_up.pass1"), 2);
  }
  uint cnt2 = FindDiagonalNeighbors_vertical_down(
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
Ath__wire* extMeasureRC::SetUpDown(Ath__wire* w2,
                                   int next_tr,
                                   bool found,
                                   Ath__wire* first_wire,
                                   Ath__array1D<Ath__wire*>* firstWireTable)
{
  if (w2 != NULL && found) {
    firstWireTable->set(next_tr, w2);
    return w2;
  } else if (w2 != NULL && !found)
    firstWireTable->set(next_tr, w2);
  else if (first_wire != NULL)
    firstWireTable->set(next_tr, first_wire);
  return NULL;
}
int extMeasureRC::FindCouplingNeighbors_down(uint dir,
                                             uint couplingDist,
                                             uint diag_met_limit)
{
  uint limitTrackNum = 10;
  Ath__array1D<Ath__wire*> firstWireTable;
  uint colCnt = _search->getColCnt();
  for (uint jj = 1; jj < colCnt; jj++) {
    Ath__grid* netGrid = _search->getGrid(dir, jj);
    firstWireTable.resetCnt();

    int tr = netGrid->getTrackCnt() - 1;
    for (; tr >= 0; tr--) {
      Ath__track* track = netGrid->getTrackPtr((uint) tr);
      if (track == NULL)
        continue;

      int start_track_index = tr - 10 >= 0 ? tr - 10 : 0;
      ResetFirstWires(
          netGrid, &firstWireTable, start_track_index, tr, limitTrackNum);

      Ath__wire* first_wire1 = track->getNextWire(NULL);
      for (Ath__wire* w = first_wire1; w != NULL; w = w->getNext()) {
        bool found = false;
        for (int next_tr = tr - 1;
             !found && tr - next_tr < limitTrackNum && next_tr >= 0;
             next_tr--) {
          Ath__wire* first_wire
              = GetNextWire(netGrid, next_tr, &firstWireTable);
          Ath__wire* w2 = FindOverlap_found(w, first_wire, found);
          if (w2 != NULL && found) {
            firstWireTable.set(next_tr, w2);
            w->_downNext = w2;
            break;
          } else if (w2 != NULL && !found) {
            firstWireTable.set(next_tr, w2);
            break;
          } else if (first_wire != NULL) {
            firstWireTable.set(next_tr, first_wire);
            break;
          }
        }
      }
    }
  }
  return 0;
}
void extMeasureRC::ResetFirstWires(Ath__grid* netGrid,
                                   Ath__array1D<Ath__wire*>* firstWireTable,
                                   int tr1,
                                   int trCnt,
                                   uint limitTrackNum)
{
  for (uint ii = tr1; ii - tr1 < limitTrackNum && ii < trCnt; ii++) {
    Ath__track* track = netGrid->getTrackPtr(ii);
    if (track == NULL) {
      firstWireTable->set(ii, NULL);
      continue;
    }
    Ath__wire* w1 = track->getNextWire(NULL);
    firstWireTable->set(ii, w1);
  }
}
void extMeasureRC::ResetFirstWires(uint m1,
                                   uint m2,
                                   uint dir,
                                   Ath__array1D<Ath__wire*>** firstWireTable)
{
  for (uint level = m1; level < m2; level++)  // for layers above
  {
    Ath__grid* upGrid = _search->getGrid(dir, level);
    uint n = upGrid->getTrackCnt();
    ResetFirstWires(upGrid, firstWireTable[level], 0, n, n);
  }
}

Ath__wire* extMeasureRC::FindOverlap(Ath__wire* w,
                                     Ath__array1D<Ath__wire*>* firstWireTable,
                                     int tr)
{
  Ath__wire* first_wire = firstWireTable->geti(tr);
  if (first_wire == NULL)
    return NULL;

  int xy1 = w->getXY();
  int len1 = w->getLen();

  Ath__wire* w2 = first_wire;
  for (; w2 != NULL; w2 = w2->getNext()) {
    int xy2 = w2->getXY();
    int len2 = w2->getLen();

    Ath__wire* w3 = w2->getNext();
    if (w3 != NULL && xy1 >= xy2 + len2
        && xy1 + len1 <= w3->getXY())  // in between!
    {
      firstWireTable->set(tr, w3);
      return NULL;
    }
    firstWireTable->set(tr, w3);
    if (xy1 + len1 <= xy2)  // on the left
      continue;
    if (xy1 >= xy2 + len2)  // on the right
      continue;

    if (xy1 >= xy2 && xy1 <= xy2 + len2)
      return w2;
    if (xy2 >= xy1 && xy2 <= xy1 + len1)
      return w2;
    if (xy1 + len1 >= xy2 && xy1 + len1 <= xy2 + len2)
      return w2;
    if (xy2 + len2 >= xy1 && xy2 + len2 <= xy1 + len1)
      return w2;
  }
  return w2;
}
Ath__wire* extMeasureRC::FindOverlapWire(Ath__wire* w, Ath__wire* first_wire)
{
  bool white_overlap_check = true;
  if (first_wire == NULL)
    return NULL;

  // int xy1 = w->getXY()+w->getWidth();
  // int len1 = w->getLen()-2*w->getWidth();
  int xy1 = w->getXY();
  int len1 = w->getLen();

  Ath__wire* prev = NULL;
  for (Ath__wire* w2 = first_wire; w2 != NULL; w2 = w2->getNext()) {
    int xy2 = w2->getXY();
    int len2 = w2->getLen();

    if (xy1 + len1 <= xy2)  // on the left
    {
      prev = w2;
      continue;
    }
    if (white_overlap_check && prev != NULL
        && Enclosed(xy1, xy1 + len1, prev->getXY(), w2->getXY()))
      return prev;

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
  return NULL;
}
Ath__wire* extMeasureRC::FindOverlap(Ath__wire* w, Ath__wire* first_wire)
{
  bool white_overlap_check = true;
  if (first_wire == NULL)
    return NULL;

  // int xy1 = w->getXY()+w->getWidth();
  // int len1 = w->getLen()-2*w->getWidth();
  int xy1 = w->getXY();
  int len1 = w->getLen();

  Ath__wire* prev = NULL;
  for (Ath__wire* w2 = first_wire; w2 != NULL; w2 = w2->getNext()) {
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
      if (xy1 >= xy2)
        return w2;
      else if (prev != NULL)
        return prev;
      else
        return w2;
    }
    if (white_overlap_check && prev != NULL
        && Enclosed(xy1, xy1 + len1, prev->getXY(), w2->getXY()))
      return prev;
    prev = w2;
  }
  return first_wire;
}
Ath__wire* extMeasureRC::FindOverlap_found(Ath__wire* w,
                                           Ath__wire* first_wire,
                                           bool& found)
{
  bool white_overlap_check = true;
  if (first_wire == NULL)
    return NULL;

  // int xy1 = w->getXY()+w->getWidth();
  // int len1 = w->getLen()-2*w->getWidth();
  int xy1 = w->getXY();
  int len1 = w->getLen();

  Ath__wire* prev = NULL;
  for (Ath__wire* w2 = first_wire; w2 != NULL; w2 = w2->getNext()) {
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
      if (xy1 >= xy2)
        return w2;
      else if (prev != NULL)
        return prev;
      else
        return w2;
    }
    if (white_overlap_check && prev != NULL
        && Enclosed(xy1, xy1 + len1, prev->getXY(), w2->getXY()))
      return prev;
    prev = w2;
  }
  return first_wire;
}
bool extMeasureRC::IsSegmentOverlap(int x1, int len1, int x2, int len2)
{
  if (x1 + len1 <= x2)  // on the left
    return false;
  if (x1 >= x2 + len2)  // on the right
    return false;
  if (x1 + len1 <= x2)  // on the left
    return false;

  if (x1 >= x2 && x1 <= x2 + len2)
    return true;
  if (x2 >= x1 && x2 <= x1 + len1)
    return true;
  if (x1 + len1 >= x2 && x1 + len1 <= x2 + len2)
    return true;
  if (x2 + len2 >= x1 && x2 + len2 <= x1 + len1)
    return true;
  return false;
}
bool extMeasureRC::IsOverlap(Ath__wire* w, Ath__wire* w2)
{
  if (w2 == NULL)
    return false;

  if (!IsSegmentOverlap(w->getXY(), w->getLen(), w2->getXY(), w2->getLen()))
    return false;

  bool overlap = IsSegmentOverlap(
      w->getBase(), w->getWidth(), w2->getBase(), w2->getWidth());
  return overlap;
}
void extMeasureRC::PrintWire(FILE* fp,
                             Ath__wire* w,
                             int level,
                             const char* prefix,
                             const char* postfix)
{
  if (w == NULL)
    return;
  uint met = w->getLevel();
  uint dir;
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
void extMeasureRC::Print5wires(FILE* fp, Ath__wire* w, uint level)
{
  fprintf(fp, "---------------------------------- \n");
  int jj = level;
  if (w->_upNext != NULL) {
    if (w->_upNext->_upNext != NULL)
      PrintWire(fp, w->_upNext->_upNext, jj);
    PrintWire(fp, w->_upNext, 0);
  }
  fprintf(fp, "\n");
  PrintWire(fp, w, jj, "", " _____ ");
  fprintf(fp, "\n");
  if (w->_downNext != NULL) {
    PrintWire(fp, w->_downNext, jj);
    if (w->_downNext->_downNext != NULL)
      PrintWire(fp, w->_downNext->_downNext, jj);
  }
  fprintf(fp, "---------------------------------- \n");
}
void extMeasureRC::PrintDiagwires(FILE* fp, Ath__wire* w, uint level)
{
  if (w->_aboveNext != NULL) {
    fprintf(fp, "Vertical up::\n");
    if (w->_aboveNext->_aboveNext != NULL)
      PrintWire(
          fp, w->_aboveNext->_aboveNext, w->_aboveNext->_aboveNext->getLevel());
    PrintWire(fp, w->_aboveNext, w->_aboveNext->getLevel());
  }
  fprintf(fp, "-------------------------------------------------------\n");
  PrintWire(fp, w, w->getLen());
  fprintf(fp, "-------------------------------------------------------\n");
  if (w->_belowNext != NULL) {
    fprintf(fp, "Vertical Down:\n");
    PrintWire(fp, w->_belowNext, w->_belowNext->getLevel());
    if (w->_belowNext->_belowNext != NULL)
      PrintWire(
          fp, w->_belowNext->_belowNext, w->_belowNext->_belowNext->getLevel());
  }
  fprintf(fp, "\n");
}
Ath__wire* extMeasureRC::GetNextWire(Ath__grid* netGrid,
                                     uint tr,
                                     Ath__array1D<Ath__wire*>* firstWireTable)
{
  Ath__track* next_track = netGrid->getTrackPtr(tr);
  if (next_track == NULL)
    return NULL;

  Ath__wire* first_wire = NULL;
  if (tr < firstWireTable->getSize())
    first_wire = firstWireTable->geti(tr);
  if (first_wire == NULL)
    first_wire = next_track->getNextWire(NULL);

  return first_wire;
}
Ath__wire* extMeasureRC::FindOverlap(Ath__wire* w,
                                     Ath__grid* netGrid,
                                     uint tr,
                                     Ath__array1D<Ath__wire*>* firstWireTable)
{
  Ath__wire* first_wire = GetNextWire(netGrid, tr, firstWireTable);
  if (first_wire == NULL)
    return NULL;

  Ath__wire* w2 = FindOverlap(w, first_wire);
  return w2;
}
bool extMeasureRC::CheckWithNeighbors(Ath__wire* w, Ath__wire* prev)
{
  if (w->_aboveNext != NULL)
    return true;

  if (prev != NULL && IsOverlap(w, prev->_aboveNext)) {
    w->_aboveNext = prev->_aboveNext;
    prev = w;
    return true;
  }
  if (w->_downNext != NULL && IsOverlap(w, w->_downNext->_aboveNext)) {
    w->_aboveNext = w->_downNext->_aboveNext;
    return true;
  }
  return false;
}
bool extMeasureRC::CheckWithNeighbors_below(Ath__wire* w, Ath__wire* prev)
{
  if (w->_belowNext != NULL)
    return true;

  if (prev != NULL && IsOverlap(w, prev->_belowNext)) {
    w->_belowNext = prev->_belowNext;
    prev = w;
    return true;
  }
  if (w->_downNext != NULL && IsOverlap(w, w->_downNext->_belowNext)) {
    w->_belowNext = w->_downNext->_belowNext;
    return true;
  }
  return false;
}
Ath__array1D<Ath__wire*>** extMeasureRC::allocMarkTable(uint n)
{
  Ath__array1D<Ath__wire*>** tbl = new Ath__array1D<Ath__wire*>*[n];
  for (uint ii = 0; ii < n; ii++)
    tbl[ii] = new Ath__array1D<Ath__wire*>(128);
  return tbl;
}
void extMeasureRC::DeleteMarkTable(Ath__array1D<Ath__wire*>** tbl, uint n)
{
  for (uint ii = 0; ii < n; ii++) {
    delete tbl[ii];
  }
  delete tbl;
}
FILE* extMeasureRC::OpenFile(const char* name, const char* perms)
{
  FILE* fp = fopen(name, perms);
  if (fp == NULL) {
    fprintf(stderr, "Cannot open file %s with %s\n", name, perms);
    exit(1);
  }
  return fp;
}
FILE* extMeasureRC::OpenPrintFile(uint dir, const char* name)
{
  char buf[100];
  sprintf(buf, "%s.%d", name, dir);
  return OpenFile(buf, "w");
}
int extMeasureRC::PrintAllGrids(uint dir, FILE* fp, uint mode)
{
  uint colCnt = _search->getColCnt();
  for (uint jj = 1; jj < colCnt; jj++)  // For all Layers
  {
    const char* vert = dir == 1 ? "H" : "V";
    Ath__grid* netGrid = _search->getGrid(dir, jj);
    fprintf(fp,
            "Metal %d  Dir %d %s Tracks=%d "
            "----------------------------------------- \n",
            jj,
            dir,
            vert,
            netGrid->getTrackCnt());
    for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Ath__track* track = netGrid->getTrackPtr(tr);
      if (track == NULL)
        continue;

      fprintf(fp, "Track %d M%d %s %d \n", tr, jj, vert, track->getBase());
      for (Ath__wire* w = track->getNextWire(NULL); w != NULL;
           w = w->getNext()) {
        if (mode == 0)
          PrintWire(fp, w, jj);
        else if (mode == 1)  // coupling neighbors
          Print5wires(fp, w, jj);
        else if (mode == 2)  // diagonal coupling neighbors
          PrintDiagwires(fp, w, jj);
      }
    }
  }
  return 0;
}

int extMeasureRC::ConnectWires(uint dir)
{
  if (_extMain->_dbgOption > 1)
    PrintAllGrids(dir, OpenPrintFile(dir, "wires.org"), 0);

  uint cnt = 0;

  uint colCnt = _search->getColCnt();
  for (uint jj = 1; jj < colCnt; jj++)  // For all Layers
  {
    Ath__grid* netGrid = _search->getGrid(dir, jj);
    for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Ath__track* track = netGrid->getTrackPtr(tr);
      if (track == NULL)
        continue;

      ConnectAllWires(track);
      /* DEBUG
    if (ConnectAllWires(track)) {
      if (_extMain->_dbgOption > 1) {
        for (Ath__wire* w = track->getNextWire(NULL); w != NULL;
             w = w->getNext())
          PrintWire(stdout, w, jj);
      }
    }
    */
    }
  }
  if (_extMain->_dbgOption > 1)
    PrintAllGrids(dir, OpenPrintFile(dir, "wires"), 0);
  return cnt;
}
uint extMeasureRC::ConnectAllWires(Ath__track* track)
{
  Ath__array1D<Ath__wire*> tbl(128);
  uint ii = track->getGrid()->searchLowMarker();
  int first_marker_index = -1;
  for (; ii <= track->getGrid()->searchHiMarker(); ii++) {
    // fprintf(stdout, "Track %d marker %d ------------------------ \n",
    // track->getTrackNum(), ii);

    for (Ath__wire* w = track->_marker[ii]; w != NULL; w = w->getNext()) {
      // PrintWire(stdout, w, 0);
      tbl.add(w);
      if (first_marker_index < 0)
        first_marker_index = ii;
    }
  }
  if (tbl.getCnt() == 0)
    return 0;

  for (uint ii = 0; ii < tbl.getCnt() - 1; ii++) {
    Ath__wire* w = tbl.get(ii);
    if (w->getNext() == NULL) {
      Ath__wire* w1 = tbl.get(ii + 1);
      w->setNext(w1);
    }
  }
  bool swap = false;
  if (tbl.getCnt() <= 1)
    return false;
  for (uint ii = 0; ii < tbl.getCnt() - 1; ii++) {
    Ath__wire* prev = NULL;
    if (ii > 0)
      prev = tbl.get(ii - 1);
    Ath__wire* w1 = tbl.get(ii);
    Ath__wire* w2 = tbl.get(ii + 1);
    if (w1->getXY() == w2->getXY()) {
      if (w1->getBase() > w2->getBase()) {
        tbl.set(ii, w2);
        tbl.set(ii + 1, w1);
        w1->setNext(w2->getNext());
        w2->setNext(w1);
        if (ii > 0)
          prev->setNext(w2);
        swap = true;
      }
    }
  }
  Ath__wire* w0 = tbl.get(0);
  track->_marker[first_marker_index] = w0;

  return swap;
}

int extMeasureRC::FindDiagonalNeighbors(uint dir,
                                        uint couplingDist,
                                        uint diag_met_limit,
                                        uint lookUpLevel,
                                        uint limitTrackNum)
{
  uint cnt = 0;

  uint levelCnt = _search->getColCnt();
  Ath__array1D<Ath__wire*>** firstWireTable = allocMarkTable(levelCnt);

  for (uint jj = 1; jj < levelCnt; jj++)  // For all Layers
  {
    Ath__grid* netGrid = _search->getGrid(dir, jj);
    for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Ath__track* track = netGrid->getTrackPtr(tr);
      if (track == NULL)
        continue;

      uint m1 = jj + 1;
      uint m2 = jj + 1 + lookUpLevel;
      if (m2 > levelCnt)
        m2 = levelCnt;

      ResetFirstWires(m1, m2, dir, firstWireTable);

      Ath__wire* prev = NULL;
      Ath__wire* first_wire1 = track->getNextWire(NULL);
      for (Ath__wire* w = first_wire1; w != NULL;
           w = w->getNext())  // for all wires in the track
      {
        if (CheckWithNeighbors(w, prev)) {
          prev = w;
          continue;
        }
        prev = w;
        for (uint level = m1; level < m2; level++)  // for layers above
        {
          Ath__grid* upGrid = _search->getGrid(dir, level);
          int up_track_num = upGrid->getTrackNum1(w->getBase());
          int start_track = up_track_num - limitTrackNum >= 0
                                ? up_track_num - limitTrackNum
                                : 0;
          int end_track = up_track_num + limitTrackNum + 1;

          bool found = false;
          for (uint next_tr = start_track;
               next_tr < end_track && next_tr < netGrid->getTrackCnt();
               next_tr++)  // for tracks overlapping wire
          {
            Ath__wire* w2
                = FindOverlap(w, netGrid, next_tr, firstWireTable[level]);
            if (w2 == NULL)
              continue;

            firstWireTable[level]->set(next_tr, w2);
            w->_aboveNext = w2;
            found = true;
            cnt++;
            break;
          }
          if (found)
            break;
        }
      }
    }
  }
  DeleteMarkTable(firstWireTable, levelCnt);
  return cnt;
}
int extMeasureRC::FindDiagonalNeighbors_vertical_power(
    uint dir,
    Ath__wire* w,
    uint couplingDist,
    uint diag_met_limit,
    uint limitTrackNum,
    Ath__array1D<Ath__wire*>** upWireTable)
{
  uint cnt = 0;
  uint current_met = w->getLevel();
  for (uint level = 1; level < _search->getColCnt(); level++)  // For all Layers
  {
    if (level == current_met)
      continue;
    upWireTable[level]->resetCnt();

    Ath__grid* grid = _search->getGrid(dir, level);
    int up_track_num = grid->getTrackNum1(w->getBase());
    int start_track
        = up_track_num - limitTrackNum >= 0 ? up_track_num - limitTrackNum : 0;
    int end_track = up_track_num + limitTrackNum + 1;

    for (uint next_tr = start_track;
         next_tr < end_track && next_tr < grid->getTrackCnt();
         next_tr++)  // for tracks overlapping wire
    {
      Ath__track* next_track = grid->getTrackPtr(next_tr);
      if (next_track == NULL)
        continue;
      Ath__wire* first = next_track->getNextWire(NULL);
      if (first == NULL)
        continue;
      if (!first->isPower())  // assume power nets occupy entire track TODO:
                              // optimize
        continue;
      Ath__wire* w2 = FindOverlap(w, first);
      if (w2 == NULL)
        continue;
      if (GetDistance(w, w2) > 0)
        continue;

      upWireTable[level]->add(w2);
      cnt++;
      break;
    }
  }
  return cnt;
}
Ath__wire* extMeasureRC::FindDiagonalNeighbors_vertical_up_down(
    Ath__wire* w,
    bool& found,
    uint dir,
    uint level,
    uint couplingDist,
    uint limitTrackNum,
    Ath__array1D<Ath__wire*>** firstWireTable)
{
  Ath__grid* upGrid = _search->getGrid(dir, level);
  int up_track_num = upGrid->getTrackNum1(w->getBase());
  int start_track
      = up_track_num - limitTrackNum >= 0 ? up_track_num - limitTrackNum : 0;
  int end_track = up_track_num + limitTrackNum + 1;
  //  if (up_track_num==3329 && level==4 && dir==1 && w->getXY()==1093580 &&
  //  w->getBase()==1331980)
  //    PrintWire(stdout, w, level);

  found = false;
  for (uint next_tr = start_track;
       next_tr < end_track && next_tr < upGrid->getTrackCnt();
       next_tr++)  // for tracks overlapping wire
  {
    Ath__wire* first = GetNextWire(upGrid, next_tr, firstWireTable[level]);
    if (first != NULL && first->isPower())  // assume power nets occupy entire
                                            // track TODO: optimize
    {
      found = true;
      return NULL;
    }
    Ath__wire* w2 = FindOverlap(w, first);
    if (w2 == NULL)
      continue;

    firstWireTable[level]->set(next_tr, w2);
    found = true;
    return w2;
  }
  return NULL;
}
int extMeasureRC::FindDiagonalNeighbors_vertical_up(uint dir,
                                                    uint couplingDist,
                                                    uint diag_met_limit,
                                                    uint lookUpLevel,
                                                    uint limitTrackNum,
                                                    bool skipCheckNeighbors)
{
  uint cnt = 0;
  int levelCnt = _search->getColCnt();
  Ath__array1D<Ath__wire*>** firstWireTable = allocMarkTable(levelCnt);

  for (uint jj = 1; jj < levelCnt - 1; jj++)  // For all Layers
  {
    Ath__grid* netGrid = _search->getGrid(dir, jj);
    for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Ath__track* track = netGrid->getTrackPtr(tr);
      if (track == NULL)
        continue;

      uint m1 = jj + 1;
      uint m2 = jj + 1 + lookUpLevel;
      if (m2 > levelCnt)
        m2 = levelCnt;

      ResetFirstWires(m1, m2, dir, firstWireTable);

      Ath__wire* prev = NULL;
      Ath__wire* first_wire1 = track->getNextWire(NULL);
      for (Ath__wire* w = first_wire1; w != NULL;
           w = w->getNext())  // for all wires in the track
      {
        if (w->isPower() || w->_aboveNext != NULL)
          continue;

        prev = w;
        if (!skipCheckNeighbors && CheckWithNeighbors(w, prev))
          continue;

        for (uint level = m1; level < m2; level++)  // for layers above
        {
          bool found = false;
          Ath__wire* w2
              = FindDiagonalNeighbors_vertical_up_down(w,
                                                       found,
                                                       dir,
                                                       level,
                                                       couplingDist,
                                                       limitTrackNum,
                                                       firstWireTable);
          if (w2 != NULL) {
            w->_aboveNext = w2;
            break;
          }
          if (found)
            break;
        }
      }
    }
  }
  DeleteMarkTable(firstWireTable, levelCnt);
  return cnt;
}
int extMeasureRC::FindDiagonalNeighbors_vertical_down(uint dir,
                                                      uint couplingDist,
                                                      uint diag_met_limit,
                                                      uint lookUpLevel,
                                                      uint limitTrackNum,
                                                      bool skipCheckNeighbors)
{
  uint cnt = 0;
  int levelCnt = _search->getColCnt();
  Ath__array1D<Ath__wire*>** firstWireTable = allocMarkTable(levelCnt);

  for (int jj = levelCnt - 1; jj > 1; jj--)  // For all Layers going down
  {
    Ath__grid* netGrid = _search->getGrid(dir, jj);
    for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Ath__track* track = netGrid->getTrackPtr(tr);
      if (track == NULL)
        continue;

      int m1 = jj - 1;
      int m2 = jj - 1 - lookUpLevel;
      if (m2 < 1)
        m2 = 1;

      ResetFirstWires(m2, m1, dir, firstWireTable);

      Ath__wire* prev = NULL;
      Ath__wire* first_wire1 = track->getNextWire(NULL);
      for (Ath__wire* w = first_wire1; w != NULL;
           w = w->getNext())  // for all wires in the track
      {
        if (w->isPower() || w->_aboveNext != NULL)
          continue;

        prev = w;
        if (!skipCheckNeighbors && CheckWithNeighbors(w, prev))
          continue;

        for (int level = m1; level > m2; level--)  // for layers above
        {
          bool found = false;
          Ath__wire* w2
              = FindDiagonalNeighbors_vertical_up_down(w,
                                                       found,
                                                       dir,
                                                       level,
                                                       couplingDist,
                                                       limitTrackNum,
                                                       firstWireTable);
          if (w2 != NULL) {
            w->_belowNext = w2;
            break;
          }
          if (found)
            break;
        }
      }
    }
  }
  DeleteMarkTable(firstWireTable, levelCnt);
  return cnt;
}
int extMeasureRC::FindDiagonalNeighbors_down(uint dir,
                                             uint couplingDist,
                                             uint diag_met_limit,
                                             uint lookUpLevel,
                                             uint limitTrackNum)
{
  uint cnt = 0;

  uint levelCnt = _search->getColCnt();
  Ath__array1D<Ath__wire*>** firstWireTable = allocMarkTable(levelCnt);

  for (int jj = 1; jj < 4 && jj < levelCnt; jj++)  // For all Layers
  {
    int m1 = jj - 1;
    if (m1 <= 0)
      continue;
    int m2 = jj - 1 - lookUpLevel;
    if (m2 < 1)
      m2 = 1;

    Ath__grid* netGrid = _search->getGrid(dir, jj);
    for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++)  // for all  tracks
    {
      Ath__track* track = netGrid->getTrackPtr(tr);
      if (track == NULL)
        continue;

      ResetFirstWires(m2, jj, dir, firstWireTable);

      Ath__wire* prev = NULL;
      Ath__wire* first_wire1 = track->getNextWire(NULL);
      for (Ath__wire* w = first_wire1; w != NULL;
           w = w->getNext())  // for all wires in the track
      {
        if (w->getXY() == 20500 && w->getBase() == 6500)
          Print5wires(stdout, w, w->getLevel());
        if (CheckWithNeighbors_below(w, prev)) {
          prev = w;
          continue;
        }
        prev = w;
        for (int level = m1; level >= m2; level--)  // for layers above
        {
          Ath__grid* upGrid = _search->getGrid(dir, level);
          int up_track_num = upGrid->getTrackNum1(w->getBase());
          int start_track = up_track_num - limitTrackNum >= 0
                                ? up_track_num - limitTrackNum
                                : 0;
          int end_track = up_track_num + limitTrackNum + 1;

          bool found = false;
          for (uint next_tr = start_track;
               next_tr < end_track && next_tr < netGrid->getTrackCnt();
               next_tr++)  // for tracks overlapping wire
          {
            Ath__wire* w2
                = FindOverlap(w, netGrid, next_tr, firstWireTable[level]);
            if (w2 == NULL)
              continue;

            firstWireTable[level]->set(next_tr, w2);
            w->_belowNext = w2;
            found = true;
            cnt++;
            break;
          }
          if (found)
            break;
        }
      }
    }
  }
  DeleteMarkTable(firstWireTable, levelCnt);
  return cnt;
}
}  // namespace rcx
