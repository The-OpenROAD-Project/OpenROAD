// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <algorithm>
#include <cstdint>
#include <cstdio>

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

namespace rcx {

int extMeasureRC::ConnectWires(uint32_t dir, BoundaryData& bounds)
{
  if (_extMain->_dbgOption > 1) {
    if (_connect_wire_FP == nullptr) {
      _connect_wire_FP = OpenPrintFile(dir, "wires.org");
    }
    PrintAllGrids(dir, _connect_wire_FP, 0);
  }
  uint32_t cnt = 0;
  uint32_t colCnt = _search->getColCnt();
  for (uint32_t jj = 1; jj < colCnt; jj++)  // For all Layers
  {
    Grid* netGrid = _search->getGrid(dir, jj);

    uint32_t tr = _lowTrackSearch[dir][jj];
    for (; tr < netGrid->getTrackCnt(); tr++) {
      Track* track = netGrid->getTrackPtr(tr);
      if (track == nullptr) {
        continue;
      }

      if (track->getBase() > bounds.hi_search[dir]) {
        _hiTrackSearch[dir][jj] = tr;
        break;
      }
      if (ConnectAllWires(track)) {
        if (_extMain->_dbgOption > 1) {
          // fprintf(stdout, "Track %d M%d %d \n", tr, jj,
          // track->getBase());
          for (Wire* w = track->getNextWire(nullptr); w != nullptr;
               w = w->getNext()) {
            PrintWire(stdout, w, jj);
          }
        }
      }
    }
    if (tr >= netGrid->getTrackCnt()) {
      _hiTrackSearch[dir][jj] = netGrid->getTrackCnt();
    }
  }
  if (_extMain->_dbgOption > 1) {
    if (_connect_FP == nullptr) {
      _connect_FP = OpenPrintFile(dir, "wires");
    }
    PrintAllGrids(dir, _connect_FP, 0);
  }
  return cnt;
}

// Find immediate coupling neighbor wires in all directions and levels for every
// Wire
int extMeasureRC::FindCouplingNeighbors(uint32_t dir, BoundaryData& bounds)
{
  // uint32_t couplingDist, uint32_t diag_met_limit;

  // uint32_t limitTrackNum = 10;
  uint32_t limitTrackNum = bounds.maxCouplingTracks;
  uint32_t couplingDist = bounds.maxCouplingTracks;

  Array1D<Wire*> firstWireTable;

  uint32_t colCnt = _search->getColCnt();
  for (uint32_t jj = 1; jj < colCnt; jj++) {
    Grid* netGrid = _search->getGrid(dir, jj);
    firstWireTable.resetCnt();

    // for (uint32_t tr = 0; tr < netGrid->getTrackCnt(); tr++)
    for (uint32_t tr = _lowTrackToExtract[dir][jj];
         tr < _hiTrackSearch[dir][jj];
         tr++) {
      Track* track = netGrid->getTrackPtr(tr);
      if (track == nullptr) {
        continue;
      }

      if (track->getBase() > bounds.extractLimitXY) {
        _hiTrackToExtract[dir][jj] = tr;
        break;
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
      if (tr >= _hiTrackSearch[dir][jj]) {
        _hiTrackToExtract[dir][jj] = _hiTrackSearch[dir][jj];
        break;
      }
    }
  }
  uint32_t diag_met_limit = bounds.diag_met_limit;
  FindCouplingNeighbors_down_opt(dir, bounds);

  if (_extMain->_dbgOption > 1) {
    PrintAllGrids(dir, OpenPrintFile(dir, "couple"), 1);
  }

  uint32_t cnt1 = FindDiagonalNeighbors_vertical_up_opt(
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
int extMeasureRC::FindCouplingNeighbors_down_opt(uint32_t dir,
                                                 BoundaryData& bounds)
{
  uint32_t limitTrackNum = bounds.maxCouplingTracks;
  Array1D<Wire*> firstWireTable;
  uint32_t colCnt = _search->getColCnt();
  for (uint32_t jj = 1; jj < colCnt; jj++) {
    Grid* netGrid = _search->getGrid(dir, jj);
    firstWireTable.resetCnt();

    // int tr = netGrid->getTrackCnt() - 1;
    // for (; tr >= 0; tr--)
    int tr = _hiTrackToExtract[dir][jj];
    for (; tr >= _lowTrackToExtract[dir][jj]; tr--) {
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

int extMeasureRC::FindDiagonalNeighbors_vertical_up_opt(uint32_t dir,
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
    // for (uint32_t tr = 0; tr < netGrid->getTrackCnt(); tr++) // for all
    // tracks
    for (uint32_t tr = _lowTrackToExtract[dir][jj];
         tr < _hiTrackToExtract[dir][jj];
         tr++)  // for all  tracks
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
        if (w->isPower() || w->getAboveNext()) {
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
int extMeasureRC::FindDiagonalNeighbors_vertical_down_opt(
    uint32_t dir,
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
    // for (uint32_t tr = 0; tr < netGrid->getTrackCnt(); tr++) // for all
    // tracks
    for (uint32_t tr = _lowTrackToExtract[dir][jj];
         tr < _hiTrackToExtract[dir][jj];
         tr++)  // for all  tracks
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
int extMeasureRC::CouplingFlow_opt(uint32_t dir,
                                   BoundaryData& bounds,
                                   int totWireCnt,
                                   uint32_t& totalWiresExtracted,
                                   float& previous_percent_extracted)
{
  uint32_t metalLevelCnt = _search->getColCnt();
  uint32_t couplingDist = bounds.maxCouplingTracks;

  CouplingConfig config(_extMain, metalLevelCnt);
  _segFP = nullptr;
  if (config.debug_enabled) {
    config.debug_fp = OpenPrintFile(dir, "Segments");
    _segFP = config.debug_fp;
  }
  CouplingState counts;

  SegmentTables segments;

  // first wire from a Track on all levels and tracks
  Array1D<Wire*>** firstWireTable = allocMarkTable(metalLevelCnt);
  allocateTables(metalLevelCnt);

  _dir = dir;

  for (int level = 1; level < metalLevelCnt; level++) {
    _met = level;
    Grid* netGrid = _search->getGrid(dir, level);
    segments.resetAll();

    config.reset_calc_flow_flag(level);

    uint32_t maxDist = 10 * netGrid->getPitch();

    for (uint32_t tr = _lowTrackToExtract[dir][level];
         tr < _hiTrackToExtract[dir][level];
         tr++) {
      Track* track = netGrid->getTrackPtr(tr);
      if (track == nullptr) {
        continue;
      }

      ResetFirstWires(level, metalLevelCnt, dir, firstWireTable);
      for (Wire* w = track->getNextWire(nullptr); w != nullptr;
           w = w->getNext()) {
        counts.wire_count++;
        if (w->isPower() || w->getRsegId() == 0) {
          continue;
        }

        if (config.debug_enabled) {
          Print5wires(_segFP, w, w->getLevel());
        }
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
        for (uint32_t ii = 0; ii < segments.wireSegmentTable.getCnt(); ii++) {
          extSegment* s = segments.wireSegmentTable.get(ii);
          CalcRes(s);
        }
        // Context -- Overlap -- Coupling Extraction
        if (config.new_calc_flow || config.length_flag) {
          _met = w->getLevel();
          _len = w->getLen();
          for (uint32_t ii = 0; ii < segments.wireSegmentTable.getCnt(); ii++) {
            extSegment* s = segments.wireSegmentTable.get(ii);
            ReleaseSegTables(metalLevelCnt);

            if (config.length_flag && s->_len < CouplingConfig::LENGTH_BOUND) {
              continue;
            }

            extSegment* white = _seqmentPool->alloc();
            white->set(dir, w, s->_xy, s->_len, nullptr, nullptr);
            _whiteSegTable[_met]->add(white);

            PrintOverlapSeg(_segFP, s, _met, "\nNEW --");

            // Array1D<extSegment*> crossOvelapTable(8);
            uint32_t overMet = _met + 1;
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
            if (overMet >= metalLevelCnt - 1) {
              CreateCouplingCaps_over(s, metalLevelCnt);
            }
          }
        }
        for (uint32_t ii = 0; (!config.new_calc_flow || config.length_flag)
                              && ii < segments.wireSegmentTable.getCnt();
             ii++) {
          extSegment* s = segments.wireSegmentTable.get(ii);

          if (config.length_flag && s->_len >= CouplingConfig::LENGTH_BOUND) {
            continue;
          }

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
  if (_segFP != nullptr) {
    fclose(_segFP);
  }

  de_allocateTables(metalLevelCnt);

  return 0;
}
}  // namespace rcx
