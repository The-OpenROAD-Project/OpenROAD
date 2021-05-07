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

#include <stdio.h>

#include <algorithm>

#include "ZInterface.h"
#include "dbLogger.h"
#include "wire.h"
//#define ZDEBUG 1
uint ttttGetDgOverlap;
//#define TEST_GetDgOverlap

uint Ath__track::trackContextOn(int orig,
                                int end,
                                int base,
                                int width,
                                uint firstContextTrack,
                                Ath__array1D<int>* context)
{
  Ath__wire* swire = getTargetWire();
  if (!swire)
    return 0;
  while (swire && swire->_xy + swire->_len < base)
    swire = nextTargetWire(0);  // context of both power/noPower
  if (!swire || swire->_xy >= (base + width)
      || (!firstContextTrack && swire->_srcId))
    return 0;
  int p1 = swire->_base > orig ? swire->_base : orig;
  int send = swire->_base + swire->_width;
  int p2 = send < end ? send : end;
  if (p2 <= p1)
    return 0;
  context->add(p1);
  context->add(p2);
  return p2 - p1;
}

void Ath__grid::gridContextOn(int orig, int len, int base, int width)
{
  Ath__array1D<int>* context = _gridtable->contextArray()[_level];
  context->resetCnt(0);
  context->add(orig);
  int end = orig + len;
  uint lowTrack = getMinMaxTrackNum(orig);
  uint hiTrack = getMinMaxTrackNum(orig + len);
  Ath__track *track, *btrack;
  uint jj;
  uint firstContextTrack = 1;
  uint clength = 0;
  bool tohi = _gridtable->targetHighTracks() > 0 ? true : false;
  for (jj = lowTrack; jj <= hiTrack; jj++) {
    btrack = _trackTable[jj];
    if (btrack == NULL)
      continue;
    track = NULL;
    while ((track = btrack->getNextSubTrack(track, tohi))) {
      clength += track->trackContextOn(
          orig, end, base, width, firstContextTrack, context);
      firstContextTrack = 0;
    }
  }
  context->add(end);
  _gridtable->setContextLength(_level, clength);
}

void Ath__grid::contextsOn(int orig, int len, int base, int width)
{
  uint sdepth = _gridtable->contextDepth();
  if (sdepth == 0)
    return;
  uint ii = _dir ? 0 : 1;
  uint jj;
  for (jj = 1; jj <= sdepth && (jj + _level) < _gridtable->getColCnt(); jj++)
    _gridtable->getGrid(ii, jj + _level)->gridContextOn(orig, len, base, width);
  for (jj = 1; jj <= sdepth && (_level - jj) > 0; jj++)
    _gridtable->getGrid(ii, _level - jj)->gridContextOn(orig, len, base, width);
}

// Extraction Coupling Caps

uint Ath__track::findOverlap(Ath__wire* origWire,
                             uint ccThreshold,
                             Ath__array1D<Ath__wire*>* wTable,
                             Ath__array1D<Ath__wire*>* nwTable,
                             Ath__grid* ccGrid,
                             Ath__array1D<Ath__wire*>* ccTable,
                             ZInterface* context,
                             uint met,
                             void (*coupleAndCompute)(int*, void*),
                             void* compPtr)
{
  int coupleOptions[20];

  AthPool<Ath__wire>* wirePool = _grid->getWirePoolPtr();

  uint NoPowerTarget = _grid->getGridTable()->noPowerTarget();
  bool srcMarked = origWire->getNet()->isMarked();
  uint TargetHighMarkedNet = _grid->getGridTable()->targetHighMarkedNet();
  bool allNet = _grid->getGridTable()->allNet();
  bool needMarkedNetW2 = !allNet && TargetHighMarkedNet && !srcMarked;
  int TTTnoInNetCC = 0;
  int len1, len2, len3, rc;
  Ath__wire* w2 = getTargetWire();
  bool targetHiTrack
      = _grid->getGridTable()->targetHighTracks() > 0 ? true : false;
  bool targetReversed = _grid->getGridTable()->targetTrackReversed();
  bool useDbSdb = _grid->getGridTable()->usingDbSdb();

  uint last = wTable->getCnt();
  bool inThreshold, skipCCgen;
  bool notExtractedW2;
  int exid;
  for (uint ii = 0; ii < last; ii++) {
    Ath__wire* w1 = wTable->get(ii);

    if (w2 == NULL) {
      nwTable->add(w1);
      continue;
    }

    while (1) {
      notExtractedW2
          = useDbSdb && !w2->isPower() && !w2->isVia()
            && w2->getNet()->getWire()
            && (!w2->getNet()->getWire()->getProperty((int) w2->_otherId, exid)
                || exid == 0);
      len1 = 0;
      len2 = 0;
      len3 = 0;
      rc = 0;

      // context->event( "CCW","xy", Z_INT, w2->_xy,"len", Z_INT,
      // w2->_len,NULL);

      inThreshold = true;
      if ((targetHiTrack
           && w2->_base >= w1->_base + (int) (w1->_width + ccThreshold))
          || (!targetHiTrack
              && w1->_base >= w2->_base + (int) (w2->_width + ccThreshold)))
        inThreshold = false;

      skipCCgen = _grid->getGridTable()->handleEmptyOnly()
                  || (needMarkedNetW2 && !w2->getNet()->isMarked());
      rc = w1->wireOverlap(w2, &len1, &len2, &len3);

      if (inThreshold == true && rc == 0) {
        if (len1 > 0) {
          // create empty wire and ADD on emptyTable!
          Ath__wire* newEmptyWire = origWire->makeWire(wirePool, w1->_xy, len1);
          nwTable->add(newEmptyWire);
        }
        Ath__wire* wtwo = w2->_srcId ? _grid->getWirePtr(w2->_srcId) : w2;
        // create cc and ADD on ccTable
        if (!notExtractedW2 && len2 > 0 && !skipCCgen
            && (!TTTnoInNetCC || wtwo->isPower() || origWire->isPower()
                || origWire->getNet() != wtwo->getNet())
            && (!targetReversed || wtwo->isPower()
                || !wtwo->getNet()->isMarked())) {
          if (coupleAndCompute == NULL) {
            Ath__wire* ovWire
                = origWire->makeCoupleWire(ccGrid->getWirePoolPtr(),
                                           targetHiTrack,
                                           wtwo,
                                           w1->_xy + len1,
                                           len2,
                                           ccGrid->defaultWireType());

            if (ovWire == NULL)
              _grid->getGridTable()->incrCCshorts();
            else {
              ccTable->add(ovWire);
              ccGrid->placeWire(ovWire);
              ovWire->_flags = ccGrid->defaultWireType();
            }
          } else {
            Ath__wire* topwire = targetHiTrack ? wtwo : origWire;
            Ath__wire* botwire = targetHiTrack ? origWire : wtwo;
            int dist = topwire->_base - (botwire->_base + botwire->_width);

            if (dist > 0) {
              _grid->contextsOn(
                  w1->_xy + len1, len2, botwire->_base + botwire->_width, dist);
              coupleOptions[0] = met;

              int bBoxId = (int) botwire->_boxId;
              if (useDbSdb)
                bBoxId = botwire->getRsegId();

              // DF 820 if (botwire->_otherId && useDbSdb && !botwire->isVia())
              // 	botwire->getNet()->getWire()->getProperty((int)botwire->_otherId,
              // bBoxId);
              coupleOptions[1] = bBoxId;  // dbRSeg id for SRC segment

              if (botwire->_otherId == 0)
                coupleOptions[1] = -bBoxId;  // POwer SBox Id

              int tBoxId = (int) topwire->_boxId;

              if (useDbSdb)
                tBoxId = topwire->getRsegId();

              // DF 820 if (topwire->_otherId && useDbSdb && !topwire->isVia())
              //	topwire->getNet()->getWire()->getProperty((int)topwire->_otherId,
              // tBoxId);
              coupleOptions[2] = tBoxId;  // dbRSeg id for TARGET segment
              if (topwire->_otherId == 0)
                coupleOptions[2] = -tBoxId;  // POwer SBox Id

              coupleOptions[3] = len2;
              coupleOptions[4] = dist;
              coupleOptions[5] = w1->_xy + len1;
              coupleOptions[6] = botwire->_dir;

              coupleOptions[7] = botwire->_width;
              coupleOptions[8] = topwire->_width;
              coupleOptions[9] = botwire->_base;
              coupleOptions[11] = targetHiTrack ? 1 : 0;
              coupleOptions[18] = botwire->_track->getTrackNum();
              coupleAndCompute(coupleOptions, compPtr);
            }
          }
        }

        // context->event( "OV","xy", Z_INT, ovWire->_xy,"len", Z_INT,
        // ovWire->_len,NULL);

        if (len3 > 0) {
          w1->setXY(w1->_xy + len1 + len2, len3);
          w2 = nextTargetWire(NoPowerTarget);
          if (!w2) {
            nwTable->add(w1);
            break;
          }
          continue;
        }
        wirePool->free(w1);
        break;
      }
      if (inThreshold == false && rc == 0) {
        if (len3 > 0) {
          w2 = nextTargetWire(NoPowerTarget);
          if (!w2) {
            nwTable->add(w1);
            break;
          }
          continue;
        } else {
          nwTable->add(w1);
          break;
        }
      }
      if (rc == 1) {
        w2 = nextTargetWire(NoPowerTarget);
        if (!w2) {
          nwTable->add(w1);
          break;
        }
        continue;
      }
      // (rc == 2)
      nwTable->add(w1);
      break;
    }
  }
  return nwTable->getCnt();
}

uint Ath__track::initTargetTracks(uint srcTrack, uint trackDist, bool tohi)
{
  uint delt = 0;
  //	uint tgtTrack = 0;
  uint trackFound = 0;
  bool noPowerTarget
      = _grid->getGridTable()->noPowerTarget() > 0 ? true : false;
  Ath__track* tstrack = this;
  while (nextSubTrackInRange(tstrack, delt, trackDist, srcTrack, tohi)) {
    tstrack->initTargetWire(noPowerTarget);
    trackFound = 1;
  }
  return trackFound;
}

Ath__track* Ath__track::nextTrackInRange(uint& delt,
                                         uint trackDist,
                                         uint srcTrack,
                                         bool tohi)
{
  Ath__track* ttrack = NULL;
  uint tgtTnum;
  while (ttrack == NULL) {
    delt++;
    if (delt > trackDist || (!tohi && srcTrack < delt))
      return NULL;
    tgtTnum = tohi ? srcTrack + delt : srcTrack - delt;
    if (tgtTnum >= _grid->getTrackCnt())
      return NULL;
    ttrack = _grid->getTrackPtr(tgtTnum);
  }
  return ttrack;
}

int Ath__track::nextSubTrackInRange(Ath__track*& tstrack,
                                    uint& delt,
                                    uint trackDist,
                                    uint srcTrack,
                                    bool tohi)
{
  tstrack = getNextSubTrack(tstrack, tohi);
  if (tstrack)
    return 1;
  Ath__track* ttrack = nextTrackInRange(delt, trackDist, srcTrack, tohi);
  if (ttrack == NULL)
    return 0;
  tstrack = tohi ? ttrack : ttrack->getLowTrack();
  return 1;
}

uint Ath__track::couplingCaps(Ath__grid* ccGrid,
                              uint srcTrack,
                              uint trackDist,
                              uint ccThreshold,
                              ZInterface* context,
                              Ath__array1D<uint>* ccIdTable,
                              uint met,
                              void (*coupleAndCompute)(int*, void*),
                              void* compPtr)
{
  Ath__track* tstrack;
  bool tohi = _grid->getGridTable()->targetHighTracks() > 0 ? true : false;
  initTargetTracks(srcTrack, trackDist, tohi);
  // need to process "empty wire" (non-coupled wire)
  // if (!trackFound)
  // 	return 0;

  uint dir = _grid->getDir();
  int coupleOptions[21];

  Ath__array1D<Ath__wire*> w1Table;
  Ath__array1D<Ath__wire*> w2Table;
  Ath__array1D<Ath__wire*>*wTable, *nwTable, *twTable;
  Ath__array1D<Ath__wire*> ccTable;

  bool useDbSdb = _grid->getGridTable()->usingDbSdb();
  int noPowerSource = _grid->getGridTable()->noPowerSource();
  uint TargetHighMarkedNet = _grid->getGridTable()->targetHighMarkedNet();
  bool allNet = _grid->getGridTable()->allNet();
  AthPool<Ath__wire>* wirePool = _grid->getWirePoolPtr();
  uint wireCnt = 0;
  Ath__wire* origWire = NULL;
  //	bool srcMarked;
  uint delt;
  int exid;

  if (ttttGetDgOverlap) {
    // to initTargetSeq
    coupleOptions[0] = -met;
    coupleOptions[5] = 1;
    coupleAndCompute(coupleOptions, compPtr);
  }
  int nexy, nelen;
  Ath__wire* wire = NULL;
  Ath__wire* pwire = NULL;
  Ath__wire* nwire = getNextWire(wire);
  for (wire = nwire; wire; pwire = wire, wire = nwire) {
    nwire = getNextWire(wire);
#ifdef TEST_GetDgOverlap
    if (ttttGetDgOverlap) {
      if (wire->isPower() || wire->_srcId > 0
          || _grid->getGridTable()->handleEmptyOnly())
        continue;
      coupleOptions[0] = -met;
      coupleOptions[1] = wire->_xy;
      coupleOptions[2] = wire->_xy + wire->_len;
      coupleOptions[3] = wire->_base;
      coupleOptions[4] = wire->_base + wire->_width;
      coupleOptions[5] = 2;
      coupleOptions[6] = wire->_dir;
      coupleAndCompute(coupleOptions, compPtr);
      continue;
    }
#endif
    if (!wire->isPower() && nwire && nwire->isPower()
        && nwire->_xy < wire->_xy + wire->_len)
      coupleOptions[19] = 0;  // bp
    if (wire->isPower() && nwire && !nwire->isPower()
        && nwire->_xy < wire->_xy + wire->_len)
      coupleOptions[19] = 0;  // bp

    if (noPowerSource && wire->isPower())
      continue;
    // if (wire->isVia())
    //	continue;

    if (!allNet && !TargetHighMarkedNet
        && !wire->getNet()->isMarked())  // when !TargetHighMarkedNet, need only
                                         // marked source
      continue;
    if (tohi
        && _grid->getMinMaxTrackNum(wire->_base + wire->_width) != srcTrack)
      continue;
    if (!tohi && wire->_srcId > 0)
      continue;
    if (useDbSdb && !wire->isPower() && !wire->isVia()
        && wire->getNet()->getWire()
        && (!wire->getNet()->getWire()->getProperty((int) wire->_otherId, exid)
            || exid == 0))
      continue;

    wireCnt++;

    // ccGrid->placeWire(wire);

    w1Table.resetCnt();
    wTable = &w1Table;
    nwTable = &w2Table;
    nexy = wire->_xy;
    nelen = wire->_len;
    int delta;
    if (pwire)
      delta = pwire->_xy + pwire->_len - wire->_xy;
    if (pwire && delta > 0
        && wire->_base + wire->_width < pwire->_base + pwire->_width) {
      nexy += delta;
      nelen -= delta;
    }
    if (nwire)
      delta = wire->_xy + wire->_len - nwire->_xy;
    if (nwire && delta > 0
        && wire->_base + wire->_width < nwire->_base + nwire->_width)
      nelen -= delta;
    // assert (nelen > 0);
    //// assert (nelen >= 0);
    //// if (nelen == 0)  // or nelen < wire->_width
    if (nelen <= 0)  // or nelen < wire->_width
      continue;
    Ath__wire* newEmptyWire = wire->makeWire(wirePool, nexy, nelen);
    wTable->add(newEmptyWire);

    delt = 0;
    tstrack = this;
    while (nextSubTrackInRange(tstrack, delt, trackDist, srcTrack, tohi)) {
      nwTable->resetCnt();

      origWire = wire->_srcId ? _grid->getWirePtr(wire->_srcId) : wire;
      tstrack->findOverlap(origWire,
                           ccThreshold,
                           wTable,
                           nwTable,
                           ccGrid,
                           &ccTable,
                           context,
                           met,
                           coupleAndCompute,
                           compPtr);

      twTable = wTable;
      wTable = nwTable;
      nwTable = twTable;

      if (wTable->getCnt() == 0)
        break;
    }
    if (coupleAndCompute != NULL) {
      bool visited = false;
      int wBoxId = 0;

      for (uint kk = 0; kk < wTable->getCnt(); kk++) {
        Ath__wire* empty = wTable->get(kk);

        coupleOptions[0] = met;

        wBoxId = (int) wire->_boxId;
        if (useDbSdb)
          wBoxId = wire->getRsegId();

        coupleOptions[1] = wBoxId;  // dbRSeg id
        if (wire->_otherId == 0)
          coupleOptions[1] = -wBoxId;  // dbRSeg id

        coupleOptions[2] = 0;  // dbRSeg id

        coupleOptions[3] = empty->_len;
        coupleOptions[4] = -1;
        coupleOptions[5] = empty->_xy;
        coupleOptions[6] = wire->_dir;

        coupleOptions[7] = wire->_width;
        coupleOptions[8] = 0;
        coupleOptions[9] = wire->_base;
        coupleOptions[10] = dir;

        coupleOptions[11] = tohi ? 1 : 0;

        bool ignore_visited = true;
        if (ignore_visited || (wire->_visited == 0 && wire->_srcWire == NULL)
            || (wire->_srcWire != NULL && wire->_srcWire->_visited == 0)) {
          // notice(0, "coupleAndCompute: net= %d-%d base %d xy %d L%d %s\n",
          // wire->getNet()->getId(), wire->_otherId, wire->_base, wire->_xy,
          // wire->_len,
          //	wire->getNet()->getConstName() );
          if (wire->_srcWire != NULL) {
            coupleOptions[20] = wire->_srcWire->_ouLen;
          } else {
            coupleOptions[20] = wire->_ouLen;
          }
          coupleAndCompute(coupleOptions, compPtr);
          visited = true;
          if (wire->_srcWire != NULL)
            wire->_srcWire->_ouLen = coupleOptions[20];
          else
            wire->_ouLen = coupleOptions[20];
        }
        wirePool->free(empty);
      }
      if (visited)
        wire->_visited = 1;
    }
  }
  if (coupleAndCompute == NULL) {
    for (uint kk = 0; kk < ccTable.getCnt(); kk++) {
      Ath__wire* v = ccTable.get(kk);
      ccIdTable->add(v->_id);

      // compute and/or add on a grid
    }
  }
  //	notice(0, "\t%d wires make %d ccaps\n", wireCnt, ccTable.getCnt());
  // return ccTable.getCnt();
  return wireCnt;
}

uint Ath__grid::couplingCaps(Ath__grid* resGrid,
                             uint couplingDist,
                             ZInterface* context,
                             Ath__array1D<uint>* ccTable,
                             void (*coupleAndCompute)(int*, void*),
                             void* compPtr)
{
  // Ath__array1D<Ath__wire*> ccTable;
  // Ath__array1D<Ath__wire*> wTable;

  uint coupleTrackNum = couplingDist;  // EXT-OPTIMIZE
  uint ccThreshold = coupleTrackNum * _pitch;
  uint TargetHighMarkedNet = _gridtable->targetHighMarkedNet();
  bool allNet = _gridtable->allNet();

  uint domainAdjust = allNet || !TargetHighMarkedNet ? 0 : couplingDist;
  initContextGrids();
  setSearchDomain(domainAdjust);
  uint cnt = 0;
  for (uint ii = _searchLowTrack; ii <= _searchHiTrack; ii++) {
    Ath__track* btrack = _trackTable[ii];
    if (btrack == NULL)
      continue;

    int base = btrack->getBase();
    _gridtable->buildDgContext(base, _level, _dir);
    if (!ttttGetDgOverlap)
      coupleAndCompute(NULL, compPtr);  // try print dgContext

    Ath__track* track = NULL;
    bool tohi = true;
    while ((track = btrack->getNextSubTrack(track, tohi))) {
      _gridtable->setHandleEmptyOnly(false);
      uint cnt1 = track->couplingCaps(resGrid,
                                      ii,
                                      coupleTrackNum,
                                      ccThreshold,
                                      context,
                                      ccTable,
                                      _level,
                                      coupleAndCompute,
                                      compPtr);
      cnt += cnt1;
      if (allNet || TargetHighMarkedNet)
        _gridtable->setHandleEmptyOnly(true);
      _gridtable->reverseTargetTrack();
      cnt1 = track->couplingCaps(resGrid,
                                 ii,
                                 coupleTrackNum,
                                 ccThreshold,
                                 context,
                                 ccTable,
                                 _level,
                                 coupleAndCompute,
                                 compPtr);
      cnt += cnt1;
      _gridtable->reverseTargetTrack();
    }
    //		notice(0, "CC: Track - %5d : %d out of %d\n", ii, cnt1, cnt);
  }

  return cnt;
}
void Ath__gridTable::setDefaultWireType(uint v)
{
  for (uint ii = 0; ii < _rowCnt; ii++) {
    for (uint jj = 0; jj < _colCnt; jj++) {
      if (_gridTable[ii][jj] == NULL)
        continue;

      _gridTable[ii][jj]->setDefaultWireType(v);
    }
  }
}

void Ath__track::getTrackWires(std::vector<Ath__wire*>& ctxwire)
{
  uint midx;
  Ath__wire* wire;
  Ath__wire* srcwire;
  for (midx = 0; midx < _markerCnt; midx++) {
    wire = _marker[midx];
    while (wire) {
      srcwire = wire->_srcId ? _grid->getWirePtr(wire->_srcId) : wire;
      if (srcwire->_ext == 0) {
        srcwire->_ext = 1;
        ctxwire.push_back(srcwire);
      }
      wire = wire->_next;
    }
  }
}

class compareAthWire
{
 public:
  bool operator()(Ath__wire* wire1, Ath__wire* wire2)
  {
    return (wire1->getXY() < wire2->getXY() ? true : false);
  }
};

// FIXME MATT
void Ath__track::buildDgContext(Ath__array1D<SEQ*>* dgContext,
                                Ath__wire**& allWire,
                                int& awcnt,
                                int& awsize)
{
  std::vector<Ath__wire*> ctxwire;
  Ath__track* track = NULL;
  bool tohi = true;
  uint tcnt = 0;
  while ((track = getNextSubTrack(track, tohi))) {
    tcnt++;
    track->getTrackWires(ctxwire);
  }
  uint ctxsize = ctxwire.size();
  if (ctxsize == 0)
    return;
  if (tcnt > 1 && ctxsize > 1)
    std::sort(ctxwire.begin(), ctxwire.end(), compareAthWire());
  uint jj;
  Ath__wire* nwire;
  uint xidx = 0;
  uint yidx = 1;
  uint lidx, bidx;
  AthPool<SEQ>* seqPool = _grid->getGridTable()->seqPool();
  SEQ* seq;
  int rsegid;
  for (jj = 0; jj < ctxsize; jj++) {
    if (awcnt == awsize) {
      awsize += 1024;
      allWire = (Ath__wire**) realloc(allWire, sizeof(Ath__wire*) * awsize);
    }
    nwire = ctxwire[jj];
    allWire[awcnt++] = nwire;
    seq = seqPool->alloc();
    lidx = _grid->getDir() == 1 ? xidx : yidx;
    bidx = _grid->getDir() == 1 ? yidx : xidx;
    seq->_ll[lidx] = nwire->_xy;
    seq->_ll[bidx] = nwire->_base;
    seq->_ur[lidx] = nwire->_xy + nwire->_len;
    seq->_ur[bidx] = nwire->_base + nwire->_width;
    if (nwire->isPower())
      seq->type = 0;
    else if (nwire->isVia())
      seq->type = 0;
    else {
      nwire->getNet()->getWire()->getProperty((int) nwire->_otherId, rsegid);
      seq->type = rsegid;
    }
    dgContext->add(seq);
  }
}

void Ath__grid::buildDgContext(int gridn, int base)
{
  static Ath__wire** allCtxwire = NULL;
  static int awcnt;
  static int awsize;
  if (allCtxwire == NULL) {
    allCtxwire = (Ath__wire**) calloc(sizeof(Ath__wire*), 4096);
    awsize = 4096;
  }
  awcnt = 0;
  uint btrackN = getMinMaxTrackNum(base);
  uint dgContextTrackRange = _gridtable->getCcFlag();
  int lowtrack
      = btrackN >= dgContextTrackRange ? -dgContextTrackRange : -btrackN;
  int hitrack = btrackN + dgContextTrackRange < _trackCnt
                    ? dgContextTrackRange
                    : _trackCnt - 1 - btrackN;
  _gridtable->dgContextBaseTrack()[gridn] = btrackN;
  _gridtable->dgContextLowTrack()[gridn] = lowtrack;
  _gridtable->dgContextHiTrack()[gridn] = hitrack;
  int tt;
  for (tt = lowtrack; tt <= hitrack; tt++) {
    Ath__array1D<SEQ*>* dgContext
        = _gridtable->renewDgContext(gridn, dgContextTrackRange + tt);
    Ath__track* ttrack = _trackTable[btrackN + tt];
    if (!ttrack)
      continue;
    _gridtable->dgContextTrackBase()[gridn][dgContextTrackRange + tt]
        = ttrack->getBase();
    ttrack->buildDgContext(dgContext, allCtxwire, awcnt, awsize);
  }
  int jj;
  for (jj = 0; jj < awcnt; jj++)
    allCtxwire[jj]->_ext = 0;
}
Ath__array1D<SEQ*>* Ath__gridTable::renewDgContext(uint gridn, uint trackn)
{
  Ath__array1D<SEQ*>* dgContext = _dgContextArray[gridn][trackn];
  for (uint ii = 0; ii < dgContext->getCnt(); ii++)
    _seqPool->free(dgContext->get(ii));
  dgContext->resetCnt(0);
  SEQ* seq = _seqPool->alloc();
  seq->_ll[0] = 0;
  seq->_ll[1] = 0;
  seq->_ur[0] = 0;
  seq->_ur[1] = 0;
  dgContext->add(seq);
  return dgContext;
}

void Ath__gridTable::buildDgContext(int base, uint level, uint dir)
{
  *_dgContextBaseLvl = level;
  *_dgContextLowLvl = (int) level - (int) *_dgContextDepth >= 1
                          ? -(int) *_dgContextDepth
                          : 1 - (int) level;
  *_dgContextHiLvl = (int) level + (int) *_dgContextDepth < (int) _colCnt
                         ? (int) *_dgContextDepth
                         : (int) _colCnt - 1 - (int) level;
  int jj;
  for (jj = *_dgContextLowLvl; jj <= *_dgContextHiLvl; jj++) {
    _gridTable[dir][level + jj]->buildDgContext(*_dgContextDepth + jj, base);
  }
  // for the glitich between def _layerCnt and extRule _layerCnt,
  // extmeasure may look up one extra layer
  int dgContextTrackRange = _ccFlag;
  int tt;
  for (jj = -(int) *_dgContextDepth; jj < *_dgContextLowLvl; jj++) {
    for (tt = -dgContextTrackRange; tt <= dgContextTrackRange; tt++)
      renewDgContext(*_dgContextDepth + jj, dgContextTrackRange + tt);
  }
  for (jj = *_dgContextHiLvl + 1; jj < (int) *_dgContextDepth; jj++) {
    for (tt = -dgContextTrackRange; tt <= dgContextTrackRange; tt++)
      renewDgContext(*_dgContextDepth + jj, dgContextTrackRange + tt);
  }
}

uint Ath__gridTable::couplingCaps(Ath__gridTable* resGridTable,
                                  uint couplingDist,
                                  ZInterface* context,
                                  Ath__array1D<uint>* ccTable,
                                  void (*coupleAndCompute)(int*, void*),
                                  void* compPtr)
{
  //	ttttGetDgOverlap= 0;
  //	if (couplingDist>20) {
  //		couplingDist= couplingDist % 10;
  //		ttttGetDgOverlap= 1;
  //	}
  ttttGetDgOverlap = 1;
  setCCFlag(couplingDist);
  _CCshorts = 0;
  uint cnt = 0;
  for (uint jj = 0; jj < _colCnt; jj++) {
    // for (uint ii= 0; ii<_rowCnt; ii++) {
    for (int ii = _rowCnt - 1; ii >= 0; ii--) {
      Ath__grid* resGrid = NULL;
      if (resGridTable != NULL)
        resGrid = resGridTable->getGrid(ii, jj);
      //			notice(0, "-----------------------------Grid
      // dir= %d Layer=%d\n", ii, jj);

      Ath__grid* netGrid = _gridTable[ii][jj];
      if (netGrid == NULL)
        continue;

      // netGrid->adjustMarkers();

      cnt += netGrid->couplingCaps(
          resGrid, couplingDist, context, ccTable, coupleAndCompute, compPtr);

#ifdef ZDEBUG
      context->event("GRID", "dir", Z_INT, ii, "layer", Z_INT, jj, NULL);
#endif
    }
  }
  notice(0, "Final %d ccaps\n", cnt);
  notice(0, "      %d interTrack shorts\n", _CCshorts);
  return cnt;
}
uint Ath__gridTable::couplingCaps(uint row,
                                  uint col,
                                  Ath__grid* resGrid,
                                  uint couplingDist,
                                  ZInterface* context)
{
  return 0;
  // return _gridTable[row][col]->couplingCaps(resGrid, couplingDist, context);
}
int Ath__grid::couplingCaps(int hiXY,
                            uint couplingDist,
                            uint& wireCnt,
                            void (*coupleAndCompute)(int*, void*),
                            void* compPtr,
                            int* limitArray)
{
  // CHECK ccTable

  // Ath__array1D<Ath__wire*> ccTable;
  // Ath__array1D<Ath__wire*> wTable;

  uint coupleTrackNum = couplingDist;  // EXT-OPTIMIZE
  uint ccThreshold = coupleTrackNum * _pitch;
  uint TargetHighMarkedNet = _gridtable->targetHighMarkedNet();
  bool allNet = _gridtable->allNet();

  uint domainAdjust = allNet || !TargetHighMarkedNet ? 0 : couplingDist;

  initContextGrids();
  setSearchDomain(domainAdjust);

  limitArray[0] = _lastFreeTrack;
  limitArray[1] = _base + _lastFreeTrack * _pitch;

  limitArray[6] = _searchHiTrack;
  limitArray[7] = _base + _searchHiTrack * _pitch;

  limitArray[2] = _currentTrack;
  limitArray[3] = _base + _currentTrack * _pitch;

  for (uint ii = _currentTrack; ii <= _searchHiTrack; ii++) {
    int baseXY = _base + _pitch * ii;  // TO_VERIFY for continuation of track
    // minExtracted= baseXY; // TO_VERIFY for continuation of track
    int hiEnd = hiXY - (ccThreshold + _pitch);
    if (baseXY >= hiEnd) {
      // if (baseXY>=hiXY) {
      _currentTrack = ii;

      limitArray[4] = ii;
      limitArray[5] = _base + ii * _pitch;

      return baseXY;
    }

    Ath__track* btrack = _trackTable[ii];
    if (btrack == NULL)
      continue;

    int base = btrack->getBase();
    _gridtable->buildDgContext(base, _level, _dir);
    if (!ttttGetDgOverlap)
      coupleAndCompute(NULL, compPtr);  // try print dgContext

    Ath__track* track = NULL;
    bool tohi = true;
    while ((track = btrack->getNextSubTrack(track, tohi))) {
      _gridtable->setHandleEmptyOnly(false);
      uint cnt1 = track->couplingCaps(NULL,
                                      ii,
                                      coupleTrackNum,
                                      ccThreshold,
                                      NULL,
                                      NULL,
                                      _level,
                                      coupleAndCompute,
                                      compPtr);
      wireCnt += cnt1;
      if (allNet || TargetHighMarkedNet)
        _gridtable->setHandleEmptyOnly(true);
      _gridtable->reverseTargetTrack();
      // cnt1= track->couplingCaps(resGrid, ii, coupleTrackNum, ccThreshold,
      // context, ccTable, _level, coupleAndCompute, compPtr);
      cnt1 = track->couplingCaps(NULL,
                                 ii,
                                 coupleTrackNum,
                                 ccThreshold,
                                 NULL,
                                 NULL,
                                 _level,
                                 coupleAndCompute,
                                 compPtr);
      wireCnt += cnt1;
      _gridtable->reverseTargetTrack();
    }
    //		notice(0, "CC: Track - %5d : %d out of %d\n", ii, cnt1, cnt);
  }
  limitArray[4] = _searchHiTrack;
  limitArray[5] = hiXY;
  return hiXY;  // finished
}
int Ath__grid::dealloc(int hiXY)
{
  //	uint cnt= 0;
  for (uint ii = _lastFreeTrack; ii <= _searchHiTrack; ii++) {
    int baseXY = _base + _pitch * ii;  // TO_VERIFY for continuation of track
    if (baseXY >= hiXY) {
      _lastFreeTrack = ii;
      return baseXY;
    }

    Ath__track* btrack = _trackTable[ii];
    if (btrack == NULL)
      continue;

    Ath__track* track = NULL;
    bool tohi = true;
    while ((track = btrack->getNextSubTrack(track, tohi))) {
      track->dealloc(_wirePoolPtr);
      _trackPoolPtr->free(track);
    }
    _trackTable[ii] = NULL;

    // btrack->dealloc(_wirePoolPtr);
    //_trackPoolPtr->free(btrack);
  }
  _lastFreeTrack = _searchHiTrack;
  return hiXY;
}
int Ath__gridTable::dealloc(uint dir, int hiXY)
{
  for (uint jj = 1; jj < _colCnt; jj++) {
    Ath__grid* netGrid = _gridTable[dir][jj];
    if (netGrid == NULL)
      continue;

    netGrid->dealloc(hiXY);
  }
  return hiXY;
}

int Ath__track::getBandWires(Ath__array1D<Ath__wire*>* bandWire)
{
  uint midx;
  Ath__wire* wire;
  Ath__wire* srcwire;
  int cnt = 0;
  for (midx = 0; midx < _markerCnt; midx++) {
    wire = _marker[midx];
    while (wire) {
      srcwire = wire->_srcId ? _grid->getWirePtr(wire->_srcId) : wire;
      if (srcwire->_ext == 0) {
        srcwire->_ext = 1;
        bandWire->add(srcwire);
        cnt++;
      }
      wire = wire->_next;
    }
  }
  return cnt;
}

int Ath__grid::getBandWires(int hiXY,
                            uint couplingDist,
                            uint& wireCnt,
                            Ath__array1D<Ath__wire*>* bandWire,
                            int* limitArray)
{
  // CHECK ccTable

  // Ath__array1D<Ath__wire*> ccTable;
  // Ath__array1D<Ath__wire*> wTable;

  uint coupleTrackNum = couplingDist;  // EXT-OPTIMIZE
  uint ccThreshold = coupleTrackNum * _pitch;
  uint TargetHighMarkedNet = _gridtable->targetHighMarkedNet();
  bool allNet = _gridtable->allNet();

  uint domainAdjust = allNet || !TargetHighMarkedNet ? 0 : couplingDist;

  // initContextGrids();
  setSearchDomain(domainAdjust);

  int hiEnd = hiXY - (ccThreshold + _pitch);
  int endTrack = getMinMaxTrackNum(hiEnd);
  //	int startTrack = _currentTrack;
  _currentTrack = endTrack;
  if (_base + _pitch * endTrack < hiEnd)
    _currentTrack++;
  if (_currentTrack > _searchHiTrack)
    _currentTrack = _searchHiTrack;
  int minExtracted = _base + _pitch * _currentTrack;
  int baseXY = minExtracted;
  if (_currentTrack == _searchHiTrack)
    baseXY = hiXY;

  int fullEndTrack = getMinMaxTrackNum(hiXY) + coupleTrackNum + 2;
  if (fullEndTrack >= (int) _trackCnt)
    fullEndTrack = _trackCnt - 1;
  int jj;
  bool tohi = true;
  Ath__track* ttrack;
  Ath__track* strack;
  bandWire->resetCnt(0);

  limitArray[0] = _lastFreeTrack;
  limitArray[1] = _base + _lastFreeTrack * _pitch;
  limitArray[6] = fullEndTrack;
  limitArray[7] = _base + fullEndTrack * _pitch;

  limitArray[2] = _currentTrack;
  limitArray[3] = _base + _currentTrack * _pitch;

  // for (jj = startTrack; jj <= _currentTrack; jj++)
  for (jj = _lastFreeTrack; jj <= fullEndTrack; jj++) {
    ttrack = _trackTable[jj];
    strack = NULL;
    while ((strack = ttrack->getNextSubTrack(strack, tohi)))
      strack->getBandWires(bandWire);
  }
  limitArray[4] = fullEndTrack;
  limitArray[5] = _base + fullEndTrack * _pitch;

  for (jj = 0; jj < (int) bandWire->getCnt(); jj++)
    bandWire->get(jj)->_ext = 0;
  return baseXY;
}

int Ath__gridTable::couplingCaps(int hiXY,
                                 uint couplingDist,
                                 uint dir,
                                 uint& wireCnt,
                                 void (*coupleAndCompute)(int*, void*),
                                 void* compPtr,
                                 bool getBandWire,
                                 int** limitArray)
{
  //	ttttGetDgOverlap= 0;
  //	if (couplingDist>20) {
  //		couplingDist= couplingDist % 10;
  //		ttttGetDgOverlap= 1;
  //	}
  ttttGetDgOverlap = 1;
  setCCFlag(couplingDist);

  if (getBandWire) {
    if (_bandWire == NULL)
      _bandWire = new Ath__array1D<Ath__wire*>(4096);
  } else {
    if (_bandWire)
      delete (_bandWire);
    _bandWire = NULL;
  }
  int minExtracted = hiXY;
  for (uint jj = 1; jj < _colCnt; jj++) {
    Ath__grid* netGrid = _gridTable[dir][jj];
    if (netGrid == NULL)
      continue;

    int lastExtracted1;
    if (getBandWire)
      lastExtracted1 = netGrid->getBandWires(
          hiXY, couplingDist, wireCnt, _bandWire, limitArray[jj]);
    else
      lastExtracted1 = netGrid->couplingCaps(hiXY,
                                             couplingDist,
                                             wireCnt,
                                             coupleAndCompute,
                                             compPtr,
                                             limitArray[jj]);

    if (minExtracted > lastExtracted1)
      minExtracted = lastExtracted1;
  }
  return minExtracted;
}
int Ath__grid::initCouplingCapLoops(uint couplingDist,
                                    void (*coupleAndCompute)(int*, void*),
                                    void* compPtr,
                                    bool startSearchTrack,
                                    int startXY)
{
  //_coupleAndCompute= coupleAndCompute;
  //_compPtr= compPtr;

  //	uint coupleTrackNum= couplingDist; // EXT-OPTIMIZE
  //	uint ccThreshold = coupleTrackNum*_pitch;
  uint TargetHighMarkedNet = _gridtable->targetHighMarkedNet();
  bool allNet = _gridtable->allNet();

  uint domainAdjust = allNet || !TargetHighMarkedNet ? 0 : couplingDist;

  initContextGrids();
  setSearchDomain(domainAdjust);
  if (startSearchTrack) {
    _currentTrack = _searchLowTrack;
  } else {
    _currentTrack = (startXY - _base) / _pitch;
  }
  _lastFreeTrack = 0;

  return _base + _pitch * _searchHiTrack;
}
void Ath__gridTable::initCouplingCapLoops(uint dir,
                                          uint couplingDist,
                                          void (*coupleAndCompute)(int*, void*),
                                          void* compPtr,
                                          int* startXY)
{
  //	ttttGetDgOverlap= 0;
  //	if (couplingDist>20) {
  //		couplingDist= couplingDist % 10;
  //		ttttGetDgOverlap= 1;
  //	}
  ttttGetDgOverlap = 1;
  setCCFlag(couplingDist);

  for (uint jj = 1; jj < _colCnt; jj++) {
    Ath__grid* netGrid = _gridTable[dir][jj];
    if (netGrid == NULL)
      continue;

    if (startXY == NULL)
      netGrid->initCouplingCapLoops(couplingDist, coupleAndCompute, compPtr);
    else
      netGrid->initCouplingCapLoops(
          couplingDist, coupleAndCompute, compPtr, false, startXY[jj]);
  }
}
