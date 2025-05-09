// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <vector>

#include "grids.h"
#include "gseq.h"

namespace rcx {

uint Track::trackContextOn(int orig,
                           int end,
                           int base,
                           int width,
                           uint firstContextTrack,
                           Ath__array1D<int>* context)
{
  Wire* swire = getTargetWire();
  if (!swire) {
    return 0;
  }
  while (swire && swire->_xy + swire->_len < base) {
    swire = nextTargetWire(0);  // context of both power/noPower
  }
  if (!swire || swire->_xy >= (base + width)
      || (!firstContextTrack && swire->_srcId)) {
    return 0;
  }
  int p1 = swire->_base > orig ? swire->_base : orig;
  int send = swire->_base + swire->_width;
  int p2 = send < end ? send : end;
  if (p2 <= p1) {
    return 0;
  }
  context->add(p1);
  context->add(p2);
  return p2 - p1;
}

void Grid::gridContextOn(int orig, int len, int base, int width)
{
  Ath__array1D<int>* context = _gridtable->contextArray()[_level];
  context->resetCnt(0);
  context->add(orig);
  int end = orig + len;
  uint lowTrack = getMinMaxTrackNum(orig);
  uint hiTrack = getMinMaxTrackNum(orig + len);
  Track *track, *btrack;
  uint jj;
  uint firstContextTrack = 1;
  bool tohi = _gridtable->targetHighTracks() > 0 ? true : false;
  for (jj = lowTrack; jj <= hiTrack; jj++) {
    btrack = _trackTable[jj];
    if (btrack == nullptr) {
      continue;
    }
    track = nullptr;
    while ((track = btrack->getNextSubTrack(track, tohi))) {
      track->trackContextOn(orig, end, base, width, firstContextTrack, context);
      firstContextTrack = 0;
    }
  }
  context->add(end);
}

void Grid::contextsOn(int orig, int len, int base, int width)
{
  uint sdepth = _gridtable->contextDepth();
  if (sdepth == 0) {
    return;
  }
  uint ii = _dir ? 0 : 1;
  uint jj;
  for (jj = 1; jj <= sdepth && (jj + _level) < _gridtable->getColCnt(); jj++) {
    _gridtable->getGrid(ii, jj + _level)->gridContextOn(orig, len, base, width);
  }
  for (jj = 1; jj <= sdepth && (_level - jj) > 0; jj++) {
    _gridtable->getGrid(ii, _level - jj)->gridContextOn(orig, len, base, width);
  }
}

// Extraction Coupling Caps

uint Track::findOverlap(Wire* origWire,
                        uint ccThreshold,
                        Ath__array1D<Wire*>* wTable,
                        Ath__array1D<Wire*>* nwTable,
                        Grid* ccGrid,
                        Ath__array1D<Wire*>* ccTable,
                        uint met,
                        rcx::CoupleAndCompute coupleAndCompute,
                        void* compPtr)
{
  rcx::CoupleOptions coupleOptions{};

  AthPool<Wire>* wirePool = _grid->getWirePoolPtr();

  uint NoPowerTarget = _grid->getGridTable()->noPowerTarget();
  bool srcMarked = origWire->getNet()->isMarked();
  uint TargetHighMarkedNet = _grid->getGridTable()->targetHighMarkedNet();
  bool allNet = _grid->getGridTable()->allNet();
  bool needMarkedNetW2 = !allNet && TargetHighMarkedNet && !srcMarked;
  int TTTnoInNetCC = 0;
  int len1, len2, len3, rc;
  Wire* w2 = getTargetWire();
  bool targetHiTrack
      = _grid->getGridTable()->targetHighTracks() > 0 ? true : false;
  bool targetReversed = _grid->getGridTable()->targetTrackReversed();
  bool useDbSdb = _grid->getGridTable()->usingDbSdb();

  uint last = wTable->getCnt();
  bool inThreshold, skipCCgen;
  bool notExtractedW2;
  int exid;
  for (uint ii = 0; ii < last; ii++) {
    Wire* w1 = wTable->get(ii);

    if (w2 == nullptr) {
      nwTable->add(w1);
      continue;
    }

    while (true) {
      notExtractedW2
          = useDbSdb && !w2->isPower() && !w2->isVia()
            && w2->getNet()->getWire()
            && (!w2->getNet()->getWire()->getProperty((int) w2->_otherId, exid)
                || exid == 0);
      len1 = 0;
      len2 = 0;
      len3 = 0;

      inThreshold = true;
      if ((targetHiTrack
           && w2->_base >= w1->_base + (int) (w1->_width + ccThreshold))
          || (!targetHiTrack
              && w1->_base >= w2->_base + (int) (w2->_width + ccThreshold))) {
        inThreshold = false;
      }

      skipCCgen = _grid->getGridTable()->handleEmptyOnly()
                  || (needMarkedNetW2 && !w2->getNet()->isMarked());
      rc = w1->wireOverlap(w2, &len1, &len2, &len3);

      if (inThreshold == true && rc == 0) {
        if (len1 > 0) {
          // create empty wire and ADD on emptyTable!
          Wire* newEmptyWire = origWire->makeWire(wirePool, w1->_xy, len1);
          nwTable->add(newEmptyWire);
        }
        Wire* wtwo = w2->_srcId ? _grid->getWirePtr(w2->_srcId) : w2;
        // create cc and ADD on ccTable
        if (!notExtractedW2 && len2 > 0 && !skipCCgen
            && (!TTTnoInNetCC || wtwo->isPower() || origWire->isPower()
                || origWire->getNet() != wtwo->getNet())
            && (!targetReversed || wtwo->isPower()
                || !wtwo->getNet()->isMarked())) {
          if (coupleAndCompute == nullptr) {
            Wire* ovWire = origWire->makeCoupleWire(ccGrid->getWirePoolPtr(),
                                                    targetHiTrack,
                                                    wtwo,
                                                    w1->_xy + len1,
                                                    len2,
                                                    ccGrid->defaultWireType());

            if (ovWire == nullptr) {
              _grid->getGridTable()->incrCCshorts();
            } else {
              ccTable->add(ovWire);
              ccGrid->placeWire(ovWire);
              ovWire->_flags = ccGrid->defaultWireType();
            }
          } else {
            Wire* topwire = targetHiTrack ? wtwo : origWire;
            Wire* botwire = targetHiTrack ? origWire : wtwo;
            int dist = topwire->_base - (botwire->_base + botwire->_width);

            if (dist > 0) {
              _grid->contextsOn(
                  w1->_xy + len1, len2, botwire->_base + botwire->_width, dist);
              coupleOptions[0] = met;

              int bBoxId = (int) botwire->_boxId;
              if (useDbSdb) {
                bBoxId = botwire->getRsegId();
              }

              coupleOptions[1] = bBoxId;  // dbRSeg id for SRC segment

              if (botwire->_otherId == 0) {
                coupleOptions[1] = -bBoxId;  // POwer SBox Id
              }

              int tBoxId = (int) topwire->_boxId;

              if (useDbSdb) {
                tBoxId = topwire->getRsegId();
              }

              coupleOptions[2] = tBoxId;  // dbRSeg id for TARGET segment
              if (topwire->_otherId == 0) {
                coupleOptions[2] = -tBoxId;  // POwer SBox Id
              }

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
        }
        nwTable->add(w1);
        break;
      }
      if (rc == 1) {
        w2 = nextTargetWire(NoPowerTarget);
        if (!w2) {
          nwTable->add(w1);
          break;
        }
        continue;
      }
      nwTable->add(w1);
      break;
    }
  }
  return nwTable->getCnt();
}

uint Track::initTargetTracks(uint srcTrack, uint trackDist, bool tohi)
{
  uint delt = 0;
  uint trackFound = 0;
  bool noPowerTarget
      = _grid->getGridTable()->noPowerTarget() > 0 ? true : false;
  Track* tstrack = this;
  while (nextSubTrackInRange(tstrack, delt, trackDist, srcTrack, tohi)) {
    tstrack->initTargetWire(noPowerTarget);
    trackFound = 1;
  }
  return trackFound;
}

Track* Track::nextTrackInRange(uint& delt,
                               uint trackDist,
                               uint srcTrack,
                               bool tohi)
{
  Track* ttrack = nullptr;
  uint tgtTnum;
  while (ttrack == nullptr) {
    delt++;
    if (delt > trackDist || (!tohi && srcTrack < delt)) {
      return nullptr;
    }
    tgtTnum = tohi ? srcTrack + delt : srcTrack - delt;
    if (tgtTnum >= _grid->getTrackCnt()) {
      return nullptr;
    }
    ttrack = _grid->getTrackPtr(tgtTnum);
  }
  return ttrack;
}

int Track::nextSubTrackInRange(Track*& tstrack,
                               uint& delt,
                               uint trackDist,
                               uint srcTrack,
                               bool tohi)
{
  tstrack = getNextSubTrack(tstrack, tohi);
  if (tstrack) {
    return 1;
  }
  Track* ttrack = nextTrackInRange(delt, trackDist, srcTrack, tohi);
  if (ttrack == nullptr) {
    return 0;
  }
  tstrack = tohi ? ttrack : ttrack->getLowTrack();
  return 1;
}

uint Track::couplingCaps(Grid* ccGrid,
                         uint srcTrack,
                         uint trackDist,
                         uint ccThreshold,
                         Ath__array1D<uint>* ccIdTable,
                         uint met,
                         CoupleAndCompute coupleAndCompute,
                         void* compPtr,
                         bool ttttGetDgOverlap)
{
  Track* tstrack;
  bool tohi = _grid->getGridTable()->targetHighTracks() > 0 ? true : false;
  initTargetTracks(srcTrack, trackDist, tohi);

  uint dir = _grid->getDir();
  rcx::CoupleOptions coupleOptions{};

  Ath__array1D<Wire*> w1Table;
  Ath__array1D<Wire*> w2Table;
  Ath__array1D<Wire*>*wTable, *nwTable, *twTable;
  Ath__array1D<Wire*> ccTable;

  bool useDbSdb = _grid->getGridTable()->usingDbSdb();
  int noPowerSource = _grid->getGridTable()->noPowerSource();
  uint TargetHighMarkedNet = _grid->getGridTable()->targetHighMarkedNet();
  bool allNet = _grid->getGridTable()->allNet();
  AthPool<Wire>* wirePool = _grid->getWirePoolPtr();
  uint wireCnt = 0;
  Wire* origWire = nullptr;
  uint delt;
  int exid;

  if (ttttGetDgOverlap) {
    // to initTargetSeq
    coupleOptions[0] = -met;
    coupleOptions[5] = 1;
    coupleAndCompute(coupleOptions, compPtr);
  }
  int nexy, nelen;
  Wire* wire = nullptr;
  Wire* pwire = nullptr;
  Wire* nwire = getNextWire(wire);
  for (wire = nwire; wire; pwire = wire, wire = nwire) {
    nwire = getNextWire(wire);
    if (!wire->isPower() && nwire && nwire->isPower()
        && nwire->_xy < wire->_xy + wire->_len) {
      coupleOptions[19] = 0;  // bp
    }
    if (wire->isPower() && nwire && !nwire->isPower()
        && nwire->_xy < wire->_xy + wire->_len) {
      coupleOptions[19] = 0;  // bp
    }

    if (noPowerSource && wire->isPower()) {
      continue;
    }

    // when !TargetHighMarkedNet, need only marked source
    if (!allNet && !TargetHighMarkedNet && !wire->getNet()->isMarked()) {
      continue;
    }
    if (tohi
        && _grid->getMinMaxTrackNum(wire->_base + wire->_width) != srcTrack) {
      continue;
    }
    if (!tohi && wire->_srcId > 0) {
      continue;
    }
    if (useDbSdb && !wire->isPower() && !wire->isVia()
        && wire->getNet()->getWire()
        && (!wire->getNet()->getWire()->getProperty((int) wire->_otherId, exid)
            || exid == 0)) {
      continue;
    }

    wireCnt++;

    w1Table.resetCnt();
    wTable = &w1Table;
    nwTable = &w2Table;
    nexy = wire->_xy;
    nelen = wire->_len;
    int delta = 0;
    if (pwire) {
      delta = pwire->_xy + pwire->_len - wire->_xy;
    }
    if (pwire && delta > 0
        && wire->_base + wire->_width < pwire->_base + pwire->_width) {
      nexy += delta;
      nelen -= delta;
    }
    if (nwire) {
      delta = wire->_xy + wire->_len - nwire->_xy;
    }
    if (nwire && delta > 0
        && wire->_base + wire->_width < nwire->_base + nwire->_width) {
      nelen -= delta;
    }
    if (nelen <= 0) {  // or nelen < wire->_width
      continue;
    }
    Wire* newEmptyWire = wire->makeWire(wirePool, nexy, nelen);
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
                           met,
                           coupleAndCompute,
                           compPtr);

      twTable = wTable;
      wTable = nwTable;
      nwTable = twTable;

      if (wTable->getCnt() == 0) {
        break;
      }
    }
    if (coupleAndCompute != nullptr) {
      bool visited = false;
      int wBoxId = 0;

      for (uint kk = 0; kk < wTable->getCnt(); kk++) {
        Wire* empty = wTable->get(kk);

        coupleOptions[0] = met;

        wBoxId = (int) wire->_boxId;
        if (useDbSdb) {
          wBoxId = wire->getRsegId();
        }

        coupleOptions[1] = wBoxId;  // dbRSeg id
        if (wire->_otherId == 0) {
          coupleOptions[1] = -wBoxId;  // dbRSeg id
        }

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
        if (ignore_visited || (wire->_visited == 0 && wire->_srcWire == nullptr)
            || (wire->_srcWire != nullptr && wire->_srcWire->_visited == 0)) {
          if (wire->_srcWire != nullptr) {
            coupleOptions[20] = wire->_srcWire->_ouLen;
          } else {
            coupleOptions[20] = wire->_ouLen;
          }
          coupleAndCompute(coupleOptions, compPtr);
          visited = true;
          if (wire->_srcWire != nullptr) {
            wire->_srcWire->_ouLen = coupleOptions[20];
          } else {
            wire->_ouLen = coupleOptions[20];
          }
        }
        wirePool->free(empty);
      }
      if (visited) {
        wire->_visited = 1;
      }
    }
  }
  if (coupleAndCompute == nullptr) {
    for (uint kk = 0; kk < ccTable.getCnt(); kk++) {
      Wire* v = ccTable.get(kk);
      ccIdTable->add(v->_id);
    }
  }
  return wireCnt;
}

void GridTable::setDefaultWireType(uint v)
{
  for (uint ii = 0; ii < _rowCnt; ii++) {
    for (uint jj = 0; jj < _colCnt; jj++) {
      if (_gridTable[ii][jj] == nullptr) {
        continue;
      }

      _gridTable[ii][jj]->setDefaultWireType(v);
    }
  }
}

void Track::getTrackWires(std::vector<Wire*>& ctxwire)
{
  uint midx;
  Wire* wire;
  Wire* srcwire;
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
  bool operator()(Wire* wire1, Wire* wire2)
  {
    return (wire1->getXY() < wire2->getXY() ? true : false);
  }
};

// FIXME MATT
void Track::buildDgContext(Ath__array1D<SEQ*>* dgContext,
                           std::vector<Wire*>& allWire)
{
  std::vector<Wire*> ctxwire;
  Track* track = nullptr;
  bool tohi = true;
  uint tcnt = 0;
  while ((track = getNextSubTrack(track, tohi))) {
    tcnt++;
    track->getTrackWires(ctxwire);
  }
  uint ctxsize = ctxwire.size();
  if (ctxsize == 0) {
    return;
  }
  if (tcnt > 1 && ctxsize > 1) {
    std::sort(ctxwire.begin(), ctxwire.end(), compareAthWire());
  }
  uint jj;
  Wire* nwire;
  uint xidx = 0;
  uint yidx = 1;
  uint lidx, bidx;
  AthPool<SEQ>* seqPool = _grid->getGridTable()->seqPool();
  SEQ* seq;
  int rsegid;
  for (jj = 0; jj < ctxsize; jj++) {
    nwire = ctxwire[jj];
    allWire.push_back(nwire);
    seq = seqPool->alloc();
    lidx = _grid->getDir() == 1 ? xidx : yidx;
    bidx = _grid->getDir() == 1 ? yidx : xidx;
    seq->_ll[lidx] = nwire->_xy;
    seq->_ll[bidx] = nwire->_base;
    seq->_ur[lidx] = nwire->_xy + nwire->_len;
    seq->_ur[bidx] = nwire->_base + nwire->_width;
    if (nwire->isPower()) {
      seq->type = 0;
    } else if (nwire->isVia()) {
      seq->type = 0;
    } else {
      nwire->getNet()->getWire()->getProperty((int) nwire->_otherId, rsegid);
      seq->type = rsegid;
    }
    dgContext->add(seq);
  }
}

void Grid::buildDgContext(int gridn, int base)
{
  std::vector<Wire*> allCtxwire;

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
    Track* ttrack = _trackTable[btrackN + tt];
    if (!ttrack) {
      continue;
    }
    _gridtable->dgContextTrackBase()[gridn][dgContextTrackRange + tt]
        = ttrack->getBase();
    ttrack->buildDgContext(dgContext, allCtxwire);
  }
  std::vector<Wire*>::size_type jj;
  for (jj = 0; jj < allCtxwire.size(); jj++) {
    allCtxwire[jj]->_ext = 0;
  }
}

Ath__array1D<SEQ*>* GridTable::renewDgContext(uint gridn, uint trackn)
{
  Ath__array1D<SEQ*>* dgContext = _dgContextArray[gridn][trackn];
  for (uint ii = 0; ii < dgContext->getCnt(); ii++) {
    _seqPool->free(dgContext->get(ii));
  }
  dgContext->resetCnt(0);
  SEQ* seq = _seqPool->alloc();
  seq->_ll[0] = 0;
  seq->_ll[1] = 0;
  seq->_ur[0] = 0;
  seq->_ur[1] = 0;
  dgContext->add(seq);
  return dgContext;
}

void GridTable::buildDgContext(int base, uint level, uint dir)
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
    for (tt = -dgContextTrackRange; tt <= dgContextTrackRange; tt++) {
      renewDgContext(*_dgContextDepth + jj, dgContextTrackRange + tt);
    }
  }
  for (jj = *_dgContextHiLvl + 1; jj < (int) *_dgContextDepth; jj++) {
    for (tt = -dgContextTrackRange; tt <= dgContextTrackRange; tt++) {
      renewDgContext(*_dgContextDepth + jj, dgContextTrackRange + tt);
    }
  }
}

int Grid::couplingCaps(int hiXY,
                       uint couplingDist,
                       uint& wireCnt,
                       rcx::CoupleAndCompute coupleAndCompute,
                       void* compPtr,
                       int* limitArray,
                       bool ttttGetDgOverlap)
{
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
    int hiEnd = hiXY - (ccThreshold + _pitch);
    if (baseXY >= hiEnd) {
      _currentTrack = ii;

      limitArray[4] = ii;
      limitArray[5] = _base + ii * _pitch;

      return baseXY;
    }

    Track* btrack = _trackTable[ii];
    if (btrack == nullptr) {
      continue;
    }

    int base = btrack->getBase();
    _gridtable->buildDgContext(base, _level, _dir);
    if (!ttttGetDgOverlap) {
      CoupleOptions coupleOptionsNull{};
      coupleAndCompute(coupleOptionsNull, compPtr);  // try print dgContext
    }

    Track* track = nullptr;
    bool tohi = true;
    while ((track = btrack->getNextSubTrack(track, tohi))) {
      _gridtable->setHandleEmptyOnly(false);
      uint cnt1 = track->couplingCaps(nullptr,
                                      ii,
                                      coupleTrackNum,
                                      ccThreshold,
                                      nullptr,
                                      _level,
                                      coupleAndCompute,
                                      compPtr,
                                      ttttGetDgOverlap);
      wireCnt += cnt1;
      if (allNet || TargetHighMarkedNet) {
        _gridtable->setHandleEmptyOnly(true);
      }
      _gridtable->reverseTargetTrack();
      cnt1 = track->couplingCaps(nullptr,
                                 ii,
                                 coupleTrackNum,
                                 ccThreshold,
                                 nullptr,
                                 _level,
                                 coupleAndCompute,
                                 compPtr,
                                 ttttGetDgOverlap);
      wireCnt += cnt1;
      _gridtable->reverseTargetTrack();
    }
  }
  limitArray[4] = _searchHiTrack;
  limitArray[5] = hiXY;
  return hiXY;
}

int Grid::dealloc(int hiXY)
{
  for (uint ii = _lastFreeTrack; ii <= _searchHiTrack; ii++) {
    int baseXY = _base + _pitch * ii;  // TO_VERIFY for continuation of track
    if (baseXY >= hiXY) {
      _lastFreeTrack = ii;
      return baseXY;
    }

    Track* btrack = _trackTable[ii];
    if (btrack == nullptr) {
      continue;
    }

    Track* track = nullptr;
    bool tohi = true;
    while ((track = btrack->getNextSubTrack(track, tohi))) {
      track->dealloc(_wirePoolPtr);
      _trackPoolPtr->free(track);
    }
    _trackTable[ii] = nullptr;
  }
  _lastFreeTrack = _searchHiTrack;
  return hiXY;
}

int GridTable::dealloc(uint dir, int hiXY)
{
  for (uint jj = 1; jj < _colCnt; jj++) {
    Grid* netGrid = _gridTable[dir][jj];
    if (netGrid == nullptr) {
      continue;
    }

    netGrid->dealloc(hiXY);
  }
  return hiXY;
}

int Track::getBandWires(Ath__array1D<Wire*>* bandWire)
{
  uint midx;
  Wire* wire;
  Wire* srcwire;
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

int Grid::getBandWires(int hiXY,
                       uint couplingDist,
                       uint& wireCnt,
                       Ath__array1D<Wire*>* bandWire,
                       int* limitArray)
{
  uint coupleTrackNum = couplingDist;  // EXT-OPTIMIZE
  uint ccThreshold = coupleTrackNum * _pitch;
  uint TargetHighMarkedNet = _gridtable->targetHighMarkedNet();
  bool allNet = _gridtable->allNet();

  uint domainAdjust = allNet || !TargetHighMarkedNet ? 0 : couplingDist;

  setSearchDomain(domainAdjust);

  int hiEnd = hiXY - (ccThreshold + _pitch);
  int endTrack = getMinMaxTrackNum(hiEnd);
  _currentTrack = endTrack;
  if (_base + _pitch * endTrack < hiEnd) {
    _currentTrack++;
  }
  if (_currentTrack > _searchHiTrack) {
    _currentTrack = _searchHiTrack;
  }
  int minExtracted = _base + _pitch * _currentTrack;
  int baseXY = minExtracted;
  if (_currentTrack == _searchHiTrack) {
    baseXY = hiXY;
  }

  int fullEndTrack = getMinMaxTrackNum(hiXY) + coupleTrackNum + 2;
  if (fullEndTrack >= (int) _trackCnt) {
    fullEndTrack = _trackCnt - 1;
  }
  int jj;
  bool tohi = true;
  Track* ttrack;
  Track* strack;
  bandWire->resetCnt(0);

  limitArray[0] = _lastFreeTrack;
  limitArray[1] = _base + _lastFreeTrack * _pitch;
  limitArray[6] = fullEndTrack;
  limitArray[7] = _base + fullEndTrack * _pitch;

  limitArray[2] = _currentTrack;
  limitArray[3] = _base + _currentTrack * _pitch;

  for (jj = _lastFreeTrack; jj <= fullEndTrack; jj++) {
    ttrack = _trackTable[jj];
    strack = nullptr;
    while ((strack = ttrack->getNextSubTrack(strack, tohi))) {
      strack->getBandWires(bandWire);
    }
  }
  limitArray[4] = fullEndTrack;
  limitArray[5] = _base + fullEndTrack * _pitch;

  for (jj = 0; jj < (int) bandWire->getCnt(); jj++) {
    bandWire->get(jj)->_ext = 0;
  }
  return baseXY;
}

int GridTable::couplingCaps(int hiXY,
                            uint couplingDist,
                            uint dir,
                            uint& wireCnt,
                            rcx::CoupleAndCompute coupleAndCompute,
                            void* compPtr,
                            bool getBandWire,
                            int** limitArray)
{
  _ttttGetDgOverlap = true;
  setCCFlag(couplingDist);

  if (getBandWire) {
    if (_bandWire == nullptr) {
      _bandWire = new Ath__array1D<Wire*>(4096);
    }
  } else {
    if (_bandWire) {
      delete (_bandWire);
    }
    _bandWire = nullptr;
  }
  int minExtracted = hiXY;
  for (uint jj = 1; jj < _colCnt; jj++) {
    Grid* netGrid = _gridTable[dir][jj];
    if (netGrid == nullptr) {
      continue;
    }

    int lastExtracted1;
    if (getBandWire) {
      lastExtracted1 = netGrid->getBandWires(
          hiXY, couplingDist, wireCnt, _bandWire, limitArray[jj]);
    } else {
      lastExtracted1 = netGrid->couplingCaps(hiXY,
                                             couplingDist,
                                             wireCnt,
                                             coupleAndCompute,
                                             compPtr,
                                             limitArray[jj],
                                             _ttttGetDgOverlap);
    }

    if (minExtracted > lastExtracted1) {
      minExtracted = lastExtracted1;
    }
  }
  return minExtracted;
}

int Grid::initCouplingCapLoops(uint couplingDist,
                               rcx::CoupleAndCompute coupleAndCompute,
                               void* compPtr,
                               bool startSearchTrack,
                               int startXY)
{
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

void GridTable::initCouplingCapLoops(uint dir,
                                     uint couplingDist,
                                     rcx::CoupleAndCompute coupleAndCompute,
                                     void* compPtr,
                                     int* startXY)
{
  _ttttGetDgOverlap = true;
  setCCFlag(couplingDist);

  for (uint jj = 1; jj < _colCnt; jj++) {
    Grid* netGrid = _gridTable[dir][jj];
    if (netGrid == nullptr) {
      continue;
    }

    if (startXY == nullptr) {
      netGrid->initCouplingCapLoops(couplingDist, coupleAndCompute, compPtr);
    } else {
      netGrid->initCouplingCapLoops(
          couplingDist, coupleAndCompute, compPtr, false, startXY[jj]);
    }
  }
}

}  // namespace rcx
