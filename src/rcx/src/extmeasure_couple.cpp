
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

struct CouplingConfig;

bool extMeasureRC::printProgress(uint totalWiresExtracted,
                                 uint totWireCnt,
                                 float& previous_percent_extracted)
{
  float percent_extracted
      = ceil(100.0 * (1.0 * totalWiresExtracted / totWireCnt));

  if ((totWireCnt > 0) && (totalWiresExtracted > 0)
      && (percent_extracted - previous_percent_extracted >= 5.0)) {
    fprintf(stdout,
            "%3d%c completion -- %6d wires have been extracted ----- \n",
            (int) percent_extracted,
            '%',
            totalWiresExtracted);
    previous_percent_extracted = percent_extracted;
    return true;
  }
  return false;
}
void extMeasureRC::allocateTables(uint colCnt)
{
  _upSegTable = allocTable(colCnt);
  _downSegTable = allocTable(colCnt);
  _ovSegTable = allocTable(colCnt);
  _whiteSegTable = allocTable(colCnt);

  _verticalPowerTable = allocTable_wire(colCnt);
}
void extMeasureRC::de_allocateTables(uint colCnt)
{
  DeleteTable(_upSegTable, colCnt);
  DeleteTable(_downSegTable, colCnt);

  DeleteTable(_ovSegTable, colCnt);
  DeleteTable(_whiteSegTable, colCnt);
  DeleteTable_wire(_verticalPowerTable, colCnt);
}

// ----------------------------------------------------------- cleanup

uint extMeasureRC::GetCoupleSegments(bool lookUp,
                                     Ath__wire* w,
                                     uint start_track,
                                     CouplingDimensionParams& opt,
                                     Ath__array1D<Ath__wire*>** firstWireTable,
                                     Ath__array1D<extSegment*>* segmentTable)
{
  Ath__array1D<Ath__wire*> wireTable;
  int level = opt.metal_level;

  // in case that there are wires at distance on the same track; looking  at
  // increasing track number vertical  or horizontal
  if (lookUp && opt.metal_level == w->getLevel()) {
    Ath__wire* w2 = FindOverlapWire(w, w->getNext());
    if (w2 != NULL)
      wireTable.add(w2);
  }

  if (lookUp)
    FindCoupleWiresOnTracks_up(
        w,
        start_track,
        opt,
        firstWireTable,
        &wireTable);  // looking on increasing track numbers
  else
    FindCoupleWiresOnTracks_down(
        w,
        start_track,
        opt,
        firstWireTable,
        &wireTable);  // looking on decreasing track numbers

  const char* msg = lookUp ? "Up Coupling Wires:" : "Down Coupling Wires:";
  PrintTable_coupleWires(opt.dbgFP, w, true, &wireTable, msg, level);

  Release(segmentTable);
  FindSegmentsTrack(w,
                    w->getXY(),
                    w->getLen(),
                    NULL,
                    0,
                    &wireTable,
                    lookUp,
                    opt.direction,
                    opt.max_distance,
                    segmentTable);

  msg = lookUp ? "Up Coupling Segments:" : "Down Coupling Segments:";
  PrintTable_segments(opt.dbgFP, w, lookUp, true, segmentTable, msg, level);
  return 0;
}
uint extMeasureRC::FindCoupleWiresOnTracks_down(
    Ath__wire* w,
    int start_track,
    CouplingDimensionParams& opt,
    Ath__array1D<Ath__wire*>** firstWireTable,
    Ath__array1D<Ath__wire*>* resTable)
{
  resTable->resetCnt();
  if (start_track < 0)
    return 0;

  uint level = opt.metal_level;
  Ath__grid* upGrid = _search->getGrid(opt.direction, level);
  // int up_track_num = upGrid->getTrackNum1(w->getBase());
  int end_track = start_track - opt.track_limit - 1;
  if (end_track < 0)
    end_track = 0;

  for (int next_tr = start_track; next_tr > end_track;
       next_tr--)  // for tracks overlapping wire
  {
    Ath__wire* first = GetNextWire(upGrid, next_tr, firstWireTable[level]);
    Ath__wire* w2 = FindOverlapWire(w, first);
    if (w2 == NULL)
      continue;

    firstWireTable[level]->set(next_tr, w2);

    bool w2_next_covered = false;
    Ath__wire* w2_next = w2->getNext();
    if (w2_next != NULL)  // TODO: because more 2 wires at different distance
                          // can reside in same track
    {
      if (OverlapOnly(
              w->getXY(), w->getLen(), w2_next->getXY(), w2_next->getLen())) {
        resTable->add(w2_next);
        firstWireTable[level]->set(next_tr, w2_next);
        w2_next_covered = Enclosed(w->getXY(),
                                   w->getXY() + w->getLen(),
                                   w2_next->getXY(),
                                   w2_next->getXY() + w2_next->getLen());
      }
    }
    resTable->add(w2);
    if (Enclosed(w->getXY(),
                 w->getXY() + w->getLen(),
                 w2->getXY(),
                 w2->getXY() + w2->getLen()))
      break;
    if (w2_next_covered)
      break;

    if (w2->isPower())
      break;
  }
  return resTable->getCnt();
}

uint extMeasureRC::FindCoupleWiresOnTracks_up(
    Ath__wire* w,
    uint start_track,
    CouplingDimensionParams& coupleOptions,
    Ath__array1D<Ath__wire*>** firstWireTable,
    Ath__array1D<Ath__wire*>* resTable)

// int extMeasureRC::FindAllNeigbors_up(Ath__wire *w, uint start_track, uint
// dir, uint level, uint couplingDist, uint limitTrackNum,
// Ath__array1D<Ath__wire *> **firstWireTable, Ath__array1D<Ath__wire *>
// *resTable)
{
  Ath__grid* upGrid
      = _search->getGrid(coupleOptions.direction, coupleOptions.metal_level);
  int end_track = start_track + coupleOptions.track_limit + 1;

  uint level = coupleOptions.metal_level;

  for (uint next_tr = start_track;
       next_tr < end_track && next_tr < upGrid->getTrackCnt();
       next_tr++)  // for tracks overlapping wire
  {
    Ath__wire* first = GetNextWire(upGrid, next_tr, firstWireTable[level]);
    Ath__wire* w2 = FindOverlapWire(w, first);
    if (w2 == NULL)
      continue;
    firstWireTable[level]->set(next_tr, w2);
    resTable->add(w2);
    if (Enclosed(w->getXY(),
                 w->getXY() + w->getLen(),
                 w2->getXY(),
                 w2->getXY() + w2->getLen()))
      break;

    if (w2->isPower())
      break;
  }
  return resTable->getCnt();
}

bool extMeasureRC::FindDiagonalCoupleSegments(
    Ath__wire* w,
    int current_level,
    int max_level,
    CouplingDimensionParams& opts,
    Ath__array1D<Ath__wire*>** firstWireTable)
{
  int diagLimit = 3;
  int diagLimitTrackNum = 4;
  int diagMaxDist = 500;
  uint dir = opts.direction;
  FILE* fp = opts.dbgFP;

  bool lookUp = true;

  for (uint jj = current_level + 1;
       jj < max_level && jj < current_level + diagLimit;
       jj++) {
    Ath__grid* upgrid = _search->getGrid(dir, jj);

    int diag_track_num = upgrid->getTrackNum1(w->getBase());
    FindAllSegments_up(fp,
                       w,
                       lookUp,
                       diag_track_num + 1,
                       dir,
                       jj,
                       opts.max_distance,
                       diagMaxDist,
                       diagLimitTrackNum,
                       firstWireTable,
                       _upSegTable);
    FindAllSegments_up(fp,
                       w,
                       !lookUp,
                       diag_track_num - 1,
                       dir,
                       jj,
                       opts.max_distance,
                       diagMaxDist,
                       diagLimitTrackNum,
                       firstWireTable,
                       _downSegTable);
  }
  for (int jj = current_level - 1; jj > 0 && jj > current_level - diagLimit;
       jj--) {
    Ath__grid* upgrid = _search->getGrid(dir, jj);
    int diag_track_num = upgrid->getTrackNum1(w->getBase());
    FindAllSegments_up(fp,
                       w,
                       lookUp,
                       diag_track_num + 1,
                       dir,
                       jj,
                       opts.max_distance,
                       diagMaxDist,
                       diagLimitTrackNum,
                       firstWireTable,
                       _upSegTable);
    FindAllSegments_up(fp,
                       w,
                       !lookUp,
                       diag_track_num - 1,
                       dir,
                       jj,
                       opts.max_distance,
                       diagMaxDist,
                       diagLimitTrackNum,
                       firstWireTable,
                       _downSegTable);
  }
  // TODO: diagonal looking down -- at least for power wires! power wires don't
  // look up -- M1 is required as width not wide
  return true;
}
bool extMeasureRC::VerticalDiagonalCouplingAndCrossOverlap(
    Ath__wire* w,
    extSegment* s,
    int overMet,
    SegmentTables& segments,
    CouplingConfig& config)
{
  int diagMaxDist
      = 500;  // same as in FindDiagonalCoupleSegments; TODO create opts

  bool dbgOverlaps = config.debug_overlaps;  // CHECK
  FILE* fp = config.debug_fp;
  bool lookUp = true;

  for (uint kk = 0; kk < _whiteSegTable[overMet - 1]->getCnt(); kk++) {
    extSegment* ww = _whiteSegTable[overMet - 1]->get(kk);
    // vertical and diag
    if (config.vertical_cap) {
      Ath__array1D<extSegment*> upVertTable;
      FindDiagonalSegments(s,
                           ww,
                           &segments.aboveTable,
                           &upVertTable,
                           dbgOverlaps,
                           fp,
                           lookUp,
                           overMet);
      VerticalCap(&upVertTable, lookUp);
      Release(&upVertTable);

      Ath__array1D<extSegment*> downVertTable;
      FindDiagonalSegments(s,
                           ww,
                           &segments.belowTable,
                           &downVertTable,
                           dbgOverlaps,
                           fp,
                           !lookUp,
                           overMet);
      VerticalCap(&downVertTable, !lookUp);
      Release(&downVertTable);
    }
    if (config.diag_cap) {
      Ath__array1D<extSegment*> upDiagTable;
      FindDiagonalSegments(
          s, ww, _upSegTable[overMet], &upDiagTable, dbgOverlaps, fp, lookUp);
      DiagCap(fp, w, lookUp, diagMaxDist, 2, &upDiagTable);
      Release(&upDiagTable);

      Ath__array1D<extSegment*> downDiagTable;
      FindDiagonalSegments(s,
                           ww,
                           _downSegTable[overMet],
                           &downDiagTable,
                           dbgOverlaps,
                           fp,
                           !lookUp);
      DiagCap(fp, w, !lookUp, diagMaxDist, 2, &downDiagTable);
      Release(&downDiagTable);
    }
    GetCrossOvelaps(w,
                    overMet,
                    ww->_xy,
                    ww->_len,
                    _dir,
                    _ovSegTable[overMet],
                    _whiteSegTable[overMet]);
  }
  PrintOvelaps(s, _met, overMet, _ovSegTable[overMet], "u");
  return true;
}
bool extMeasureRC::CreateCouplingCaps_overUnder(extSegment* s, uint overMet)
{
  if (_met == 1) {
    OverUnder(s, _met, 0, overMet, _ovSegTable[overMet], "OverSubUnderM");
    return true;  // break the flow
  }
  for (uint oo = 0; oo < _ovSegTable[overMet]->getCnt();
       oo++) {  // looking down
    extSegment* v = _ovSegTable[overMet]->get(oo);
    OverlapDown(overMet, s, v, _dir);
    if (_whiteSegTable[1]->getCnt() > 0) {
      PrintOvelaps(v, _met, 0, _whiteSegTable[1], "OverSubUnderMet");
      OverUnder(s, _met, 0, overMet, _whiteSegTable[1], "OverSubUnderM");
      Release(_whiteSegTable[1]);
    }
  }
  return false;
}
bool extMeasureRC::CreateCouplingCaps_over(extSegment* s, uint metalLevelCnt)
{
  if (_met == 1) {
    PrintOvelaps(s, _met, 0, _whiteSegTable[metalLevelCnt - 1], "OverSub");
    OverUnder(s, _met, 0, -1, _whiteSegTable[metalLevelCnt - 1], "OverSub");
  } else {
    for (uint oo = 0; oo < _whiteSegTable[metalLevelCnt - 1]->getCnt();
         oo++) {  // looking down
      extSegment* v = _whiteSegTable[metalLevelCnt - 1]->get(oo);
      OverlapDown(-1, s, v, _dir);  // OverMet
    }
    PrintOvelaps(s, _met, 0, _whiteSegTable[1], "OverSub");
    OverUnder(s, _met, 0, -1, _whiteSegTable[1], "OverSub");
  }
  return true;
}
bool extMeasureRC::GetCouplingSegments(
    int tr,
    Ath__wire* w,
    CouplingConfig& config,
    CouplingDimensionParams& coupleOptions,
    SegmentTables& segments,
    Ath__array1D<Ath__wire*>** firstWireTable)
{
  bool lookUp = true;
  uint level = coupleOptions.metal_level;
  uint dir = coupleOptions.direction;
  uint metalLevelCnt = config.metal_level_count;
  uint maxDist = coupleOptions.max_distance;

  // Find all direct projections of wires on increasing tracks -- on one side of
  // wire
  GetCoupleSegments(
      lookUp, w, tr + 1, coupleOptions, firstWireTable, _upSegTable[level]);

  // Find all direct projections of wires on decreasing tracks -- on other side
  // of wire
  GetCoupleSegments(
      !lookUp, w, tr - 1, coupleOptions, firstWireTable, _downSegTable[level]);

  // alternative call FindAllSegments_up(fp, w, lookUp, tr + 1, dir, level,
  // maxDist, couplingDist, limitTrackNum, firstWireTable, _upSegTable);
  // FindAllSegments_up(fp, w, !lookUp, tr - 1, dir, level, maxDist,
  // couplingDist, limitTrackNum, firstWireTable, _downSegTable);

  // Create coupling segments with both or one way or neighboring wires in
  // wireSegmentTable
  CreateCouplingSEgments(w,
                         &segments.wireSegmentTable,
                         _upSegTable[level],
                         _downSegTable[level],
                         config.debug_enabled,
                         config.debug_fp);

  // find overlapping power wires; result in _verticalPowerTable
  if (FindDiagonalNeighbors_vertical_power(
          dir, w, 10000, 100, 3, _verticalPowerTable)
      > 0)  // power
    PrintTable_wires(config.debug_fp,
                     config.debug_enabled,
                     metalLevelCnt,
                     _verticalPowerTable,
                     "Vertical Power Wires:");

  // find coupling segments (=Diagonal) on differen levels; results in
  // _upSegTable, _downSegTable
  FindDiagonalCoupleSegments(
      w, level, metalLevelCnt, coupleOptions, firstWireTable);

  // ----------------------------------------------------------------------------------------------------Diagonal
  segments.aboveTable.resetCnt();  // over vertical segment list
  segments.belowTable.resetCnt();  // under vertical segment list

  // Note: Ath__wire->_up holds the first above vertical wire
  FindAllSegments_vertical(
      config.debug_fp,
      w,
      lookUp,
      dir,
      maxDist,
      &segments.aboveTable);  // Note: _up holds the above vertical wire

  // Note: Ath__wire->_down holds the first below vertical wire
  FindAllSegments_vertical(
      config.debug_fp,
      w,
      !lookUp,
      dir,
      maxDist,
      &segments.belowTable);  // Note: _down holds the above vertical wire

  return true;
}
// ------------------------------------------------------------------------ v2
// cleanup

int extMeasureRC::CouplingFlow(uint dir,
                               uint couplingDist,
                               uint diag_met_limit,
                               int totWireCnt,
                               uint& totalWiresExtracted,
                               float& previous_percent_extracted)
{
  uint metalLevelCnt = _search->getColCnt();

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
    // DBG _extMain->getPeakMemory("CouplingFlow Level:", level);

    config.reset_calc_flow_flag(level);

    uint maxDist = 10 * netGrid->_pitch;
    for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++) {
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
        // TODO: use progress object _progressTracker->updateProgress();

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

            if (config.length_flag && s->_len < config.LENGTH_BOUND)
              continue;

            Release(_whiteSegTable[_met]);

            extSegment* white = _seqmentPool->alloc();
            white->set(dir, w, s->_xy, s->_len, NULL, NULL);

            _whiteSegTable[_met]->add(white);
            PrintOverlapSeg(_segFP, s, _met, "\nNEW --");

            Ath__array1D<extSegment*> crossOvelapTable(8);
            uint overMet = _met + 1;
            for (; overMet < metalLevelCnt; overMet++) {
              Release(_ovSegTable[overMet]);
              Release(_whiteSegTable[overMet]);

              // Coupling Caps
              VerticalDiagonalCouplingAndCrossOverlap(
                  w, s, overMet, segments, config);

              // Over Sub Under met -- Create Coupling Capacitors
              if (CreateCouplingCaps_overUnder(s, overMet))
                continue;

              // Looking up metal levels, all overlaps are blocked
              if (_whiteSegTable[overMet]->getCnt() == 0)
                break;
            }
            if (overMet >= metalLevelCnt - 1)
              CreateCouplingCaps_over(s, metalLevelCnt);
          }
        }
        // DELETE for (uint ii = 0; ii < !new_calc_flow && segTable.getCnt();
        // ii++)
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
void extMeasureRC::ReleaseSegTables(uint metalLevelCnt)
{
  for (uint jj = 0; jj < metalLevelCnt; jj++) {
    Release(_downSegTable[jj]);
    Release(_upSegTable[jj]);
    Release(_ovSegTable[jj]);
    Release(_whiteSegTable[jj]);
  }
}
void extMeasureRC::releaseAll(SegmentTables& segments)
{
  Release(&segments.upTable);
  Release(&segments.downTable);
  Release(&segments.verticalUpTable);
  Release(&segments.verticalDownTable);
  Release(&segments.wireSegmentTable);
  Release(&segments.aboveTable);
  Release(&segments.belowTable);
  Release(&segments.whiteTable);
}
void extMeasureRC::Release(Ath__array1D<extSegment*>* segTable)
{
  for (uint ii = 0; ii < segTable->getCnt(); ii++) {
    extSegment* s = segTable->get(ii);
    _seqmentPool->free(s);
  }
  segTable->resetCnt();
}

bool extMeasureRC::CalcRes(extSegment* s)
{
  double deltaRes[10];
  for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
    deltaRes[jj] = 0;
    _rc[jj]->Reset();
  }
  _met = s->_wire->getLevel();
  _rsegSrcId = s->_wire->getRsegId();
  _netSrcId = s->_wire->getBoxId();

  _netId = 0;
  if (IsDebugNet()) {
    _netId = _netSrcId;
    OpenDebugFile();
    if (_debugFP != NULL) {
      fprintf(stdout,
              "CalcRes %d met= %d  len= %d  dist= %d r1= %d r2= %d\n",
              _totSignalSegCnt,
              _met,
              _len,
              _dist,
              _netSrcId,
              0);
      fprintf(_debugFP,
              "init_measureRC %d met= %d  len= %d  dist= %d r1= %d r2= %d\n",
              _totSignalSegCnt,
              _met,
              _len,
              _dist,
              _netSrcId,
              0);
    }
  }
  dbRSeg* rseg1 = dbRSeg::getRSeg(_block, _rsegSrcId);
  int dits1 = s->_dist < s->_dist_down ? s->_dist : s->_dist_down;
  int dits2 = s->_dist > s->_dist_down ? s->_dist : s->_dist_down;

  calcRes(_rsegSrcId, s->_len, dits1, dits2, _met);
  calcRes0(deltaRes, _met, s->_len);

  rcNetInfo();

  for (uint jj = 0; jj < _metRCTable.getCnt(); jj++) {
    double totR1 = _rc[jj]->getRes();
    if (totR1 > 0) {
      totR1 -= deltaRes[jj];
      if (totR1 != 0.0)
        _extMain->updateRes(rseg1, totR1, jj);
    }
  }
  if (IsDebugNet1()) {
    DebugEnd_res(stdout, _rsegSrcId, s->_len, "END");
    DebugEnd_res(_debugFP, _rsegSrcId, s->_len, "END");
    printNetCaps(stdout, "after END");
    printNetCaps(_debugFP, "after END");
    dbNet* net = rseg1->getNet();
    dbSet<dbRSeg> rSet = net->getRSegs();
    odb::dbSet<odb::dbRSeg>::iterator rc_itr;

    for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
      odb::dbRSeg* rc = *rc_itr;
      fprintf(stdout, "r%d %g\n", rc->getId(), rc->getResistance(0));
    }
  }

  rcNetInfo();
  if (IsDebugNet1())
    DebugEnd_res(stdout, _rsegSrcId, s->_len, "AFTER CalcRes");
  return true;
}
bool extMeasureRC::FindDiagonalSegments(extSegment* s,
                                        extSegment* w1,
                                        Ath__array1D<extSegment*>* segDiagTable,
                                        Ath__array1D<extSegment*>* diagSegTable,
                                        bool dbgOverlaps,
                                        FILE* fp,
                                        bool lookUp,
                                        int tgt_met)
{
  // vertical and diag
  if (segDiagTable->getCnt() == 0)
    return false;

  extSegment* white = _seqmentPool->alloc();
  white->set(_dir, s->_wire, w1->_xy, w1->_len, NULL, NULL);

  if (!lookUp)
    white->_up = s->_wire;
  else
    white->_down = s->_wire;

  Ath__array1D<extSegment*> resultTable;
  Ath__array1D<extSegment*> whiteTable;
  whiteTable.add(white);
  if (tgt_met > 0)  // vertical
  {
    Ath__array1D<extSegment*> vertTable;
    for (uint ii = 0; ii < segDiagTable->getCnt(); ii++) {
      extSegment* v = segDiagTable->get(ii);
      if (lookUp && v->_up->getLevel() == tgt_met)
        vertTable.add(v);
      if (!lookUp && v->_down->getLevel() == tgt_met)
        vertTable.add(v);
    }
    if (vertTable.getCnt() == 0) {
      _seqmentPool->free(white);
      // delete white;
      return false;
    }
    if (lookUp)
      CreateCouplingSEgments(
          s->_wire, &resultTable, &vertTable, &whiteTable, dbgOverlaps, NULL);
    else
      CreateCouplingSEgments(
          s->_wire, &resultTable, &whiteTable, &vertTable, dbgOverlaps, NULL);
  } else {
    if (lookUp)
      CreateCouplingSEgments(
          s->_wire, &resultTable, segDiagTable, &whiteTable, dbgOverlaps, NULL);
    else
      CreateCouplingSEgments(
          s->_wire, &resultTable, &whiteTable, segDiagTable, dbgOverlaps, NULL);
  }
  const char* msg = lookUp ? "Up" : "Down";
  if (fp != NULL) {
    if (tgt_met > 0)
      fprintf(fp, "%s Vertical Overlap %d ----- \n", msg, resultTable.getCnt());
    else
      fprintf(fp, "%s Diag Overlap %d ----- \n", msg, resultTable.getCnt());
  }

  for (uint ii = 0; ii < resultTable.getCnt(); ii++) {
    extSegment* s1 = resultTable.get(ii);
    if (s1->_up == NULL || s1->_down == NULL) {
      // delete s1;
      _seqmentPool->free(s1);
      continue;
    }
    diagSegTable->add(s1);
    PrintUpDown(fp, s1);
    PrintUpDownNet(fp, s1->_up, s1->_dist, "\t");  // TODO _up should extSegment
    PrintUpDownNet(
        fp, s1->_down, s1->_dist_down, "\t");  // TODO _down should extSegment
  }
  // delete white;
  _seqmentPool->free(white);
  return false;

  if (whiteTable.getCnt() == 0)
    return false;

  return false;
}
void extMeasureRC::OverlapDown(int overMet,
                               extSegment* coupSeg,
                               extSegment* overlapSeg,
                               uint dir)
{
  extSegment* ov = _seqmentPool->alloc();
  ov->set(dir, coupSeg->_wire, overlapSeg->_xy, overlapSeg->_len, NULL, NULL);

  Release(_whiteSegTable[_met]);
  _whiteSegTable[_met]->add(ov);
  for (int underMet = _met - 1; underMet > 0; underMet--) {
    Release(_ovSegTable[underMet]);
    Release(_whiteSegTable[underMet]);

    for (uint kk = 0; kk < _whiteSegTable[underMet + 1]->getCnt(); kk++) {
      extSegment* ww = _whiteSegTable[underMet + 1]->get(kk);
      GetCrossOvelaps(coupSeg->_wire,
                      underMet,
                      ww->_xy,
                      ww->_len,
                      dir,
                      _ovSegTable[underMet],
                      _whiteSegTable[underMet]);
    }
    if (_ovSegTable[underMet]->getCnt() > 0) {
      PrintOvelaps(overlapSeg, _met, underMet, _ovSegTable[underMet], "ou");
      OverUnder(coupSeg, _met, underMet, overMet, _ovSegTable[underMet], "OU");
    }
    if (_whiteSegTable[underMet]->getCnt() == 0)
      break;

    // _ovSegTable[underMet] holds overUnder
  }
}
uint extMeasureRC::FindAllSegments_vertical(
    FILE* fp,
    Ath__wire* w,
    bool lookUp,
    uint dir,
    uint maxDist,
    Ath__array1D<extSegment*>* aboveTable)
{
  bool dbgOverlaps = true;
  Ath__wire* w2_next = lookUp ? w->_aboveNext : w->_belowNext;
  if (w2_next == NULL)
    return 0;

  Ath__array1D<Ath__wire*> wTable;
  wTable.add(w2_next);
  Ath__wire* w2_next_next = lookUp ? w2_next->_aboveNext : w2_next->_belowNext;
  if (w2_next_next != NULL)
    wTable.add(w2_next_next);
  // TODO, optimize, not accurate

  if (dbgOverlaps) {
    const char* msg = lookUp ? "Up Vertical Wires:" : "Down Vertical Wires:";
    char buff[100];
    sprintf(buff, "M%d %s", w2_next->getLevel(), msg);
    PrintTable_coupleWires(fp, w, dbgOverlaps, &wTable, buff);
  }
  Ath__array1D<extSegment*> sTable;
  FindSegmentsTrack(w,
                    w->getXY(),
                    w->getLen(),
                    w2_next,
                    0,
                    &wTable,
                    lookUp,
                    dir,
                    maxDist,
                    &sTable);
  for (uint ii = 0; ii < sTable.getCnt(); ii++) {
    extSegment* s = sTable.get(ii);
    if (!lookUp && s->_down != NULL && !s->_down->isPower())
      continue;
    aboveTable->add(s);
  }
  if (dbgOverlaps) {
    const char* msg = lookUp ? "Up Vertical Segments:"
                             : "Down Vertical Segments (power only):";
    char buff[100];
    sprintf(buff, "M%d %s", w2_next->getLevel(), msg);
    PrintTable_segments(fp, w, lookUp, dbgOverlaps, aboveTable, buff);
  }
  return 0;
}

uint extMeasureRC::FindAllSegments_up(FILE* fp,
                                      Ath__wire* w,
                                      bool lookUp,
                                      uint start_track,
                                      uint dir,
                                      uint level,
                                      uint maxDist,
                                      uint couplingDist,
                                      uint limitTrackNum,
                                      Ath__array1D<Ath__wire*>** firstWireTable,
                                      Ath__array1D<extSegment*>** UpSegTable)
{
  bool dbgOverlaps = true;

  Ath__array1D<Ath__wire*> UpTable;

  // --------------------------- in case that there are wires at distance on the
  // same track
  if (lookUp && level == w->getLevel()) {
    Ath__wire* w2 = FindOverlapWire(w, w->getNext());
    if (w2 != NULL)
      UpTable.add(w2);
  }
  // -----------------------------------------------------------

  if (lookUp)
    FindAllNeigbors_up(w,
                       start_track,
                       dir,
                       level,
                       couplingDist,
                       limitTrackNum,
                       firstWireTable,
                       &UpTable);
  else
    FindAllNeigbors_down(w,
                         start_track,
                         dir,
                         level,
                         couplingDist,
                         limitTrackNum,
                         firstWireTable,
                         &UpTable);

  FILE* fp1 = fp;
  const char* msg = lookUp ? "Up Coupling Wires:" : "Down Coupling Wires:";
  char buff[100];
  sprintf(buff, "M%d %s", level, msg);
  PrintTable_coupleWires(fp1, w, dbgOverlaps, &UpTable, buff);

  Release(UpSegTable[level]);
  FindSegmentsTrack(w,
                    w->getXY(),
                    w->getLen(),
                    NULL,
                    0,
                    &UpTable,
                    lookUp,
                    dir,
                    maxDist,
                    UpSegTable[level]);

  msg = lookUp ? "Up Coupling Segments:" : "Down Coupling Segments:";
  sprintf(buff, "M%d %s", level, msg);
  PrintTable_segments(fp1, w, lookUp, dbgOverlaps, UpSegTable[level], buff);
  return 0;
}
uint extMeasureRC::FindAllNeigbors_up(Ath__wire* w,
                                      uint start_track,
                                      uint dir,
                                      uint level,
                                      uint couplingDist,
                                      uint limitTrackNum,
                                      Ath__array1D<Ath__wire*>** firstWireTable,
                                      Ath__array1D<Ath__wire*>* resTable)
{
  Ath__grid* upGrid = _search->getGrid(dir, level);
  // int up_track_num = upGrid->getTrackNum1(w->getBase());
  int end_track = start_track + limitTrackNum + 1;

  for (uint next_tr = start_track;
       next_tr < end_track && next_tr < upGrid->getTrackCnt();
       next_tr++)  // for tracks overlapping wire
  {
    Ath__wire* first = GetNextWire(upGrid, next_tr, firstWireTable[level]);
    Ath__wire* w2 = FindOverlapWire(w, first);
    if (w2 == NULL)
      continue;
    firstWireTable[level]->set(next_tr, w2);
    resTable->add(w2);
    if (Enclosed(w->getXY(),
                 w->getXY() + w->getLen(),
                 w2->getXY(),
                 w2->getXY() + w2->getLen()))
      break;

    if (w2->isPower())
      break;
  }
  return resTable->getCnt();
}
uint extMeasureRC::FindAllNeigbors_down(
    Ath__wire* w,
    int start_track,
    uint dir,
    uint level,
    uint couplingDist,
    uint limitTrackNum,
    Ath__array1D<Ath__wire*>** firstWireTable,
    Ath__array1D<Ath__wire*>* resTable)
{
  resTable->resetCnt();
  if (start_track < 0)
    return 0;

  Ath__grid* upGrid = _search->getGrid(dir, level);
  // int up_track_num = upGrid->getTrackNum1(w->getBase());
  int end_track = start_track - limitTrackNum - 1;
  if (end_track < 0)
    end_track = 0;

  for (int next_tr = start_track; next_tr > end_track;
       next_tr--)  // for tracks overlapping wire
  {
    Ath__wire* first = GetNextWire(upGrid, next_tr, firstWireTable[level]);
    Ath__wire* w2 = FindOverlapWire(w, first);
    if (w2 == NULL)
      continue;

    firstWireTable[level]->set(next_tr, w2);

    bool w2_next_covered = false;
    Ath__wire* w2_next = w2->getNext();
    if (w2_next != NULL) {
      // TODO: because more 2 wires at different distance
      // can reside in same track
      if (OverlapOnly(
              w->getXY(), w->getLen(), w2_next->getXY(), w2_next->getLen())) {
        resTable->add(w2_next);
        firstWireTable[level]->set(next_tr, w2_next);
        w2_next_covered = Enclosed(w->getXY(),
                                   w->getXY() + w->getLen(),
                                   w2_next->getXY(),
                                   w2_next->getXY() + w2_next->getLen());
      }
    }
    resTable->add(w2);
    if (Enclosed(w->getXY(),
                 w->getXY() + w->getLen(),
                 w2->getXY(),
                 w2->getXY() + w2->getLen()))
      break;
    if (w2_next_covered)
      break;

    if (w2->isPower())
      break;
  }
  return resTable->getCnt();
}
uint extMeasureRC::CreateCouplingSEgments(Ath__wire* w,
                                          Ath__array1D<extSegment*>* segTable,
                                          Ath__array1D<extSegment*>* upTable,
                                          Ath__array1D<extSegment*>* downTable,
                                          bool dbgOverlaps,
                                          FILE* fp)
{
  uint cnt1 = 0;
  /* TODO
              if (upTable->getCnt() == 0 || downTable->getCnt() == 0)
              {
                  oneEmptyTable++;
                  // CreateUpDownSegment(w, NULL, w->getXY(), w->getLen(), NULL,
     &segTable);
              }
              if (upTable->getCnt() == 1 || downTable->getCnt() == 1)
                  oneCntTable++;
  */
  if (!CheckOrdered(upTable))
    BubbleSort(upTable);
  if (!CheckOrdered(downTable))
    BubbleSort(downTable);

  if (upTable->getCnt() == 0
      && downTable->getCnt() == 0)  // OpenEnded both sides
  {
    CreateUpDownSegment(w, NULL, w->getXY(), w->getLen(), NULL, segTable);
  } else if (upTable->getCnt() == 0
             && downTable->getCnt() > 0)  // OpenEnded Down
    cnt1 += CopySegments(false, downTable, 0, downTable->getCnt(), segTable);
  else if (upTable->getCnt() > 0 && downTable->getCnt() == 0)  // OpenEnded Up
    cnt1 += CopySegments(true, upTable, 0, upTable->getCnt(), segTable);
  else if (upTable->getCnt() == 1 && downTable->getCnt() > 0)  // 1 up,
    cnt1 += FindUpDownSegments(upTable, downTable, segTable);
  else if (upTable->getCnt() > 0 && downTable->getCnt() == 1)  // 1 up,
    cnt1 += FindUpDownSegments(upTable, downTable, segTable);
  else if (upTable->getCnt() > 0 && downTable->getCnt() > 0)
    cnt1 += FindUpDownSegments(upTable, downTable, segTable);

  if (dbgOverlaps) {
    PrintUpDown(fp, segTable);
    // PrintUpDown(stdout, &segTable);
  }
  if (!CheckOrdered(segTable)) {
    BubbleSort(segTable);
    if (!CheckOrdered(segTable)) {
      fprintf(fp, "======> segTable NOT SORTED after Buggble\n");
    }
  }
  return cnt1;
}
bool extMeasureRC::GetCrossOvelaps(Ath__wire* w,
                                   uint tgt_met,
                                   int x1,
                                   int len,
                                   uint dir,
                                   Ath__array1D<extSegment*>* segTable,
                                   Ath__array1D<extSegment*>* whiteTable)
{
  bool dbg = false;
  SEQ s1;
  s1._ll[!dir] = x1;
  s1._ur[!dir] = x1 + len;
  s1._ll[dir] = w->getBase();
  s1._ur[dir] = w->getBase();

  Ath__array1D<SEQ*> table(4);
  getOverlapSeq(tgt_met, &s1, &table);

  int totLen = 0;
  for (uint ii = 0; ii < table.getCnt(); ii++) {
    SEQ* p = table.get(ii);

    int len1 = p->_ur[!dir] - p->_ll[!dir];

    extSegment* s = _seqmentPool->alloc();
    s->set(dir, w, p->_ll[!dir], len1, NULL, NULL);

    if (p->type == 0)
      whiteTable->add(s);
    else {
      segTable->add(s);
      totLen += len1;
    }
    _pixelTable->release(p);
  }
  if (dbg)
    PrintCrossOvelaps(w, tgt_met, x1, len, segTable, totLen, "\n\tOU");

  if (totLen + w->getWidth() >= len)
    return true;

  return false;
}
int extMeasureRC::wireOverlap(int X1,
                              int DX,
                              int x1,
                              int dx,
                              int* len1,
                              int* len2,
                              int* len3)
{
  int dx1 = X1 - x1;
  //*len1= dx1;
  if (dx1 >= 0)  // on left side
  {
    int dlen = dx - dx1;
    if (dlen <= 0)
      return 1;

    *len1 = 0;
    int DX2 = dlen - DX;

    if (DX2 <= 0) {
      *len2 = dlen;
      *len3 = -DX2;
    } else {
      *len2 = DX;
      //*len3= DX2;
      *len3 = 0;
    }
  } else {
    *len1 = -dx1;

    if (dx1 + DX <= 0)  // outside right side
      return 2;

    int DX2 = (x1 + dx) - (X1 + DX);
    if (DX2 > 0) {
      *len2 = DX + dx1;  // dx1 is negative
      *len3 = 0;
    } else {
      *len2 = dx;
      *len3 = -DX2;
    }
  }
  return 0;
}

bool extMeasureRC::PrintInit(FILE* fp,
                             bool dbgOverlaps,
                             Ath__wire* w,
                             int x,
                             int y)
{
  if (dbgOverlaps) {
    if (fp == NULL)
      return false;

    fprintf(fp, "\n");
    Print5wires(fp, w, w->getLevel());
  }
  if (w->getXY() == x && w->getBase() == y) {
    Print5wires(stdout, w, w->getLevel());
    return true;
  }
  return false;
}
void extMeasureRC::PrintTable_coupleWires(FILE* fp1,
                                          Ath__wire* w,
                                          bool dbgOverlaps,
                                          Ath__array1D<Ath__wire*>* UpTable,
                                          const char* msg,
                                          int level)
{
  if (fp1 == NULL)
    return;

  if (dbgOverlaps && UpTable->getCnt() > 0) {
    if (level > 0) {
      char buff[100];
      sprintf(buff, "M%d %s", level, msg);
      fprintf(fp1, "\n%s %d\n", buff, UpTable->getCnt());

    } else {
      fprintf(fp1, "\n%s %d\n", msg, UpTable->getCnt());
    }
    Print(fp1, UpTable, "");
  }
}

void extMeasureRC::PrintTable_segments(FILE* fp1,
                                       Ath__wire* w,
                                       bool lookUp,
                                       bool dbgOverlaps,
                                       Ath__array1D<extSegment*>* segmentTable,
                                       const char* msg,
                                       int level)
{
  if (fp1 == NULL)
    return;

  if (dbgOverlaps && segmentTable->getCnt() > 0) {
    if (level > 0) {
      char buff[100];
      sprintf(buff, "M%d %s", level, msg);
      fprintf(fp1, "\n%s %d\n", buff, segmentTable->getCnt());

    } else {
      fprintf(fp1, "\n%s %d\n", msg, segmentTable->getCnt());
    }
    Print(fp1, segmentTable, !_dir, lookUp);
  }
}
void extMeasureRC::PrintTable_wires(
    FILE* fp,
    bool dbgOverlaps,
    uint colCnt,
    Ath__array1D<Ath__wire*>** verticalPowerTable,
    const char* msg)
{
  if (dbgOverlaps) {
    fprintf(fp, "%s\n", msg);
    for (uint ii = 1; ii < colCnt; ii++)
      Print(fp, verticalPowerTable[ii], "");
  }
}
bool extMeasureRC::DebugWire(Ath__wire* w, int x, int y, int netId)
{
  if (w->getBoxId() == netId)
    Print5wires(stdout, w, w->getLevel());

  return true;
}
Ath__array1D<extSegment*>** extMeasureRC::allocTable(uint n)
{
  Ath__array1D<extSegment*>** tbl = new Ath__array1D<extSegment*>*[n];
  for (uint ii = 0; ii < n; ii++)
    tbl[ii] = new Ath__array1D<extSegment*>(128);
  return tbl;
}
void extMeasureRC::DeleteTable(Ath__array1D<extSegment*>** tbl, uint n)
{
  for (uint ii = 0; ii < n; ii++) {
    delete tbl[ii];
  }
  delete tbl;
}
Ath__array1D<Ath__wire*>** extMeasureRC::allocTable_wire(uint n)
{
  Ath__array1D<Ath__wire*>** tbl = new Ath__array1D<Ath__wire*>*[n];
  for (uint ii = 0; ii < n; ii++)
    tbl[ii] = new Ath__array1D<Ath__wire*>(128);
  return tbl;
}
void extMeasureRC::DeleteTable_wire(Ath__array1D<Ath__wire*>** tbl, uint n)
{
  for (uint ii = 0; ii < n; ii++)
    delete tbl[ii];

  delete tbl;
}
void extMeasureRC::OverUnder(extSegment* cc,
                             uint met,
                             int underMet,
                             int overMet,
                             Ath__array1D<extSegment*>* segTable,
                             const char* ou)
{
  if (segTable->getCnt() == 0)
    return;
  if (_segFP != NULL)
    fprintf(_segFP,
            "\n%7.3f %7.3f  %dL M%d cnt=%d\n",
            GetDBcoords(cc->_xy),
            GetDBcoords(cc->_xy + cc->_len),
            cc->_len,
            met,
            segTable->getCnt());

  for (uint ii = 0; ii < segTable->getCnt(); ii++) {
    extSegment* s = segTable->get(ii);
    if (_segFP)
      PrintOUSeg(_segFP,
                 s->_xy,
                 s->_len,
                 met,
                 overMet,
                 underMet,
                 "\t",
                 cc->_dist,
                 cc->_dist_down);

    if (cc->_dist < 0 && cc->_dist_down < 0)
      OpenEnded2(cc, s->_len, met, overMet, underMet, _segFP);
    else if (cc->_dist < 0 || cc->_dist_down < 0)
      OpenEnded1(cc, s->_len, met, underMet, overMet, _segFP);
    else if ((cc->_dist > 0 && cc->_dist_down > 0)
             && (cc->_dist != cc->_dist_down)
             && (cc->_dist == 200 || cc->_dist_down == 200))  // Model1
      Model1(cc, s->_len, met, underMet, overMet, _segFP);
    else
      OverUnder(cc, s->_len, met, underMet, overMet, _segFP);
  }
}
dbRSeg* extMeasureRC::GetRSeg(extSegment* cc)
{
  uint rsegId = cc->_wire->getRsegId();
  if (rsegId == 0)
    return NULL;
  dbRSeg* rseg1 = dbRSeg::getRSeg(_block, rsegId);
  if (rseg1 == NULL)
    return NULL;

  return rseg1;
}
dbRSeg* extMeasureRC::GetRSeg(uint rsegId)
{
  if (rsegId == 0)
    return NULL;
  dbRSeg* rseg1 = dbRSeg::getRSeg(_block, rsegId);
  if (rseg1 == NULL)
    return NULL;

  return rseg1;
}
void extMeasureRC::OpenEnded2(extSegment* cc,
                              uint len,
                              int met,
                              int overMet,
                              int underMet,
                              FILE* segFP)
{
  dbRSeg* rseg1 = GetRSeg(cc);
  if (rseg1 == NULL)
    return;

  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    extDistRC* rc = OverUnderRC(rcModel,
                                -1,
                                cc->_wire->getWidth(),
                                -1,
                                len,
                                met,
                                underMet,
                                overMet,
                                segFP);

    double inf_cc = 2 * len * (rc->_coupling + rc->_fringe);
    _extMain->updateTotalCap(rseg1, inf_cc, ii);
  }
}
void extMeasureRC::OpenEnded1(extSegment* cc,
                              uint len,
                              int met,
                              int metUnder,
                              int metOver,
                              FILE* segFP)
{
  bool CHECK_COUPLING_THRESHOLD = true;
  int open = 0;
  dbRSeg* rseg = GetRSeg(cc);
  int dist = cc->_dist;
  if (dist < 0)
    dist = cc->_dist_down;

  dbRSeg* rseg2 = cc->_down != NULL ? GetRSeg(cc->_down->getRsegId())
                                    : GetRSeg(cc->_up->getRsegId());

  dbCCSeg* ccCap = NULL;
  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    extDistRC* rc = OverUnderRC(rcModel,
                                open,
                                cc->_wire->getWidth(),
                                dist,
                                len,
                                met,
                                metUnder,
                                metOver,
                                segFP);

    if (rc == NULL)
      continue;
    double fr2 = 2 * len * rc->_fringe;
    double cc = 2 * len * rc->_coupling;

    _extMain->updateTotalCap(rseg, fr2, ii);

    if (CHECK_COUPLING_THRESHOLD) {
      if (ii == 0) {
        // check if the cap value is over the couplingThreshold, then create
        // coupling cap object dbCCSeg
        ccCap = makeCcap(rseg, rseg2, cc);
      }
      if (ccCap != nullptr)
        addCCcap(ccCap, cc / 2, ii);
      else
        addFringe(rseg, rseg2, cc / 2, ii);

    } else {
      updateCoupCap(rseg, rseg2, ii, cc);
    }
  }
}
void extMeasureRC::OverUnder(extSegment* cc,
                             uint len,
                             int met,
                             int metUnder,
                             int metOver,
                             FILE* segFP)
{
  bool CHECK_COUPLING_THRESHOLD = true;
  int open = -1;
  dbRSeg* rseg = GetRSeg(cc);
  dbRSeg* rseg_down = GetRSeg(cc->_down->getRsegId());
  dbRSeg* rseg_up = GetRSeg(cc->_up->getRsegId());

  dbCCSeg* ccCap_up = NULL;
  dbCCSeg* ccCap_down = NULL;
  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    extDistRC* rc_up = OverUnderRC(rcModel,
                                   open,
                                   cc->_wire->getWidth(),
                                   cc->_dist,
                                   len,
                                   met,
                                   metUnder,
                                   metOver,
                                   segFP);
    extDistRC* rc_down = OverUnderRC(rcModel,
                                     open,
                                     cc->_wire->getWidth(),
                                     cc->_dist_down,
                                     len,
                                     met,
                                     metUnder,
                                     metOver,
                                     segFP);

    double fr2 = len * (rc_up->_fringe + rc_down->_fringe);
    double cc_up = len * rc_up->_coupling;
    double cc_down = len * rc_down->_coupling;

    _extMain->updateTotalCap(rseg, fr2, ii);

    if (CHECK_COUPLING_THRESHOLD) {
      if (ii == 0) {
        // check if the cap value is over the couplingThreshold, then create
        // coupling cap object dbCCSeg
        ccCap_up = makeCcap(rseg, rseg_up, cc_up);
        ccCap_down = makeCcap(rseg, rseg_down, cc_down);
      }
      if (ccCap_up != nullptr)
        addCCcap(ccCap_up, cc_up / 2, ii);
      else
        addFringe(rseg, rseg_up, cc_up / 2, ii);

      if (ccCap_down != nullptr)
        addCCcap(ccCap_down, cc_down / 2, ii);
      else
        addFringe(rseg, rseg_down, cc_down / 2, ii);
    } else {
      updateCoupCap(rseg, rseg_up, ii, cc_up);
      updateCoupCap(rseg, rseg_down, ii, cc_down);
    }
  }
}
void extMeasureRC::Model1(extSegment* cc,
                          uint len,
                          int met,
                          int metUnder,
                          int metOver,
                          FILE* segFP)
{
  bool CHECK_COUPLING_THRESHOLD = true;
  int open = 1;
  dbRSeg* rseg1 = GetRSeg(cc);
  dbRSeg* rseg_down = GetRSeg(cc->_down->getRsegId());
  dbRSeg* rseg_up = GetRSeg(cc->_up->getRsegId());

  int dist = cc->_dist;
  if (dist < cc->_dist_down)
    dist = cc->_dist_down;

  dbCCSeg* ccCap_up = NULL;
  dbCCSeg* ccCap_down = NULL;
  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    extMetRCTable* rcModel = _metRCTable.get(ii);
    extDistRC* rc_up = OverUnderRC(rcModel,
                                   -1,
                                   cc->_wire->getWidth(),
                                   cc->_dist,
                                   len,
                                   met,
                                   metUnder,
                                   metOver,
                                   segFP);
    double cc_up = len * rc_up->_coupling;
    // updateCoupCap(rseg1, rseg_up, ii, cc_up);

    extDistRC* rc_down = OverUnderRC(rcModel,
                                     -1,
                                     cc->_wire->getWidth(),
                                     cc->_dist_down,
                                     len,
                                     met,
                                     metUnder,
                                     metOver,
                                     segFP);
    double cc_down = len * rc_down->_coupling;
    // updateCoupCap(rseg1, rseg_down, ii, cc_down);

    extDistRC* rc_fr = OverUnderRC(rcModel,
                                   open,
                                   cc->_wire->getWidth(),
                                   dist,
                                   len,
                                   met,
                                   metUnder,
                                   metOver,
                                   segFP);
    double fr2 = 2 * len * rc_fr->_fringe;
    _extMain->updateTotalCap(rseg1, fr2, ii);

    if (CHECK_COUPLING_THRESHOLD) {
      if (ii == 0) {
        // check if the cap value is over the couplingThreshold, then create
        // coupling cap object dbCCSeg
        ccCap_up = makeCcap(rseg1, rseg_up, cc_up);
        ccCap_down = makeCcap(rseg1, rseg_down, cc_down);
      }
      if (ccCap_up != nullptr)
        addCCcap(ccCap_up, cc_up / 2, ii);
      else
        addFringe(rseg1, rseg_up, cc_up / 2, ii);

      if (ccCap_down != nullptr)
        addCCcap(ccCap_down, cc_down / 2, ii);
      else
        addFringe(rseg1, rseg_down, cc_down / 2, ii);
    } else {
      updateCoupCap(rseg1, rseg_up, ii, cc_up);
      updateCoupCap(rseg1, rseg_down, ii, cc_down);
    }
  }
}
double extMeasureRC::updateCoupCap(dbRSeg* rseg1,
                                   dbRSeg* rseg2,
                                   int jj,
                                   double v)
{
  if (rseg2 == NULL) {
    double tot = _extMain->updateTotalCap(rseg1, v, jj);
    return tot;
  }
  dbCCSeg* ccap
      = dbCCSeg::create(dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
                        dbCapNode::getCapNode(_block, rseg2->getTargetNode()),
                        true);

  v /= 2;
  ccap->addCapacitance(v, jj);

  double cc = ccap->getCapacitance(jj);

  _extMain->printUpdateCoup(
      rseg1->getNet()->getId(), rseg2->getNet()->getId(), v, 2 * v, cc);

  return cc;
}
extDistRC* extMeasureRC::OverUnderRC(extMetRCTable* rcModel,
                                     int open,
                                     uint width,
                                     int dist,
                                     uint len,
                                     int met,
                                     int metUnder,
                                     int metOver,
                                     FILE* segFP)
{
  extDistRC* rc = NULL;
  if (metOver <= 0)
    rc = getOverRC_Dist(rcModel, width, met, metUnder, dist, open);
  else if (metUnder <= 0)
    rc = getUnderRC_Dist(rcModel, width, met, metOver, dist, open);
  else
    rc = getOverUnderRC_Dist(
        rcModel, width, met, metUnder, metOver, dist, open);

  return rc;
}

}  // namespace rcx
