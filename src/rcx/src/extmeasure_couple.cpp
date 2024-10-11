
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

namespace rcx
{

    using utl::RCX;

    using namespace odb;


    bool extMeasureRC::printProgress(uint totalWiresExtracted, uint totWireCnt, float &previous_percent_extracted)
    {
        float percent_extracted = ceil(100.0 * (1.0 * totalWiresExtracted / totWireCnt));

        if ((totWireCnt > 0) && (totalWiresExtracted > 0) && (percent_extracted - previous_percent_extracted >= 5.0))
        {
            fprintf(stdout, "%3d%c completion -- %6d wires have been extracted\n",
                    (int)percent_extracted, '%', totalWiresExtracted);
            previous_percent_extracted = percent_extracted;
            return true;
        }
        return false;
    }

    int extMeasureRC::CouplingFlow(uint dir, uint couplingDist, uint diag_met_limit, int totWireCnt, uint &totalWiresExtracted, float &previous_percent_extracted)
    {
        bool DBG= _extMain->_dbgOption>0;
        uint progress_interval= _extMain->_wire_extracted_progress_count;

        bool new_calc_flow= true;
        int metal_flag= _extMain->_metal_flag_22;

        bool length_flag= false; // dkf 061124 skip cap calculation when new_calc_flow= true if length of wire less than LENGTH_BOUND
        int LENGTH_BOUND= 7000;

        bool verticalCap= true;
        bool diagCapFlag= true;
        bool diagCapPower= true;

        uint notOrderCnt = 0;
        uint oneEmptyTable = 0;
        uint oneCntTable = 0;
        uint wireCnt = 0;

        bool dbgOverlaps = false;
        _segFP=  NULL;
        if (DBG)
        {
            dbgOverlaps = true;
            _segFP = OpenPrintFile(dir, "Segments");
        }
        FILE *fp = _segFP;

        Ath__wire *t = NULL;
        uint limitTrackNum = 10;
        Ath__array1D<extSegment *> upTable;
        Ath__array1D<extSegment *> downTable;
        Ath__array1D<extSegment *> verticalUpTable;
        Ath__array1D<extSegment *> verticalDownTable;
        Ath__array1D<extSegment *> segTable;
        Ath__array1D<extSegment *> aboveTable;
        Ath__array1D<extSegment *> belowTable;
        Ath__array1D<extSegment *> whiteTable;

         Ath__array1D<Ath__wire *> UpTable;

        bool lookUp= true;
        _dir= dir;

        uint colCnt = _search->getColCnt();
        Ath__array1D<Ath__wire *> **firstWireTable = allocMarkTable(colCnt);
        _upSegTable= allocTable(colCnt);
        _downSegTable= allocTable(colCnt);

        _ovSegTable= allocTable(colCnt);
        _whiteSegTable= allocTable(colCnt);

        // TODO need to add in constructor/destructor
        _verticalPowerTable= new Ath__array1D<Ath__wire *> *[colCnt];
        for (uint ii=0; ii<colCnt; ii++)
            _verticalPowerTable[ii]= new Ath__array1D<Ath__wire *>(4);

        for (int level = 1; level < colCnt; level++)
        {
            if (metal_flag>0)
                new_calc_flow= level <= metal_flag ? true : false;

          //  if (DBG)
          //      notice(0, "level= %d new_calc_flow=%d\n", level, new_calc_flow);

            _met = level;
            Ath__grid *netGrid = _search->getGrid(dir, level);
            upTable.resetCnt();

            uint maxDist = 10 * netGrid->_pitch;
            for (uint tr = 0; tr < netGrid->getTrackCnt(); tr++)
            {
                Ath__track *track = netGrid->getTrackPtr(tr);
                if (track == NULL)
                    continue;
                
                // ResetFirstWires(level, level+1, dir, firstWireTable);
                ResetFirstWires(level, colCnt, dir, firstWireTable);
                for (Ath__wire *w = track->getNextWire(NULL); w != NULL; w = w->getNext())
                {
                    wireCnt++;
                    if (w->isPower() || w->getRsegId()==0)
                        continue;
                    
                    if (DBG)
                        Print5wires(_segFP, w, w->getLevel());

                    // DebugWire(w, 0, 0, 17091);

                    // wireCnt++;
                    totalWiresExtracted ++;
                    if (wireCnt%progress_interval==0) {
                        // if (DBG)
                        //    notice(0, "\t%d wires have been extracted\n", wireCnt);
                        printProgress(totalWiresExtracted, totWireCnt, previous_percent_extracted);
                    }

                    // PrintInit(fp, dbgOverlaps, w, 639320, 1720);
                    // PrintInit(fp, dbgOverlaps, w, 64100, 722500);35.720   2.120
                    // PrintInit(fp, dbgOverlaps, w, 35720,  2120);
                    bool found_dbg= false;
                    // found_dbg= PrintInit(fp, dbgOverlaps, w, 50200, 407720);
                    // found_dbg= PrintInit(fp, dbgOverlaps, w, 5157200, 186320);
                    // found_dbg= PrintInit(fp, dbgOverlaps, w, 5157200, 187040);5157.200 313.840
                    // found_dbg= PrintInit(fp, dbgOverlaps, w, 5157200, 313840); 13946.660 530.920
                    // found_dbg= PrintInit(fp, dbgOverlaps, w, 13946660, 530920); 577.300 477.600
                    // found_dbg= PrintInit(fp, dbgOverlaps, w, 577300, 477600); 13946.660 475.640
                    // found_dbg= PrintInit(fp, dbgOverlaps, w, 13946660, 475640); 13946.660 569.000
                    // found_dbg= PrintInit(fp, dbgOverlaps, w, 13946660, 569000);

                    // ------------------------------------------------------------------------------------- Coupling Up
                    FindAllSegments_up(fp, w, lookUp, tr + 1, dir, level, maxDist, couplingDist, limitTrackNum, firstWireTable, _upSegTable);
                    FindAllSegments_up(fp, w, !lookUp, tr - 1, dir, level, maxDist, couplingDist, limitTrackNum, firstWireTable, _downSegTable);
                    uint cnt1 = CreateCouplingSEgments(w, &segTable, _upSegTable[level], _downSegTable[level], dbgOverlaps, fp);

                    if (FindDiagonalNeighbors_vertical_power(dir, w, 10000, 100, 3, _verticalPowerTable) > 0) // power
                        PrintTable_wires(fp, dbgOverlaps, colCnt, _verticalPowerTable, "Vertical Power Wires:");
                    
                    int diagLimit= 3;
                    int diagLimitTrackNum= 4;
                    int diagMaxDist= 500;
                    for (uint jj= level + 1; jj<colCnt && jj< level + diagLimit; jj++)
                    {
                        Ath__grid *upgrid = _search->getGrid(dir, jj);
                        int diag_track_num = upgrid->getTrackNum1(w->getBase());
                        FindAllSegments_up(fp, w,  lookUp, diag_track_num + 1, dir, jj, maxDist, diagMaxDist, diagLimitTrackNum, firstWireTable, _upSegTable);
                        FindAllSegments_up(fp, w,  !lookUp, diag_track_num - 1, dir, jj, maxDist, diagMaxDist, diagLimitTrackNum, firstWireTable, _downSegTable);
                    }
                    for (int jj= level - 1; jj>0 && jj> level - diagLimit; jj--)
                    {
                        Ath__grid *upgrid = _search->getGrid(dir, jj);
                        int diag_track_num = upgrid->getTrackNum1(w->getBase());
                        FindAllSegments_up(fp, w,  lookUp, diag_track_num + 1, dir, jj, maxDist, diagMaxDist, diagLimitTrackNum, firstWireTable, _upSegTable);
                        FindAllSegments_up(fp, w,  !lookUp, diag_track_num - 1, dir, jj, maxDist, diagMaxDist, diagLimitTrackNum, firstWireTable, _downSegTable);
                    }
                    // TODO: diagonal looking down -- at least for power wires! power wires don't look up -- M1 is required as width not wide
                   
                    //     DebugWire(w, 0, 0, 369); 
                    // TODO measureRC_res_init(w->getRsegId()); // reset res value

                   // ----------------------------------------------------------------------------------------------------Diagonal
                    aboveTable.resetCnt();
                    uint upvcnt= FindAllSegments_vertical(fp, w, lookUp, dir,  maxDist, &aboveTable); // Note: _up holds the above vertical wire
                    belowTable.resetCnt(); // for power nets only
                    uint downvcnt= FindAllSegments_vertical(fp, w, !lookUp, dir,  maxDist, &belowTable); // Note: _down holds the above vertical wire

                    for (uint ii = 0; ii < segTable.getCnt(); ii++)
                    {
                        uint lenCovered = 0;
                        extSegment *s = segTable.get(ii);

                        CalcRes(s);
                        if (IsDebugNet1())
                            DebugEnd_res(stdout, _rsegSrcId, s->_len, "AFTER CalcRes");
                    }
                    
                    if (new_calc_flow || length_flag)
                    {
                        _met = w->getLevel();
                        _len = w->getLen();
                        // _segFP= stdout;
                        for (uint ii = 0; ii < segTable.getCnt(); ii++)
                        {
                            uint lenCovered = 0;
                            extSegment *s = segTable.get(ii);

                            /* MOVED outside the loop 
                            CalcRes(s);
                            if  (IsDebugNet1())
                                DebugEnd_res(stdout, _rsegSrcId, s->_len, "AFTER CalcRes");
                            */
                            if (length_flag && s->_len<LENGTH_BOUND)
                                continue;

                            extSegment *white = new extSegment(dir, w, s->_xy, s->_len, NULL, NULL);
                            // _whiteSegTable[_met]->resetCnt();
                            Release(_whiteSegTable[_met]);
                            _whiteSegTable[_met]->add(white);
                            PrintOverlapSeg(_segFP, s, _met, "\nNEW --");

                            Ath__array1D<extSegment *> crossOvelapTable(8);
                            bool fully_blocked_up = false;
                            int lastOverMet = _met + 1;
                            uint overMet = _met + 1;
                            for (; overMet < colCnt; overMet++)
                            {
                                _ovSegTable[overMet]->resetCnt();
                                _whiteSegTable[overMet]->resetCnt();
                               
                                uint di = 1000000;
                                for (uint kk = 0; kk < _whiteSegTable[overMet - 1]->getCnt(); kk++)
                                {
                                    extSegment *ww = _whiteSegTable[overMet - 1]->get(kk);

                                    // vertical and diag
                                    if (verticalCap)
                                    {
                                        Ath__array1D<extSegment *> upVertTable;
                                        FindDiagonalSegments(s, ww, &aboveTable, &upVertTable, dbgOverlaps, fp, lookUp, overMet);
                                        VerticalCap(&upVertTable, lookUp);
                                        Release(&upVertTable);
                                        
                                        Ath__array1D<extSegment *> downVertTable;
                                        FindDiagonalSegments(s, ww, &belowTable, &downVertTable, dbgOverlaps, fp, !lookUp, overMet);
                                        VerticalCap(&downVertTable, !lookUp);
                                        Release(&downVertTable);
                                    }
                                    if (diagCapFlag)
                                    {
                                        Ath__array1D<extSegment *> upDiagTable;
                                        FindDiagonalSegments(s, ww, _upSegTable[overMet], &upDiagTable, dbgOverlaps, fp, lookUp);
                                        DiagCap(fp, w, lookUp, diagMaxDist, 2, &upDiagTable);
                                        Release(&upDiagTable);

                                        Ath__array1D<extSegment *> downDiagTable;
                                        FindDiagonalSegments(s, ww, _downSegTable[overMet], &downDiagTable, dbgOverlaps, fp, !lookUp);
                                        DiagCap(fp, w, !lookUp, diagMaxDist, 2, &downDiagTable);
                                        Release(&downDiagTable);
                                    }

/*
                                    for (; di < _upSegTable[overMet]->getCnt(); di++)
                                    {
                                        extSegment *dg = _upSegTable[overMet]->get(di);
                                        int len1=0;
                                        int len2=0;
                                        int len3=0;
                                        int ovCode= wireOverlap(ww->_xy, ww->_len, dg->_xy, dg->_len, &len1, &len2, &len3);
                                        if (len2>0)
                                        {
                                            // diagClac
                                            if (len3> 0)
                                                break;
                                        } else {
                                            if (len1<=0)
                                                continue;
                                        }
                                        if (len3>0)
                                            break;
                                    }
*/
                                    GetCrossOvelaps(w, overMet, ww->_xy, ww->_len, dir, _ovSegTable[overMet], _whiteSegTable[overMet]);
                                }
                                PrintOvelaps(s, _met, overMet, _ovSegTable[overMet], "u");
                                if (_met == 1)
                                {
                                    OverUnder(s, _met, 0, overMet, _ovSegTable[overMet], "OverSubUnderM");
                                    continue;
                                }
                                for (uint oo = 0; oo < _ovSegTable[overMet]->getCnt(); oo++)
                                { // looking down
                                    extSegment *v = _ovSegTable[overMet]->get(oo);
                                     OverlapDown(overMet, s, v, dir);
                                    if (_whiteSegTable[1]->getCnt()>0)
                                    {
                                        PrintOvelaps(v, _met, 0, _whiteSegTable[1], "OverSubUnderMet");
                                        OverUnder(s, _met, 0, overMet, _whiteSegTable[1], "OverSubUnderM");
                                        _whiteSegTable[1]->resetCnt();
                                    }
                                }
                                if (_whiteSegTable[overMet]->getCnt() == 0)
                                {
                                    lastOverMet = overMet;
                                    break;
                                }
                            }
                            if (overMet >= colCnt - 1)
                            {
                                if (_met == 1)
                                {
                                    PrintOvelaps(s, _met, 0, _whiteSegTable[colCnt - 1], "OverSub");
                                    OverUnder(s, _met, 0, -1, _whiteSegTable[colCnt - 1], "OverSub");
                                }
                                else
                                {
                                    for (uint oo = 0; oo < _whiteSegTable[colCnt - 1]->getCnt(); oo++)
                                    { // looking down
                                        extSegment *v = _whiteSegTable[colCnt - 1]->get(oo);
                                        OverlapDown(-1, s, v, dir); // OverMet
                                    }
                                    PrintOvelaps(s, _met, 0, _whiteSegTable[1], "OverSub");
                                    OverUnder(s, _met, 0, -1, _whiteSegTable[1], "OverSub");
                                }
                            }
                        }
                    }
                    // for (uint ii = 0; ii < !new_calc_flow && segTable.getCnt(); ii++)
                    for (uint ii = 0; (!new_calc_flow || length_flag) && ii < segTable.getCnt(); ii++)
                    {
                        extSegment *s = segTable.get(ii);

                        if (length_flag && s->_len>=LENGTH_BOUND)
                                continue;
                        PrintOverlapSeg(_segFP, s, _met, "\nmeasure_RC_new .........................\n");

                        measure_RC_new(s, true);
                    }
                    /*
                    if (found_dbg && DBG) {
                        fclose(fp);
                        // exit(0);
                    }
                    */

                    for (uint jj = 1; jj < colCnt; jj++)
                    {
                        Release(_ovSegTable[jj]);
                        Release(_whiteSegTable[jj]);
                    }
                    Release(&upTable);
                    Release(&downTable);
                    Release(&segTable);
                    Release(&aboveTable);
                    Release(&belowTable);
                }
            }
            // fprintf(stdout, "\nDir=%d  wireCnt=%d  NotOrderedCnt=%d  oneEmptyTable=%d  oneCntTable=%d\n",
            //        dir, wireCnt, notOrderCnt, oneEmptyTable, oneCntTable);
        }
        if (_segFP!=NULL)
            fclose(_segFP);

        return 0;
    }
    bool extMeasureRC::CalcRes(extSegment *s)
    {
        double deltaRes[10];
        for (uint jj = 0; jj < _metRCTable.getCnt(); jj++)
        {
            deltaRes[jj]= 0;
            _rc[jj]->Reset();
        }
         _met = s->_wire->getLevel();
        _rsegSrcId = s->_wire->getRsegId();
        _netSrcId = s->_wire->getBoxId();
        
        _netId= 0;
        if (IsDebugNet())
        {
            _netId= _netSrcId;
            OpenDebugFile();
            if (_debugFP != NULL) {
                fprintf(stdout,
                        "CalcRes %d met= %d  len= %d  dist= %d r1= %d r2= %d\n", _totSignalSegCnt,
                        _met, _len, _dist, _netSrcId, 0);
                 fprintf(_debugFP,
                        "init_measureRC %d met= %d  len= %d  dist= %d r1= %d r2= %d\n", _totSignalSegCnt,
                        _met, _len, _dist, _netSrcId, 0);
            }
        }
        dbRSeg *rseg1 = dbRSeg::getRSeg(_block, _rsegSrcId);
        int dits1= s->_dist < s->_dist_down ? s->_dist : s->_dist_down;
        int dits2= s->_dist > s->_dist_down ? s->_dist : s->_dist_down;
        
        calcRes(_rsegSrcId, s->_len, dits1, dits2, _met);
        calcRes0(deltaRes, _met, s->_len);

        rcNetInfo();

        for (uint jj = 0; jj < _metRCTable.getCnt(); jj++)
        {
            double totR1 = _rc[jj]->getRes();
            if (totR1 > 0)
            {
                totR1 -= deltaRes[jj];
                if (totR1 != 0.0)
                    _extMain->updateRes(rseg1, totR1, jj);
            }
        }
        if (IsDebugNet1())
        {
            DebugEnd_res(stdout, _rsegSrcId, s->_len, "END");
            DebugEnd_res(_debugFP, _rsegSrcId, s->_len, "END");
            printNetCaps(stdout, "after END");
            printNetCaps(_debugFP, "after END");
            dbNet *net = rseg1->getNet();
            dbSet<dbRSeg> rSet = net->getRSegs();
            odb::dbSet<odb::dbRSeg>::iterator rc_itr;

            for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr)
            {
                odb::dbRSeg *rc = *rc_itr;
                fprintf(stdout, "r%d %g\n", rc->getId(), rc->getResistance(0));
            }
        }

        rcNetInfo();
        return true;
    }
    bool extMeasureRC::FindDiagonalSegments(extSegment *s, extSegment *w1, Ath__array1D<extSegment *> *segDiagTable, Ath__array1D<extSegment *> *diagSegTable, bool dbgOverlaps, FILE *fp, bool lookUp, int tgt_met)
    {
        // vertical and diag
        if (segDiagTable->getCnt() == 0)
            return false;

        extSegment *white = new extSegment(_dir, s->_wire, w1->_xy, w1->_len, NULL, NULL);
        if (!lookUp)
            white->_up= s->_wire;
        else
            white->_down= s->_wire;

        uint diag_cnt1 = 0;

        Ath__array1D<extSegment *> resultTable;
        Ath__array1D<extSegment *> whiteTable;
        whiteTable.add(white);
        if (tgt_met > 0) // vertical
        {
            Ath__array1D<extSegment *> vertTable;
            for (uint ii = 0; ii < segDiagTable->getCnt(); ii++)
            {
                extSegment *v = segDiagTable->get(ii);
                if (lookUp && v->_up->getLevel() == tgt_met)
                    vertTable.add(v);
                if (!lookUp && v->_down->getLevel() == tgt_met)
                    vertTable.add(v);
            }
            if (vertTable.getCnt() == 0)
            {
                delete white;
                return false;
            }
            if (lookUp)
                diag_cnt1 = CreateCouplingSEgments(s->_wire, &resultTable, &vertTable, &whiteTable, dbgOverlaps, NULL);
            else
                diag_cnt1 = CreateCouplingSEgments(s->_wire, &resultTable, &whiteTable, &vertTable, dbgOverlaps, NULL);
        }
        else
        {
            if (lookUp)
                diag_cnt1 = CreateCouplingSEgments(s->_wire, &resultTable, segDiagTable, &whiteTable, dbgOverlaps, NULL);
            else
                diag_cnt1 = CreateCouplingSEgments(s->_wire, &resultTable, &whiteTable, segDiagTable, dbgOverlaps, NULL);
        }
        const char *msg = lookUp ? "Up" : "Down";
        if (fp != NULL)
        {
            if (tgt_met > 0)
                fprintf(fp, "%s Vertical Overlap %d ----- \n", msg, resultTable.getCnt());
            else
                fprintf(fp, "%s Diag Overlap %d ----- \n", msg, resultTable.getCnt());
        }

        for (uint ii = 0; ii < resultTable.getCnt(); ii++)
        {
            extSegment *s1 = resultTable.get(ii);
            if (s1->_up == NULL || s1->_down == NULL)
            {
                delete s1;
                continue;
            }
            diagSegTable->add(s1);
            PrintUpDown(fp, s1);
            PrintUpDownNet(fp, s1->_up, s1->_dist, "\t"); // TODO _up should extSegment
            PrintUpDownNet(fp, s1->_down, s1->_dist_down, "\t"); // TODO _down should extSegment
        }
        delete white;
        return false;

        if (whiteTable.getCnt() == 0)
            return false;

        return false;
    }
    void extMeasureRC::OverlapDown(int overMet, extSegment *coupSeg, extSegment *overlapSeg, uint dir)
    {
        extSegment *ov = new extSegment(dir, coupSeg->_wire, overlapSeg->_xy, overlapSeg->_len, NULL, NULL);
        _whiteSegTable[_met]->resetCnt();
        _whiteSegTable[_met]->add(ov);
        for (int underMet = _met - 1; underMet > 0; underMet--)
        {
            _ovSegTable[underMet]->resetCnt();
            _whiteSegTable[underMet]->resetCnt();
            for (uint kk = 0; kk < _whiteSegTable[underMet + 1]->getCnt(); kk++)
            {
                extSegment *ww = _whiteSegTable[underMet + 1]->get(kk);
                GetCrossOvelaps(coupSeg->_wire, underMet, ww->_xy, ww->_len, dir, _ovSegTable[underMet], _whiteSegTable[underMet]);
            }
            if (_ovSegTable[underMet]->getCnt() > 0)
            {
                PrintOvelaps(overlapSeg, _met, underMet, _ovSegTable[underMet], "ou");
                OverUnder(coupSeg, _met, underMet, overMet, _ovSegTable[underMet], "OU");
            }
            if (_whiteSegTable[underMet]->getCnt() == 0)
                break;

            // _ovSegTable[underMet] holds overUnder
        }
    }
    uint extMeasureRC::FindAllSegments_vertical(FILE *fp, Ath__wire *w, bool lookUp, uint dir, uint maxDist, Ath__array1D<extSegment *> *aboveTable)
    {
        bool dbgOverlaps = true;
        Ath__wire* w2_next= lookUp ? w->_aboveNext : w->_belowNext;
        if (w2_next == NULL)
            return 0;

        Ath__array1D<Ath__wire *> wTable;
        wTable.add(w2_next);
        Ath__wire* w2_next_next= lookUp ? w2_next->_aboveNext : w2_next->_belowNext;
        if (w2_next_next!=NULL)
            wTable.add(w2_next_next);
// TODO, optimize, not accurate 

        if (dbgOverlaps)
        {
            const char *msg = lookUp ? "Up Vertical Wires:" : "Down Vertical Wires:";
            char buff[100];
            sprintf(buff, "M%d %s", w2_next->getLevel(), msg);
            PrintTable_coupleWires(fp, w, dbgOverlaps, &wTable, buff);
        }
        Ath__array1D<extSegment *> sTable;
        FindSegmentsTrack(w, w->getXY(), w->getLen(), w2_next, 0, &wTable, lookUp, dir, maxDist, &sTable);
        for (uint ii= 0; ii<sTable.getCnt(); ii++)
        {
            extSegment *s= sTable.get(ii);
            if (!lookUp && s->_down!=NULL && !s->_down->isPower())
                continue;
            aboveTable->add(s);
        }
        if (dbgOverlaps)
        {
            const char *msg = lookUp ? "Up Vertical Segments:" : "Down Vertical Segments (power only):";
            char buff[100];
            sprintf(buff, "M%d %s", w2_next->getLevel(), msg);
            PrintTable_segments(fp, w, lookUp, dbgOverlaps, aboveTable, buff);
        }
        /*
         uint len2 = FindSegments(true, dir, maxDist, w, w->getXY(), w->getLen(), w->_aboveNext, &aboveTable);
                    if (dbgOverlaps)
                    {
                        if (w->_aboveNext != NULL)
               
                 {
                            fprintf(fp, "Above Diag Wire\n");
                            PrintWire(fp, w->_aboveNext, 0);
                        }
                        fprintf(fp, "Above Diag Segments %d\n", aboveTable.getCnt());
                        Print(fp, &aboveTable, dir, true);
                    }
        */
       return 0;
    }
    uint extMeasureRC::FindAllSegments_up(FILE *fp, Ath__wire *w, bool lookUp, uint start_track, uint dir, uint level, uint maxDist, uint couplingDist, uint limitTrackNum, Ath__array1D<Ath__wire *> **firstWireTable, Ath__array1D<extSegment *> **UpSegTable)
    {
        bool dbgOverlaps = true;
        /*
        if (dbgOverlaps && fp != NULL)
        {
            fprintf(fp, "-----------------------------\n");
            Print5wires(fp, w, w->getLevel());
            fprintf(fp, "-----------------------------\n");
        }*/
        Ath__array1D<Ath__wire *> UpTable;

        // --------------------------- in case that there are wires at distance on the same track
        if (lookUp && level==w->getLevel())
        {
            Ath__wire *w2 = FindOverlapWire(w, w->getNext());
            if (w2 != NULL)
                UpTable.add(w2);
        }
        // -----------------------------------------------------------

        if (lookUp)
            FindAllNeigbors_up(w, start_track, dir, level, couplingDist, limitTrackNum, firstWireTable, &UpTable);
        else
            FindAllNeigbors_down(w, start_track, dir, level, couplingDist, limitTrackNum, firstWireTable, &UpTable);

        FILE *fp1 = fp;
        const char *msg= lookUp ? "Up Coupling Wires:" : "Down Coupling Wires:";
        char buff[100];
        sprintf(buff, "M%d %s", level, msg);
        PrintTable_coupleWires(fp1, w, dbgOverlaps, &UpTable, buff);

        Release(UpSegTable[level]);
        FindSegmentsTrack(w, w->getXY(), w->getLen(), NULL, 0, &UpTable, lookUp, dir, maxDist, UpSegTable[level]);

        msg= lookUp ? "Up Coupling Segments:" : "Down Coupling Segments:";
        sprintf(buff, "M%d %s", level, msg);
        PrintTable_segments(fp1, w, lookUp, dbgOverlaps, UpSegTable[level], buff);
    }
    uint extMeasureRC::FindAllNeigbors_up(Ath__wire *w, uint start_track, uint dir, uint level, uint couplingDist, uint limitTrackNum, Ath__array1D<Ath__wire *> **firstWireTable, Ath__array1D<Ath__wire *> *resTable)
    {
        Ath__grid *upGrid = _search->getGrid(dir, level);
        // int up_track_num = upGrid->getTrackNum1(w->getBase());
        int end_track = start_track + limitTrackNum + 1;

        for (uint next_tr = start_track; next_tr < end_track && next_tr < upGrid->getTrackCnt(); next_tr++) // for tracks overlapping wire
        {
            Ath__wire *first = GetNextWire(upGrid, next_tr, firstWireTable[level]);
            Ath__wire *w2 = FindOverlapWire(w, first);
            if (w2 == NULL) 
                continue;
            firstWireTable[level]->set(next_tr, w2);
            resTable->add(w2);
            if (Enclosed(w->getXY(), w->getXY()+w->getLen(), w2->getXY(), w2->getXY()+w2->getLen()))
                break;

            if (w2->isPower())
                break;
        }
        return resTable->getCnt();
    }
    uint extMeasureRC::FindAllNeigbors_down(Ath__wire *w, int start_track, uint dir, uint level, uint couplingDist, uint limitTrackNum, Ath__array1D<Ath__wire *> **firstWireTable, Ath__array1D<Ath__wire *> *resTable)
    {
        resTable->resetCnt();
        if (start_track<0)
            return 0;

        Ath__grid *upGrid = _search->getGrid(dir, level);
        // int up_track_num = upGrid->getTrackNum1(w->getBase());
        int end_track = start_track - limitTrackNum - 1;
        if (end_track<0)
            end_track= 0;

        for (int next_tr = start_track; next_tr > end_track; next_tr--) // for tracks overlapping wire
        {
            Ath__wire *first = GetNextWire(upGrid, next_tr, firstWireTable[level]);
            Ath__wire *w2 = FindOverlapWire(w, first);
            if (w2 == NULL) 
                continue;

            firstWireTable[level]->set(next_tr, w2);

            bool w2_next_covered= false;
            Ath__wire *w2_next = w2->getNext();
            if (w2_next != NULL) // TODO: because more 2 wires at different distance can reside in same track
            {
                if (OverlapOnly(w->getXY(), w->getLen(), w2_next->getXY(), w2_next->getLen()))
                {
                    resTable->add(w2_next);
                    firstWireTable[level]->set(next_tr, w2_next);
                    w2_next_covered= Enclosed(w->getXY(), w->getXY() + w->getLen(), w2_next->getXY(), w2_next->getXY() + w2_next->getLen());
                }
            }
            resTable->add(w2);
            if (Enclosed(w->getXY(), w->getXY() + w->getLen(), w2->getXY(), w2->getXY() + w2->getLen()))
                break;
            if (w2_next_covered)
                break;

            if (w2->isPower())
                break;
        }
        return resTable->getCnt();
    }
    uint extMeasureRC::CreateCouplingSEgments(Ath__wire *w, Ath__array1D<extSegment *> *segTable, Ath__array1D<extSegment *> *upTable, Ath__array1D<extSegment *> *downTable, bool dbgOverlaps, FILE *fp)
    {
        uint cnt1 = 0;
        /*
                    if (upTable->getCnt() == 0 || downTable->getCnt() == 0)
                    {
                        oneEmptyTable++;
                        // CreateUpDownSegment(w, NULL, w->getXY(), w->getLen(), NULL, &segTable);
                    }
                    if (upTable->getCnt() == 1 || downTable->getCnt() == 1)
                        oneCntTable++;
        */
        if (!CheckOrdered(upTable))
            BubbleSort(upTable);
        if (!CheckOrdered(downTable))
            BubbleSort(downTable);

        if (upTable->getCnt() == 0 && downTable->getCnt() == 0) // OpenEnded both sides
        {
            CreateUpDownSegment(w, NULL, w->getXY(), w->getLen(), NULL, segTable);
        }
        else if (upTable->getCnt() == 0 && downTable->getCnt() > 0) // OpenEnded Down
            cnt1 += CopySegments(false, downTable, 0, downTable->getCnt(), segTable);
        else if (upTable->getCnt() > 0 && downTable->getCnt() == 0) // OpenEnded Up
            cnt1 += CopySegments(true, upTable, 0, upTable->getCnt(), segTable);
        else if (upTable->getCnt() == 1 && downTable->getCnt() > 0) // 1 up,
            cnt1 += FindUpDownSegments(upTable, downTable, segTable);
        else if (upTable->getCnt() > 0 && downTable->getCnt() == 1) // 1 up,
            cnt1 += FindUpDownSegments(upTable, downTable, segTable);
        else if (upTable->getCnt() > 0 && downTable->getCnt() > 0)
            cnt1 += FindUpDownSegments(upTable, downTable, segTable);

        if (dbgOverlaps)
        {
            PrintUpDown(fp, segTable);
            // PrintUpDown(stdout, &segTable);
        }
        if (!CheckOrdered(segTable))
        {
            BubbleSort(segTable);
            if (!CheckOrdered(segTable))
            {
                fprintf(fp, "======> segTable NOT SORTED after Buggble\n");
                fprintf(stdout, "======> segTable NOT SORTED after Buggble\n");
            }
        }
        return cnt1;
    }
    bool extMeasureRC::GetCrossOvelaps(Ath__wire *w, uint tgt_met, int x1, int len, uint dir, Ath__array1D<extSegment *> *segTable, Ath__array1D<extSegment *> *whiteTable)
    {
        bool dbg= false;
        SEQ s1;
        s1._ll[!dir] = x1;
        s1._ur[!dir] = x1 + len;
        s1._ll[dir] = w->getBase();
        s1._ur[dir] = w->getBase();

        Ath__array1D<SEQ *> table(4);
        int ovlen = getOverlapSeq(tgt_met, &s1, &table);
        /*
        if (ovlen>0 && ovlen + w->getWidth() >= len)
        {
            extSegment *s = new extSegment(dir, w, x1, len, NULL, NULL);
            segTable->add(s);
            PrintCrossOvelaps(w, tgt_met, x1, len, segTable, ovlen, "\n\tOU");

            release(&table);
            return true;
        }*/
        int totLen = 0;
        for (uint ii = 0; ii < table.getCnt(); ii++)
        {
            SEQ *p = table.get(ii);

            int len1 = p->_ur[!dir] - p->_ll[!dir];
            extSegment *s = new extSegment(dir, w, p->_ll[!dir], len1, NULL, NULL);

            if (p->type == 0)
                whiteTable->add(s);
            else
            {
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
    int extMeasureRC::wireOverlap(int X1, int DX, int x1, int dx, int *len1, int *len2, int *len3)
    {
        int dx1 = X1 - x1;
        //*len1= dx1;
        if (dx1 >= 0) // on left side
        {
            int dlen = dx - dx1;
            if (dlen <= 0)
                return 1;

            *len1 = 0;
            int DX2 = dlen - DX;

            if (DX2 <= 0)
            {
                *len2 = dlen;
                *len3 = -DX2;
            }
            else
            {
                *len2 = DX;
                //*len3= DX2;
                *len3 = 0;
            }
        }
        else
        {
            *len1 = -dx1;

            if (dx1 + DX <= 0) // outside right side
                return 2;

            int DX2 = (x1 + dx) - (X1 + DX);
            if (DX2 > 0)
            {
                *len2 = DX + dx1; // dx1 is negative
                *len3 = 0;
            }
            else
            {
                *len2 = dx;
                *len3 = -DX2;
            }
        }
        return 0;
    }

    bool extMeasureRC::PrintInit(FILE *fp, bool dbgOverlaps, Ath__wire *w, int x, int y)
    {
        if (dbgOverlaps)
        {
            if (fp==NULL)
                return false;

            fprintf(fp, "\n");
            Print5wires(fp, w, w->getLevel());
        }
        if (w->getXY() == x && w->getBase() == y)
        {
            Print5wires(stdout, w, w->getLevel());
            return true;
        }
        return false;
    }
    void extMeasureRC::PrintTable_coupleWires(FILE *fp1, Ath__wire *w, bool dbgOverlaps, Ath__array1D<Ath__wire *> *UpTable, const char *msg)
    {
        if (fp1==NULL)
            return;
        if (dbgOverlaps && UpTable->getCnt()>0)
        {
            fprintf(fp1, "\n%s %d\n", msg, UpTable->getCnt());
            Print(fp1, UpTable, "");
        }
    }

    void extMeasureRC::PrintTable_segments(FILE *fp1, Ath__wire *w, bool lookUp, bool dbgOverlaps, Ath__array1D<extSegment *> *UpSegTable, const char *msg)
    {
        if (fp1==NULL)
            return;
        if (dbgOverlaps && UpSegTable->getCnt()>0)
        {
            fprintf(fp1, "\n%s %d\n", msg, UpSegTable->getCnt());
            Print(fp1, UpSegTable, !_dir, lookUp);
        }
    }
    void extMeasureRC::PrintTable_wires(FILE *fp, bool dbgOverlaps, uint colCnt, Ath__array1D<Ath__wire *> **verticalPowerTable, const char *msg)
    {
        if (dbgOverlaps)
        {
            fprintf(fp, "%s\n", msg);
            for (uint ii = 1; ii < colCnt; ii++)
                Print(fp, verticalPowerTable[ii], "");
        }
    }
    bool extMeasureRC::DebugWire(Ath__wire *w, int x, int  y, int netId)
    {
        if (w->getBoxId()==netId) 
            Print5wires(stdout, w, w->getLevel());
 
        // if (w->getXY() == 32500 && w->getBase() == 27700)
        //    Print5wires(stdout, w, w->getLevel());
        // else
        //    continue;
        // 32.520   4.120
        // if (w->getXY()==32520 && w->getBase()== 4120)
        //    Print5wires(stdout, w, w->getLevel());
        // else
        //    continue;

        // if (w->getXY()==20500 && w->getBase()==6500)
        //    Print5wires(stdout, w, w->getLevel());
        // if (w->getXY()==64100 && w->getBase()==722500)
        //   Print5wires(stdout, w, w->getLevel());
        // else
        //    continue;

        // if (netId==365 || netId==366)
        //     Print5wires(stdout, w, w->getLevel());

        // if (w->getXY() == 638100 && w->getBase() == 21700)
         //   Print5wires(stdout, w, w->getLevel());
        // else
        //    continue;

       // if (w->getXY() == 127720 && w->getBase() == 2120)
        //    Print5wires(stdout, w, w->getLevel());
        // if (w->getXY() == 13400 && w->getBase() == 4120)
        //    Print5wires(stdout, w, w->getLevel());
        return true;
    }
    Ath__array1D<extSegment *> **extMeasureRC::allocTable(uint n)
    {
        Ath__array1D<extSegment *> **tbl = new Ath__array1D<extSegment *> *[n];
        for (uint ii = 0; ii < n; ii++)
            tbl[ii] = new Ath__array1D<extSegment *>(128);
        return tbl;
    }
    void extMeasureRC::DeleteTable(Ath__array1D<extSegment *> **tbl, uint n)
    {
        for (uint ii = 0; ii < n; ii++)
        {
            delete tbl[ii];
        }
        delete tbl;
    }
    void extMeasureRC::OverUnder(extSegment *cc, uint met, int underMet, int overMet, Ath__array1D<extSegment *> *segTable, const char *ou)
    {
        if (segTable->getCnt() == 0)
            return;
        if (_segFP != NULL)
            fprintf(_segFP, "\n%7.3f %7.3f  %dL M%d cnt=%d\n", GetDBcoords(cc->_xy), GetDBcoords(cc->_xy + cc->_len), cc->_len, met, segTable->getCnt());
        // fprintf(_segFP, "\n%7.3f %7.3f  %dL M%doM%duM%d cnt=%d\n", GetDBcoords(w->_xy), GetDBcoords(w->_xy + w->_len), w->_len, met, ou, tgt_met, segTable->getCnt());
           // fprintf(stdout, "\n%7.3f %7.3f  %dL M%d cnt=%d\n", GetDBcoords(cc->_xy), GetDBcoords(cc->_xy + cc->_len), cc->_len, met, segTable->getCnt());

        for (uint ii = 0; ii < segTable->getCnt(); ii++)
        {
            extSegment *s = segTable->get(ii);
            if (_segFP)
                PrintOUSeg(_segFP, s->_xy, s->_len, met, overMet, underMet, "\t", cc->_dist, cc->_dist_down);
                // PrintOUSeg(stdout, s->_xy, s->_len, met, overMet, underMet, "\t", cc->_dist, cc->_dist_down);
            
/*
            _width= cc->_wire->getWidth();
            _met= met;
            _underMet= underMet;
            _overMet= overMet;

            
            if (overMet>0 && underMet>0)
                computeOverUnderRC(s->_len);
            else if (underMet>0)
                computeOverRC(s->_len);
            else if (overMet)
                computeUnderRC(s->_len);

*/        
            if (cc->_dist<0 && cc->_dist_down<0)
                OpenEnded2(cc, s->_len, met, overMet, underMet, _segFP) ;
            else if (cc->_dist<0 || cc->_dist_down<0) 
                OpenEnded1(cc, s->_len, met, underMet, overMet, _segFP) ;
            else if ((cc->_dist>0 && cc->_dist_down>0) && (cc->_dist != cc->_dist_down) && ( cc->_dist==200 || cc->_dist_down==200)) // Model1
                Model1(cc, s->_len, met, underMet, overMet, _segFP) ;
            else
                OverUnder(cc, s->_len, met, underMet, overMet, _segFP);
        }
    }
    dbRSeg *extMeasureRC::GetRSeg(extSegment *cc)
    {
        uint rsegId = cc->_wire->getRsegId();
        if (rsegId == 0)
            return NULL;
        dbRSeg *rseg1 = dbRSeg::getRSeg(_block, rsegId);
        if (rseg1 == NULL)
            return NULL;

        return rseg1;
    }
    dbRSeg *extMeasureRC::GetRSeg(uint rsegId)
    {
        if (rsegId == 0)
            return NULL;
        dbRSeg *rseg1 = dbRSeg::getRSeg(_block, rsegId);
        if (rseg1 == NULL)
            return NULL;

        return rseg1;
    }
    void extMeasureRC::OpenEnded2(extSegment *cc, uint len, int met, int overMet, int underMet, FILE *segFP)
    {
        dbRSeg *rseg1= GetRSeg(cc);
        if (rseg1==NULL)
            return;

        for (uint ii = 0; ii < _metRCTable.getCnt(); ii++)
        {
            extMetRCTable *rcModel = _metRCTable.get(ii);
            extDistRC *rc= OverUnderRC(rcModel, -1, cc->_wire->getWidth(), -1, len, met, underMet, overMet,  segFP);

            double inf_cc = 2 * len * ( rc->_coupling + rc->_fringe);
            double tot = _extMain->updateTotalCap(rseg1, inf_cc, ii);

            // addRC(rc, 2 * len, ii, inf_cc);
        }
    }
    void extMeasureRC::OpenEnded1(extSegment *cc, uint len, int met, int metUnder, int metOver, FILE *segFP)
    {
        int open = 0;
        dbRSeg *rseg = GetRSeg(cc);
        int dist= cc->_dist;
        if (dist<0)
            dist= cc->_dist_down;

        dbRSeg *rseg2 = cc->_down!=NULL ? GetRSeg(cc->_down->getRsegId()) : GetRSeg(cc->_up->getRsegId());
        
        for (uint ii = 0; ii < _metRCTable.getCnt(); ii++)
        {
            extMetRCTable *rcModel = _metRCTable.get(ii);
            extDistRC *rc=   OverUnderRC(rcModel, open, cc->_wire->getWidth(), dist,      len, met, metUnder, metOver, segFP);

            if (rc==NULL)
                continue;
            double fr2 = 2 * len * rc->_fringe;
            double cc = 2 * len * rc->_coupling;

            double tot = _extMain->updateTotalCap(rseg, fr2, ii);

            updateCoupCap(rseg, rseg2, ii, cc);
        }
    }
    void extMeasureRC::OverUnder(extSegment *cc, uint len, int met, int metUnder, int metOver, FILE *segFP)
    {
        int open = -1;
        dbRSeg *rseg = GetRSeg(cc);
        dbRSeg *rseg_down = GetRSeg(cc->_down->getRsegId());
        dbRSeg *rseg_up = GetRSeg(cc->_up->getRsegId());

        for (uint ii = 0; ii < _metRCTable.getCnt(); ii++)
        {
            extMetRCTable *rcModel = _metRCTable.get(ii);
            extDistRC *rc_up= OverUnderRC(rcModel, open, cc->_wire->getWidth(), cc->_dist, len, met, metUnder, metOver, segFP);
            extDistRC *rc_down= OverUnderRC(rcModel, open, cc->_wire->getWidth(), cc->_dist_down, len, met, metUnder, metOver, segFP);

            double fr2 = len * (rc_up->_fringe + rc_down->_fringe);
            double cc_up = len * rc_up->_coupling;
            double cc_down = len * rc_down->_coupling;

            double tot = _extMain->updateTotalCap(rseg, fr2, ii);
            updateCoupCap(rseg, rseg_up, ii, cc_up);
            updateCoupCap(rseg, rseg_down, ii, cc_down);
        }
    }
    void extMeasureRC::Model1(extSegment *cc, uint len, int met, int metUnder, int metOver, FILE *segFP)
    {
        int open= 1;
        dbRSeg *rseg1= GetRSeg(cc);
        dbRSeg *rseg_down = GetRSeg(cc->_down->getRsegId());
        dbRSeg *rseg_up = GetRSeg(cc->_up->getRsegId());

        int dist= cc->_dist;
        if (dist < cc->_dist_down)
            dist= cc->_dist_down;

        for (uint ii = 0; ii < _metRCTable.getCnt(); ii++)
        {
            extMetRCTable *rcModel = _metRCTable.get(ii);
            extDistRC *rc_up= OverUnderRC(rcModel, -1, cc->_wire->getWidth(), cc->_dist, len, met, metUnder, metOver, segFP);
            double cc_up = len * rc_up->_coupling;
            updateCoupCap(rseg1, rseg_up, ii, cc_up);
            
            extDistRC *rc_down= OverUnderRC(rcModel, -1, cc->_wire->getWidth(), cc->_dist_down, len, met, metUnder, metOver, segFP);
            double cc_down = len * rc_down->_coupling;
            updateCoupCap(rseg1, rseg_down, ii, cc_down);

            extDistRC *rc_fr= OverUnderRC(rcModel, open, cc->_wire->getWidth(), dist, len, met, metUnder, metOver, segFP);
            double fr2 = 2 * len * rc_fr->_fringe;
            double tot = _extMain->updateTotalCap(rseg1, fr2, ii);
        }
    }
    double extMeasureRC::updateCoupCap(dbRSeg *rseg1, dbRSeg *rseg2, int jj, double v)
    {
        if (rseg2==NULL)
        {
            double tot = _extMain->updateTotalCap(rseg1, v, jj);
            return tot;
        }
        dbCCSeg *ccap = dbCCSeg::create(
            dbCapNode::getCapNode(_block, rseg1->getTargetNode()),
            dbCapNode::getCapNode(_block, rseg2->getTargetNode()), true);

        
        v /= 2;
        ccap->addCapacitance(v, jj);

        double cc= ccap->getCapacitance(jj);
        
        _extMain->printUpdateCoup(rseg1->getNet()->getId(), rseg2->getNet()->getId(), v, 2*v, cc);

        return cc;
    }
    extDistRC *extMeasureRC::OverUnderRC(extMetRCTable *rcModel, int open, uint width, int dist, uint len, int met, int metUnder, int metOver, FILE *segFP)
    {
        extDistRC *rc = NULL;
        if (metOver <= 0)
            rc = getOverRC_Dist(rcModel, width, met, metUnder, dist, open);
        else if (metUnder <= 0)
            rc = getUnderRC_Dist(rcModel, width, met, metOver, dist, open);
        else
            rc = getOverUnderRC_Dist(rcModel, width, met, metUnder, metOver, dist, open);

        return rc;
    }

    } // namespace rcx

 /*
                    int lastOverMet = _met + 1;
                    for (uint jj = _met + 1; jj < colCnt; jj++)
                    {
                        if (GetCrossOvelaps(w, jj, w->getXY(), w->getLen(), dir, _ovSegTable[jj], _whiteSegTable[jj]))
                        {
                            lastOverMet = jj;
                            break;
                        }
                    }
                    int lastUnderMet = _met + 1;
                    for (int jj = _met - 1; jj > 0; jj--)
                    {
                        if (GetCrossOvelaps(w, jj, w->getXY(), w->getLen(), dir, _ovSegTable[jj], _whiteSegTable[jj]))
                        {
                            lastUnderMet = jj;
                            break;
                        }
                        // QUESTION: _whiteSegTable[1] is over SUB ?
                    }
                    extSegment *white = new extSegment(dir, w, w->getXY(), w->getLen(), NULL, NULL);
                    _whiteSegTable[_met]->add(white);
                    */
/*
                     // intersect with white _ovSegTable[underMet]
                            // intersect _ovSegTable[overMet] _ovSegTable[underMet]
                            crossOvelapTable.resetCnt();
                            uint cnt = FindUpDownSegments(_ovSegTable[overMet], _ovSegTable[underMet], &crossOvelapTable, overMet, underMet);
                            PrintCrossOvelaps(w, _met, s->_xy, s->_len, &crossOvelapTable, s->_len, "\n\tOUcalc-all");

                            // CALCULATE OU segs : crossOvelapTable: metUnder>0,metOver>0
                            PrintCrossOvelapsOU(w, _met, s->_xy, s->_len, &crossOvelapTable, 0, "OverUnder-calc", overMet, underMet);

                            // lenCovered +=
                            if (fully_blocked_down)
                                break;
                            // all segments with metUnder>0 should cross down

                            // covered -- calculate
                            // if all covered, break
                            // for white
                            }
                            // Under Metal, Over Sub : crossOvelapTable: metUnder=-1
                            if (!fully_blocked_down)
                            {
                                if (crossOvelapTable.getCnt() > 0)
                                {
                                    PrintCrossOvelapsOU(w, _met, s->_xy, s->_len, &crossOvelapTable, 0, "OverSub-calc", overMet, -1);

                                    // find all white (metUnder==-1) == overSub)
                                }
                            }
                            else
                            {
                                break;

                                // if all covered, break
                            }
                            // not all covered: lenCovered -- overSub
                            if (!fully_blocked_up) // white space of M1 is Metal Under X over Sub
                            {
                                // white space of last met
                                int lastMet = colCnt - 1;
                            }
                        }
                    }
*/

/*


                        //   if (_ovSegTable[overMet]->getCnt() == 0)
                        //      continue;
                        /* TODO: for met1 ------------------------------ IMPORTANT
                        int lastUnderMet = _met - 1;

                        bool fully_blocked_down = false;
                        for (int underMet = _met - 1; underMet > 0; underMet--)
                        {
                            _ovSegTable[underMet]->resetCnt();
                            _whiteSegTable[underMet]->resetCnt();
                            for (uint kk = 0; kk < _whiteSegTable[underMet + 1]->getCnt(); kk++)
                            {
                                extSegment *ww = _whiteSegTable[underMet + 1]->get(kk);
                                GetCrossOvelaps(w, underMet, ww->_xy, ww->_len, dir, _ovSegTable[underMet], _whiteSegTable[underMet]);
                            }
                            PrintOvelaps(s, _met, underMet, _ovSegTable[underMet], "o");
                            if (_whiteSegTable[underMet]->getCnt() == 0)
                            {
                                lastUnderMet = underMet;
                                break;
                            }
                        }
                    
                        for (uint overMet = _met + 1; overMet < lastOverMet + 1; overMet++)
                        {
                            for (int underMet = _met - 1; underMet > 0 && underMet > lastUnderMet - 1; underMet--)
                            {
                                crossOvelapTable.resetCnt();
                                if (_ovSegTable[overMet]->getCnt() == 0 || _ovSegTable[underMet]->getCnt() == 0)
                                    continue;

                                uint cnt = FindUpDownSegments(_ovSegTable[overMet], _ovSegTable[underMet], &crossOvelapTable, overMet, underMet);
                                PrintOvelaps(s, underMet, overMet, &crossOvelapTable, "OU");
                                continue;
                                if (_ovSegTable[overMet]->getCnt() == 0 && _ovSegTable[underMet]->getCnt() > 0)
                                {
                                    PrintOvelaps(s, 0, overMet, _ovSegTable[underMet], "ou-Over");
                                }
                                else if (_ovSegTable[overMet]->getCnt() > 0 && _ovSegTable[underMet]->getCnt() == 0)
                                {
                                    PrintOvelaps(s, underMet, 0, _ovSegTable[underMet], "ou-Under");
                                }
                                else
                                {
                                    uint cnt = FindUpDownSegments(_ovSegTable[overMet], _ovSegTable[underMet], &crossOvelapTable, overMet, underMet);
                                    PrintOvelaps(s, underMet, overMet, &crossOvelapTable, "ou");
                                }
                                // PrintCrossOvelaps(w, _met, s->_xy, s->_len, &crossOvelapTable, s->_len, "\n\tOverUnder");
                            }
                        }
                        
                        for (int underMet = _met - 1; underMet > 0; underMet--) // over sub
                        {
                            crossOvelapTable.resetCnt();
                            if (_ovSegTable[underMet]->getCnt() == 0 || _ovSegTable[1]->getCnt() == 0)
                                continue;
                            uint cnt = FindUpDownSegments(_ovSegTable[underMet], _ovSegTable[1], &crossOvelapTable, underMet, 0);
                            PrintCrossOvelaps(w, _met, s->_xy, s->_len, &crossOvelapTable, s->_len, "\n\tOverSub");
                        }
                        */
