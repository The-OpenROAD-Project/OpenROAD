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

#include "dbLogger.h"
#include "wire.h"

//#define SINGLE_WIRE

Ath__box::Ath__box()
{
  set(0, 0, 0, 0);
}
int Ath__box::getXlo(int bound)
{
  if (_xlo < bound)
    return bound;
  else
    return _xlo;
}
int Ath__box::getYlo(int bound)
{
  if (_ylo < bound)
    return bound;
  else
    return _ylo;
}
int Ath__box::getXhi(int bound)
{
  if (_xhi > bound)
    return bound;
  else
    return _xhi;
}
int Ath__box::getYhi(int bound)
{
  if (_yhi > bound)
    return bound;
  else
    return _yhi;
}
uint Ath__box::getDir()
{
  uint dx = getDX();
  uint dy = getDY();

  return (dx < dy) ? 0 : 1;
}
uint Ath__box::getWidth(uint* dir)
{
  uint dx = getDX();
  uint dy = getDY();
  *dir = 1;       // horizontal
  if (dx < dy) {  // vertical
    *dir = 0;
    return dx;
  } else {
    return dy;
  }
}

Ath__box::Ath__box(int x1, int y1, int x2, int y2, uint units)
{
  set(x1, y1, x2, y2, units);
}
void Ath__box::set(int x1, int y1, int x2, int y2, uint units)
{
  _xlo = x1 * units;
  _ylo = y1 * units;
  _xhi = x2 * units;
  _yhi = y2 * units;
  _valid = 1;
  _id = 0;
}
uint Ath__box::getOwner()
{
  return 0;
}
uint Ath__box::getDX()
{
  return _xhi - _xlo;
}
uint Ath__box::getDY()
{
  return _yhi - _ylo;
}
uint Ath__box::getLength()
{
  uint dx = _xhi - _xlo;
  uint dy = _yhi - _ylo;
  if (dx < dy)
    return dy;
  else
    return dx;
}
void Ath__box::invalidateBox()
{
  _valid = 0;
}
void Ath__box::set(Ath__box* bb)
{
  _xlo = bb->_xlo;
  _ylo = bb->_ylo;
  _xhi = bb->_xhi;
  _yhi = bb->_yhi;
}

bool Ath__box::outside(int x1, int y1, int x2, int y2)
{
  if (_valid == 0)
    return false;

  if (x1 >= _xhi)
    return true;
  if (y1 >= _yhi)
    return true;

  if (x2 <= _xlo)
    return true;
  if (y2 <= _ylo)
    return true;

  return false;
}

void Ath__searchBox::set(int x1, int y1, int x2, int y2, uint l, int dir)
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
bool Ath__searchBox::outside(int x1, int y1, int x2, int y2)
{
  if (x1 >= _ur[0])
    return true;
  if (y1 >= _ur[1])
    return true;

  if (x2 <= _ll[0])
    return true;
  if (y2 <= _ll[1])
    return true;

  return false;
}
Ath__searchBox::Ath__searchBox()
{
}
Ath__searchBox::Ath__searchBox(Ath__box* bb, uint l, int dir)
{
  set(bb->_xlo, bb->_ylo, bb->_xhi, bb->_yhi, l, dir);
}
Ath__searchBox::Ath__searchBox(Ath__searchBox* bb, uint l, int dir)
{
  set(bb->_ll[0], bb->_ll[1], bb->_ur[0], bb->_ur[1], l, dir);
}
Ath__searchBox::Ath__searchBox(int x1, int y1, int x2, int y2, uint l, int dir)
{
  set(x1, y1, x2, y2, l, dir);
}
void Ath__searchBox::printCoords(FILE* fp, char* msg)
{
  fprintf(fp, "%s  %d %d  %d %d\n", msg, _ll[0], _ll[1], _ur[0], _ur[1]);
}
void Ath__searchBox::setMidPointSearch()
{
  for (uint i = 0; i < 2; i++) {
    _ll[i] = (_ll[i] + _ur[i]) / 2;
    _ur[i] = _ll[i] + 1;
  }
}

void Ath__searchBox::setMaxBox(Ath__searchBox* bb)
{
  for (uint i = 0; i < 2; i++) {
    if (bb->_ll[i] < _ll[i])
      _ll[i] = bb->_ll[i];

    if (bb->_ur[i] > _ur[i])
      _ur[i] = bb->_ur[i];
  }
}
void Ath__searchBox::setLo(uint d, int xy)
{
  _ll[d] = xy;
}
void Ath__searchBox::setHi(uint d, int xy)
{
  _ur[d] = xy;
}
int Ath__searchBox::loXY(uint d)
{
  return _ll[d];
}
int Ath__searchBox::hiXY(uint d)
{
  return _ur[d];
}
int Ath__searchBox::loXY(uint d, int loBound)
{
  if (_ll[d] < loBound)
    return loBound;
  else
    return _ll[d];
}
int Ath__searchBox::hiXY(uint d, int hiBound)
{
  if (_ur[d] > hiBound)
    return hiBound;
  else
    return _ur[d];
}
uint Ath__searchBox::getDir()
{
  return _dir;
}
uint Ath__searchBox::getLevel()
{
  return _level;
}
uint Ath__searchBox::getOwnerId()
{
  return _ownId;
}
uint Ath__searchBox::getOtherId()
{
  return _otherId;
}
uint Ath__searchBox::getType()
{
  return _type;
}
void Ath__searchBox::setOwnerId(uint v, uint other)
{
  _ownId = v;
  _otherId = other;
}
void Ath__searchBox::setType(uint v)
{
  _type = v;
}
void Ath__searchBox::setDir(int dir)
{
  if (dir >= 0) {
    _dir = dir;
  } else {
    _dir = 1;  // horizontal
    int dx = _ur[0] - _ll[0];
    if (dx < _ur[1] - _ll[1])
      _dir = 0;  // vertical
  }
}
uint Ath__searchBox::getLength()
{
  if (_dir > 0)
    return _ur[0] - _ll[0];
  else
    return _ur[1] - _ll[1];
}
void Ath__searchBox::setLevel(uint v)
{
  _level = v;
}

void Ath__wire::reset()
{
  _boxId = 0;
  _srcId = 0;
  _otherId = 0;
  _track = NULL;
  _next = NULL;

  _xy = 0;  // offset from track start
  _len = 0;

  _base = 0;
  _width = 0;
  _flags = 0;  // ownership
               // 0=wire, 1=obs, 2=pin, 3=power
  _dir = 0;
  _ext = 0;
  _ouLen = 0;
}
bool Ath__wire::isTilePin()
{
  if (_flags == 1)
    return true;
  else
    return false;
}
bool Ath__wire::isTileBus()
{
  if (_flags == 2)
    return true;
  else
    return false;
}
bool Ath__wire::isPower()
{
  uint power_wire_id = 11;  // see db/dbSearch.cpp
  if (_flags == power_wire_id)
    return true;
  else
    return false;
}
bool Ath__wire::isVia()
{
  uint via_wire_id = 5;  // see db/dbSearch.cpp
  if (_flags == via_wire_id)
    return true;
  else
    return false;
}
void Ath__wire::setOtherId(uint id)
{
  _otherId = id;
}
int Ath__wire::getRsegId()
{
  int wBoxId = _boxId;
  if (!(_otherId > 0))
    return wBoxId;

  if (isVia()) {
    wBoxId = getShapeProperty(_otherId);
  } else {
    getNet()->getWire()->getProperty((int) _otherId, wBoxId);
  }
  return wBoxId;
}
int Ath__wire::getShapeProperty(int id)
{
  dbNet* net = getNet();
  if (net == NULL)
    return 0;
  char buff[64];
  sprintf(buff, "%d", id);
  char const* pchar = strdup(buff);
  dbIntProperty* p = dbIntProperty::find(net, pchar);
  if (p == NULL)
    return 0;
  int rcid = p->getValue();
  return rcid;
}
dbNet* Ath__wire::getNet()
{
  Ath__gridTable* gtb = _track->getGrid()->getGridTable();
  dbBlock* block = gtb->getBlock();
  if (_otherId == 0)
    return (dbSBox::getSBox(block, _boxId)->getSWire()->getNet());
  if (gtb->usingDbSdb())
    return dbNet::getNet(block, _boxId);
  else
    return (dbRSeg::getRSeg(block, _boxId)->getNet());
}
uint Ath__wire::getBoxId()
{
  return _boxId;
}
uint Ath__wire::getOtherId()
{
  return _otherId;
}
uint Ath__wire::getSrcId()
{
  return _srcId;
}
void Ath__wire::set(uint dir, int* ll, int* ur)
{
  _boxId = 0;
  _srcId = 0;
  _track = NULL;
  _next = NULL;

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
Ath__wire* Ath__track::getTargetWire()
{
  return _targetWire;
}
void Ath__track::initTargetWire(int noPowerWire)
{
  _targetWire = NULL;
  for (_targetMarker = 0; _targetMarker < _markerCnt; _targetMarker++) {
    if (_marker[_targetMarker] == NULL)
      continue;
    _targetWire = _marker[_targetMarker];
    while (_targetWire && noPowerWire && _targetWire->isPower())
      _targetWire = _targetWire->_next;
    if (_targetWire)
      break;
  }
}
Ath__wire* Ath__track::nextTargetWire(int noPowerWire)
{
  if (_targetWire) {
    _targetWire = _targetWire->_next;
    while (_targetWire && noPowerWire && _targetWire->isPower())
      _targetWire = _targetWire->_next;
    if (!_targetWire)
      _targetMarker++;
  }
  if (_targetWire)
    return _targetWire;
  for (; _targetMarker < _markerCnt; _targetMarker++) {
    if (_marker[_targetMarker] == NULL)
      continue;
    _targetWire = _marker[_targetMarker];
    while (_targetWire && noPowerWire && _targetWire->isPower())
      _targetWire = _targetWire->_next;
    if (_targetWire)
      break;
  }
  return _targetWire;
}

int Ath__wire::wireOverlap(Ath__wire* w, int* len1, int* len2, int* len3)
{
  // _xy, _len : reference rect

  int X1 = _xy;
  int DX = _len;
  int x1 = w->_xy;
  int dx = w->_len;

  //	fprintf(stdout, "%d %d   %d %d  : ", X1, DX,   x1, dx);

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
void Ath__wire::getCoords(int* x1, int* y1, int* x2, int* y2, uint* dir)
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
void Ath__wire::getCoords(Ath__searchBox* box)
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

Ath__track* Ath__track::getNextSubTrack(Ath__track* subt, bool tohi)
{
  if (!subt)
    return tohi ? this : this->getLowTrack();
  if (tohi)
    return subt->getHiTrack()->_lowest ? NULL : subt->getHiTrack();
  else
    return subt->_lowest ? NULL : subt->getLowTrack();
}

void Ath__track::setHiTrack(Ath__track* hitrack)
{
  _hiTrack = hitrack;
}
void Ath__track::setLowTrack(Ath__track* lowtrack)
{
  _lowTrack = lowtrack;
}
Ath__track* Ath__track::getHiTrack()
{
  return _hiTrack;
}
Ath__track* Ath__track::getLowTrack()
{
  return _lowTrack;
}

void Ath__track::set(Ath__grid* g,
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
  _markerModulo = markerLen;

  if (markerCnt <= 4) {
    _markerCnt = markerCnt;
    _marker = new Ath__wire*[4];
    _eMarker = new Ath__wire*[4];
  } else {
    _markerCnt = markerCnt;
    _marker = new Ath__wire*[_markerCnt];
    _eMarker = new Ath__wire*[_markerCnt];
  }
  for (uint ii = 0; ii < _markerCnt; ii++) {
    _marker[ii] = NULL;
    _eMarker[ii] = NULL;
  }

  _blocked = 1;
  _tmpHead = NULL;
  _ordered = false;

  _hiTrack = this;
  _lowTrack = this;
  _lowest = 0;
  _base = base;
}
void Ath__track::freeWires(AthPool<Ath__wire>* pool)
{
  for (uint ii = 0; ii < _markerCnt; ii++) {
    Ath__wire* w = _marker[ii];
    while (w != NULL) {
      Ath__wire* a = w->getNext();

      pool->free(w);
      w = a;
    }
  }
}
void Ath__track::dealloc(AthPool<Ath__wire>* pool)
{
  freeWires(pool);
  delete[] _marker;
  delete[] _eMarker;
}

uint Ath__grid::getAbsTrackNum(int xy)
{
  int dist = xy - _base;

  assert(dist >= 0);

  uint n = dist / _pitch;

  assert(n < _trackCnt);

  return n;
}
uint Ath__grid::getMinMaxTrackNum(int xy)
{
  int dist = xy - _base;

  if (dist < 0)
    return 0;

  uint n = dist / _pitch;

  if (n >= _trackCnt)
    return _trackCnt - 1;

  return n;
}

void Ath__grid::initContextTracks()
{
  setSearchDomain(1);
  Ath__track *track, *btrack;
  uint ii;
  bool noPowerTarget = _gridtable->noPowerTarget() > 0 ? true : false;
  for (ii = _searchLowTrack; ii <= _searchHiTrack; ii++) {
    btrack = _trackTable[ii];
    if (btrack == NULL)
      continue;
    track = NULL;
    bool tohi = true;
    while ((track = btrack->getNextSubTrack(track, tohi)) != nullptr)
      track->initTargetWire(noPowerTarget);
  }
}

void Ath__grid::initContextGrids()
{
  uint sdepth = _gridtable->contextDepth();
  if (sdepth == 0)
    return;
  uint ii = _dir ? 0 : 1;
  uint jj;
  for (jj = 1; jj <= sdepth && (jj + _level) < _gridtable->getColCnt(); jj++)
    _gridtable->getGrid(ii, jj + _level)->initContextTracks();
  for (jj = 1; jj <= sdepth && (_level - jj) > 0; jj++)
    _gridtable->getGrid(ii, _level - jj)->initContextTracks();
}

void Ath__grid::setSearchDomain(uint domainAdjust)
{
  if (_gridtable->allNet()) {
    _searchLowTrack = 0;
    _searchHiTrack = _trackCnt - 1;
    _searchLowMarker = 0;
    _searchHiMarker = _markerCnt - 1;
    return;
  }
  Ath__box* searchBox = _gridtable->maxSearchBox();
  int lo = _dir ? searchBox->_ylo : searchBox->_xlo;
  int hi = _dir ? searchBox->_yhi : searchBox->_xhi;
  int ltrack = (int) getMinMaxTrackNum(lo) - (int) domainAdjust;
  _searchLowTrack = ltrack < 0 ? 0 : ltrack;
  _searchHiTrack = getMinMaxTrackNum(hi) + domainAdjust;
  if (_searchHiTrack >= _trackCnt)
    _searchHiTrack = _trackCnt - 1;
  int mlo = _dir ? searchBox->_xlo : searchBox->_ylo;
  int mhi = _dir ? searchBox->_xhi : searchBox->_yhi;
  _searchLowMarker = getBucketNum(mlo);
  _searchHiMarker = getBucketNum(mhi);
}

Ath__track* Ath__grid::addTrack(uint ii, uint markerCnt, int base)
{
  Ath__track* track = _trackPoolPtr->alloc();
  track->set(this, _start, _end, ii, _width, _markerLen, markerCnt, base);
  return track;
}
Ath__track* Ath__grid::addTrack(uint ii, uint markerCnt)
{
  int trackBase = _base + _pitch * ii;
  addTrack(ii, markerCnt, trackBase);
  return NULL;
}
Ath__track* Ath__grid::getTrackPtr(uint ii, uint markerCnt, int base)
{
  if (ii >= _trackCnt)
    return NULL;

  if (_blockedTrackTable[ii] > 0)
    return NULL;

  Ath__track* ntrack;
  Ath__track* ttrack = _trackTable[ii];
  while (ttrack) {
    if (ttrack->getBase() == base)
      break;
    ntrack = ttrack->getHiTrack();
    ttrack = ntrack == _trackTable[ii] ? NULL : ntrack;
  }
  if (ttrack)
    return ttrack;
  ttrack = addTrack(ii, markerCnt, base);
  if (_trackTable[ii] == NULL) {
    _trackTable[ii] = ttrack;
    ttrack->setLowest(1);
    return ttrack;
  }
  _subTrackCnt[ii]++;
  ntrack = _trackTable[ii];
  while (1) {
    if (ntrack->getBase() > base)
      break;
    ntrack = ntrack->getHiTrack();
    if (ntrack == _trackTable[ii])
      break;
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
Ath__track* Ath__grid::getTrackPtr(uint ii, uint markerCnt)
{
  int trackBase = _base + _pitch * ii;
  return getTrackPtr(ii, markerCnt, trackBase);
}
bool Ath__track::place(Ath__wire* w, int markIndex1, int markIndex2)
{
  assert(markIndex1 >= 0);
  assert(markIndex2 >= 0);

  for (int ii = markIndex1 + 1; ii <= markIndex2; ii++)
    _marker[ii] = w;

  if (_marker[markIndex1] == NULL) {
    _marker[markIndex1] = w;
    return true;
  }

  Ath__wire* a = _marker[markIndex1];
  if (w->_xy < a->_xy) {
    if (w->_xy + w->_len >= a->_xy) {
      fprintf(stdout, "overlap %d %d \n", w->_xy, a->_xy);
      return false;
    }
    w->setNext(a);
    _marker[markIndex1] = w;
    return true;
  }

  Ath__wire* e = _marker[markIndex1];
  for (; e != NULL; e = e->_next) {
    if (w->_xy < e->_xy) {
      continue;
    }
    if (w->_xy + w->_len >= a->_xy) {
      fprintf(stdout, "overlap %d %d \n", w->_xy, a->_xy);
      return false;
    }
    w->setNext(e);
    break;
  }
  return false;
}
void Ath__wire::search(int xy1, int xy2, uint& cnt, Ath__array1D<uint>* idTable)
{
  Ath__wire* e = this;
  for (; e != NULL; e = e->_next) {
    if (xy2 <= e->_xy)
      break;
    /*
    if (xy1>=e->_xy+e->_len)
            continue;
    */

    if ((xy1 <= e->_xy) && (xy2 >= e->_xy)) {
      idTable->add(e->_boxId);
      cnt++;
    } else if ((e->_xy <= xy1) && (e->_xy + e->_len >= xy1)) {
      idTable->add(e->_boxId);
      cnt++;
    }
  }
}
void Ath__wire::search1(int xy1,
                        int xy2,
                        uint& cnt,
                        Ath__array1D<uint>* idTable)
{
  Ath__wire* e = this;
  for (; e != NULL; e = e->_next) {
    if (xy2 <= e->_xy)
      break;
    /*
    if (xy1>=e->_xy+e->_len)
            continue;
    */

    if ((xy1 <= e->_xy) && (xy2 >= e->_xy)) {
      idTable->add(e->_id);
      cnt++;
    } else if ((e->_xy <= xy1) && (e->_xy + e->_len >= xy1)) {
      idTable->add(e->_id);
      cnt++;
    }
  }
}
uint Ath__track::search(int xy1,
                        int xy2,
                        uint markIndex1,
                        uint markIndex2,
                        Ath__array1D<uint>* idTable)
{
  uint cnt = 0;
  if (_eMarker[markIndex1])
    _eMarker[markIndex1]->search(xy1, xy2, cnt, idTable);
  for (uint ii = markIndex1; ii <= markIndex2; ii++) {
    if (_marker[ii] == NULL)
      continue;
    _marker[ii]->search(xy1, xy2, cnt, idTable);
  }
  return cnt;
}
void Ath__track::resetExtFlag(uint markerCnt)
{
  for (uint ii = 0; ii < markerCnt; ii++) {
    Ath__wire* e = _marker[ii];
    for (; e != NULL; e = e->_next) {
      e->_ext = 0;
    }
  }
}
uint Ath__track::getAllWires(Ath__array1D<Ath__wire*>* boxTable, uint markerCnt)
{
  for (uint ii = 0; ii < markerCnt; ii++) {
    Ath__wire* e = _marker[ii];
    for (; e != NULL; e = e->_next) {
      if (e->_ext > 0)
        continue;

      e->_ext = 1;
      boxTable->add(e);
    }
  }
  resetExtFlag(markerCnt);
  return boxTable->getCnt();
}
uint Ath__track::search1(int xy1,
                         int xy2,
                         uint markIndex1,
                         uint markIndex2,
                         Ath__array1D<uint>* idTable)
{
  if (!_ordered) {
    markIndex1 = 0;
  }

  uint cnt = 0;
  if (_eMarker[markIndex1])
    _eMarker[markIndex1]->search1(xy1, xy2, cnt, idTable);
  for (uint ii = markIndex1; ii <= markIndex2; ii++) {
    if (_marker[ii] == NULL)
      continue;
    _marker[ii]->search1(xy1, xy2, cnt, idTable);
  }
  return cnt;
}
uint Ath__track::setExtrusionMarker(int markerCnt, int start, uint markerLen)
{
  _ordered = true;

  int jj;
  int cnt = 0;
  int ii;
  for (ii = 0; ii < markerCnt; ii++)
    _eMarker[ii] = NULL;
  for (ii = 0; ii < markerCnt - 1; ii++) {
    for (Ath__wire* e = _marker[ii]; e != NULL; e = e->_next) {
      int tailMark = (e->_xy + e->_len - start) / markerLen;
      if (tailMark == ii)
        continue;
      if (tailMark > markerCnt - 1)
        tailMark = markerCnt - 1;
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
bool Ath__track::placeTrail(Ath__wire* w, uint m1, uint m2)
{
  for (uint ii = m1 + 1; ii <= m2; ii++) {
    if (_marker[ii] == NULL) {
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
bool Ath__track::checkAndplacerOnMarker(Ath__wire* w, int markIndex)
{
  if (_marker[markIndex] == NULL) {
    _marker[markIndex] = w;
    return true;
  }
  return false;
}
bool Ath__track::checkMarker(int markIndex)
{
  if (_marker[markIndex] == NULL) {
    return true;
  }
  return false;
}
bool Ath__track::checkAndplace(Ath__wire* w, int markIndex1)
{
  if (_marker[markIndex1] == NULL) {
    _marker[markIndex1] = w;
    return true;
  }

  Ath__wire* a = _marker[markIndex1];
  if (w->_xy <= a->_xy) {
    if (w->_xy + w->_len > a->_xy)
      return false;

    w->setNext(a);
    _marker[markIndex1] = w;

    return true;
  }
  Ath__wire* prev = _marker[markIndex1];
  Ath__wire* e = _marker[markIndex1];
  for (; e != NULL; e = e->_next) {
    if (w->_xy <= e->_xy) {
      if (w->_xy + w->_len > e->_xy)
        return false;

      w->setNext(e);
      prev->setNext(w);
      return true;
    }
    prev = e;
  }

  if (prev->_xy + prev->_len > w->_xy)
    return false;
  prev->setNext(w);
  return true;
}
void Ath__track::insertWire(Ath__wire* w, int mark1, int mark2)
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

bool Ath__track::place2(Ath__wire* w, int mark1, int mark2)
{
  assert(mark1 >= 0);

  w->_next = NULL;
  if (_marker[mark1] == NULL) {
    insertWire(w, mark1, mark2);
    return true;
  }
  bool status = true;

  Ath__wire* a = _marker[mark1];
  if (w->_xy <= a->_xy) {
    /*
    if (w->_xy+w->_len>a->_xy) {
            fprintf(stdout, "overlap %d %d \n", w->_xy, a->_xy);
            status= false;
    }
    */
    w->setNext(a);
    _marker[mark1] = w;

    w->_track = this;

    return true;
  }
  Ath__wire* prev = _marker[mark1];
  Ath__wire* e = _marker[mark1];
  for (; e != NULL; e = e->_next) {
    if (w->_xy <= e->_xy) {
      w->setNext(e);
      prev->setNext(w);

      w->_track = this;
      return true;
    }
    prev = e;
  }
  if (e == NULL) {  // at the end of the list
    prev->setNext(w);
    insertWire(w, mark1, mark2);
    return true;
  }

  // TODO : check Overlap;
  /*
  if (a->_xy+a->_len>w->_xy) {
          fprintf(stdout, "overlap %d %d \n", w->_xy, a->_xy);
          status= false;
  }
  */
  if (!status)
    fprintf(stdout, "OVERLAP placement\n");

  return status;
}
int SdbPlaceWireNoTouchCheckForOverlap = 0;
void Ath__track::linkWire(Ath__wire*& w1, Ath__wire*& w2)
{
  Ath__overlapAdjust adj
      = (Ath__overlapAdjust) _grid->getGridTable()->getOverlapAdjust();
  int nend, oend, tend;
  nend = w1->_xy + w1->_len;
  oend = w2->_xy + w2->_len;
  tend = nend > oend ? nend : oend;
  if (adj == Z_noAdjust || nend <= w2->_xy)
    w1->setNext(w2);
  else if (w1->_base != w2->_base || w1->_width != w2->_width) {
    _grid->getGridTable()->incrOfflineOverlapCnt();
    if (!_grid->getGridTable()->getOverlapTouchCheck()
        || w1->_base > w2->_base + w2->_width
        || w2->_base > w1->_base + w1->_width)
      w1->setNext(w2);
    else {  // only good for adj == Z_endAdjust
      _grid->getGridTable()->incrOfflineOverlapTouch();
      if (w1->_base < w2->_base) {  // w1 is wider?
        w2->_xy = nend;
        w1->setNext(w2);
        if (nend >= oend)
          w2->_len = 0;
        else
          w2->_len = oend - nend;
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
    if (nend >= oend)
      w2->_len = 0;
    else
      w2->_len = oend - nend;
  }
}

bool Ath__track::place(Ath__wire* w, int markIndex1)
{
  assert(markIndex1 >= 0);

  w->_track = this;

  if (_marker[markIndex1] == NULL) {
    _marker[markIndex1] = w;
    return true;
  }
  Ath__overlapAdjust adj
      = (Ath__overlapAdjust) _grid->getGridTable()->getOverlapAdjust();
  bool status = true;

  Ath__wire* a = _marker[markIndex1];
  if (w->_xy <= a->_xy) {
    /*
    if (w->_xy+w->_len>a->_xy) {
            fprintf(stdout, "overlap %d %d \n", w->_xy, a->_xy);
            status= false;
    }
    */
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
  Ath__wire* prev = _marker[markIndex1];
  Ath__wire* e = _marker[markIndex1];
  for (; e != NULL; e = e->_next) {
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
    } else if (adj != Z_noAdjust && w->_xy + w->_len <= e->_xy + e->_len
               && e->_base <= w->_base
               && e->_base + e->_width >= w->_base + w->_width) {  // w inside e
      return true;
    }
    prev = e;
  }
  if (e == NULL) {
    linkWire(prev, w);
  }

  // TODO : check Overlap;
  /*
  if (a->_xy+a->_len>w->_xy) {
          fprintf(stdout, "overlap %d %d \n", w->_xy, a->_xy);
          status= false;
  }
  */
  if (!status)
    fprintf(stdout, "OVERLAP placement\n");

  return status;
}
Ath__wire* Ath__track::getNextWire(Ath__wire* wire)
{
  Ath__wire* nwire;
  if (!wire) {
    _searchMarkerIndex = _grid->searchLowMarker();
    nwire = _marker[_searchMarkerIndex];
  } else
    nwire = wire->_next;
  if (nwire)
    return nwire;
  for (_searchMarkerIndex++; _searchMarkerIndex <= _grid->searchHiMarker();
       _searchMarkerIndex++) {
    nwire = _marker[_searchMarkerIndex];
    if (nwire)
      return nwire;
  }
  return NULL;
}

Ath__wire* Ath__track::getWire_Linear(uint markerCnt, uint id)
{
  for (uint ii = 0; ii < markerCnt; ii++) {
    Ath__wire* e = _marker[ii];
    for (; e != NULL; e = e->_next) {
      if (e->_id == id)
        return e;
    }
  }
  return NULL;
}
void Ath__track::adjustOverlapMakerEnd(uint markerCnt,
                                       int start,
                                       uint markerLen)
{
  _ordered = true;

  Ath__wire* e;
  uint tailMark;
  uint jj;
  for (uint ii = 0; ii < markerCnt - 1; ii++) {
    if ((e = _marker[ii]) == NULL)
      continue;
    for (; e->_next != NULL; e = e->_next) {
      ;
    }
    tailMark = (e->_xy + e->_len - start) / markerLen;
    if (tailMark == ii)
      continue;
    if (tailMark > markerCnt - 1)
      tailMark = markerCnt - 1;
    for (jj = ii + 1; jj <= tailMark; jj++) {
      _eMarker[jj] = e;
      if (_marker[jj]) {
        jj++;
        break;
      }
    }
    jj--;
    if (_marker[jj] != NULL && e->_xy + e->_len > _marker[jj]->_xy)
      e->_len = _marker[jj]->_xy - e->_xy;
    ii = jj - 1;
  }
}
void Ath__track::adjustOverlapMakerEnd(uint markerCnt)
{
  Ath__wire* e;
  uint jj;
  for (uint ii = 0; ii < markerCnt - 1; ii++) {
    if ((e = _marker[ii]) == NULL)
      continue;
    for (; e->_next != NULL; e = e->_next) {
      ;
    }
    for (jj = ii + 1; jj < markerCnt && _marker[jj] == NULL; jj++) {
      ;
    }
    if (jj == markerCnt)
      continue;
    if (e->_xy + e->_len > _marker[jj]->_xy)
      e->_len = _marker[jj]->_xy - e->_xy;
    ii = jj - 1;
  }
}
void Ath__track::adjustMetalFill()
{
  uint wsrcId;
  int wstart, wend;
  Ath__wire* wire = getNextWire(NULL);
  Ath__wire* gwire = NULL;
  Ath__wire* nwire = NULL;
  Ath__wire* nnwire = NULL;
  Ath__wire* pnwire = NULL;
  Ath__wire* prevwire = NULL;
  for (; wire; prevwire = wire, wire = nwire) {
    nwire = getNextWire(wire);
    if (!nwire || wire->_xy + wire->_len <= nwire->_xy)
      continue;
    if (wire->_base != nwire->_base || wire->_width != nwire->_width) {
      _grid->getGridTable()->incrNotAlignedOverlap(wire, nwire);
      // notice(0,"Overlapped but not aligned wires!\n");
      continue;
    }
    if (wire->isPower() == nwire->isPower()) {
      if (!wire->isPower()) {
        _grid->getGridTable()->incrSignalOverlap();
        // notice(0,"Overlapped signal wires!\n");
      } else {
        _grid->getGridTable()->incrPowerOverlap();
        // notice(0,"Overlapped power wires!\n");
        if (wire->_xy >= nwire->_xy)
          continue;  // bp
        wire->_len = nwire->_xy - wire->_xy;
      }
      continue;
    }
    if (!wire->isPower())
    // signal wire extends into power wire
    {
      _grid->getGridTable()->incrSignalToPowerOverlap();
      if (wire->_xy + wire->_len >= nwire->_xy + nwire->_len) {
        if (wire->_next == nwire)
          wire->_next = nwire->_next;
        else {
          assert(wire->_next == NULL);
          assert(_marker[_searchMarkerIndex] == nwire);
          _marker[_searchMarkerIndex] = nwire->_next;
        }
        _grid->getWirePoolPtr()->free(nwire);
        nwire = getNextWire(wire);
      } else
        nwire->_xy = wire->_xy + wire->_len;
      continue;
    }
    // power wire extends into signal wire
    _grid->getGridTable()->incrPowerToSignallOverlap();
    if (wire->_xy + wire->_len
        <= nwire->_xy
               + nwire->_len) {     // power wire does not end after signal wire
      if (wire->_xy >= nwire->_xy)  // can be > by prvious adjustment
      {
        if (_marker[_searchMarkerIndex] == wire)
          _marker[_searchMarkerIndex] = nwire;
        else {
          assert(prevwire->_next == wire);
          prevwire->_next = nwire;
        }
        _grid->getWirePoolPtr()->free(wire);
        wire = prevwire;
      } else {
        wire->_len = nwire->_xy - wire->_xy;
      }
      continue;
    } else {  // power wire ends after signal wire
      wstart = nwire->_xy + nwire->_len;
      wend = wire->_xy + wire->_len;
      wsrcId = wire->_srcId;
      if (wire->_xy >= nwire->_xy)  // can be > by prvious adjustment
      {
        if (_marker[_searchMarkerIndex] == wire)
          _marker[_searchMarkerIndex] = nwire;
        else
          prevwire->_next = nwire;
        _grid->getWirePoolPtr()->free(wire);
        wire = prevwire;
      } else {
        wire->_len = nwire->_xy - wire->_xy;
      }
      if (wsrcId != 0)
        continue;
      pnwire = nwire;
      for (nnwire = getNextWire(nwire); nnwire;
           pnwire = nnwire, nnwire = getNextWire(nnwire)) {
        if (wend <= nnwire->_xy) {
          gwire
              = wire->makeWire(_grid->getWirePoolPtr(), wstart, wend - wstart);
          _grid->placeWire(gwire);
          wire = gwire;
          nwire = nnwire;
          break;
        }
        if (nwire->_base != nnwire->_base || nwire->_width != nnwire->_width) {
          _grid->getGridTable()->incrNotAlignedOverlap(nwire, nnwire);
          // notice(0,"Overlapped but not aligned wires!\n");
        }
        if (nnwire->isPower()) {
          // notice (0, "Overlapped power wire\n");
          _grid->getGridTable()->incrPowerOverlap();
        }
        gwire = NULL;
        if (wstart < nnwire->_xy) {
          gwire = wire->makeWire(
              _grid->getWirePoolPtr(), wstart, nnwire->_xy - wstart);
          _grid->placeWire(gwire);
        }
        wstart = nnwire->_xy + nnwire->_len;
        if (wend <= wstart) {
          wire = gwire ? gwire : pnwire;
          nwire = nnwire;
          break;
        }
      }
      if (nnwire == NULL)
        break;
    }  // end of power wire extends into signal wire
  }
}
bool Ath__track::isAscendingOrdered(uint markerCnt, uint* wCnt)
{
  uint cnt = 0;
  for (uint ii = 0; ii < markerCnt; ii++) {
    Ath__wire* e = _marker[ii];
    for (; e != NULL; e = e->_next) {
      Ath__wire* w = e->_next;
      cnt++;

      if (w == NULL)
        break;

      if (w->_xy < e->_xy) {
        *wCnt = cnt;
        return false;
      }
    }
  }
  *wCnt += cnt;
  return true;
}
bool Ath__track::overlapCheck(Ath__wire* w,
                              int markIndex1,
                              int /* unused: markIndex2 */)
{
  assert(markIndex1 >= 0);

  Ath__wire* e = _marker[markIndex1];
  for (; e != NULL; e = e->_next) {
    if (w->_xy + w->_len >= e->_xy) {
      return true;
    }
  }
  return false;
}
void Ath__grid::makeTrackTable(uint width, uint pitch, uint space)
{
  if (width > 0)
    _width = width;

  if (space > 0)
    _pitch = _width + space;
  else if (pitch > 0)
    _pitch = pitch;

  _markerLen = ((_end - _start) / _width) / _markerCnt;

  _trackCnt = _max;  // for LINUX assert
  _trackCnt = getAbsTrackNum(_max) + 1;
  _subTrackCnt = (uint*) calloc(sizeof(uint), _trackCnt);
  _trackFilledCnt = 0;
  _trackTable = new Ath__track*[_trackCnt];
  _blockedTrackTable = new uint[_trackCnt];

  for (uint ii = 0; ii < _trackCnt; ii++) {
    _trackTable[ii] = NULL;
    _blockedTrackTable[ii] = 0;
  }
  //	_viaTable= new AthArray<Ath__via*>(64);
  //	_wireTable= new AthArray<Ath__wire*>(64);

  _ignoreFlag = 0;
}
void Ath__grid::setBoundaries(uint dir, int xlo, int ylo, int xhi, int yhi)
{
  _lo[0] = xlo;
  _lo[1] = ylo;
  _hi[0] = xhi;
  _hi[1] = yhi;

  _dir = dir;
  if (_dir == 0) {  // vertical
    _base = xlo;
    _max = xhi;
    _start = ylo;
    _end = yhi;
  } else {
    _base = ylo;
    _max = yhi;
    _start = xlo;
    _end = xhi;
  }
}
Ath__grid::Ath__grid(Ath__gridTable* gt,
                     AthPool<Ath__track>* trackPool,
                     AthPool<Ath__wire>* wirePool,
                     uint level,
                     uint num,
                     uint markerCnt)
{
  _gridtable = gt;
  _trackPoolPtr = trackPool;
  _wirePoolPtr = wirePool;
  _markerCnt = markerCnt;
  _level = level;
  _layer = num;
  _placed = 0;
  _schema = 0;
}

void Ath__grid::setTracks(uint dir,
                          uint width,
                          uint pitch,
                          int xlo,
                          int ylo,
                          int xhi,
                          int yhi,
                          uint markerLen)
{
  setBoundaries(dir, xlo, ylo, xhi, yhi);
  makeTrackTable(width, pitch);

  if (markerLen > 0) {
    _markerLen = markerLen;
    _markerCnt = ((_end - _start) / _width) / markerLen;
    if (_markerCnt == 0)
      _markerCnt = 1;
  }
}
void Ath__grid::setSchema(uint v)
{
  _schema = v;
}
Ath__grid::Ath__grid(Ath__gridTable* gt,
                     AthPool<Ath__track>* trackPool,
                     AthPool<Ath__wire>* wirePool,
                     Ath__box* bb,
                     uint level,
                     uint dir,
                     uint num,
                     uint width,
                     uint pitch,
                     uint markerCnt)
{
  _gridtable = gt;
  _trackPoolPtr = trackPool;
  _wirePoolPtr = wirePool;
  _markerCnt = markerCnt;

  _level = level;
  _layer = num;

  setBoundaries(dir, bb->_xlo, bb->_ylo, bb->_xhi, bb->_yhi);
  makeTrackTable(width, pitch);
  _placed = 0;
  _schema = 0;
}
void Ath__grid::getBbox(Ath__searchBox* bb)
{
  if (_dir == 0)  // vertical
    bb->set(_base, _start, _max, _end, _level, _dir);
  else
    bb->set(_start, _base, _end, _max, _level, _dir);
}
void Ath__grid::getBbox(Ath__box* bb)
{
  if (_dir == 0) {  // vertical
    bb->_xlo = _base;
    bb->_xhi = _max;
    bb->_ylo = _start;
    bb->_yhi = _end;
  } else {
    bb->_ylo = _base;
    bb->_yhi = _max;
    bb->_xlo = _start;
    bb->_xhi = _end;
  }
}

void Ath__grid::freeTracksAndTables()
{
  /* TODO-OPTIMIZE
  for (uint jj= 0; jj<_wireTable->getLast(); jj++)
          _wirePoolPtr->free(_wireTable->get(jj));
*/
  /* deleted by "delete _trackPool" in Ath__gridTable::~Ath__gridTable()
  for (uint ii= 0; ii<_trackCnt; ii++) {
          Ath__track *tr= _trackTable[ii];
          if (tr==NULL)
                  continue;

          //tr->freeWires(_wirePoolPtr);

          _trackPoolPtr->free(tr);
  }
*/
  delete[] _trackTable;
  delete[] _blockedTrackTable;

  //	delete _viaTable;
  //	delete _wireTable;
}
Ath__grid::~Ath__grid()
{
  freeTracksAndTables();
}
uint Ath__grid::getLevel()
{
  return _level;
}
uint Ath__grid::getDir()
{
  return _dir;
}
int Ath__grid::getTrackHeight(uint track)
{
  return _base + track * _pitch;
}
Ath__grid* Ath__track::getGrid()
{
  return _grid;
}
uint Ath__track::removeMarkedNetWires()
{
  uint cnt = 0;
  Ath__wire *wire, *pwire, *nwire;
  for (uint jj = 0; jj < _markerCnt; jj++) {
    pwire = NULL;
    wire = _marker[jj];
    _marker[jj] = NULL;
    for (; wire != NULL; wire = nwire) {
      nwire = wire->_next;
      uint kk = 0;
      if (wire->isPower() || !wire->getNet()->isMarked()) {
        if (wire->isPower() || wire->getNet()->getWire() != NULL) {
          pwire = wire;
          if (_marker[jj] == NULL)
            _marker[jj] = wire;
          continue;
        } else
          kk = 0;  // bp
      }
      if (pwire)
        pwire->_next = nwire;
      for (kk = jj + 1; kk < _markerCnt; kk++) {
        if (_eMarker[kk] == wire)
          _eMarker[kk] = nwire;
      }
      _grid->getWirePoolPtr()->free(wire);
      cnt++;
    }
  }
  return cnt;
}
uint Ath__grid::defaultWireType()
{
  return _wireType;
}
void Ath__grid::setDefaultWireType(uint v)
{
  _wireType = v;  // TODO-OPTIMIZE : can it be 32-bit?
}
uint Ath__grid::getBoxes(uint trackIndex, Ath__array1D<uint>* table)
{
  Ath__track* tr = _trackTable[trackIndex];
  if (tr == NULL)
    return 0;
  if (_blockedTrackTable[trackIndex] > 0)
    return 0;

  for (uint k = 0; k < tr->_markerCnt; k++) {
    if (tr->_marker[k] == NULL)
      continue;
    for (Ath__wire* w = tr->_marker[k]; w != NULL; w = w->_next) {
      table->add(w->_boxId);
    }
  }
  return table->getCnt();
}
void Ath__grid::getBoxes(Ath__array1D<uint>* table)
{
  for (uint ii = 0; ii < _trackCnt; ii++) {
    Ath__track* tr = _trackTable[ii];
    if (tr == NULL)
      continue;
    if (_blockedTrackTable[ii] > 0)
      continue;

    //		for (uint k= 0; k<4; k++) {
    for (Ath__wire* w = tr->_marker[0]; w != NULL; w = w->_next) {
      table->add(w->_boxId);
    }
    //		}
  }
}

uint Ath__grid::blockTracks(dbBlock* block, Ath__array1D<uint>* idTable)
{
  for (uint ii = 0; ii < idTable->getCnt(); ii++) {
    uint boxId = idTable->get(ii);

    dbBox* bb = dbBox::getBox(block, boxId);

    dbNet* net = (dbNet*) bb->getBoxOwner();

    dbSigType type = net->getSigType();

    if (!((type == dbSigType::POWER) || (type == dbSigType::GROUND)))
      continue;

    Ath__searchBox sbb(
        bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax(), _level, _dir);

    blockTracks(&sbb);
  }
  return setFreeTracks();
}
uint Ath__grid::setFreeTracks()
{
  uint cnt = 0;
  for (uint ii = 0; ii < _trackCnt; ii++)
    cnt += _blockedTrackTable[ii];

  _freeTrackCnt = _trackCnt - cnt;

  return _freeTrackCnt;
}
uint Ath__grid::blockTracks(Ath__searchBox* box, bool ignoreLevel)
{
  if ((ignoreLevel) && (box->getLevel() != _level))
    return 0;

  uint track1 = getMinMaxTrackNum(box->loXY(_dir));
  uint track2 = getMinMaxTrackNum(box->hiXY(_dir));

  for (uint ii = track1; ii <= track2; ii++)
    _blockedTrackTable[ii] = 1;

  return track2 - track1 + 1;
}
uint Ath__grid::blockTracks(Ath__box* box, uint ignoreLevel)
{
  if ((ignoreLevel == 0) && (box->_layer != _level))
    return 0;

  int ll[2] = {box->_xlo, box->_ylo};
  int ur[2] = {box->_xhi, box->_yhi};

  uint track1 = getMinMaxTrackNum(ll[_dir]);
  uint track2 = getMinMaxTrackNum(ur[_dir]);

  for (uint ii = track1; ii <= track2; ii++)
    _blockedTrackTable[ii] = 1;

  return track2 - track1 + 1;
}
bool Ath__grid::addOnTrack(uint track, Ath__wire* w, uint mark1, uint mark2)
{
  if (_blockedTrackTable[track] > 0)
    return false;

  Ath__track* tr = NULL;
  if (_trackTable[track] == NULL) {
    tr = addTrack(track, _markerCnt);
    _trackTable[track] = tr;
    if (tr->place(w, mark1, mark2)) {
      w->_track = tr;
      return true;
    } else
      return false;
  }
  if (!_trackTable[track]->overlapCheck(w, mark1, mark2)) {
    // return _trackTable[track]->place(w, mark1, mark2);
    tr = _trackTable[track];
    if (tr->place(w, mark1, mark2)) {
      w->_track = tr;
      return true;
    } else
      return false;
  }
  return false;
}
uint Ath__grid::addWireCut(uint cutFlag,
                           uint initTrack,
                           Ath__box* box,
                           int sortedOrder,
                           int* height)
{
  uint id, markIndex1, markIndex2;

  Ath__wire* w = makeWireCut(box, &id, &markIndex1, &markIndex2, cutFlag);

  _trackFilledCnt++;

  return placeWire(initTrack, w, markIndex1, markIndex2, sortedOrder, height);
}
uint Ath__grid::addWireExt(uint cutFlag,
                           uint initTrack,
                           Ath__box* box,
                           int sortedOrder,
                           int* height)
{
  uint id, markIndex1, markIndex2;

  Ath__wire* w
      = makeWireExt(box, &id, &markIndex1, &markIndex2, cutFlag, *height);

  _trackFilledCnt++;
  return placeWire(initTrack, w, markIndex1, markIndex2, sortedOrder, height);
}
uint Ath__grid::placeWire(uint initTrack,
                          Ath__wire* w,
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
      if (status)
        break;
    }
    nextTrack = track + 1;
  } else {
    for (; (!status) && (track > 0) && (track > initTrack - check); track--) {
      status = addOnTrack(track, w, mark1, mark2);
      if (status)
        break;
    }
    nextTrack = track - 1;
  }
  if (!status) {
    if (_ignoreFlag > 0) {
      *height = getTrackHeight(_trackCnt - 1);
      return _trackCnt - 1;
    } else {
      fprintf(stdout, "Cannot place at track# %d\n", initTrack);
      *height = getTrackHeight(initTrack);
      return initTrack;
    }
  }
  *height = getTrackHeight(track);
  return nextTrack;
}
void Ath__grid::setIgnoreFlag(uint v)
{
  _ignoreFlag = v;
}
uint Ath__grid::addWireList(Ath__box* list)
{
  uint cnt = 0;
  for (Ath__box* e = list; e != NULL; e = e->_next) {
    if (e->_layer != _level)
      continue;
    if (e->getDir() != _dir)
      continue;

    uint initTrack = getTrackNum(e);
    int height;

    // uint n= getTrackNum(e);
    // uint track= addWire(initTrack, e, 1, &height);
    addWire(initTrack, e, 1, &height);
    cnt++;
  }
  return cnt;
}
uint Ath__grid::addWireNext(uint initTrack,
                            Ath__box* box,
                            int sortedOrder,
                            int* height)
{
  uint id, markIndex1, markIndex2;
  Ath__wire* w = makeWire(box, &id, &markIndex1, &markIndex2, 0);
  uint bestTrack = getTrackNum(box);

  if (initTrack < bestTrack)
    initTrack = bestTrack;

  return placeWire(initTrack, w, markIndex1, markIndex2, sortedOrder, height);
}
uint Ath__grid::addWire(uint initTrack,
                        Ath__box* box,
                        int sortedOrder,
                        int* height)
{
  uint id, markIndex1, markIndex2;
  Ath__wire* w = makeWire(box, &id, &markIndex1, &markIndex2, 0);

  return placeWire(initTrack, w, markIndex1, markIndex2, sortedOrder, height);
}
Ath__track* Ath__grid::getTrackPtr(int xy)
{
  uint trackNum = getMinMaxTrackNum(xy);

  return getTrackPtr(trackNum, _markerCnt);
}
Ath__track* Ath__grid::getTrackPtr(int* ll)
{
  uint trackNum = getMinMaxTrackNum(ll[_dir]);

  return getTrackPtr(trackNum, _markerCnt);
}
int Ath__grid::placeTileWire(uint /* unused: wireType */,
                             uint /* unused: id */,
                             Ath__searchBox* /* unused: bb */,
                             int /* unused: loMarker */,
                             int /* unused: initTrack */,
                             bool /* unused: skipResetSize */)
{
  fprintf(stdout, "TODO: adjust to new makeWire API\n");
  /*
  uint maxRange= 30;

  uint d= ! _dir;

  int loXY= bb->loXY(d, _start);
  int hiXY= bb->hiXY(d, _end);

  int ll[2]= { x1, y1 };
  int ur[2]= { x2, y2 };

  Ath__wire *w= makeWire(loXY, hiXY, id, wireType);

  if (loMarker<0)
  {
          loMarker= getBucketNum(loXY);
          uint loMarker2= getBucketNum(hiXY);
          if (loMarker2>loMarker)
          {
                  if (loXY-_start>_end-hiXY)
                          loMarker= loMarker2;
          }
  }

  if (initTrack<0)
          initTrack= getMinMaxTrackNum(bb->loXY(_dir));

  Ath__track *track;
  int trackNum;

  uint ii= 0;
  for ( ; ii<maxRange; ii++) {

          trackNum= initTrack+ii;
          track= getTrackPtr(trackNum, _markerCnt);
          if ((track!=NULL)&&(track->checkAndplacerOnMarker(w, loMarker)))
                  break;

          trackNum= initTrack-ii;
          if (trackNum<0)
                  continue;

          track= getTrackPtr(trackNum, _markerCnt);
          if ((track!=NULL)&&(track->checkAndplacerOnMarker(w, loMarker)))
                  break;
  }
  if (ii==maxRange)
          return -1;

  int h= getTrackHeight(trackNum);
  bb->setLo(_dir, h);
  bb->setHi(_dir, h+_pitch/2);
  bb->setOwnerId(w->_id);
  if ( ! skipResetSize)
  {
          if (wireType==2) //bus box
          {
      // clipping
                  bb->setLo(d, loXY);  // was _start
                  bb->setHi(d, hiXY);  // was _end
          }
          else // pin
          {
                  if (loMarker>0) // src Pin at end of grid
                          bb->setLo(d, _end - 3*_pitch/2);
                  else
                          bb->setHi(d, _start + 3*_pitch/2);
          }
  }
  return trackNum;
  */
  return 0;
}
uint Ath__grid::placeBox(uint id, int x1, int y1, int x2, int y2)
{
  int ll[2] = {x1, y1};
  int ur[2] = {x2, y2};

  uint d = (_dir > 0) ? 0 : 1;

  int xy1 = ll[d];

  uint m1 = getBucketNum(xy1);
  int width = ur[_dir] - ll[_dir];

  uint trackNum1 = getMinMaxTrackNum(ll[_dir]);
  uint trackNum2 = trackNum1;
  if (width > _pitch)
    trackNum2 = getMinMaxTrackNum(ur[_dir]);

  for (uint ii = trackNum1; ii <= trackNum2; ii++) {
    Ath__wire* w = makeWire(_dir, ll, ur, id, 0);

    Ath__track* track = getTrackPtr(ii, _markerCnt);

    if (track->place(w, m1)) {
      w->_track = track;
    } else {
      fprintf(stdout, "OVERLAP placement\n");
    }
  }

  return trackNum1;
}
/* NEWWIRE
        uint d= ! _dir;

        uint loTrackNum= getTrackNum1(bb->loXY(_dir));
        if (loTrackNum>0)
                loTrackNum --;

        uint hiTrackNum= getTrackNum1(bb->hiXY(_dir));

        int loXY= bb->loXY(d);
        int hiXY= bb->hiXY(d);
        uint loMarker= getBucketNum(loXY);
        uint hiMarker= getBucketNum(hiXY);
*/
void Ath__wire::setXY(int xy1, uint len)
{
  _xy = xy1;  // offset from track start??
  _len = len;
}
Ath__wire* Ath__wire::makeCoupleWire(AthPool<Ath__wire>* wirePool,
                                     int targetHighTracks,
                                     Ath__wire* w2,
                                     int xy1,
                                     uint len,
                                     uint /* unused: wtype */)
{
  int dist;
  if (targetHighTracks)
    dist = w2->_base - (_base + _width);
  else
    dist = _base - (w2->_base + w2->_width);
  if (dist <= 0)
    return NULL;

  Ath__wire* w = getPoolWire(wirePool);
  w->_srcId = 0;

  w->reset();

  w->_xy = xy1;  // offset from track start??
  w->_len = len;

  w->_width = dist;
  w->_boxId = _id;
  w->_otherId = w2->_id;
  w->_flags = _flags;
  w->_dir = _dir;
  if (targetHighTracks)
    w->_base = _base + _width;  // small dimension
  else
    w->_base = w2->_base + w2->_width;
  return w;
}
Ath__wire* Ath__wire::getPoolWire(AthPool<Ath__wire>* wirePool)
{
  uint n;
  uint getRecycleFlag = 0;
  Ath__wire* w = wirePool->alloc(&getRecycleFlag, &n);
  if (getRecycleFlag == 0)
    w->_id = n;
  return w;
}
Ath__wire* Ath__wire::makeWire(AthPool<Ath__wire>* wirePool, int xy1, uint len)
{
  Ath__wire* w = getPoolWire(wirePool);

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
Ath__wire* Ath__grid::getPoolWire()
{
  uint n;
  uint getRecycleFlag = 0;
  Ath__wire* w = _wirePoolPtr->alloc(&getRecycleFlag, &n);
  if (getRecycleFlag == 0)
    w->_id = n;
  return w;
}
Ath__wire* Ath__grid::makeWire(Ath__wire* v, uint type)
{
  Ath__wire* w = getPoolWire();
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

uint Ath__grid::placeWire(Ath__searchBox* bb)
{
  uint d = !_dir;

  int xy1 = bb->loXY(d);

  int ll[2] = {bb->loXY(0), bb->loXY(1)};
  int ur[2] = {bb->hiXY(0), bb->hiXY(1)};

  uint m1 = getBucketNum(xy1);

#ifdef SINGLE_WIRE
  uint width = bb->hiXY(_dir) - bb->loXY(_dir);
  // uint trackNum1= getMinMaxTrackNum(bb->loXY(_dir));
  uint trackNum1 = getMinMaxTrackNum((bb->loXY(_dir) + bb->loXY(_dir)) / 2);
  uint trackNum2 = trackNum1;
  if (width > _pitch)
    trackNum2 = getMinMaxTrackNum(bb->hiXY(_dir));
    // ** wire base is not always at track base
#else
  uint trackNum1 = getMinMaxTrackNum(bb->loXY(_dir));
  uint trackNum2 = getMinMaxTrackNum(bb->hiXY(_dir));
#endif

  uint wireType = bb->getType();

  Ath__wire* w
      = makeWire(_dir, ll, ur, bb->getOwnerId(), bb->getOtherId(), wireType);
  Ath__track* track;
  int TTTsubt = 1;
  if (TTTsubt)
    track = getTrackPtr(trackNum1, _markerCnt, w->_base);
  else
    track = getTrackPtr(trackNum1, _markerCnt);
  // track->place2(w, m1, m2);
  track->place(w, m1);
  uint wCnt = 1;
  for (uint ii = trackNum1 + 1; ii <= trackNum2; ii++) {
    Ath__wire* w1 = makeWire(w, wireType);
    w1->_srcId = w->_id;
    w1->_srcWire = w;
    _gridtable->incrMultiTrackWireCnt(w->isPower());
    Ath__track* track = getTrackPtr(ii, _markerCnt);
    // track->place2(w1, m1, m2);
    track->place(w1, m1);
    wCnt++;
  }

  return trackNum1;
}
uint Ath__grid::placeWire(Ath__wire* w)
{
  uint m1 = getBucketNum(w->_xy);

  uint trackNum1 = getMinMaxTrackNum(w->_base);
  uint trackNum2 = getMinMaxTrackNum(w->_base + w->_width);

  Ath__track* track = getTrackPtr(trackNum1, _markerCnt);
  track->place(w, m1);

  for (uint ii = trackNum1 + 1; ii <= trackNum2; ii++) {
    Ath__wire* w1 = makeWire(w, w->_flags);
    w1->_srcId = w->_id;
    _gridtable->incrMultiTrackWireCnt(w->isPower());
    Ath__track* track = getTrackPtr(ii, _markerCnt);
    track->place(w1, m1);
  }

  return trackNum1;
}
uint Ath__grid::placeBox(dbBox* box, uint wtype, uint id)
{
  int ll[2] = {box->xMin(), box->yMin()};
  int ur[2] = {box->xMax(), box->yMax()};

  uint d = (_dir > 0) ? 0 : 1;

  int xy1 = ll[d];

  uint m1 = getBucketNum(xy1);

  int width = ur[_dir] - ll[_dir];

  uint trackNum1 = getMinMaxTrackNum(ll[_dir]);
  uint trackNum2 = trackNum1;
  if (width > _pitch)
    trackNum2 = getMinMaxTrackNum(ur[_dir]);

  if (id == 0)
    id = box->getId();
  Ath__wire* w = makeWire(_dir, ll, ur, id, 0, wtype);
  Ath__track* track = getTrackPtr(trackNum1, _markerCnt);
  track->place(w, m1);

  for (uint ii = trackNum1 + 1; ii <= trackNum2; ii++) {
    Ath__wire* w1 = makeWire(w);
    w1->_srcId = w->_id;
    _gridtable->incrMultiTrackWireCnt(w->isPower());
    Ath__track* track = getTrackPtr(ii, _markerCnt);
    track->place(w1, m1);
  }
  return trackNum1;
}
uint Ath__grid::setExtrusionMarker()
{
  Ath__track *track, *tstr;
  uint cnt = 0;
  for (uint ii = 0; ii < _trackCnt; ii++) {
    track = _trackTable[ii];
    if (track == NULL)
      continue;
    tstr = NULL;
    bool tohi = true;
    while ((tstr = track->getNextSubTrack(tstr, tohi)) != nullptr)
      cnt += tstr->setExtrusionMarker(_markerCnt, _start, _markerLen);
  }
  return cnt;
}
uint Ath__grid::placeBox(Ath__box* box)
{
  int ll[2] = {box->_xlo, box->_ylo};
  int ur[2] = {box->_xhi, box->_yhi};

  uint markIndex1;
  Ath__wire* w = makeWire(ll, ur, box->getOwner(), &markIndex1);

  Ath__track* track = getTrackPtr(ll);

  if (!track->place(w, markIndex1))
    fprintf(stdout, "OVERLAP placement\n");
  else
    w->_track = track;

  return track->_num;
}
Ath__wire* Ath__grid::getWirePtr(uint wireId)
{
  return _wirePoolPtr->get(wireId);
}
void Ath__grid::getBoxIds(Ath__array1D<uint>* wireIdTable,
                          Ath__array1D<uint>* idtable)
{
  // remove duplicate entries

  for (uint ii = 0; ii < wireIdTable->getCnt(); ii++) {
    uint wid = wireIdTable->get(ii);
    Ath__wire* w = getWirePtr(wid);

    uint boxId = w->_boxId;
    if (w->_srcId > 0) {
      w = getWirePtr(w->_srcId);
      boxId = w->_boxId;
    }
    if (w->_ext > 0)
      continue;

    w->_ext = 1;
    idtable->add(boxId);
  }

  for (uint jj = 0; jj < wireIdTable->getCnt(); jj++) {
    Ath__wire* w = getWirePtr(wireIdTable->get(jj));
    w->_ext = 0;

    if (w->_srcId > 0) {
      w = getWirePtr(w->_srcId);
      w->_ext = 0;
    }
  }
}
void Ath__grid::getWireIds(Ath__array1D<uint>* wireIdTable,
                           Ath__array1D<uint>* idtable)
{
  // remove duplicate entries

  for (uint ii = 0; ii < wireIdTable->getCnt(); ii++) {
    uint wid = wireIdTable->get(ii);
    Ath__wire* w = getWirePtr(wid);

    if (w->_srcId > 0) {
      w = getWirePtr(w->_srcId);
      wid = w->_id;
    }
    if (w->_ext > 0)
      continue;

    w->_ext = 1;
    idtable->add(wid);
  }

  for (uint jj = 0; jj < wireIdTable->getCnt(); jj++) {
    Ath__wire* w = getWirePtr(wireIdTable->get(jj));
    w->_ext = 0;
    if (w->_srcId > 0) {
      w = getWirePtr(w->_srcId);
      w->_ext = 0;
    }
  }
}
uint Ath__grid::search(Ath__searchBox* bb,
                       Ath__array1D<uint>* idtable,
                       bool wireIdFlag)
{
  Ath__array1D<uint> wireIdTable(16000);

  // uint d= (_dir>0) ? 0 : 1;
  uint d = !_dir;

  uint loTrackNum = getTrackNum1(bb->loXY(_dir));
  if (loTrackNum > 0)
    loTrackNum--;

  uint hiTrackNum = getTrackNum1(bb->hiXY(_dir));

  int loXY = bb->loXY(d);
  int hiXY = bb->hiXY(d);
  uint loMarker = getBucketNum(loXY);
  uint hiMarker = getBucketNum(hiXY);

  Ath__track* tstrack;
  uint cnt = 0;
  for (uint ii = loTrackNum; ii <= hiTrackNum; ii++) {
    Ath__track* track = _trackTable[ii];
    if (track == NULL)
      continue;

    tstrack = NULL;
    bool tohi = true;
    while ((tstrack = track->getNextSubTrack(tstrack, tohi)) != nullptr) {
      if (_schema > 0)
        cnt += tstrack->search1(loXY, hiXY, loMarker, hiMarker, &wireIdTable);
      else
        cnt += tstrack->search(loXY, hiXY, loMarker, hiMarker, idtable);
    }
  }
  if (wireIdFlag)
    getWireIds(&wireIdTable, idtable);
  else
    getBoxIds(&wireIdTable, idtable);

  return idtable->getCnt();
}
uint Ath__grid::search(Ath__searchBox* bb,
                       uint* gxy,
                       Ath__array1D<uint>* idtable,
                       Ath__grid* g)
{
  Ath__array1D<uint> wireIdTable(1024);

  AthPool<Ath__wire>* wirePool = _wirePoolPtr;
  if (g != NULL)
    wirePool = g->getWirePoolPtr();

  uint d = !_dir;

  uint loTrackNum = getTrackNum1(bb->loXY(_dir));
  if (loTrackNum > 0)
    loTrackNum--;

  uint hiTrackNum = getTrackNum1(bb->hiXY(_dir));

  int loXY = bb->loXY(d);
  int hiXY = bb->hiXY(d);
  uint loMarker = getBucketNum(loXY);
  uint hiMarker = getBucketNum(hiXY);

  uint cnt = 0;
  for (uint ii = loTrackNum; ii <= hiTrackNum; ii++) {
    Ath__track* track = _trackTable[ii];
    if (track == NULL)
      continue;

    wireIdTable.resetCnt();
    uint cnt1 = track->search1(loXY, hiXY, loMarker, hiMarker, &wireIdTable);
    if (cnt1 <= 0)
      continue;

    cnt += cnt1;

    Ath__wire* w0 = _wirePoolPtr->get(wireIdTable.get(0));
    Ath__wire* w1 = w0->makeWire(wirePool, w0->_xy, w0->_len);

    if (g != NULL)
      g->placeWire(w1);
    idtable->add(w1->_id);

    for (uint jj = 1; jj < cnt1; jj++) {
      Ath__wire* w = _wirePoolPtr->get(wireIdTable.get(jj));

      uint dist = w->_xy - (w1->_xy + w1->_len);
      if (dist <= gxy[d]) {
        w1->setXY(w1->_xy, w->_xy + w->_len - w1->_xy);
      } else  // start new
      {
        w1 = w0->makeWire(wirePool, w->_xy, w->_len);

        if (g != NULL)
          g->placeWire(w1);
        idtable->add(w1->_id);
      }
    }
  }

  //	fprintf(stdout, "Grid %d - dir %d : %d wires were reduced %d\n", _level,
  //_dir, 		cnt, idtable->getCnt()-reducedCnt);

  return idtable->getCnt();
}
Ath__wire* Ath__grid::makeWhite(uint dir,
                                int* lo,
                                int* hi,
                                Ath__array1D<uint>* idtable,
                                Ath__grid* g)
{
  Ath__wire* w1 = NULL;
  if (g != NULL) {
    w1 = g->makeWire(dir, lo, hi, 0, 0, 0);
    g->placeWire(w1);
  } else {
    w1 = makeWire(dir, lo, hi, 0, 0, 0);
  }

  idtable->add(w1->_id);
  return w1;
}
uint Ath__grid::white(Ath__searchBox* bb,
                      Ath__array1D<uint>* idtable,
                      Ath__grid* g)
{
  Ath__array1D<uint> wireIdTable(1024);

  // AthPool<Ath__wire>* wirePool = _wirePoolPtr;
  // if (g != NULL)
  //   wirePool = g->getWirePoolPtr();

  int lo[2] = {_lo[0], _lo[1]};
  int hi[2] = {_hi[0], _hi[1]};

  uint d = !_dir;

  uint loTrackNum = getTrackNum1(bb->loXY(_dir));
  if (loTrackNum > 0)
    loTrackNum--;

  uint hiTrackNum = getTrackNum1(bb->hiXY(_dir));

  int loXY = bb->loXY(d);
  int hiXY = bb->hiXY(d);
  uint loMarker = getBucketNum(loXY);
  uint hiMarker = getBucketNum(hiXY);

  lo[d] = loXY;
  hi[d] = hiXY;
  lo[_dir] = bb->loXY(_dir);
  hi[_dir] = bb->hiXY(_dir);

  uint cnt = 0;
  for (uint ii = loTrackNum; ii <= hiTrackNum; ii++) {
    Ath__track* track = _trackTable[ii];
    if (track == NULL)
      continue;

    wireIdTable.resetCnt();
    uint cnt1 = track->search1(loXY, hiXY, loMarker, hiMarker, &wireIdTable);
    if (cnt1 <= 0) {
      lo[_dir] = _base + ii * _pitch;
      hi[_dir] = lo[_dir] + _pitch;

      Ath__wire* w1 = makeWhite(_dir, lo, hi, idtable, g);
      w1->_track = track;
      continue;
    }

    cnt += cnt1;

    Ath__wire* w = _wirePoolPtr->get(wireIdTable.get(0));

    lo[d] = loXY;
    hi[d] = w->_xy;
    lo[_dir] = w->_base;
    hi[_dir] = w->_base + w->_width;

    if (hi[d] - lo[d] > 100) {
      Ath__wire* w1 = makeWhite(_dir, lo, hi, idtable, g);
      w1->_track = track;
    }
    lo[d] += w->_len;
    for (uint jj = 1; jj < cnt1; jj++) {
      w = _wirePoolPtr->get(wireIdTable.get(jj));
      hi[d] = w->_xy;

      if (hi[d] - lo[d] > 100) {
        lo[_dir] = w->_base;
        hi[_dir] = w->_base + w->_width;

        Ath__wire* w1 = makeWhite(_dir, lo, hi, idtable, g);
        w1->_track = track;
        lo[d] = w->_xy + w->_len;
      }
    }
    hi[d] = hiXY;
    if (hi[d] - lo[d] > 100) {
      Ath__wire* w1 = makeWhite(_dir, lo, hi, idtable, g);
      w1->_track = track;
    }
  }
  /*
  uint whiteCnt= idtable->getCnt()-reducedCnt;

    fprintf(stdout, "%d wires were reduced %d\n", _level, _dir,
    cnt, idtable->getCnt()-reducedCnt);
  */
  return idtable->getCnt();
}
uint Ath__grid::searchAllMarkers(Ath__searchBox* bb,
                                 Ath__array1D<uint>* idtable,
                                 bool wireIdFlag)
{
  Ath__array1D<uint> wireIdTable(16000);

  // uint d= (_dir>0) ? 0 : 1;
  uint d = !_dir;

  uint loTrackNum = getTrackNum1(bb->loXY(_dir));
  if (loTrackNum > 0)
    loTrackNum--;

  uint hiTrackNum = getTrackNum1(bb->hiXY(_dir));

  int loXY = bb->loXY(d);
  int hiXY = bb->hiXY(d);
  uint loMarker = 0;
  uint hiMarker = _markerCnt - 1;
  ;

  uint cnt = 0;
  for (uint ii = loTrackNum; ii <= hiTrackNum; ii++) {
    Ath__track* track = _trackTable[ii];
    if (track == NULL)
      continue;

    if (_schema > 0)
      cnt += track->search1(loXY, hiXY, loMarker, hiMarker, &wireIdTable);
    else
      cnt += track->search(loXY, hiXY, loMarker, hiMarker, idtable);
  }
  if (wireIdFlag)
    getWireIds(&wireIdTable, idtable);
  else
    getBoxIds(&wireIdTable, idtable);

  return idtable->getCnt();
}
void Ath__grid::getBuses(Ath__array1D<Ath__box*>* boxTable, uint width)
{
  Ath__array1D<Ath__wire*> wireTable(32);

  for (uint ii = 0; ii < _trackCnt; ii++) {
    if (_blockedTrackTable[ii] > 0)
      continue;

    Ath__track* track = _trackTable[ii];
    if (track == NULL)
      continue;

    if (!(_schema > 0))
      continue;

    uint height = _base + ii * _pitch;

    wireTable.resetCnt();
    track->getAllWires(&wireTable, _markerCnt);

    for (uint jj = 0; jj < wireTable.getCnt(); jj++) {
      Ath__wire* e = wireTable.get(jj);
      if (!e->isTileBus())
        continue;

      Ath__box* bb = new Ath__box();
      if (_dir > 0)
        bb->set(e->_xy, height, e->_xy + e->_len, height + width);
      else
        bb->set(height, e->_xy, height + width, e->_xy + e->_len);

      bb->_layer = _level;

      boxTable->add(bb);
    }
  }
}
Ath__wire* Ath__grid::getWire_Linear(uint id)
{
  for (uint ii = 0; ii < _trackCnt; ii++) {
    Ath__track* tr = _trackTable[ii];
    if (tr == NULL)
      continue;

    Ath__wire* w = tr->getWire_Linear(_markerCnt, id);
    if (w != NULL)
      return w;
  }
  return NULL;
}
void Ath__grid::adjustOverlapMakerEnd()
{
  int TTTnewAdj = 1;
  Ath__track *track, *tstr;
  for (uint ii = 0; ii < _trackCnt; ii++) {
    track = _trackTable[ii];
    if (track == NULL)
      continue;
    tstr = NULL;
    bool tohi = true;
    while ((tstr = track->getNextSubTrack(tstr, tohi)) != nullptr)
      if (TTTnewAdj)
        tstr->adjustOverlapMakerEnd(_markerCnt, _start, _markerLen);
      else
        tstr->adjustOverlapMakerEnd(_markerCnt);
  }
}
void Ath__grid::adjustMetalFill()
{
  // int TTTnewAdj = 1;
  // bool ordered= true;
  Ath__track *track, *tstr;
  _searchLowMarker = 0;
  _searchHiMarker = _markerCnt - 1;
  for (int ii = _trackCnt - 1; ii >= 0; ii--) {
    track = _trackTable[ii];
    if (track == NULL)
      continue;
    tstr = NULL;
    bool tohi = false;
    while ((tstr = track->getNextSubTrack(tstr, tohi)) != nullptr)
      tstr->adjustMetalFill();
  }
}
bool Ath__grid::isOrdered(bool /* unused: ascending */, uint* cnt)
{
  bool ordered = true;
  for (uint ii = 0; ii < _trackCnt; ii++) {
    Ath__track* tr = _trackTable[ii];
    if (tr == NULL)
      continue;

    if (!tr->isAscendingOrdered(_markerCnt, cnt)) {
      fprintf(stdout, "Track #%d is not ordered\n", ii);
      ordered = false;
    }
  }
  return ordered;
}
uint Ath__grid::getBucketNum(int xy)
{
  int offset = xy - _start;
  if (offset < 0) {
    return 0;
  }
  uint b = offset / _markerLen;
  if (b == 0)
    return 0;

  if (b >= _markerCnt) {
    return _markerCnt - 1;
  }
  return b;
}
uint Ath__grid::getWidth()
{
  return _width;
}

int Ath__grid::getXYbyWidth(int xy, uint* mark)
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
uint Ath__grid::getTrackNum1(int xy)
{
  int a = xy - _base;

  // if (a<0)
  if (xy < _base)
    return 0;

  uint b = a / _pitch;
  if (b >= _trackCnt)
    return _trackCnt - 1;
  else
    return b;
}
uint Ath__grid::getTrackNum(int* ll, uint d, uint* marker)
{
  *marker = getBucketNum(ll[d]);

  int a = ll[_dir] - _base;

  if (a < 0)
    return 0;

  uint b = a / _pitch;
  if (b >= _trackCnt)
    return _trackCnt - 1;
  else
    return b;
}
uint Ath__grid::getTrackNum(Ath__box* box)
{
  int ll[2] = {box->_xlo, box->_ylo};

  int a = ll[_dir] - _base;

  if (a < 0)
    return 0;
  else
    return a / _pitch;
}
Ath__wire* Ath__grid::makeWire(uint dir,
                               int* ll,
                               int* ur,
                               uint id1,
                               uint id2,
                               uint type)
{
  Ath__wire* w = getPoolWire();
  w->_srcId = 0;
  w->_srcWire = NULL;

  w->reset();
  w->set(dir, ll, ur);
  w->_boxId = id1;
  w->_otherId = id2;

  w->_flags = type;

  return w;
}
Ath__wire* Ath__grid::makeWire(int* ll, int* ur, uint id, uint* m1)
{
  uint d = (_dir > 0) ? 0 : 1;

  int xy1 = ll[d];
  // int xy2= ur[d];
  *m1 = getBucketNum(xy1);

  Ath__wire* w = getPoolWire();
  w->_srcId = 0;
  w->_otherId = 0;

  w->reset();
  w->set(_dir, ll, ur);
  w->_boxId = id;

  return w;
}
Ath__wire* Ath__grid::makeWire(Ath__box* box,
                               uint* id,
                               uint* m1,
                               uint* m2,
                               uint /* unused: fullTrack */)
{
  int ll[2] = {box->_xlo, box->_ylo};
  int ur[2] = {box->_xhi, box->_yhi};

  // int d = (_dir > 0) ? 0 : 1;

  // uint xy1 = 0;
  // uint xy2 = _end / _width;
  *m1 = 0;
  *m2 = 3;
  // if (fullTrack == 0) {
  //   xy1 = getXYbyWidth(ll[d], m1);
  //   xy2 = getXYbyWidth(ur[d], m2);
  // }
  Ath__wire* w = getPoolWire();
  w->_otherId = 0;

  *id = w->_id;
  //	*id= _wireTable->add(w);
  w->reset();
  w->set(_dir, ll, ur);
  w->_boxId = box->_id;
  w->_srcId = 0;
  /*
   *m1= xy1/_markerLen;
   *m2= xy2/_markerLen;
   */
  return w;
}
Ath__wire* Ath__grid::getNewWire(Ath__box* box, int* ll, int* ur, uint* id)
{
  Ath__wire* w = getPoolWire();
  w->_otherId = 0;

  *id = w->_id;
  //	*id= _wireTable->add(w);

  w->reset();
  w->set(_dir, ll, ur);
  w->_boxId = box->_id;
  w->_srcId = 0;

  return w;
}

Ath__wire* Ath__grid::makeWireCut(Ath__box* box,
                                  uint* id,
                                  uint* m1,
                                  uint* m2,
                                  uint /* unused: cutFlag */)
{
  int ll[2] = {box->_xlo, box->_ylo};
  int ur[2] = {box->_xhi, box->_yhi};

  // int d= (_dir>0) ? 0 : 1;

  // int xy1 = 0;
  // int xy2 = _end / _width;
  *m1 = 0;
  *m2 = 3;
  // if (cutFlag == 0)  // cut Low
  //   xy1 = getXYbyWidth(_start + _trackFilledCnt * _width, m1);
  // else
  //   xy2 = getXYbyWidth(_end - _trackFilledCnt * _width, m2);

  return getNewWire(box, ll, ur, id);
}
Ath__wire* Ath__grid::makeWireExt(Ath__box* box,
                                  uint* id,
                                  uint* m1,
                                  uint* m2,
                                  uint /* unused: extFlag */,
                                  int /* unused: height */)
{
  int ll[2] = {box->_xlo, box->_ylo};
  int ur[2] = {box->_xhi, box->_yhi};

  // int d= (_dir>0) ? 0 : 1;

  // int xy1 = 0;
  // int xy2 = _end / _width;
  *m1 = 0;
  *m2 = 3;
  // if (extFlag == 0)  // cut Low
  //   xy1 = getXYbyWidth(height, m1);
  // else
  //   xy2 = getXYbyWidth(height, m2);

  return getNewWire(box, ll, ur, id);
}
uint Ath__grid::getFirstTrack(uint divider)
{
  int xy = _base + (_max - _base) / divider;

  return getAbsTrackNum(xy);
}
int Ath__grid::getClosestTrackCoord(int xy)
{
  int track1 = getAbsTrackNum(xy);
  int ii;
  for (ii = track1 - 1; ii < (int) _trackCnt; ii++) {
    if (_trackTable[ii] != NULL)
      break;
  }
  int h1 = _max;
  if (ii < (int) _trackCnt)
    h1 = getTrackHeight(ii);

  for (ii = track1 + 1; ii >= 0; ii--) {
    if (_trackTable[ii] != NULL)
      break;
  }
  int h2 = _base;
  if (ii > 0)
    h2 = getTrackHeight(ii);

  if (xy - h2 < h1 - xy)
    return h2 + _width / 2;
  else
    return h1 + _width / 2;
}
int Ath__grid::findEmptyTrack(int ll[2], int ur[2])
{
  uint track1 = getAbsTrackNum(ll[_dir]);
  uint track2 = getAbsTrackNum(ur[_dir]);
  uint cnt = 0;
  for (uint ii = track1; ii <= track2; ii++) {
    if (_trackTable[ii] == NULL) {
      cnt++;
      continue;
    }
    Ath__wire w;
    w.reset();

    int xy1 = (ll[_dir % 1] - _start) / _width;
    int xy2 = (ur[_dir % 1] - _start) / _width;

    w.set(_dir, ll, ur);

    int markIndex1 = xy1 / _markerLen;
    int markIndex2 = xy2 / _markerLen;

    if (_trackTable[ii]->overlapCheck(&w, markIndex1, markIndex2))
      continue;
    cnt++;
  }
  if (cnt == track2 - track1 + 1)
    return track1;

  return -1;
}

bool Ath__intersect(int X1, int DX, int x1, int dx, int* ix1, int* ix2)
{
  //	fprintf(stdout, "%d %d   %d %d  : ", X1, DX,   x1, dx);

  *ix1 = X1;
  *ix2 = X1 + DX;

  int dx1 = X1 - x1;
  if (dx1 >= 0) {  // on left side
    int dlen = dx - dx1;
    if (dlen <= 0)
      return false;

    if (dlen < DX)
      *ix2 = x1 + dx;
  } else {
    *ix1 = x1;
    if (dx1 + DX <= 0)  // outside right side
      return false;

    if (*ix2 > x1 + dx)
      *ix2 = x1 + dx;
  }
  return true;
}
Ath__gridTile::Ath__gridTile(uint levelCnt,
                             int x1,
                             int y1,
                             int x2,
                             int y2,
                             AthPool<Ath__track>* trackPoolPtr,
                             AthPool<Ath__wire>* wirePoolPtr)
{
  assert(levelCnt > 0);
  _levelCnt = levelCnt;

  _bb.reset(x1, y1, x2, y2);

  _gTable = new Ath__grid*[levelCnt];
  for (uint ii = 0; ii < levelCnt; ii++)
    _gTable[ii] = NULL;

  _poolFlag = true;
  if (trackPoolPtr != NULL) {
    _trackPool = trackPoolPtr;
    _wirePool = wirePoolPtr;
    _poolFlag = false;
  } else {
    _trackPool = new AthPool<Ath__track>(false, 512);
    _wirePool = new AthPool<Ath__wire>(false, 512);
  }
}
Ath__gridTile::~Ath__gridTile()
{
  for (uint ii = 1; ii < _levelCnt; ii++) {
    if (_gTable[ii] != NULL)
      delete _gTable[ii];
  }
  delete[] _gTable;

  if (_poolFlag) {
    delete _trackPool;
    delete _wirePool;
  }
}
Ath__grid* Ath__gridTile::getGrid(uint level)
{
  return _gTable[level];
}
void Ath__gridTile::addGrid(Ath__grid* g)
{
  Ath__searchBox sbb;
  g->getBbox(&sbb);

  uint level = sbb.getLevel();
  assert(level < _levelCnt);
  _gTable[level] = g;
}
Ath__grid* Ath__gridTile::addGrid(Ath__box* bb,
                                  uint level,
                                  uint dir,
                                  uint layerNum,
                                  uint width,
                                  uint pitch)
{
  assert(level < _levelCnt);
  _gTable[level] = new Ath__grid(
      NULL, _trackPool, _wirePool, bb, level, dir, layerNum, width, pitch);

  return _gTable[level];
}
void Ath__gridTile::searchWires(Ath__searchBox* bb, Ath__array1D<uint>* idtable)
{
  for (uint level = 1; level < _levelCnt; level++) {
    _gTable[level]->searchAllMarkers(bb, idtable, true);
  }
}

void Ath__gridTile::getBuses(Ath__array1D<Ath__box*>* boxTable, dbTech* tech)
{
  for (uint level = 1; level < _levelCnt; level++) {
    uint width = tech->findRoutingLayer(level)->getWidth();

    _gTable[level]->getBuses(boxTable, width);
  }
}

void Ath__gridTile::getBounds(int* x1, int* y1, int* x2, int* y2)
{
  *x1 = _bb.xMin();
  *y1 = _bb.yMin();
  *x2 = _bb.xMax();
  *y2 = _bb.yMax();
}

void Ath__gridTable::init1(uint memChunk,
                           uint rowSize,
                           uint colSize,
                           uint dx,
                           uint dy)
{
  _trackPool = new AthPool<Ath__track>(false, memChunk);
  _wirePool = new AthPool<Ath__wire>(false, memChunk * 1000);

  _wirePool->alloc();  // so all wire ids>0

  _rowSize = rowSize;
  _colSize = colSize;
  _rowCnt = dy / rowSize + 1;
  _colCnt = dx / colSize + 1;

  _wireCnt = 0;
  resetMaxArea();
}

Ath__gridTable::Ath__gridTable(Ath__box* bb,
                               uint rowSize,
                               uint colSize,
                               uint layer,
                               uint dir,
                               uint width,
                               uint pitch)
{
  init1(1024, rowSize, colSize, bb->getDX(), bb->getDY());
  _bbox.set(bb);
  _schema = 0;
  _offlineOverlapTouch = 0;
  _offlineOverlapCnt = 0;
  _overlapTouchCheck = 1;
  _noPowerSource = 0;
  _noPowerTarget = 0;
  _CCshorts = 0;
  _CCtargetHighTracks = 1;
  _targetTrackReversed = false;
  _ccContextDepth = 0;
  _ccContextArray = NULL;
  _allNet = true;
  _useDbSdb = true;
  _overlapAdjust = Z_noAdjust;
  _powerMultiTrackWire = 0;
  _signalMultiTrackWire = 0;
  _bandWire = NULL;

  _gridTable = new Ath__grid**[_rowCnt];
  int y1 = bb->_ylo;
  for (uint ii = 0; ii < _rowCnt; ii++) {
    _gridTable[ii] = new Ath__grid*[_colCnt];

    int y2 = y1 + rowSize;
    int x1 = bb->_xlo;
    for (uint jj = 0; jj < _colCnt; jj++) {
      int x2 = x1 + colSize;
      uint num = ii * 1000 + jj;

      Ath__box box;
      box.set(x1, y1, x2, y2);
      _gridTable[ii][jj] = new Ath__grid(
          this, _trackPool, _wirePool, &box, layer, dir, num, width, pitch, 32);

      x1 = x2;
    }
    y1 = y2;
  }
}
Ath__gridTable::Ath__gridTable(dbBox* bb,
                               uint rowSize,
                               uint colSize,
                               uint layer,
                               uint dir,
                               uint width,
                               uint pitch,
                               uint minWidth)
{
  init1(1024, rowSize, colSize, bb->getDX(), bb->getDY());
  bb->getBox(_rectBB);
  _schema = 1;
  _offlineOverlapTouch = 0;
  _offlineOverlapCnt = 0;
  _overlapTouchCheck = 1;
  _noPowerSource = 0;
  _noPowerTarget = 0;
  _CCshorts = 0;
  _CCtargetHighTracks = 1;
  _targetTrackReversed = false;
  _ccContextDepth = 0;
  _ccContextArray = NULL;
  _allNet = true;
  _useDbSdb = true;
  _overlapAdjust = Z_noAdjust;
  _powerMultiTrackWire = 0;
  _signalMultiTrackWire = 0;
  _bandWire = NULL;

  uint maxCellNumPerMarker = 16;
  uint markerCnt = (bb->getDX() / minWidth) / maxCellNumPerMarker;

  _gridTable = new Ath__grid**[_rowCnt];
  int y1 = bb->yMin();
  for (uint ii = 0; ii < _rowCnt; ii++) {
    _gridTable[ii] = new Ath__grid*[_colCnt];

    int y2 = y1 + rowSize;
    int x1 = bb->xMin();
    for (uint jj = 0; jj < _colCnt; jj++) {
      int x2 = x1 + colSize;
      uint num = ii * 1000 + jj;
      // Rect rectBB(x1, y1, x2, y2);
      _gridTable[ii][jj]
          = new Ath__grid(this, _trackPool, _wirePool, layer, num, markerCnt);

      _gridTable[ii][jj]->setTracks(dir, width, pitch, x1, y1, x2, y2);
      _gridTable[ii][jj]->setSchema(_schema);
      x1 = x2;
    }
    y1 = y2;
  }
}
void Ath__gridTable::releaseWire(uint wireId)
{
  Ath__wire* w = _wirePool->get(wireId);
  _wirePool->free(w);
}
Ath__wire* Ath__gridTable::getWirePtr(uint id)
{
  return _wirePool->get(id);
}
uint Ath__gridTable::getRowCnt()
{
  return _rowCnt;
}
uint Ath__gridTable::getColCnt()
{
  return _colCnt;
}
void Ath__gridTable::dumpTrackCounts(FILE* fp)
{
  fprintf(fp, "Multiple_track_power_wires : %d\n", _powerMultiTrackWire);
  fprintf(fp, "Multiple_track_signal_wires : %d\n", _signalMultiTrackWire);
  fprintf(fp,
          "layer  dir   alloc    live offbase  expand  tsubtn   toptk  stn\n");
  Ath__grid* tgrid;
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
      for (trn = 0; trn < (int) tgrid->_trackCnt; trn++) {
        if (tgrid->_trackTable[trn] == NULL)
          continue;
        liveCnt++;
        if (tgrid->_base + tgrid->_pitch * trn
            != tgrid->_trackTable[trn]->_base)
          offbase++;
        if (tgrid->_subTrackCnt[trn] == 0)
          continue;
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
Ath__gridTable::Ath__gridTable(Rect* bb,
                               uint rowCnt,
                               uint colCnt,
                               uint* /* unused: width */,
                               uint* pitch,
                               uint* /* unused: spacing */,
                               int* X1,
                               int* Y1)
{
  // for net wires
  init1(1024, bb->dy(), bb->dx(), bb->dx(), bb->dy());
  _rectBB.reset(bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax());
  //	bb->getBox(_rectBB);
  _rowCnt = rowCnt;
  _colCnt = colCnt;
  _schema = 1;
  _offlineOverlapTouch = 0;
  _offlineOverlapCnt = 0;
  _overlapTouchCheck = 1;
  _noPowerSource = 0;
  _noPowerTarget = 0;
  _CCshorts = 0;
  _CCtargetHighTracks = 1;
  _targetTrackReversed = false;
  _ccContextDepth = 0;
  _ccContextArray = NULL;
  _allNet = true;
  _useDbSdb = true;
  _overlapAdjust = Z_noAdjust;
  _powerMultiTrackWire = 0;
  _signalMultiTrackWire = 0;
  _bandWire = NULL;

  uint markerLen = 500000;  // EXT-DEFAULT

  // int x1= bb->xMin();
  // int y1= bb->yMin();
  int x1, y1;
  int x2 = bb->xMax();
  int y2 = bb->yMax();

  _gridTable = new Ath__grid**[_rowCnt];
  for (uint ii = 0; ii < _rowCnt; ii++) {
    _gridTable[ii] = new Ath__grid*[_colCnt];
    _gridTable[ii][0] = NULL;

    for (uint jj = 1; jj < _colCnt; jj++) {
      uint num = ii * 1000 + jj;

      _gridTable[ii][jj]
          = new Ath__grid(this, _trackPool, _wirePool, jj, num, 10);
      x1 = X1 ? X1[jj] : bb->xMin();
      y1 = Y1 ? Y1[jj] : bb->yMin();
      _gridTable[ii][jj]->setTracks(
          ii, 1, pitch[jj], x1, y1, x2, y2, markerLen);
      _gridTable[ii][jj]->setSchema(_schema);
    }
  }
}

Ath__gridTable::Ath__gridTable(Rect* bb,
                               uint layer,
                               uint dir,
                               uint width,
                               uint pitch,
                               uint minWidth)
{
  // init1(1024, bb->getDY(), bb->getDX(), bb->getDX(), bb->getDY());
  init1(1024, bb->dy(), bb->dx(), bb->dx(), bb->dy());
  _colCnt = 1;
  _rowCnt = 1;
  //	bb->getBox(_rectBB);
  _rectBB.reset(bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax());
  _schema = 1;
  _offlineOverlapTouch = 0;
  _offlineOverlapCnt = 0;
  _overlapTouchCheck = 1;
  _noPowerSource = 0;
  _noPowerTarget = 0;
  _CCshorts = 0;
  _CCtargetHighTracks = 1;
  _targetTrackReversed = false;
  _ccContextDepth = 0;
  _ccContextArray = NULL;
  _allNet = true;
  _useDbSdb = true;
  _overlapAdjust = Z_noAdjust;
  _powerMultiTrackWire = 0;
  _signalMultiTrackWire = 0;
  _bandWire = NULL;

  uint maxCellNumPerMarker = 16;
  uint markerCnt = (bb->dx() / minWidth) / maxCellNumPerMarker;
  if (markerCnt == 0)
    markerCnt = 1;

  Ath__grid* g
      = new Ath__grid(this, _trackPool, _wirePool, layer, 1, markerCnt);

  g->setTracks(
      dir, width, pitch, bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax());
  g->setSchema(_schema);

  _gridTable = new Ath__grid**[1];
  _gridTable[0] = new Ath__grid*[1];

  _gridTable[0][0] = g;
}
Ath__gridTable::~Ath__gridTable()
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
int Ath__gridTable::xMin()
{
  if (_schema > 0)
    return _rectBB.xMin();
  else
    return _bbox._xlo;
}
int Ath__gridTable::xMax()
{
  if (_schema > 0)
    return _rectBB.xMax();
  else
    return _bbox._xhi;
}
int Ath__gridTable::yMin()
{
  if (_schema > 0)
    return _rectBB.yMin();
  else
    return _bbox._ylo;
}
int Ath__gridTable::yMax()
{
  if (_schema > 0)
    return _rectBB.yMax();
  else
    return _bbox._yhi;
}
uint Ath__gridTable::getRowNum(int y)
{
  int dy = y - yMin();
  if (dy < 0)
    return 0;
  return dy / _rowSize;
}
uint Ath__gridTable::getColNum(int x)
{
  int dx = x - xMin();
  if (dx < 0)
    return 0;

  return dx / _colSize;
}
uint Ath__gridTable::white(Ath__searchBox* bb,
                           uint row,
                           uint col,
                           Ath__array1D<uint>* idTable,
                           Ath__grid* g)
{
  return _gridTable[row][col]->white(bb, idTable, g);
}
uint Ath__gridTable::search(Ath__searchBox* bb,
                            uint row,
                            uint col,
                            Ath__array1D<uint>* idTable,
                            bool wireIdFlag)
{
  return _gridTable[row][col]->search(bb, idTable, wireIdFlag);
}
uint Ath__gridTable::search(Ath__searchBox* bb,
                            uint* gxy,
                            uint row,
                            uint col,
                            Ath__array1D<uint>* idtable,
                            Ath__grid* g)
{
  return _gridTable[row][col]->search(bb, gxy, idtable, g);
}
uint Ath__gridTable::search(Ath__searchBox* bb, Ath__array1D<uint>* idTable)
{
  uint row1 = getRowNum(bb->loXY(1));
  if (row1 > 0)
    row1--;

  uint row2 = getRowNum(bb->hiXY(1));

  uint col1 = getColNum(bb->loXY(0));
  if (col1 > 0)
    col1--;

  uint col2 = getColNum(bb->hiXY(0));

  for (uint ii = row1; ii < _rowCnt && ii <= row2; ii++) {
    for (uint jj = col1; jj < _colCnt && jj <= col2; jj++) {
      // uint cnt1= _gridTable[ii][jj]->search(bb, idTable);
      _gridTable[ii][jj]->search(bb, idTable);
    }
  }
  return 0;
}
bool Ath__gridTable::getRowCol(int x1, int y1, uint* row, uint* col)
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
uint Ath__gridTable::setExtrusionMarker(uint startRow, uint startCol)
{
  uint cnt = 0;
  for (uint ii = startRow; ii < _rowCnt; ii++) {
    for (uint jj = startCol; jj < _colCnt; jj++) {
      cnt += _gridTable[ii][jj]->setExtrusionMarker();
    }
  }
  return cnt;
}
AthPool<Ath__wire>* Ath__grid::getWirePoolPtr()
{
  return _wirePoolPtr;
}

uint Ath__grid::removeMarkedNetWires()
{
  uint cnt = 0;
  for (uint ii = 0; ii < _trackCnt; ii++) {
    Ath__track* btrack = _trackTable[ii];
    if (btrack == NULL)
      continue;

    Ath__track* track = NULL;
    bool tohi = true;
    while ((track = btrack->getNextSubTrack(track, tohi)) != nullptr)
      cnt += track->removeMarkedNetWires();
  }
  return cnt;
}

Ath__grid* Ath__gridTable::getGrid(uint row, uint col)
{
  return _gridTable[row][col];
}
bool Ath__gridTable::addBox(uint row, uint col, dbBox* bb)
{
  Ath__grid* g = _gridTable[row][col];

  g->placeBox(bb, 0, 0);

  return true;
}
Ath__wire* Ath__gridTable::addBox(dbBox* bb, uint wtype, uint id)
{
  /*
  uint row1; uint col1;
  if (!getRowCol(bb->xMin(), bb->yMin(), &row1, &col1))
          return NULL;

  uint row2; uint col2;
  if (!getRowCol(bb->xMax(), bb->yMax(), &row2, &col2))
          return NULL;
*/
  uint row = 0;
  uint col = 0;
  Ath__grid* g = _gridTable[row][col];

  g->placeBox(bb, wtype, id);

  return NULL;
}
Ath__wire* Ath__gridTable::addBox(Ath__box* bb)
{
  uint row;
  uint col;
  if (!getRowCol(bb->_xlo, bb->_ylo, &row, &col))
    return NULL;

  Ath__grid* g = _gridTable[row][col];
  g->placeBox(bb);

  return NULL;
}
Ath__wire* Ath__gridTable::getWire_Linear(uint instId)
{
  // bool ordered= true;

  // uint cnt= 0;
  for (uint ii = 0; ii < _rowCnt; ii++) {
    for (uint jj = 0; jj < _colCnt; jj++) {
      Ath__wire* w = _gridTable[ii][jj]->getWire_Linear(instId);
      if (w != NULL)
        return w;
    }
  }
  return NULL;
}
void Ath__gridTable::adjustOverlapMakerEnd()
{
  if (_overlapAdjust != Z_endAdjust)
    return;
  for (uint ii = 0; ii < _rowCnt; ii++) {
    for (uint jj = 0; jj < _colCnt; jj++) {
      if (_gridTable[ii][jj])
        _gridTable[ii][jj]->adjustOverlapMakerEnd();
    }
  }
}
void Ath__gridTable::adjustMetalFill()
{
  _signalPowerNotAlignedOverlap = 0;
  _powerNotAlignedOverlap = 0;
  _signalNotAlignedOverlap = 0;
  _signalOverlap = 0;
  _powerOverlap = 0;
  _signalPowerOverlap = 0;
  _powerSignalOverlap = 0;
  for (uint ii = 0; ii < _rowCnt; ii++) {
    for (uint jj = 0; jj < _colCnt; jj++) {
      if (_gridTable[ii][jj])
        _gridTable[ii][jj]->adjustMetalFill();
    }
  }
  if (_signalPowerNotAlignedOverlap)
    notice(0,
           "%d not aligned power-signal overlaps\n",
           _signalPowerNotAlignedOverlap);
  if (_powerNotAlignedOverlap)
    notice(0, "%d not aligned power overlaps\n", _powerNotAlignedOverlap);
  if (_signalNotAlignedOverlap)
    notice(0, "%d not aligned signal overlaps\n", _signalNotAlignedOverlap);
  if (_powerSignalOverlap)
    notice(0, "%d power-signal overlaps\n", _powerSignalOverlap);
  if (_signalPowerOverlap)
    notice(0, "%d signal-power overlaps\n", _signalPowerOverlap);
  if (_powerOverlap)
    notice(0, "%d power overlaps\n", _powerOverlap);
  if (_signalOverlap)
    notice(0, "%d signal overlaps\n", _signalOverlap);
  setExtrusionMarker(0, 1);
}
void Ath__gridTable::incrNotAlignedOverlap(Ath__wire* w1, Ath__wire* w2)
{
  if (w1->isPower() != w2->isPower())
    _signalPowerNotAlignedOverlap++;
  else if (w1->isPower())
    _powerNotAlignedOverlap++;
  else
    _signalNotAlignedOverlap++;
}
void Ath__gridTable::incrSignalOverlap()
{
  _signalOverlap++;
}
void Ath__gridTable::incrPowerOverlap()
{
  _powerOverlap++;
}
void Ath__gridTable::incrSignalToPowerOverlap()
{
  _signalPowerOverlap++;
}
void Ath__gridTable::incrPowerToSignallOverlap()
{
  _powerSignalOverlap++;
}
void Ath__gridTable::incrMultiTrackWireCnt(bool isPower)
{
  if (isPower)
    _powerMultiTrackWire++;
  else
    _signalMultiTrackWire++;
}
bool Ath__gridTable::isOrdered(bool /* unused: ascending */)
{
  bool ordered = true;

  uint cnt = 0;
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
      cnt += cnt1;
    }
  }
  return ordered;
}

void Ath__gridTable::removeMarkedNetWires()
{
  uint cnt = 0;
  for (uint jj = 1; jj < _colCnt; jj++) {
    for (int ii = _rowCnt - 1; ii >= 0; ii--) {
      Ath__grid* netGrid = _gridTable[ii][jj];
      cnt += netGrid->removeMarkedNetWires();
    }
  }
  fprintf(stdout, "remove %d sdb wires.\n", cnt);
}

void Ath__gridTable::setExtControl(dbBlock* block,
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
  if (ccUp == 2)
    _CCtargetHighMarkedNet = 1;
  else
    _CCtargetHighMarkedNet = 0;
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
void Ath__gridTable::reverseTargetTrack()
{
  _CCtargetHighTracks = _CCtargetHighTracks == 2 ? 0 : 2;
  _targetTrackReversed = _targetTrackReversed ? false : true;
}

void Ath__gridTable::setMaxArea(int x1, int y1, int x2, int y2)
{
  _maxSearchBox.set(x1, y1, x2, y2, 1);
  _setMaxArea = true;
}
void Ath__gridTable::resetMaxArea()
{
  _setMaxArea = false;
  _maxSearchBox.invalidateBox();
}
void Ath__gridTable::getBox(uint wid,
                            int* x1,
                            int* y1,
                            int* x2,
                            int* y2,
                            uint* level,
                            uint* id1,
                            uint* id2,
                            uint* wtype)
{
  Ath__wire* w = getWirePtr(wid);

  *id1 = w->_boxId;
  *id2 = w->_otherId;
  *wtype = w->_flags;
  *level = w->_track->getGrid()->getLevel();
  uint dir;
  w->getCoords(x1, y1, x2, y2, &dir);
}
uint Ath__gridTable::addBox(int x1,
                            int y1,
                            int x2,
                            int y2,
                            uint level,
                            uint id1,
                            uint id2,
                            uint wireType)
{
  Ath__searchBox bb(x1, y1, x2, y2, level);
  bb.setOwnerId(id1, id2);
  bb.setType(wireType);

  uint dir = bb.getDir();
  uint trackNum = getGrid(dir, level)->placeWire(&bb);
  _wireCnt++;
  return trackNum;
  // uint wireId= getGrid(dir, level)->placeWire(&bb);
  // return wireId;
}
uint Ath__gridTable::getWireCnt()
{
  return _wireCnt;
}
uint Ath__gridTable::search(int x1,
                            int y1,
                            int x2,
                            int y2,
                            uint row,
                            uint col,
                            Ath__array1D<uint>* idTable,
                            bool /* unused: wireIdFlag */)
{
  Ath__searchBox bb(x1, y1, x2, y2, col, row);

  return search(&bb, row, col, idTable, true);  // single grid
}
void Ath__gridTable::getCoords(Ath__searchBox* bb, uint wireId)
{
  Ath__wire* w = getWirePtr(wireId);
  w->getCoords(bb);
}
void Ath__gridTable::getCCdist(uint wid,
                               uint* width,
                               uint* len,
                               uint* id1,
                               uint* id2)
{
  Ath__wire* w = getWirePtr(wid);

  *width = w->_width;
  *len = w->_len;
  *id1 = w->_boxId;
  *id2 = w->_otherId;
}
void Ath__gridTable::getIds(uint wid, uint* id1, uint* id2, uint* wtype)
{
  Ath__wire* w = getWirePtr(wid);

  *wtype = w->_flags;
  *id1 = w->_boxId;
  *id2 = w->_otherId;
}

/****************************************** TRANSFER FUNCTIONS from Extraction
**************************************** void
Ath__gridTable::setDefaultWireType(uint v)
{
        for (uint ii= 0; ii<_rowCnt; ii++) {
                for (uint jj= 0; jj<_colCnt; jj++) {

                        if (_gridTable[ii][jj]==NULL)
                                continue;

                        _gridTable[ii][jj]->setDefaultWireType(v);
                }
        }
}

uint Ath__gridTable::couplingCaps(Ath__gridTable *resGridTable, uint
couplingDist, ZInterface *context, Ath__array1D<uint> *ccTable, void
(*coupleAndCompute)(int *, void *), void *compPtr)
{
//	ttttGetDgOverlap= 0;
//	if (couplingDist>20) {
//		couplingDist= couplingDist % 10;
//		ttttGetDgOverlap= 1;
//	}
        ttttGetDgOverlap= 1;
        setCCFlag (couplingDist);
        _CCshorts = 0;
        uint cnt= 0;
        for (uint jj= 0; jj<_colCnt; jj++) {
                // for (uint ii= 0; ii<_rowCnt; ii++) {
                for (int ii= _rowCnt-1; ii>=0; ii--) {

                        Ath__grid *resGrid= NULL;
                        if (resGridTable!=NULL)
                                resGrid= resGridTable->getGrid(ii, jj);
//			notice(0, "-----------------------------Grid dir= %d
Layer=%d\n", ii, jj);

                        Ath__grid *netGrid= _gridTable[ii][jj];
                        if (netGrid==NULL)
                                continue;

                        // netGrid->adjustMarkers();

                        cnt += netGrid->couplingCaps(resGrid, couplingDist,
context, ccTable, coupleAndCompute, compPtr);

#ifdef ZDEBUG
                        context->event( "GRID",	"dir", Z_INT, ii, "layer",
Z_INT, jj,	NULL); #endif

                }
        }
        notice(0, "Final %d ccaps\n", cnt);
        notice(0, "      %d interTrack shorts\n", _CCshorts);
        return cnt;
}
uint Ath__gridTable::couplingCaps(uint row, uint col, Ath__grid *resGrid, uint
couplingDist, ZInterface *context)
{
        return 0;
        //return _gridTable[row][col]->couplingCaps(resGrid, couplingDist,
context);
}

uint Ath__track::couplingCaps(Ath__grid *ccGrid, uint srcTrack, uint trackDist,
uint ccThreshold, ZInterface *context, Ath__array1D<uint> *ccIdTable, uint met,
void (*coupleAndCompute)(int *, void *), void *compPtr)
{
        Ath__track *tstrack;
        bool tohi = _grid->getGridTable()->targetHighTracks()>0 ? true : false;
        initTargetTracks (srcTrack, trackDist, tohi);
        // need to process "empty wire" (non-coupled wire)
        // if (!trackFound)
        // 	return 0;

        uint dir= _grid->getDir();
        int coupleOptions[20];

        Ath__array1D<Ath__wire*> w1Table;
        Ath__array1D<Ath__wire*> w2Table;
        Ath__array1D<Ath__wire*> *wTable, *nwTable, *twTable;
        Ath__array1D<Ath__wire*> ccTable;

        bool useDbSdb = _grid->getGridTable()->usingDbSdb();
        int noPowerSource = _grid->getGridTable()->noPowerSource();
        uint TargetHighMarkedNet = _grid->getGridTable()->targetHighMarkedNet();
        bool allNet = _grid->getGridTable()->allNet();
        AthPool<Ath__wire> *wirePool= _grid->getWirePoolPtr();
        uint wireCnt= 0;
        Ath__wire* origWire= NULL;
//	bool srcMarked;
        uint delt;
        int exid;

        if (ttttGetDgOverlap)
        {
                // to initTargetSeq
                coupleOptions[0] = -met;
                coupleOptions[5] = 1;
                coupleAndCompute(coupleOptions, compPtr);
        }
        int nexy, nelen;
        Ath__wire* wire= NULL;
        Ath__wire* pwire= NULL;
        Ath__wire* nwire= getNextWire(wire);
        for (wire = nwire; wire; pwire = wire, wire = nwire)
        {
                nwire = getNextWire(wire);
#ifdef TEST_GetDgOverlap
                if (ttttGetDgOverlap)
                {
                        if (wire->isPower() || wire->_srcId > 0 ||
_grid->getGridTable()->handleEmptyOnly()) continue; coupleOptions[0] = -met;
                        coupleOptions[1] = wire->_xy;
                        coupleOptions[2] = wire->_xy + wire->_len;
                        coupleOptions[3] = wire->_base;
                        coupleOptions[4] = wire->_base + wire->_width;
                        coupleOptions[5] = 2;
                        coupleOptions[6]= wire->_dir;
                        coupleAndCompute(coupleOptions, compPtr);
                        continue;
                }
#endif
                if (!wire->isPower() && nwire && nwire->isPower() && nwire->_xy
< wire->_xy + wire->_len) coupleOptions[19] = 0;  // bp if (wire->isPower() &&
nwire && !nwire->isPower() && nwire->_xy < wire->_xy + wire->_len)
                        coupleOptions[19] = 0;  // bp

                if (noPowerSource && wire->isPower())
                        continue;
                if (!allNet && !TargetHighMarkedNet &&
!wire->getNet()->isMarked())  // when !TargetHighMarkedNet, need only marked
source continue; if (tohi && _grid->getMinMaxTrackNum(wire->_base+wire->_width)
!= srcTrack) continue; if (!tohi && wire->_srcId > 0) continue; if (useDbSdb &&
!wire->isPower() && wire->getNet()->getWire() &&
(!wire->getNet()->getWire()->getProperty((int)wire->_otherId,exid) || exid==0))
                        continue;
                wireCnt++;

                // ccGrid->placeWire(wire);

                w1Table.resetCnt();
                wTable = &w1Table;
                nwTable = &w2Table;
                nexy =  wire->_xy;
                nelen = wire->_len;
                int delta;
                if (pwire)
                        delta = pwire->_xy + pwire->_len - wire->_xy;
                if (pwire && delta > 0 && wire->_base + wire->_width <
pwire->_base + pwire->_width)
                {
                        nexy += delta;
                        nelen -= delta;
                }
                if (nwire)
                        delta = wire->_xy + wire->_len - nwire->_xy;
                if (nwire && delta > 0 && wire->_base + wire->_width <
nwire->_base + nwire->_width) nelen -= delta;
                // assert (nelen > 0);
                //// assert (nelen >= 0);
                //// if (nelen == 0)  // or nelen < wire->_width
                if (nelen <= 0)  // or nelen < wire->_width
                        continue;
                Ath__wire *newEmptyWire= wire->makeWire(wirePool, nexy, nelen);
                wTable->add(newEmptyWire);

                delt = 0;
                tstrack = this;
                while (nextSubTrackInRange(tstrack, delt, trackDist, srcTrack,
tohi))
                {
                        nwTable->resetCnt();

                        origWire = wire->_srcId ?
_grid->getWirePtr(wire->_srcId) : wire; tstrack->findOverlap(origWire,
ccThreshold, wTable, nwTable, ccGrid, &ccTable, context, met, coupleAndCompute,
compPtr);

                        twTable = wTable;
                        wTable = nwTable;
                        nwTable = twTable;

                        if (wTable -> getCnt() == 0)
                                break;
                }
                if (coupleAndCompute!=NULL) {
                        for (uint kk= 0; kk<wTable->getCnt(); kk++) {
                                Ath__wire *empty= wTable->get(kk);

                                coupleOptions[0]= met;

                                                int wBoxId = (int)wire->_boxId;
                                                if (wire->_otherId && useDbSdb)
                                                        wire->getNet()->getWire()->getProperty((int)wire->_otherId,
wBoxId); coupleOptions[1]= wBoxId; // dbRSeg id if (wire->_otherId==0)
                                        coupleOptions[1]= -wBoxId; // dbRSeg id

                                coupleOptions[2]= 0; // dbRSeg id

                                coupleOptions[3]= empty->_len;
                                coupleOptions[4]= -1;
                                coupleOptions[5]= empty->_xy;
                                coupleOptions[6]= wire->_dir;

                                coupleOptions[7]= wire->_width;
                                coupleOptions[8]= 0;
                                coupleOptions[9]= wire->_base;
                                coupleOptions[10]= dir;

                                coupleOptions[11]= tohi ? 1 : 0;
                                coupleAndCompute(coupleOptions, compPtr);
                                wirePool->free(empty);
                        }
                }
        }
        if (coupleAndCompute==NULL) {
                for (uint kk= 0; kk<ccTable.getCnt(); kk++)
                {
                        Ath__wire *v= ccTable.get(kk);
                        ccIdTable->add(v->_id);

                        // compute and/or add on a grid
                }
        }
        //	notice(0, "\t%d wires make %d ccaps\n", wireCnt,
ccTable.getCnt());
        //return ccTable.getCnt();
        return wireCnt;
}


uint Ath__grid::couplingCaps(Ath__grid *resGrid, uint couplingDist, ZInterface
*context, Ath__array1D<uint> *ccTable, void (*coupleAndCompute)(int *, void *),
void *compPtr)
{
        // Ath__array1D<Ath__wire*> ccTable;
        // Ath__array1D<Ath__wire*> wTable;

        uint coupleTrackNum= couplingDist; // EXT-OPTIMIZE
        uint ccThreshold = coupleTrackNum*_pitch;
        uint TargetHighMarkedNet =  _gridtable->targetHighMarkedNet();
        bool allNet = _gridtable->allNet();

        uint domainAdjust = allNet||!TargetHighMarkedNet ? 0 : couplingDist;
        initContextGrids();
        setSearchDomain(domainAdjust);
        uint cnt= 0;
        for (uint ii= _searchLowTrack; ii<=_searchHiTrack; ii++)
        {
                Ath__track *btrack= _trackTable[ii];
                if (btrack==NULL)
                        continue;

                int base = btrack -> getBase();
                _gridtable->buildDgContext (base, _level, _dir);
                if (!ttttGetDgOverlap)
                        coupleAndCompute (NULL, compPtr);  // try print
dgContext

                Ath__track *track = NULL;
                bool tohi = true;
                while ((track = btrack->getNextSubTrack(track, tohi)))
                {
                        _gridtable->setHandleEmptyOnly (false);
                        uint cnt1= track->couplingCaps(resGrid, ii,
coupleTrackNum, ccThreshold, context, ccTable, _level, coupleAndCompute,
compPtr); cnt += cnt1; if (allNet || TargetHighMarkedNet)
                                _gridtable->setHandleEmptyOnly (true);
                        _gridtable->reverseTargetTrack();
                        cnt1= track->couplingCaps(resGrid, ii, coupleTrackNum,
ccThreshold, context, ccTable, _level, coupleAndCompute, compPtr); cnt += cnt1;
                        _gridtable->reverseTargetTrack();
                }
//		notice(0, "CC: Track - %5d : %d out of %d\n", ii, cnt1, cnt);
        }

        return cnt;
}
*/
