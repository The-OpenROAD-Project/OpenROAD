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

#include "grids.h"

#include <cstdio>

namespace rcx {

Box::Box()
{
  set(0, 0, 0, 0);
  _layer = 0;
}

int Box::getXlo(const int bound) const
{
  return std::max(_rect.xMin(), bound);
}

int Box::getYlo(const int bound) const
{
  return std::max(_rect.yMin(), bound);
}

int Box::getXhi(const int bound) const
{
  return std::min(_rect.xMax(), bound);
}

int Box::getYhi(const int bound) const
{
  return std::min(_rect.yMax(), bound);
}

uint Box::getDir() const
{
  const uint dx = getDX();
  const uint dy = getDY();

  return (dx < dy) ? 0 : 1;
}

uint Box::getWidth(uint* dir) const
{
  const uint dx = getDX();
  const uint dy = getDY();
  if (dx < dy) {
    *dir = 0;  // vertical
    return dx;
  }
  *dir = 1;  // horizontal
  return dy;
}

Box::Box(int x1, int y1, int x2, int y2, int units)
{
  set(x1, y1, x2, y2, units);
}

void Box::set(int x1, int y1, int x2, int y2, int units)
{
  _rect = {x1 * units, y1 * units, x2 * units, y2 * units};
  _valid = 1;
  _id = 0;
}

uint Box::getOwner() const
{
  return 0;
}

uint Box::getDX() const
{
  return _rect.dx();
}

uint Box::getDY() const
{
  return _rect.dy();
}

uint Box::getLength() const
{
  return std::max(getDX(), getDY());
}

void Box::invalidateBox()
{
  _valid = 0;
}

void Box::set(Box* bb)
{
  _rect = bb->_rect;
}

void SearchBox::set(int x1, int y1, int x2, int y2, uint l, int dir)
{
  _ll[0] = x1;
  _ll[1] = y1;
  _ur[0] = x2;
  _ur[1] = y2;
  _level = l;
  setDir(dir);
  _ownId = 0;
  _otherId = 0;
  _type = 0;
}
SearchBox::SearchBox(int x1, int y1, int x2, int y2, uint l, int dir)
{
  set(x1, y1, x2, y2, l, dir);
}

void SearchBox::setLo(uint d, int xy)
{
  _ll[d] = xy;
}
void SearchBox::setHi(uint d, int xy)
{
  _ur[d] = xy;
}
int SearchBox::loXY(uint d) const
{
  return _ll[d];
}
int SearchBox::hiXY(uint d) const
{
  return _ur[d];
}
int SearchBox::loXY(uint d, int loBound) const
{
  if (_ll[d] < loBound) {
    return loBound;
  }
  return _ll[d];
}
int SearchBox::hiXY(uint d, int hiBound) const
{
  if (_ur[d] > hiBound) {
    return hiBound;
  }
  return _ur[d];
}
uint SearchBox::getDir() const
{
  return _dir;
}
uint SearchBox::getLevel() const
{
  return _level;
}
uint SearchBox::getOwnerId() const
{
  return _ownId;
}
uint SearchBox::getOtherId() const
{
  return _otherId;
}
uint SearchBox::getType() const
{
  return _type;
}
void SearchBox::setOwnerId(uint v, uint other)
{
  _ownId = v;
  _otherId = other;
}
void SearchBox::setType(uint v)
{
  _type = v;
}
void SearchBox::setDir(int dir)
{
  if (dir >= 0) {
    _dir = dir;
  } else {
    _dir = 1;  // horizontal
    int dx = _ur[0] - _ll[0];
    if (dx < _ur[1] - _ll[1]) {
      _dir = 0;  // vertical
    }
  }
}
uint SearchBox::getLength() const
{
  if (_dir > 0) {
    return _ur[0] - _ll[0];
  }
  return _ur[1] - _ll[1];
}
void SearchBox::setLevel(uint v)
{
  _level = v;
}

void Wire::reset()
{
  _boxId = 0;
  _srcId = 0;
  _otherId = 0;
  _track = nullptr;
  _next = nullptr;
  _upNext = nullptr;
  _downNext = nullptr;
  _aboveNext = nullptr;
  _belowNext = nullptr;
  _ouLen = 0;

  _xy = 0;  // offset from track start
  _len = 0;

  _base = 0;
  _width = 0;
  _flags = 0;  // ownership
               // 0=wire, 1=obs, 2=pin, 3=power
  _dir = 0;
  _ext = 0;
}

bool Wire::isTileBus()
{
  return _flags == 2;
}
bool Wire::isPower()
{
  const uint power_wire_id = 11;  // see db/dbSearch.cpp
  return _flags == power_wire_id;
}
bool Wire::isVia()
{
  const uint via_wire_id = 5;  // see db/dbSearch.cpp
  return _flags == via_wire_id;
}
void Wire::setOtherId(uint id)
{
  _otherId = id;
}
int Wire::getRsegId()
{
  int wBoxId = _boxId;
  if (!(_otherId > 0)) {
    return wBoxId;
  }

  if (isVia()) {
    wBoxId = getShapeProperty(_otherId);
  } else {
    getNet()->getWire()->getProperty((int) _otherId, wBoxId);
  }
  return wBoxId;
}
int Wire::getShapeProperty(int id)
{
  dbNet* net = getNet();
  if (net == nullptr) {
    return 0;
  }
  char buff[64];
  sprintf(buff, "%d", id);
  auto* p = odb::dbIntProperty::find(net, buff);
  if (p == nullptr) {
    return 0;
  }
  int rcid = p->getValue();
  return rcid;
}
dbNet* Wire::getNet()
{
  GridTable* gtb = _track->getGrid()->getGridTable();
  dbBlock* block = gtb->getBlock();
  if (_otherId == 0) {
    return (odb::dbSBox::getSBox(block, _boxId)->getSWire()->getNet());
  }
  if (gtb->usingDbSdb()) {
    return dbNet::getNet(block, _boxId);
  }
  return (odb::dbRSeg::getRSeg(block, _boxId)->getNet());
}
uint Wire::getBoxId()
{
  return _boxId;
}
uint Wire::getOtherId()
{
  return _otherId;
}
uint Wire::getSrcId()
{
  return _srcId;
}
void Wire::set(uint dir, const int* ll, const int* ur)
{
  _boxId = 0;
  _srcId = 0;
  _track = nullptr;
  _next = nullptr;

  _dir = dir;
  uint d = (_dir > 0) ? 0 : 1;

  int xy1 = ll[d];
  int xy2 = ur[d];

  int yx1 = ll[dir];
  int yx2 = ur[dir];

  _xy = xy1;  // offset from track start
  _len = xy2 - xy1;

  _base = yx1;  // small dimension
  _width = yx2 - yx1;
  // OpenRCX
  _visited = 0;
  _ouLen = 0;
}
Wire* Track::getTargetWire()
{
  return _targetWire;
}
void Track::initTargetWire(int noPowerWire)
{
  _targetWire = nullptr;
  for (_targetMarker = 0; _targetMarker < _markerCnt; _targetMarker++) {
    if (_marker[_targetMarker] == nullptr) {
      continue;
    }
    _targetWire = _marker[_targetMarker];
    while (_targetWire && noPowerWire && _targetWire->isPower()) {
      _targetWire = _targetWire->_next;
    }
    if (_targetWire) {
      break;
    }
  }
}
Wire* Track::nextTargetWire(int noPowerWire)
{
  if (_targetWire) {
    _targetWire = _targetWire->_next;
    while (_targetWire && noPowerWire && _targetWire->isPower()) {
      _targetWire = _targetWire->_next;
    }
    if (!_targetWire) {
      _targetMarker++;
    }
  }
  if (_targetWire) {
    return _targetWire;
  }
  for (; _targetMarker < _markerCnt; _targetMarker++) {
    if (_marker[_targetMarker] == nullptr) {
      continue;
    }
    _targetWire = _marker[_targetMarker];
    while (_targetWire && noPowerWire && _targetWire->isPower()) {
      _targetWire = _targetWire->_next;
    }
    if (_targetWire) {
      break;
    }
  }
  return _targetWire;
}

int Wire::wireOverlap(Wire* w, int* len1, int* len2, int* len3)
{
  // _xy, _len : reference rect

  int X1 = _xy;
  int DX = _len;
  int x1 = w->_xy;
  int dx = w->_len;

  int dx1 = X1 - x1;
  //*len1= dx1;
  if (dx1 >= 0)  // on left side
  {
    int dlen = dx - dx1;
    if (dlen <= 0) {
      return 1;
    }

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

    if (dx1 + DX <= 0) {  // outside right side
      return 2;
    }

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
void Wire::getCoords(int* x1, int* y1, int* x2, int* y2, uint* dir)
{
  if (_dir > 0)  // horizontal
  {
    *x1 = _xy;
    *y1 = _base;
    *x2 = _xy + _len;
    *y2 = _base + _width;
  } else {
    *y1 = _xy;
    *x1 = _base;
    *y2 = _xy + _len;
    *x2 = _base + _width;
  }
  *dir = _dir;
}
void Wire::getCoords(SearchBox* box)
{
  uint level = _track->getGrid()->getLevel();
  if (_dir > 0)  // horizontal
  {
    box->set(_xy, _base, _xy + _len, _base + _width, level, _dir);
  } else {
    box->set(_base, _xy, _base + _width, _xy + _len, level, _dir);
  }
  box->setType(_flags);
}

Track* Track::getNextSubTrack(Track* subt, bool tohi)
{
  if (!subt) {
    return tohi ? this : this->getLowTrack();
  }
  if (tohi) {
    return subt->getHiTrack()->_lowest ? nullptr : subt->getHiTrack();
  }
  return subt->_lowest ? nullptr : subt->getLowTrack();
}

void Track::setHiTrack(Track* hitrack)
{
  _hiTrack = hitrack;
}
void Track::setLowTrack(Track* lowtrack)
{
  _lowTrack = lowtrack;
}
Track* Track::getHiTrack()
{
  return _hiTrack;
}
Track* Track::getLowTrack()
{
  return _lowTrack;
}

void Track::set(Grid* g,
                int x,
                int y,
                uint n,
                uint width,
                uint markerLen,
                uint markerCnt,
                int base)
{
  _grid = g;
  _x = x;
  _y = y;
  _num = n;
  _width = width;

  if (markerCnt <= 4) {
    _markerCnt = markerCnt;
    _marker = new Wire*[4];
    _eMarker = new Wire*[4];
  } else {
    _markerCnt = markerCnt;
    _marker = new Wire*[_markerCnt];
    _eMarker = new Wire*[_markerCnt];
  }
  for (uint ii = 0; ii < _markerCnt; ii++) {
    _marker[ii] = nullptr;
    _eMarker[ii] = nullptr;
  }

  _blocked = 1;
  _ordered = false;

  _hiTrack = this;
  _lowTrack = this;
  _lowest = 0;
  _base = base;
}
void Track::freeWires(AthPool<Wire>* pool)
{
  for (uint ii = 0; ii < _markerCnt; ii++) {
    Wire* w = _marker[ii];
    while (w != nullptr) {
      Wire* a = w->getNext();

      pool->free(w);
      w = a;
    }
  }
}
void Track::dealloc(AthPool<Wire>* pool)
{
  freeWires(pool);
  delete[] _marker;
  delete[] _eMarker;
}

int Grid::getAbsTrackNum(const int xy)
{
  const int dist = xy - _base;

  assert(dist >= 0);

  const uint n = dist / _pitch;

  assert(n < _trackCnt);

  return n;
}
int Grid::getMinMaxTrackNum(const int xy)
{
  const int dist = xy - _base;

  if (dist <= 0) {
    return 0;
  }

  return std::min(dist / _pitch, _trackCnt - 1);
}

void Grid::initContextTracks()
{
  setSearchDomain(1);
  const bool noPowerTarget = _gridtable->noPowerTarget() > 0;
  for (uint ii = _searchLowTrack; ii <= _searchHiTrack; ii++) {
    Track* btrack = _trackTable[ii];
    if (btrack == nullptr) {
      continue;
    }
    const bool tohi = true;
    Track* track = nullptr;
    while ((track = btrack->getNextSubTrack(track, tohi))) {
      track->initTargetWire(noPowerTarget);
    }
  }
}

void Grid::initContextGrids()
{
  const uint sdepth = _gridtable->contextDepth();
  if (sdepth == 0) {
    return;
  }
  const uint i = _dir ? 0 : 1;
  for (int j = 1; j <= sdepth && (j + _level) < _gridtable->getColCnt(); j++) {
    _gridtable->getGrid(i, j + _level)->initContextTracks();
  }
  for (int j = 1; j <= sdepth && (_level - j) > 0; j++) {
    _gridtable->getGrid(i, _level - j)->initContextTracks();
  }
}

void Grid::setSearchDomain(const int domainAdjust)
{
  if (_gridtable->allNet()) {
    _searchLowTrack = 0;
    _searchHiTrack = _trackCnt - 1;
    _searchLowMarker = 0;
    _searchHiMarker = _markerCnt - 1;
    return;
  }
  const Box* searchBox = _gridtable->maxSearchBox();
  const odb::Rect rect = searchBox->getRect();
  const odb::Orientation2D dir = _dir ? odb::horizontal : odb::vertical;
  const int lo = rect.low(dir.turn_90());
  const int hi = rect.high(dir.turn_90());
  const int ltrack = getMinMaxTrackNum(lo) - domainAdjust;
  _searchLowTrack = std::max(ltrack, 0);
  _searchHiTrack
      = std::min(getMinMaxTrackNum(hi) + domainAdjust, _trackCnt - 1);
  const int mlo = rect.low(dir);
  const int mhi = rect.high(dir);
  _searchLowMarker = getBucketNum(mlo);
  _searchHiMarker = getBucketNum(mhi);
}

Track* Grid::addTrack(const uint ii, const uint markerCnt, const int base)
{
  Track* track = _trackPoolPtr->alloc();
  track->set(this, _start, _end, ii, _width, _markerLen, markerCnt, base);
  return track;
}
Track* Grid::addTrack(const uint ii, const uint markerCnt)
{
  const int trackBase = _base + _pitch * ii;
  return addTrack(ii, markerCnt, trackBase);
}
Track* Grid::getTrackPtr(const uint ii, const uint markerCnt, const int base)
{
  if (ii >= _trackCnt) {
    return nullptr;
  }

  if (_blockedTrackTable[ii] > 0) {
    return nullptr;
  }

  Track* ttrack = _trackTable[ii];
  while (ttrack) {
    if (ttrack->getBase() == base) {
      break;
    }
    Track* ntrack = ttrack->getHiTrack();
    ttrack = ntrack == _trackTable[ii] ? nullptr : ntrack;
  }
  if (ttrack) {
    return ttrack;
  }
  ttrack = addTrack(ii, markerCnt, base);
  if (_trackTable[ii] == nullptr) {
    _trackTable[ii] = ttrack;
    ttrack->setLowest(1);
    return ttrack;
  }
  _subTrackCnt[ii]++;
  Track* ntrack = _trackTable[ii];
  while (true) {
    if (ntrack->getBase() > base) {
      break;
    }
    ntrack = ntrack->getHiTrack();
    if (ntrack == _trackTable[ii]) {
      break;
    }
  }
  ntrack->getLowTrack()->setHiTrack(ttrack);
  ttrack->setHiTrack(ntrack);
  ttrack->setLowTrack(ntrack->getLowTrack());
  ntrack->setLowTrack(ttrack);
  if (base < _trackTable[ii]->getBase()) {
    _trackTable[ii]->setLowest(0);
    ttrack->setLowest(1);
    _trackTable[ii] = ttrack;
  }
  return ttrack;
}

Track* Grid::getTrackPtr(const uint ii, const uint markerCnt)
{
  const int trackBase = _base + _pitch * ii;
  return getTrackPtr(ii, markerCnt, trackBase);
}

bool Track::place(Wire* w, const int markIndex1, const int markIndex2)
{
  assert(markIndex1 >= 0);
  assert(markIndex2 >= 0);

  for (int ii = markIndex1 + 1; ii <= markIndex2; ii++) {
    _marker[ii] = w;
  }

  if (_marker[markIndex1] == nullptr) {
    _marker[markIndex1] = w;
    return true;
  }

  Wire* a = _marker[markIndex1];
  if (w->_xy < a->_xy) {
    if (w->_xy + w->_len >= a->_xy) {
      return false;
    }
    w->setNext(a);
    _marker[markIndex1] = w;
    return true;
  }

  Wire* e = _marker[markIndex1];
  for (; e != nullptr; e = e->_next) {
    if (w->_xy < e->_xy) {
      continue;
    }
    if (w->_xy + w->_len >= a->_xy) {
      return false;
    }
    w->setNext(e);
    break;
  }
  return false;
}
void Wire::search(int xy1, int xy2, uint& cnt, Ath__array1D<uint>* idTable)
{
  Wire* e = this;
  for (; e != nullptr; e = e->_next) {
    if (xy2 <= e->_xy) {
      break;
    }

    if ((xy1 <= e->_xy) && (xy2 >= e->_xy)) {
      idTable->add(e->_boxId);
      cnt++;
    } else if ((e->_xy <= xy1) && (e->_xy + e->_len >= xy1)) {
      idTable->add(e->_boxId);
      cnt++;
    }
  }
}
void Wire::search1(int xy1, int xy2, uint& cnt, Ath__array1D<uint>* idTable)
{
  Wire* e = this;
  for (; e != nullptr; e = e->_next) {
    if (xy2 <= e->_xy) {
      break;
    }

    if ((xy1 <= e->_xy) && (xy2 >= e->_xy)) {
      idTable->add(e->_id);
      cnt++;
    } else if ((e->_xy <= xy1) && (e->_xy + e->_len >= xy1)) {
      idTable->add(e->_id);
      cnt++;
    }
  }
}
uint Track::search(int xy1,
                   int xy2,
                   uint markIndex1,
                   uint markIndex2,
                   Ath__array1D<uint>* idTable)
{
  uint cnt = 0;
  if (_eMarker[markIndex1]) {
    _eMarker[markIndex1]->search(xy1, xy2, cnt, idTable);
  }
  for (uint ii = markIndex1; ii <= markIndex2; ii++) {
    if (_marker[ii] == nullptr) {
      continue;
    }
    _marker[ii]->search(xy1, xy2, cnt, idTable);
  }
  return cnt;
}
void Track::resetExtFlag(uint markerCnt)
{
  for (uint ii = 0; ii < markerCnt; ii++) {
    Wire* e = _marker[ii];
    for (; e != nullptr; e = e->_next) {
      e->_ext = 0;
    }
  }
}
uint Track::getAllWires(Ath__array1D<Wire*>* boxTable, uint markerCnt)
{
  for (uint ii = 0; ii < markerCnt; ii++) {
    Wire* e = _marker[ii];
    for (; e != nullptr; e = e->_next) {
      if (e->_ext > 0) {
        continue;
      }

      e->_ext = 1;
      boxTable->add(e);
    }
  }
  resetExtFlag(markerCnt);
  return boxTable->getCnt();
}
uint Track::search1(int xy1,
                    int xy2,
                    uint markIndex1,
                    uint markIndex2,
                    Ath__array1D<uint>* idTable)
{
  if (!_ordered) {
    markIndex1 = 0;
  }

  uint cnt = 0;
  if (_eMarker[markIndex1]) {
    _eMarker[markIndex1]->search1(xy1, xy2, cnt, idTable);
  }
  for (uint ii = markIndex1; ii <= markIndex2; ii++) {
    if (_marker[ii] == nullptr) {
      continue;
    }
    _marker[ii]->search1(xy1, xy2, cnt, idTable);
  }
  return cnt;
}
uint Track::setExtrusionMarker(int markerCnt, int start, uint markerLen)
{
  _ordered = true;

  int jj;
  int cnt = 0;
  int ii;
  for (ii = 0; ii < markerCnt; ii++) {
    _eMarker[ii] = nullptr;
  }
  for (ii = 0; ii < markerCnt - 1; ii++) {
    for (Wire* e = _marker[ii]; e != nullptr; e = e->_next) {
      int tailMark = (e->_xy + e->_len - start) / markerLen;
      if (tailMark == ii) {
        continue;
      }
      if (tailMark > markerCnt - 1) {
        tailMark = markerCnt - 1;
      }
      for (jj = ii + 1; jj <= tailMark; jj++) {
        _eMarker[jj] = e;
        if (_marker[jj]) {
          jj++;
          break;
        }
      }
      ii = jj - 2;
      cnt++;
      break;
    }
  }
  return cnt;
}
bool Track::placeTrail(Wire* w, uint m1, uint m2)
{
  for (uint ii = m1 + 1; ii <= m2; ii++) {
    if (_marker[ii] == nullptr) {
      _marker[ii] = w;
      continue;
    }
    if (w->_xy <= _marker[ii]->_xy) {
      w->setNext(_marker[ii]);
      _marker[ii] = w;
    } else {
      w->setNext(_marker[ii]->_next);
      _marker[ii]->setNext(w);
    }
  }
  return true;
}
bool Track::checkAndplacerOnMarker(Wire* w, int markIndex)
{
  if (_marker[markIndex] == nullptr) {
    _marker[markIndex] = w;
    return true;
  }
  return false;
}
bool Track::checkMarker(int markIndex)
{
  if (_marker[markIndex] == nullptr) {
    return true;
  }
  return false;
}
bool Track::checkAndplace(Wire* w, int markIndex1)
{
  if (_marker[markIndex1] == nullptr) {
    _marker[markIndex1] = w;
    return true;
  }

  Wire* a = _marker[markIndex1];
  if (w->_xy <= a->_xy) {
    if (w->_xy + w->_len > a->_xy) {
      return false;
    }

    w->setNext(a);
    _marker[markIndex1] = w;

    return true;
  }
  Wire* prev = _marker[markIndex1];
  Wire* e = _marker[markIndex1];
  for (; e != nullptr; e = e->_next) {
    if (w->_xy <= e->_xy) {
      if (w->_xy + w->_len > e->_xy) {
        return false;
      }

      w->setNext(e);
      prev->setNext(w);
      return true;
    }
    prev = e;
  }

  if (prev->_xy + prev->_len > w->_xy) {
    return false;
  }
  prev->setNext(w);
  return true;
}
void Track::insertWire(Wire* w, int mark1, int mark2)
{
  w->_track = this;
  for (int ii = mark1; ii < mark2; ii++) {
    _marker[ii] = w;
  }
  if (mark2 > mark1) {
    w->setNext(_marker[mark2]);
    _marker[mark2] = w;
  }
}

bool Track::place2(Wire* w, int mark1, int mark2)
{
  assert(mark1 >= 0);

  w->_next = nullptr;
  if (_marker[mark1] == nullptr) {
    insertWire(w, mark1, mark2);
    return true;
  }
  bool status = true;

  Wire* a = _marker[mark1];
  if (w->_xy <= a->_xy) {
    w->setNext(a);
    _marker[mark1] = w;

    w->_track = this;

    return true;
  }
  Wire* prev = _marker[mark1];
  Wire* e = _marker[mark1];
  for (; e != nullptr; e = e->_next) {
    if (w->_xy <= e->_xy) {
      w->setNext(e);
      prev->setNext(w);

      w->_track = this;
      return true;
    }
    prev = e;
  }
  if (e == nullptr) {  // at the end of the list
    prev->setNext(w);
    insertWire(w, mark1, mark2);
    return true;
  }

  if (!status) {
    fprintf(stdout, "OVERLAP placement\n");
  }

  return status;
}

void Track::linkWire(Wire*& w1, Wire*& w2)
{
  OverlapAdjust adj = (OverlapAdjust) _grid->getGridTable()->getOverlapAdjust();
  int nend, oend, tend;
  nend = w1->_xy + w1->_len;
  oend = w2->_xy + w2->_len;
  tend = nend > oend ? nend : oend;
  if (adj == Z_noAdjust || nend <= w2->_xy) {
    w1->setNext(w2);
  } else if (w1->_base != w2->_base || w1->_width != w2->_width) {
    if (!_grid->getGridTable()->getOverlapTouchCheck()
        || w1->_base > w2->_base + w2->_width
        || w2->_base > w1->_base + w1->_width) {
      w1->setNext(w2);
    } else {                        // only good for adj == Z_endAdjust
      if (w1->_base < w2->_base) {  // w1 is wider?
        w2->_xy = nend;
        w1->setNext(w2);
        if (nend >= oend) {
          w2->_len = 0;
        } else {
          w2->_len = oend - nend;
        }
      } else {  // todo: nend > oend
        w1->setNext(w2);
        w1->_len = w2->_xy - w1->_xy;
      }
    }
  } else if (adj == Z_merge) {
    w1->_len = tend - w1->_xy;
    w1->setNext(w2->_next);
    w2 = w1;
  } else {  // adj == Z_endAdjust
    w2->_xy = nend;
    w1->setNext(w2);
    if (nend >= oend) {
      w2->_len = 0;
    } else {
      w2->_len = oend - nend;
    }
  }
}

bool Track::place(Wire* w, int markIndex1)
{
  assert(markIndex1 >= 0);

  w->_track = this;

  if (_marker[markIndex1] == nullptr) {
    _marker[markIndex1] = w;
    return true;
  }
  OverlapAdjust adj = (OverlapAdjust) _grid->getGridTable()->getOverlapAdjust();
  bool status = true;

  Wire* a = _marker[markIndex1];
  if (w->_xy <= a->_xy) {
    if (adj != Z_noAdjust && w->_xy + w->_len > a->_xy + a->_len
        && w->_base <= a->_base
        && w->_base + w->_width >= a->_base + a->_width) {  // a inside w
      w->_next = _marker[markIndex1]->_next;
      _marker[markIndex1] = w;
      return true;
    }
    linkWire(w, a);
    _marker[markIndex1] = w;

    return true;
  }
  Wire* prev = _marker[markIndex1];
  Wire* e = _marker[markIndex1];
  for (; e != nullptr; e = e->_next) {
    if (w->_xy <= e->_xy) {
      if (adj != Z_noAdjust && w->_xy + w->_len >= e->_xy + e->_len
          && w->_base <= e->_base
          && w->_base + w->_width >= e->_base + e->_width) {  // e inside w
        prev->_next = w;
        w->_next = e->_next;
        return true;
      }
      linkWire(prev, w);
      linkWire(w, e);
      return true;
    }
    if (adj != Z_noAdjust && w->_xy + w->_len <= e->_xy + e->_len
        && e->_base <= w->_base
        && e->_base + e->_width >= w->_base + w->_width) {  // w inside e
      return true;
    }
    prev = e;
  }
  if (e == nullptr) {
    linkWire(prev, w);
  }

  if (!status) {
    fprintf(stdout, "OVERLAP placement\n");
  }

  return status;
}
Wire* Track::getNextWire(Wire* wire)
{
  Wire* nwire;
  if (!wire) {
    _searchMarkerIndex = _grid->searchLowMarker();
    nwire = _marker[_searchMarkerIndex];
  } else {
    nwire = wire->_next;
  }
  if (nwire) {
    return nwire;
  }
  for (_searchMarkerIndex++; _searchMarkerIndex <= _grid->searchHiMarker();
       _searchMarkerIndex++) {
    nwire = _marker[_searchMarkerIndex];
    if (nwire) {
      return nwire;
    }
  }
  return nullptr;
}

Wire* Track::getWire_Linear(uint markerCnt, uint id)
{
  for (uint ii = 0; ii < markerCnt; ii++) {
    Wire* e = _marker[ii];
    for (; e != nullptr; e = e->_next) {
      if (e->_id == id) {
        return e;
      }
    }
  }
  return nullptr;
}
void Track::adjustOverlapMakerEnd(uint markerCnt, int start, uint markerLen)
{
  _ordered = true;

  Wire* e;
  uint tailMark;
  uint jj;
  for (uint ii = 0; ii < markerCnt - 1; ii++) {
    e = _marker[ii];
    if (e == nullptr) {
      continue;
    }
    for (; e->_next != nullptr; e = e->_next) {
      ;
    }
    tailMark = (e->_xy + e->_len - start) / markerLen;
    if (tailMark == ii) {
      continue;
    }
    if (tailMark > markerCnt - 1) {
      tailMark = markerCnt - 1;
    }
    for (jj = ii + 1; jj <= tailMark; jj++) {
      _eMarker[jj] = e;
      if (_marker[jj]) {
        jj++;
        break;
      }
    }
    jj--;
    if (_marker[jj] != nullptr && e->_xy + e->_len > _marker[jj]->_xy) {
      e->_len = _marker[jj]->_xy - e->_xy;
    }
    ii = jj - 1;
  }
}
void Track::adjustOverlapMakerEnd(uint markerCnt)
{
  Wire* e;
  uint jj;
  for (uint ii = 0; ii < markerCnt - 1; ii++) {
    e = _marker[ii];
    if (e == nullptr) {
      continue;
    }
    for (; e->_next != nullptr; e = e->_next) {
      ;
    }
    for (jj = ii + 1; jj < markerCnt && _marker[jj] == nullptr; jj++) {
      ;
    }
    if (jj == markerCnt) {
      continue;
    }
    if (e->_xy + e->_len > _marker[jj]->_xy) {
      e->_len = _marker[jj]->_xy - e->_xy;
    }
    ii = jj - 1;
  }
}

bool Track::isAscendingOrdered(uint markerCnt, uint* wCnt)
{
  uint cnt = 0;
  for (uint ii = 0; ii < markerCnt; ii++) {
    Wire* e = _marker[ii];
    for (; e != nullptr; e = e->_next) {
      Wire* w = e->_next;
      cnt++;

      if (w == nullptr) {
        break;
      }

      if (w->_xy < e->_xy) {
        *wCnt = cnt;
        return false;
      }
    }
  }
  *wCnt += cnt;
  return true;
}
bool Track::overlapCheck(Wire* w, int markIndex1, int /* unused: markIndex2 */)
{
  assert(markIndex1 >= 0);

  Wire* e = _marker[markIndex1];
  for (; e != nullptr; e = e->_next) {
    if (w->_xy + w->_len >= e->_xy) {
      return true;
    }
  }
  return false;
}
void Grid::makeTrackTable(uint width, uint pitch, uint space)
{
  if (width > 0) {
    _width = width;
  }

  if (space > 0) {
    _pitch = _width + space;
  } else if (pitch > 0) {
    _pitch = pitch;
  }

  _markerLen = ((_end - _start) / _width) / _markerCnt;

  _trackCnt = _max;  // for LINUX assert
  _trackCnt = getAbsTrackNum(_max) + 1;
  _subTrackCnt = (uint*) calloc(sizeof(uint), _trackCnt);
  _trackTable = new Track*[_trackCnt];
  _blockedTrackTable = new uint[_trackCnt];

  for (uint ii = 0; ii < _trackCnt; ii++) {
    _trackTable[ii] = nullptr;
    _blockedTrackTable[ii] = 0;
  }
}

void Grid::setBoundaries(uint dir, const odb::Rect& rect)
{
  _lo[0] = rect.xMin();
  _lo[1] = rect.yMin();
  _hi[0] = rect.xMax();
  _hi[1] = rect.yMax();

  _dir = dir;
  if (_dir == 0) {  // vertical
    _base = rect.xMin();
    _max = rect.xMax();
    _start = rect.yMin();
    _end = rect.yMax();
  } else {
    _base = rect.yMin();
    _max = rect.yMax();
    _start = rect.xMin();
    _end = rect.xMax();
  }
}
Grid::Grid(GridTable* gt,
           AthPool<Track>* trackPool,
           AthPool<Wire>* wirePool,
           uint level,
           uint markerCnt)
{
  _gridtable = gt;
  _trackPoolPtr = trackPool;
  _wirePoolPtr = wirePool;
  _markerCnt = markerCnt;
  _level = level;
}

void Grid::setTracks(uint dir,
                     uint width,
                     uint pitch,
                     int xlo,
                     int ylo,
                     int xhi,
                     int yhi,
                     uint markerLen)
{
  setBoundaries(dir, {xlo, ylo, xhi, yhi});
  makeTrackTable(width, pitch);

  if (markerLen > 0) {
    _markerLen = markerLen;
    _markerCnt = ((_end - _start) / _width) / markerLen;
    if (_markerCnt == 0) {
      _markerCnt = 1;
    }
  }
}
void Grid::getBbox(SearchBox* bb)
{
  if (_dir == 0) {  // vertical
    bb->set(_base, _start, _max, _end, _level, _dir);
  } else {
    bb->set(_start, _base, _end, _max, _level, _dir);
  }
}
void Grid::getBbox(Box* bb)
{
  if (_dir == 0) {  // vertical
    bb->setRect({_base, _start, _max, _end});
  } else {
    bb->setRect({_start, _base, _end, _max});
  }
}

void Grid::freeTracksAndTables()
{
  delete[] _trackTable;
  delete[] _blockedTrackTable;
}
Grid::~Grid()
{
  freeTracksAndTables();
}
uint Grid::getLevel()
{
  return _level;
}
uint Grid::getDir()
{
  return _dir;
}
int Grid::getTrackHeight(uint track)
{
  return _base + track * _pitch;
}
Grid* Track::getGrid()
{
  return _grid;
}
uint Track::removeMarkedNetWires()
{
  uint cnt = 0;
  Wire *wire, *pwire, *nwire;
  for (uint jj = 0; jj < _markerCnt; jj++) {
    pwire = nullptr;
    wire = _marker[jj];
    _marker[jj] = nullptr;
    for (; wire != nullptr; wire = nwire) {
      nwire = wire->_next;
      if (wire->isPower() || !wire->getNet()->isMarked()) {
        if (wire->isPower() || wire->getNet()->getWire() != nullptr) {
          pwire = wire;
          if (_marker[jj] == nullptr) {
            _marker[jj] = wire;
          }
          continue;
        }
      }
      if (pwire) {
        pwire->_next = nwire;
      }
      for (uint kk = jj + 1; kk < _markerCnt; kk++) {
        if (_eMarker[kk] == wire) {
          _eMarker[kk] = nwire;
        }
      }
      _grid->getWirePoolPtr()->free(wire);
      cnt++;
    }
  }
  return cnt;
}
uint Grid::defaultWireType()
{
  return _wireType;
}
void Grid::setDefaultWireType(uint v)
{
  _wireType = v;  // TODO-OPTIMIZE : can it be 32-bit?
}
uint Grid::getBoxes(uint trackIndex, Ath__array1D<uint>* table)
{
  Track* tr = _trackTable[trackIndex];
  if (tr == nullptr) {
    return 0;
  }
  if (_blockedTrackTable[trackIndex] > 0) {
    return 0;
  }

  for (uint k = 0; k < tr->_markerCnt; k++) {
    if (tr->_marker[k] == nullptr) {
      continue;
    }
    for (Wire* w = tr->_marker[k]; w != nullptr; w = w->_next) {
      table->add(w->_boxId);
    }
  }
  return table->getCnt();
}
void Grid::getBoxes(Ath__array1D<uint>* table)
{
  for (uint ii = 0; ii < _trackCnt; ii++) {
    Track* tr = _trackTable[ii];
    if (tr == nullptr) {
      continue;
    }
    if (_blockedTrackTable[ii] > 0) {
      continue;
    }

    for (Wire* w = tr->_marker[0]; w != nullptr; w = w->_next) {
      table->add(w->_boxId);
    }
  }
}

bool Grid::addOnTrack(uint track, Wire* w, uint mark1, uint mark2)
{
  if (_blockedTrackTable[track] > 0) {
    return false;
  }

  Track* tr = nullptr;
  if (_trackTable[track] == nullptr) {
    tr = addTrack(track, _markerCnt);
    _trackTable[track] = tr;
    if (tr->place(w, mark1, mark2)) {
      w->_track = tr;
      return true;
    }
    return false;
  }
  if (!_trackTable[track]->overlapCheck(w, mark1, mark2)) {
    tr = _trackTable[track];
    if (tr->place(w, mark1, mark2)) {
      w->_track = tr;
      return true;
    }
    return false;
  }
  return false;
}
uint Grid::placeWire(uint initTrack,
                     Wire* w,
                     uint mark1,
                     uint mark2,
                     int sortedOrder,
                     int* height)
{
  uint check = 20;
  uint track = initTrack;

  uint nextTrack = track;

  bool status = false;
  if (sortedOrder > 0) {
    for (; (track < _trackCnt) && (track < initTrack + check); track++) {
      status = addOnTrack(track, w, mark1, mark2);
      if (status) {
        break;
      }
    }
    nextTrack = track + 1;
  } else {
    for (; (!status) && (track > 0) && (track > initTrack - check); track--) {
      status = addOnTrack(track, w, mark1, mark2);
      if (status) {
        break;
      }
    }
    nextTrack = track - 1;
  }
  if (!status) {
    fprintf(stdout, "Cannot place at track# %d\n", initTrack);
    *height = getTrackHeight(initTrack);
    return initTrack;
  }
  *height = getTrackHeight(track);
  return nextTrack;
}

uint Grid::addWire(uint initTrack, Box* box, int sortedOrder, int* height)
{
  uint id, markIndex1, markIndex2;
  Wire* w = makeWire(box, &id, &markIndex1, &markIndex2, 0);

  return placeWire(initTrack, w, markIndex1, markIndex2, sortedOrder, height);
}
Track* Grid::getTrackPtr(int xy)
{
  uint trackNum = getMinMaxTrackNum(xy);

  return getTrackPtr(trackNum, _markerCnt);
}
Track* Grid::getTrackPtr(int* ll)
{
  uint trackNum = getMinMaxTrackNum(ll[_dir]);

  return getTrackPtr(trackNum, _markerCnt);
}
uint Grid::placeBox(uint id, int x1, int y1, int x2, int y2)
{
  int ll[2] = {x1, y1};
  int ur[2] = {x2, y2};

  uint d = (_dir > 0) ? 0 : 1;

  int xy1 = ll[d];

  uint m1 = getBucketNum(xy1);
  int width = ur[_dir] - ll[_dir];

  uint trackNum1 = getMinMaxTrackNum(ll[_dir]);
  uint trackNum2 = trackNum1;
  if (width > _pitch) {
    trackNum2 = getMinMaxTrackNum(ur[_dir]);
  }

  for (uint ii = trackNum1; ii <= trackNum2; ii++) {
    Wire* w = makeWire(_dir, ll, ur, id, 0);

    Track* track = getTrackPtr(ii, _markerCnt);

    if (track->place(w, m1)) {
      w->_track = track;
    } else {
      fprintf(stdout, "OVERLAP placement\n");
    }
  }

  return trackNum1;
}
void Wire::setXY(int xy1, uint len)
{
  _xy = xy1;  // offset from track start??
  _len = len;
}
Wire* Wire::makeCoupleWire(AthPool<Wire>* wirePool,
                           int targetHighTracks,
                           Wire* w2,
                           int xy1,
                           uint len,
                           uint /* unused: wtype */)
{
  int dist;
  if (targetHighTracks) {
    dist = w2->_base - (_base + _width);
  } else {
    dist = _base - (w2->_base + w2->_width);
  }
  if (dist <= 0) {
    return nullptr;
  }

  Wire* w = getPoolWire(wirePool);
  w->_srcId = 0;

  w->reset();

  w->_xy = xy1;  // offset from track start??
  w->_len = len;

  w->_width = dist;
  w->_boxId = _id;
  w->_otherId = w2->_id;
  w->_flags = _flags;
  w->_dir = _dir;
  if (targetHighTracks) {
    w->_base = _base + _width;  // small dimension
  } else {
    w->_base = w2->_base + w2->_width;
  }
  return w;
}
Wire* Wire::getPoolWire(AthPool<Wire>* wirePool)
{
  int n;
  int getRecycleFlag = 0;
  Wire* w = wirePool->alloc(&getRecycleFlag, &n);
  if (getRecycleFlag == 0) {
    w->_id = n;
  }
  return w;
}
Wire* Wire::makeWire(AthPool<Wire>* wirePool, int xy1, uint len)
{
  Wire* w = getPoolWire(wirePool);

  w->_srcId = 0;

  w->reset();

  w->_xy = xy1;  // offset from track start
  w->_len = len;

  w->_base = _base;  // small dimension
  w->_width = _width;

  w->_boxId = _boxId;
  w->_otherId = _otherId;

  w->_flags = _flags;
  w->_dir = _dir;

  return w;
}
Wire* Grid::getPoolWire()
{
  int n;
  int getRecycleFlag = 0;
  Wire* w = _wirePoolPtr->alloc(&getRecycleFlag, &n);
  if (getRecycleFlag == 0) {
    w->_id = n;
  }
  return w;
}
Wire* Grid::makeWire(Wire* v, uint type)
{
  Wire* w = getPoolWire();
  w->_srcId = 0;

  w->reset();

  w->_xy = v->_xy;  // offset from track start
  w->_len = v->_len;

  w->_base = v->_base;  // small dimension
  w->_width = v->_width;

  w->_boxId = v->_boxId;
  w->_otherId = v->_otherId;

  w->_flags = type;

  w->_dir = v->_dir;

  return w;
}

uint Grid::placeWire(SearchBox* bb)
{
  uint d = !_dir;

  int xy1 = bb->loXY(d);

  int ll[2] = {bb->loXY(0), bb->loXY(1)};
  int ur[2] = {bb->hiXY(0), bb->hiXY(1)};

  uint m1 = getBucketNum(xy1);

  uint trackNum1 = getMinMaxTrackNum(bb->loXY(_dir));
  uint trackNum2 = getMinMaxTrackNum(bb->hiXY(_dir));

  uint wireType = bb->getType();

  Wire* w
      = makeWire(_dir, ll, ur, bb->getOwnerId(), bb->getOtherId(), wireType);
  Track* track;
  int TTTsubt = 1;
  if (TTTsubt) {
    track = getTrackPtr(trackNum1, _markerCnt, w->_base);
  } else {
    track = getTrackPtr(trackNum1, _markerCnt);
  }
  // track->place2(w, m1, m2);
  track->place(w, m1);
  for (uint ii = trackNum1 + 1; ii <= trackNum2; ii++) {
    Wire* w1 = makeWire(w, wireType);
    w1->_srcId = w->_id;
    w1->_srcWire = w;
    _gridtable->incrMultiTrackWireCnt(w->isPower());
    Track* track = getTrackPtr(ii, _markerCnt);
    track->place(w1, m1);
  }

  return trackNum1;
}
uint Grid::placeWire(Wire* w)
{
  uint m1 = getBucketNum(w->_xy);

  uint trackNum1 = getMinMaxTrackNum(w->_base);
  uint trackNum2 = getMinMaxTrackNum(w->_base + w->_width);

  Track* track = getTrackPtr(trackNum1, _markerCnt);
  track->place(w, m1);

  for (uint ii = trackNum1 + 1; ii <= trackNum2; ii++) {
    Wire* w1 = makeWire(w, w->_flags);
    w1->_srcId = w->_id;
    _gridtable->incrMultiTrackWireCnt(w->isPower());
    Track* track = getTrackPtr(ii, _markerCnt);
    track->place(w1, m1);
  }

  return trackNum1;
}
uint Grid::placeBox(dbBox* box, uint wtype, uint id)
{
  int ll[2] = {box->xMin(), box->yMin()};
  int ur[2] = {box->xMax(), box->yMax()};

  uint d = (_dir > 0) ? 0 : 1;

  int xy1 = ll[d];

  uint m1 = getBucketNum(xy1);

  int width = ur[_dir] - ll[_dir];

  uint trackNum1 = getMinMaxTrackNum(ll[_dir]);
  uint trackNum2 = trackNum1;
  if (width > _pitch) {
    trackNum2 = getMinMaxTrackNum(ur[_dir]);
  }

  if (id == 0) {
    id = box->getId();
  }
  Wire* w = makeWire(_dir, ll, ur, id, 0, wtype);
  Track* track = getTrackPtr(trackNum1, _markerCnt);
  track->place(w, m1);

  for (uint ii = trackNum1 + 1; ii <= trackNum2; ii++) {
    Wire* w1 = makeWire(w);
    w1->_srcId = w->_id;
    _gridtable->incrMultiTrackWireCnt(w->isPower());
    Track* track = getTrackPtr(ii, _markerCnt);
    track->place(w1, m1);
  }
  return trackNum1;
}
uint Grid::setExtrusionMarker()
{
  Track *track, *tstr;
  uint cnt = 0;
  for (uint ii = 0; ii < _trackCnt; ii++) {
    track = _trackTable[ii];
    if (track == nullptr) {
      continue;
    }
    tstr = nullptr;
    bool tohi = true;
    while ((tstr = track->getNextSubTrack(tstr, tohi)) != nullptr) {
      cnt += tstr->setExtrusionMarker(_markerCnt, _start, _markerLen);
    }
  }
  return cnt;
}
uint Grid::placeBox(Box* box)
{
  const odb::Rect rect = box->getRect();
  int ll[2] = {rect.xMin(), rect.yMin()};
  int ur[2] = {rect.xMax(), rect.yMax()};

  uint markIndex1;
  Wire* w = makeWire(ll, ur, box->getOwner(), &markIndex1);

  Track* track = getTrackPtr(ll);

  if (!track->place(w, markIndex1)) {
    fprintf(stdout, "OVERLAP placement\n");
  } else {
    w->_track = track;
  }

  return track->_num;
}
Wire* Grid::getWirePtr(uint wireId)
{
  return _wirePoolPtr->get(wireId);
}
void Grid::getBoxIds(Ath__array1D<uint>* wireIdTable,
                     Ath__array1D<uint>* idtable)
{
  // remove duplicate entries

  for (uint ii = 0; ii < wireIdTable->getCnt(); ii++) {
    uint wid = wireIdTable->get(ii);
    Wire* w = getWirePtr(wid);

    uint boxId = w->_boxId;
    if (w->_srcId > 0) {
      w = getWirePtr(w->_srcId);
      boxId = w->_boxId;
    }
    if (w->_ext > 0) {
      continue;
    }

    w->_ext = 1;
    idtable->add(boxId);
  }

  for (uint jj = 0; jj < wireIdTable->getCnt(); jj++) {
    Wire* w = getWirePtr(wireIdTable->get(jj));
    w->_ext = 0;

    if (w->_srcId > 0) {
      w = getWirePtr(w->_srcId);
      w->_ext = 0;
    }
  }
}
void Grid::getWireIds(Ath__array1D<uint>* wireIdTable,
                      Ath__array1D<uint>* idtable)
{
  // remove duplicate entries

  for (uint ii = 0; ii < wireIdTable->getCnt(); ii++) {
    uint wid = wireIdTable->get(ii);
    Wire* w = getWirePtr(wid);

    if (w->_srcId > 0) {
      w = getWirePtr(w->_srcId);
      wid = w->_id;
    }
    if (w->_ext > 0) {
      continue;
    }

    w->_ext = 1;
    idtable->add(wid);
  }

  for (uint jj = 0; jj < wireIdTable->getCnt(); jj++) {
    Wire* w = getWirePtr(wireIdTable->get(jj));
    w->_ext = 0;
    if (w->_srcId > 0) {
      w = getWirePtr(w->_srcId);
      w->_ext = 0;
    }
  }
}
uint Grid::search(SearchBox* bb, Ath__array1D<uint>* idtable, bool wireIdFlag)
{
  Ath__array1D<uint> wireIdTable(16000);

  // uint d= (_dir>0) ? 0 : 1;
  uint d = !_dir;

  uint loTrackNum = getTrackNum1(bb->loXY(_dir));
  if (loTrackNum > 0) {
    loTrackNum--;
  }

  uint hiTrackNum = getTrackNum1(bb->hiXY(_dir));

  int loXY = bb->loXY(d);
  int hiXY = bb->hiXY(d);
  uint loMarker = getBucketNum(loXY);
  uint hiMarker = getBucketNum(hiXY);

  Track* tstrack;
  for (uint ii = loTrackNum; ii <= hiTrackNum; ii++) {
    Track* track = _trackTable[ii];
    if (track == nullptr) {
      continue;
    }

    tstrack = nullptr;
    bool tohi = true;
    while ((tstrack = track->getNextSubTrack(tstrack, tohi)) != nullptr) {
      tstrack->search1(loXY, hiXY, loMarker, hiMarker, &wireIdTable);
    }
  }
  if (wireIdFlag) {
    getWireIds(&wireIdTable, idtable);
  } else {
    getBoxIds(&wireIdTable, idtable);
  }

  return idtable->getCnt();
}
uint Grid::search(SearchBox* bb,
                  const uint* gxy,
                  Ath__array1D<uint>* idtable,
                  Grid* g)
{
  Ath__array1D<uint> wireIdTable(1024);

  AthPool<Wire>* wirePool = _wirePoolPtr;
  if (g != nullptr) {
    wirePool = g->getWirePoolPtr();
  }

  uint d = !_dir;

  uint loTrackNum = getTrackNum1(bb->loXY(_dir));
  if (loTrackNum > 0) {
    loTrackNum--;
  }

  uint hiTrackNum = getTrackNum1(bb->hiXY(_dir));

  int loXY = bb->loXY(d);
  int hiXY = bb->hiXY(d);
  uint loMarker = getBucketNum(loXY);
  uint hiMarker = getBucketNum(hiXY);

  for (uint ii = loTrackNum; ii <= hiTrackNum; ii++) {
    Track* track = _trackTable[ii];
    if (track == nullptr) {
      continue;
    }

    wireIdTable.resetCnt();
    uint cnt1 = track->search1(loXY, hiXY, loMarker, hiMarker, &wireIdTable);
    if (cnt1 <= 0) {
      continue;
    }

    Wire* w0 = _wirePoolPtr->get(wireIdTable.get(0));
    Wire* w1 = w0->makeWire(wirePool, w0->_xy, w0->_len);

    if (g != nullptr) {
      g->placeWire(w1);
    }
    idtable->add(w1->_id);

    for (uint jj = 1; jj < cnt1; jj++) {
      Wire* w = _wirePoolPtr->get(wireIdTable.get(jj));

      uint dist = w->_xy - (w1->_xy + w1->_len);
      if (dist <= gxy[d]) {
        w1->setXY(w1->_xy, w->_xy + w->_len - w1->_xy);
      } else  // start new
      {
        w1 = w0->makeWire(wirePool, w->_xy, w->_len);

        if (g != nullptr) {
          g->placeWire(w1);
        }
        idtable->add(w1->_id);
      }
    }
  }

  return idtable->getCnt();
}
void Grid::getBuses(Ath__array1D<Box*>* boxTable, uint width)
{
  Ath__array1D<Wire*> wireTable(32);

  for (uint ii = 0; ii < _trackCnt; ii++) {
    if (_blockedTrackTable[ii] > 0) {
      continue;
    }

    Track* track = _trackTable[ii];
    if (track == nullptr) {
      continue;
    }

    uint height = _base + ii * _pitch;

    wireTable.resetCnt();
    track->getAllWires(&wireTable, _markerCnt);

    for (uint jj = 0; jj < wireTable.getCnt(); jj++) {
      Wire* e = wireTable.get(jj);
      if (!e->isTileBus()) {
        continue;
      }

      Box* bb = new Box();
      if (_dir > 0) {
        bb->set(e->_xy, height, e->_xy + e->_len, height + width);
      } else {
        bb->set(height, e->_xy, height + width, e->_xy + e->_len);
      }

      bb->setLayer(_level);

      boxTable->add(bb);
    }
  }
}
Wire* Grid::getWire_Linear(uint id)
{
  for (uint ii = 0; ii < _trackCnt; ii++) {
    Track* tr = _trackTable[ii];
    if (tr == nullptr) {
      continue;
    }

    Wire* w = tr->getWire_Linear(_markerCnt, id);
    if (w != nullptr) {
      return w;
    }
  }
  return nullptr;
}
void Grid::adjustOverlapMakerEnd()
{
  int TTTnewAdj = 1;
  Track *track, *tstr;
  for (uint ii = 0; ii < _trackCnt; ii++) {
    track = _trackTable[ii];
    if (track == nullptr) {
      continue;
    }
    tstr = nullptr;
    bool tohi = true;
    while ((tstr = track->getNextSubTrack(tstr, tohi)) != nullptr) {
      if (TTTnewAdj) {
        tstr->adjustOverlapMakerEnd(_markerCnt, _start, _markerLen);
      } else {
        tstr->adjustOverlapMakerEnd(_markerCnt);
      }
    }
  }
}

bool Grid::isOrdered(bool /* unused: ascending */, uint* cnt)
{
  bool ordered = true;
  for (uint ii = 0; ii < _trackCnt; ii++) {
    Track* tr = _trackTable[ii];
    if (tr == nullptr) {
      continue;
    }

    if (!tr->isAscendingOrdered(_markerCnt, cnt)) {
      fprintf(stdout, "Track #%d is not ordered\n", ii);
      ordered = false;
    }
  }
  return ordered;
}
uint Grid::getBucketNum(int xy)
{
  int offset = xy - _start;
  if (offset < 0) {
    return 0;
  }
  uint b = offset / _markerLen;
  if (b == 0) {
    return 0;
  }

  if (b >= _markerCnt) {
    return _markerCnt - 1;
  }
  return b;
}
uint Grid::getWidth()
{
  return _width;
}

int Grid::getXYbyWidth(int xy, uint* mark)
{
  int offset = xy - _start;
  if (offset < 0) {
    *mark = 0;
    return 0;
  }
  uint a = offset / _width;
  int b = a / _markerLen;
  if (b > 3) {
    *mark = 3;
  } else {
    *mark = b;
  }
  return a;
}
uint Grid::getTrackNum1(int xy)
{
  int a = xy - _base;

  // if (a<0)
  if (xy < _base) {
    return 0;
  }

  uint b = a / _pitch;
  if (b >= _trackCnt) {
    return _trackCnt - 1;
  }
  return b;
}
uint Grid::getTrackNum(int* ll, uint d, uint* marker)
{
  *marker = getBucketNum(ll[d]);

  int a = ll[_dir] - _base;

  if (a < 0) {
    return 0;
  }

  uint b = a / _pitch;
  if (b >= _trackCnt) {
    return _trackCnt - 1;
  }
  return b;
}
uint Grid::getTrackNum(Box* box)
{
  const odb::Rect rect = box->getRect();
  int ll[2] = {rect.xMin(), rect.yMin()};

  int a = ll[_dir] - _base;

  if (a < 0) {
    return 0;
  }
  return a / _pitch;
}
Wire* Grid::makeWire(uint dir, int* ll, int* ur, uint id1, uint id2, uint type)
{
  Wire* w = getPoolWire();
  w->_srcId = 0;
  w->_srcWire = nullptr;

  w->reset();
  w->set(dir, ll, ur);
  w->_boxId = id1;
  w->_otherId = id2;

  w->_flags = type;

  return w;
}
Wire* Grid::makeWire(int* ll, int* ur, uint id, uint* m1)
{
  uint d = (_dir > 0) ? 0 : 1;

  int xy1 = ll[d];
  // int xy2= ur[d];
  *m1 = getBucketNum(xy1);

  Wire* w = getPoolWire();
  w->_srcId = 0;
  w->_otherId = 0;

  w->reset();
  w->set(_dir, ll, ur);
  w->_boxId = id;

  return w;
}
Wire* Grid::makeWire(Box* box,
                     uint* id,
                     uint* m1,
                     uint* m2,
                     uint /* unused: fullTrack */)
{
  const odb::Rect rect = box->getRect();
  int ll[2] = {rect.xMin(), rect.yMin()};
  int ur[2] = {rect.xMax(), rect.yMax()};

  *m1 = 0;
  *m2 = 3;
  Wire* w = getPoolWire();
  w->_otherId = 0;

  *id = w->_id;
  w->reset();
  w->set(_dir, ll, ur);
  w->_boxId = box->getId();
  w->_srcId = 0;
  return w;
}

uint Grid::getFirstTrack(uint divider)
{
  int xy = _base + (_max - _base) / divider;

  return getAbsTrackNum(xy);
}
int Grid::getClosestTrackCoord(int xy)
{
  int track1 = getAbsTrackNum(xy);
  int ii;
  for (ii = track1 - 1; ii < _trackCnt; ii++) {
    if (_trackTable[ii] != nullptr) {
      break;
    }
  }
  int h1 = _max;
  if (ii < _trackCnt) {
    h1 = getTrackHeight(ii);
  }

  for (ii = track1 + 1; ii >= 0; ii--) {
    if (_trackTable[ii] != nullptr) {
      break;
    }
  }
  int h2 = _base;
  if (ii > 0) {
    h2 = getTrackHeight(ii);
  }

  if (xy - h2 < h1 - xy) {
    return h2 + _width / 2;
  }
  return h1 + _width / 2;
}
int Grid::findEmptyTrack(int ll[2], int ur[2])
{
  uint track1 = getAbsTrackNum(ll[_dir]);
  uint track2 = getAbsTrackNum(ur[_dir]);
  uint cnt = 0;
  for (uint ii = track1; ii <= track2; ii++) {
    if (_trackTable[ii] == nullptr) {
      cnt++;
      continue;
    }
    Wire w;
    w.reset();

    int xy1 = (ll[_dir % 1] - _start) / _width;
    int xy2 = (ur[_dir % 1] - _start) / _width;

    w.set(_dir, ll, ur);

    int markIndex1 = xy1 / _markerLen;
    int markIndex2 = xy2 / _markerLen;

    if (_trackTable[ii]->overlapCheck(&w, markIndex1, markIndex2)) {
      continue;
    }
    cnt++;
  }
  if (cnt == track2 - track1 + 1) {
    return track1;
  }

  return -1;
}

void GridTable::releaseWire(uint wireId)
{
  Wire* w = _wirePool->get(wireId);
  _wirePool->free(w);
}
Wire* GridTable::getWirePtr(uint id)
{
  return _wirePool->get(id);
}
uint GridTable::getRowCnt()
{
  return _rowCnt;
}
uint GridTable::getColCnt()
{
  return _colCnt;
}
void GridTable::dumpTrackCounts(FILE* fp)
{
  fprintf(fp, "Multiple_track_power_wires : %d\n", _powerMultiTrackWire);
  fprintf(fp, "Multiple_track_signal_wires : %d\n", _signalMultiTrackWire);
  fprintf(fp,
          "layer  dir   alloc    live offbase  expand  tsubtn   toptk  stn\n");
  Grid* tgrid;
  uint topBigTrack = 0;
  uint topSubtNum;
  uint totalSubtNum;
  uint expTrackNum;
  int trn;
  uint offbase;
  uint liveCnt;
  uint talloc = 0;
  uint tlive = 0;
  uint toffbase = 0;
  uint texpand = 0;
  uint ttsubtn = 0;
  for (uint layer = 1; layer < _colCnt; layer++) {
    for (uint dir = 0; dir < _rowCnt; dir++) {
      topBigTrack = 0;
      topSubtNum = 0;
      totalSubtNum = 0;
      expTrackNum = 0;
      offbase = 0;
      liveCnt = 0;
      tgrid = _gridTable[dir][layer];
      for (trn = 0; trn < tgrid->_trackCnt; trn++) {
        if (tgrid->_trackTable[trn] == nullptr) {
          continue;
        }
        liveCnt++;
        if (tgrid->_base + tgrid->_pitch * trn
            != tgrid->_trackTable[trn]->_base) {
          offbase++;
        }
        if (tgrid->_subTrackCnt[trn] == 0) {
          continue;
        }
        expTrackNum++;
        totalSubtNum += tgrid->_subTrackCnt[trn];
        if (tgrid->_subTrackCnt[trn] > topSubtNum) {
          topSubtNum = tgrid->_subTrackCnt[trn];
          topBigTrack = trn;
        }
      }
      fprintf(fp,
              "%5d%5d%8d%8d%8d%8d%8d%8d%5d\n",
              layer,
              dir,
              tgrid->_trackCnt,
              liveCnt,
              offbase,
              expTrackNum,
              totalSubtNum,
              topBigTrack,
              topSubtNum);
      talloc += tgrid->_trackCnt;
      tlive += liveCnt;
      toffbase += offbase;
      texpand += expTrackNum;
      ttsubtn += totalSubtNum;
    }
  }
  fprintf(fp,
          "---------------------------------------------------------------\n");
  fprintf(fp,
          "          %8d%8d%8d%8d%8d\n",
          talloc,
          tlive,
          toffbase,
          texpand,
          ttsubtn);
}
GridTable::GridTable(Rect* bb,
                     uint rowCnt,
                     uint colCnt,
                     uint* pitch,
                     const int* X1,
                     const int* Y1)
{
  const int memChunk = 1024;
  _trackPool = new AthPool<Track>(memChunk);
  _wirePool = new AthPool<Wire>(memChunk * 1000);

  _wirePool->alloc();  // so all wire ids>0

  _rowSize = bb->dy();
  _colSize = bb->dx();

  _wireCnt = 0;
  resetMaxArea();

  _rectBB.reset(bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax());
  _rowCnt = rowCnt;
  _colCnt = colCnt;

  const uint markerLen = 500000;

  const int x2 = bb->xMax();
  const int y2 = bb->yMax();

  _gridTable = new Grid**[_rowCnt];
  for (uint ii = 0; ii < _rowCnt; ii++) {
    _gridTable[ii] = new Grid*[_colCnt];
    _gridTable[ii][0] = nullptr;

    for (uint jj = 1; jj < _colCnt; jj++) {
      _gridTable[ii][jj] = new Grid(this, _trackPool, _wirePool, jj, 10);
      const int x1 = X1 ? X1[jj] : bb->xMin();
      const int y1 = Y1 ? Y1[jj] : bb->yMin();
      _gridTable[ii][jj]->setTracks(
          ii, 1, pitch[jj], x1, y1, x2, y2, markerLen);
    }
  }
}

GridTable::~GridTable()
{
  delete _trackPool;
  delete _wirePool;

  for (uint ii = 0; ii < _rowCnt; ii++) {
    for (uint jj = 0; jj < _rowCnt; jj++) {
      delete _gridTable[ii][jj];
    }
    delete[] _gridTable[ii];
  }
  delete[] _gridTable;
}
int GridTable::xMin()
{
  return _rectBB.xMin();
}
int GridTable::xMax()
{
  return _rectBB.xMax();
}
int GridTable::yMin()
{
  return _rectBB.yMin();
}
int GridTable::yMax()
{
  return _rectBB.yMax();
}
uint GridTable::getRowNum(int y)
{
  int dy = y - yMin();
  if (dy < 0) {
    return 0;
  }
  return dy / _rowSize;
}
uint GridTable::getColNum(int x)
{
  int dx = x - xMin();
  if (dx < 0) {
    return 0;
  }

  return dx / _colSize;
}
uint GridTable::search(SearchBox* bb,
                       uint row,
                       uint col,
                       Ath__array1D<uint>* idTable,
                       bool wireIdFlag)
{
  return _gridTable[row][col]->search(bb, idTable, wireIdFlag);
}
uint GridTable::search(SearchBox* bb,
                       uint* gxy,
                       uint row,
                       uint col,
                       Ath__array1D<uint>* idtable,
                       Grid* g)
{
  return _gridTable[row][col]->search(bb, gxy, idtable, g);
}
uint GridTable::search(SearchBox* bb, Ath__array1D<uint>* idTable)
{
  uint row1 = getRowNum(bb->loXY(1));
  if (row1 > 0) {
    row1--;
  }

  uint row2 = getRowNum(bb->hiXY(1));

  uint col1 = getColNum(bb->loXY(0));
  if (col1 > 0) {
    col1--;
  }

  uint col2 = getColNum(bb->hiXY(0));

  for (uint ii = row1; ii < _rowCnt && ii <= row2; ii++) {
    for (uint jj = col1; jj < _colCnt && jj <= col2; jj++) {
      _gridTable[ii][jj]->search(bb, idTable);
    }
  }
  return 0;
}
bool GridTable::getRowCol(int x1, int y1, uint* row, uint* col)
{
  *row = getRowNum(y1);
  if (*row >= _rowCnt) {
    fprintf(stderr, "Y=%d Out of Row Range %d\n", y1, _rowCnt);
    return false;
  }
  *col = getColNum(x1);
  if (*col >= _colCnt) {
    fprintf(stderr, "X=%d Out of Col Range %d\n", x1, _colCnt);
    return false;
  }
  return true;
}
uint GridTable::setExtrusionMarker(uint startRow, uint startCol)
{
  uint cnt = 0;
  for (uint ii = startRow; ii < _rowCnt; ii++) {
    for (uint jj = startCol; jj < _colCnt; jj++) {
      cnt += _gridTable[ii][jj]->setExtrusionMarker();
    }
  }
  return cnt;
}
AthPool<Wire>* Grid::getWirePoolPtr()
{
  return _wirePoolPtr;
}

uint Grid::removeMarkedNetWires()
{
  uint cnt = 0;
  for (uint ii = 0; ii < _trackCnt; ii++) {
    Track* btrack = _trackTable[ii];
    if (btrack == nullptr) {
      continue;
    }

    Track* track = nullptr;
    bool tohi = true;
    while ((track = btrack->getNextSubTrack(track, tohi)) != nullptr) {
      cnt += track->removeMarkedNetWires();
    }
  }
  return cnt;
}

Grid* GridTable::getGrid(uint row, uint col)
{
  return _gridTable[row][col];
}
bool GridTable::addBox(uint row, uint col, dbBox* bb)
{
  Grid* g = _gridTable[row][col];

  g->placeBox(bb, 0, 0);

  return true;
}
Wire* GridTable::addBox(dbBox* bb, uint wtype, uint id)
{
  uint row = 0;
  uint col = 0;
  Grid* g = _gridTable[row][col];

  g->placeBox(bb, wtype, id);

  return nullptr;
}
Wire* GridTable::addBox(Box* bb)
{
  uint row;
  uint col;
  const odb::Rect rect = bb->getRect();
  if (!getRowCol(rect.xMin(), rect.yMin(), &row, &col)) {
    return nullptr;
  }

  Grid* g = _gridTable[row][col];
  g->placeBox(bb);

  return nullptr;
}
Wire* GridTable::getWire_Linear(uint instId)
{
  // bool ordered= true;

  // uint cnt= 0;
  for (uint ii = 0; ii < _rowCnt; ii++) {
    for (uint jj = 0; jj < _colCnt; jj++) {
      Wire* w = _gridTable[ii][jj]->getWire_Linear(instId);
      if (w != nullptr) {
        return w;
      }
    }
  }
  return nullptr;
}
void GridTable::adjustOverlapMakerEnd()
{
  if (_overlapAdjust != Z_endAdjust) {
    return;
  }
  for (uint ii = 0; ii < _rowCnt; ii++) {
    for (uint jj = 0; jj < _colCnt; jj++) {
      if (_gridTable[ii][jj]) {
        _gridTable[ii][jj]->adjustOverlapMakerEnd();
      }
    }
  }
}

void GridTable::incrMultiTrackWireCnt(bool isPower)
{
  if (isPower) {
    _powerMultiTrackWire++;
  } else {
    _signalMultiTrackWire++;
  }
}
bool GridTable::isOrdered(bool /* unused: ascending */)
{
  bool ordered = true;

  for (uint ii = 0; ii < _rowCnt; ii++) {
    for (uint jj = 0; jj < _colCnt; jj++) {
      uint cnt1 = 0;
      if (!_gridTable[ii][jj]->isOrdered(true, &cnt1)) {
        ordered = false;
        fprintf(stdout,
                "NOT ordered grid [%d][%d] -- has %d wires\n",
                ii,
                jj,
                cnt1);
      } else {
        fprintf(
            stdout, "Ordered grid [%d][%d] -- has %d wires\n", ii, jj, cnt1);
      }
    }
  }
  return ordered;
}

void GridTable::removeMarkedNetWires()
{
  uint cnt = 0;
  for (uint jj = 1; jj < _colCnt; jj++) {
    for (int ii = _rowCnt - 1; ii >= 0; ii--) {
      Grid* netGrid = _gridTable[ii][jj];
      cnt += netGrid->removeMarkedNetWires();
    }
  }
  fprintf(stdout, "remove %d sdb wires.\n", cnt);
}

void GridTable::setExtControl(dbBlock* block,
                              bool useDbSdb,
                              uint adj,
                              uint npsrc,
                              uint nptgt,
                              uint ccUp,
                              bool allNet,
                              uint contextDepth,
                              Ath__array1D<int>** contextArray,
                              Ath__array1D<SEQ*>*** dgContextArray,
                              uint* dgContextDepth,
                              uint* dgContextPlanes,
                              uint* dgContextTracks,
                              uint* dgContextBaseLvl,
                              int* dgContextLowLvl,
                              int* dgContextHiLvl,
                              uint* dgContextBaseTrack,
                              int* dgContextLowTrack,
                              int* dgContextHiTrack,
                              int** dgContextTrackBase,
                              AthPool<SEQ>* seqPool)
{
  _block = block;
  _useDbSdb = useDbSdb;
  _overlapAdjust = adj;
  _noPowerSource = npsrc;
  _noPowerTarget = nptgt;
  _CCtargetHighTracks = ccUp;
  if (ccUp == 2) {
    _CCtargetHighMarkedNet = 1;
  } else {
    _CCtargetHighMarkedNet = 0;
  }
  _targetTrackReversed = false;
  _ccContextDepth = contextDepth;
  _ccContextArray = contextArray;
  _allNet = allNet;
  _dgContextArray = dgContextArray;
  _dgContextDepth = dgContextDepth;
  _dgContextPlanes = dgContextPlanes;
  _dgContextTracks = dgContextTracks;
  _dgContextBaseLvl = dgContextBaseLvl;
  _dgContextLowLvl = dgContextLowLvl;
  _dgContextHiLvl = dgContextHiLvl;
  _dgContextBaseTrack = dgContextBaseTrack;
  _dgContextLowTrack = dgContextLowTrack;
  _dgContextHiTrack = dgContextHiTrack;
  _dgContextTrackBase = dgContextTrackBase;
  _seqPool = seqPool;
}
void GridTable::setExtControl_v2(dbBlock* block,
                                 bool useDbSdb,
                                 uint adj,
                                 uint npsrc,
                                 uint nptgt,
                                 uint ccUp,
                                 bool allNet,
                                 uint contextDepth,
                                 Ath__array1D<int>** contextArray,
                                 uint* contextLength,
                                 Ath__array1D<SEQ*>*** dgContextArray,
                                 uint* dgContextDepth,
                                 uint* dgContextPlanes,
                                 uint* dgContextTracks,
                                 uint* dgContextBaseLvl,
                                 int* dgContextLowLvl,
                                 int* dgContextHiLvl,
                                 uint* dgContextBaseTrack,
                                 int* dgContextLowTrack,
                                 int* dgContextHiTrack,
                                 int** dgContextTrackBase,
                                 AthPool<SEQ>* seqPool)
{
  _block = block;
  _useDbSdb = useDbSdb;
  _overlapAdjust = adj;
  _noPowerSource = npsrc;
  _noPowerTarget = nptgt;
  _CCtargetHighTracks = ccUp;
  if (ccUp == 2) {
    _CCtargetHighMarkedNet = 1;
  } else {
    _CCtargetHighMarkedNet = 0;
  }
  _targetTrackReversed = false;
  _ccContextDepth = contextDepth;
  _ccContextArray = contextArray;
  _ccContextLength = contextLength;
  _allNet = allNet;
  _dgContextArray = dgContextArray;
  _dgContextDepth = dgContextDepth;
  _dgContextPlanes = dgContextPlanes;
  _dgContextTracks = dgContextTracks;
  _dgContextBaseLvl = dgContextBaseLvl;
  _dgContextLowLvl = dgContextLowLvl;
  _dgContextHiLvl = dgContextHiLvl;
  _dgContextBaseTrack = dgContextBaseTrack;
  _dgContextLowTrack = dgContextLowTrack;
  _dgContextHiTrack = dgContextHiTrack;
  _dgContextTrackBase = dgContextTrackBase;
  _seqPool = seqPool;
}
void GridTable::reverseTargetTrack()
{
  _CCtargetHighTracks = _CCtargetHighTracks == 2 ? 0 : 2;
  _targetTrackReversed = _targetTrackReversed ? false : true;
}

void GridTable::setMaxArea(int x1, int y1, int x2, int y2)
{
  _maxSearchBox.set(x1, y1, x2, y2, 1);
  _setMaxArea = true;
}
void GridTable::resetMaxArea()
{
  _setMaxArea = false;
  _maxSearchBox.invalidateBox();
}
void GridTable::getBox(uint wid,
                       int* x1,
                       int* y1,
                       int* x2,
                       int* y2,
                       uint* level,
                       uint* id1,
                       uint* id2,
                       uint* wtype)
{
  Wire* w = getWirePtr(wid);

  *id1 = w->_boxId;
  *id2 = w->_otherId;
  *wtype = w->_flags;
  *level = w->_track->getGrid()->getLevel();
  uint dir;
  w->getCoords(x1, y1, x2, y2, &dir);
}
uint GridTable::addBox(int x1,
                       int y1,
                       int x2,
                       int y2,
                       uint level,
                       uint id1,
                       uint id2,
                       uint wireType)
{
  SearchBox bb(x1, y1, x2, y2, level);
  bb.setOwnerId(id1, id2);
  bb.setType(wireType);

  uint dir = bb.getDir();
  uint trackNum = !_v2 ? getGrid(dir, level)->placeWire(&bb)
                       : getGrid(dir, level)->placeWire_v2(&bb);
  _wireCnt++;
  return trackNum;
}
uint GridTable::getWireCnt()
{
  return _wireCnt;
}
uint GridTable::search(int x1,
                       int y1,
                       int x2,
                       int y2,
                       uint row,
                       uint col,
                       Ath__array1D<uint>* idTable,
                       bool /* unused: wireIdFlag */)
{
  SearchBox bb(x1, y1, x2, y2, col, row);

  return search(&bb, row, col, idTable, true);  // single grid
}
void GridTable::getCoords(SearchBox* bb, uint wireId)
{
  Wire* w = getWirePtr(wireId);
  w->getCoords(bb);
}

}  // namespace rcx
