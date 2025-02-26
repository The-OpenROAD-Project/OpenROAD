
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

int extMeasureRC::ConnectWires(uint dir, BoundaryData& bounds)
{
  if (_extMain->_dbgOption > 1) {
    if (_connect_wire_FP == nullptr)
      _connect_wire_FP = OpenPrintFile(dir, "wires.org");
    PrintAllGrids(dir, _connect_wire_FP, 0);
  }
  uint cnt = 0;
  uint colCnt = _search->getColCnt();
  for (uint jj = 1; jj < colCnt; jj++)  // For all Layers
  {
    Ath__grid* netGrid = _search->getGrid(dir, jj);

    uint tr = _lowTrackSearch[dir][jj];
    for (; tr < netGrid->getTrackCnt(); tr++) {
      Ath__track* track = netGrid->getTrackPtr(tr);
      if (track == NULL)
        continue;

      if (track->getBase() > bounds.hi_search[dir]) {
        _hiTrackSearch[dir][jj] = tr;
        break;
      }
      if (ConnectAllWires(track)) {
        if (_extMain->_dbgOption > 1) {
          // fprintf(stdout, "Track %d M%d %d \n", tr, jj,
          // track->getBase());
          for (Ath__wire* w = track->getNextWire(NULL); w != NULL;
               w = w->getNext())
            PrintWire(stdout, w, jj);
        }
      }
    }
    if (tr >= netGrid->getTrackCnt())
      _hiTrackSearch[dir][jj] = netGrid->getTrackCnt();
  }
  if (_extMain->_dbgOption > 1) {
    if (_connect_FP == nullptr)
      _connect_FP = OpenPrintFile(dir, "wires");
    PrintAllGrids(dir, _connect_FP, 0);
  }
  return cnt;
}

// Find immediate coupling neighbor wires in all directions and levels for every
// Ath__wire
int extMeasureRC::FindCouplingNeighbors(uint dir, BoundaryData& bounds)
{
  // uint couplingDist, uint diag_met_limit;

  // uint limitTrackNum = 10;
  uint limitTrackNum = bounds.maxCouplingTracks;
  uint couplingDist = bounds.maxCouplingTracks;

  Ath__array1D<Ath__wire*> firstWireTable;

  uint colCnt = _search->getColCnt();
  for (uint jj = 1; jj < colCnt; jj++) {
    Ath__grid* netGrid = _search->getGrid(dir, jj);
    firstWireTable.resetCnt();

    // for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++)
    for (uint tr = _lowTrackToExtract[dir][jj]; tr < _hiTrackSearch[dir][jj];
         tr++) {
      Ath__track* track = netGrid->getTrackPtr(tr);
      if (track == NULL)
        continue;

      if (track->getBase() > bounds.extractLimitXY) {
        _hiTrackToExtract[dir][jj] = tr;
        break;
      }
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
      if (tr >= _hiTrackSearch[dir][jj]) {
        _hiTrackToExtract[dir][jj] = _hiTrackSearch[dir][jj];
        break;
      }
    }
  }
  uint diag_met_limit = bounds.diag_met_limit;
  FindCouplingNeighbors_down_opt(dir, bounds);

  if (_extMain->_dbgOption > 1)
    PrintAllGrids(dir, OpenPrintFile(dir, "couple"), 1);

  uint cnt1 = FindDiagonalNeighbors_vertical_up_opt(
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
int extMeasureRC::FindCouplingNeighbors_down_opt(uint dir, BoundaryData& bounds)
{
  uint limitTrackNum = bounds.maxCouplingTracks;
  Ath__array1D<Ath__wire*> firstWireTable;
  uint colCnt = _search->getColCnt();
  for (uint jj = 1; jj < colCnt; jj++) {
    Ath__grid* netGrid = _search->getGrid(dir, jj);
    firstWireTable.resetCnt();

    // int tr = netGrid->getTrackCnt() - 1;
    // for (; tr >= 0; tr--)
    int tr = _hiTrackToExtract[dir][jj];
    for (; tr >= _lowTrackToExtract[dir][jj]; tr--) {
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

int extMeasureRC::FindDiagonalNeighbors_vertical_up_opt(uint dir,
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
    // for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++) // for all  tracks
    for (uint tr = _lowTrackToExtract[dir][jj]; tr < _hiTrackToExtract[dir][jj];
         tr++)  // for all  tracks
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
int extMeasureRC::FindDiagonalNeighbors_vertical_down_opt(
    uint dir,
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
    // for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++) // for all  tracks
    for (uint tr = _lowTrackToExtract[dir][jj]; tr < _hiTrackToExtract[dir][jj];
         tr++)  // for all  tracks
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
int extMeasureRC::CouplingFlow_opt(uint dir,
                                   BoundaryData& bounds,
                                   int totWireCnt,
                                   uint& totalWiresExtracted,
                                   float& previous_percent_extracted)
{
  uint metalLevelCnt = _search->getColCnt();
  uint couplingDist = bounds.maxCouplingTracks;

  CouplingConfig config(_extMain, metalLevelCnt);
  _segFP = NULL;
  if (config.debug_enabled) {
    config.debug_fp = OpenPrintFile(dir, "Segments");
    _segFP = config.debug_fp;
  }
  CouplingState counts;

  SegmentTables segments;

  // first wire from a Track on all levels and tracks
  Ath__array1D<Ath__wire*>** firstWireTable = allocMarkTable(metalLevelCnt);
  allocateTables(metalLevelCnt);

  _dir = dir;

  for (int level = 1; level < metalLevelCnt; level++) {
    _met = level;
    Ath__grid* netGrid = _search->getGrid(dir, level);
    segments.resetAll();

    config.reset_calc_flow_flag(level);

    uint maxDist = 10 * netGrid->_pitch;

    // for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++) {

    for (uint tr = _lowTrackToExtract[dir][level];
         tr < _hiTrackToExtract[dir][level];
         tr++) {
      Ath__track* track = netGrid->getTrackPtr(tr);
      if (track == NULL)
        continue;

      ResetFirstWires(level, metalLevelCnt, dir, firstWireTable);
      for (Ath__wire* w = track->getNextWire(NULL); w != NULL;
           w = w->getNext()) {
        counts.wire_count++;
        if (w->isPower() || w->getRsegId() == 0)
          continue;

        if (config.debug_enabled)
          Print5wires(_segFP, w, w->getLevel());
        // DebugWire(w, 0, 0, 17091); --- Placeholder for stopping during

        totalWiresExtracted++;
        // TODO use progress object _progressTracker->updateProgress();

        if (counts.wire_count % _extMain->_wire_extracted_progress_count == 0) {
          printProgress(
              totalWiresExtracted, totWireCnt, previous_percent_extracted);
        }
        CouplingDimensionParams coupleOptions(dir,
                                              level,
                                              maxDist,
                                              couplingDist,
                                              config.limit_track_num,
                                              config.debug_fp);

        // Find all detailed Coupling neighbors in both directions and levels to
        // calculate Lateral and Diagonal Coupling
        GetCouplingSegments(
            tr, w, config, coupleOptions, segments, firstWireTable);

        // Distance based Resistance Calculation based on coupling segments of
        // the wire
        for (uint ii = 0; ii < segments.wireSegmentTable.getCnt(); ii++) {
          extSegment* s = segments.wireSegmentTable.get(ii);
          CalcRes(s);
        }
        // Context -- Overlap -- Coupling Extraction
        if (config.new_calc_flow || config.length_flag) {
          _met = w->getLevel();
          _len = w->getLen();
          for (uint ii = 0; ii < segments.wireSegmentTable.getCnt(); ii++) {
            extSegment* s = segments.wireSegmentTable.get(ii);
            ReleaseSegTables(metalLevelCnt);

            if (config.length_flag && s->_len < config.LENGTH_BOUND)
              continue;

            extSegment* white = _seqmentPool->alloc();
            white->set(dir, w, s->_xy, s->_len, NULL, NULL);
            _whiteSegTable[_met]->add(white);

            PrintOverlapSeg(_segFP, s, _met, "\nNEW --");

            // Ath__array1D<extSegment*> crossOvelapTable(8);
            uint overMet = _met + 1;
            for (; overMet < metalLevelCnt; overMet++) {
              // _ovSegTable[overMet]->resetCnt();
              // _whiteSegTable[overMet]->resetCnt();
              Release(_ovSegTable[overMet]);
              Release(_whiteSegTable[overMet]);

              // Coupling Caps
              VerticalDiagonalCouplingAndCrossOverlap(
                  w, s, overMet, segments, config);

              // Over Sub Under met -- Create Coupling Capacitors
              if (CreateCouplingCaps_overUnder(s, overMet)) {
                Release(_ovSegTable[overMet]);
                Release(_whiteSegTable[overMet]);
                continue;
              }

              // Looking up metal levels, all overlaps are blocked
              if (_whiteSegTable[overMet]->getCnt() == 0) {
                break;
              };
            }
            if (overMet >= metalLevelCnt - 1)
              CreateCouplingCaps_over(s, metalLevelCnt);
          }
        }
        for (uint ii = 0; (!config.new_calc_flow || config.length_flag)
                          && ii < segments.wireSegmentTable.getCnt();
             ii++) {
          extSegment* s = segments.wireSegmentTable.get(ii);

          if (config.length_flag && s->_len >= config.LENGTH_BOUND)
            continue;

          PrintOverlapSeg(
              _segFP, s, _met, "\nmeasure_RC_new .........................\n");

          measure_RC_new(s, true);
        }
        ReleaseSegTables(metalLevelCnt);
        // segments.releaseAll();
        releaseAll(segments);
      }
    }
    // fprintf(stdout, "\nDir=%d  wireCnt=%d  NotOrderedCnt=%d  oneEmptyTable=%d
    // oneCntTable=%d\n",
    //        dir, wireCnt, notOrderCnt, oneEmptyTable, oneCntTable);
  }
  if (_segFP != NULL)
    fclose(_segFP);

  de_allocateTables(metalLevelCnt);

  return 0;
}
}  // namespace rcx
