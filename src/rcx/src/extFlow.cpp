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

#include <wire.h>

#include <map>
#include <vector>

#include "dbUtil.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

using namespace odb;

uint extMain::getBucketNum(int base, int max, uint step, int xy)
{
  if (xy >= max)
    xy = max - 1;

  int delta = xy - base;
  if (delta < 0)
    return 0;

  uint n = delta / step;
  return n;
}
uint extMain::addNetOnTable(uint netId,
                            uint dir,
                            Rect* maxRect,
                            uint* nm_step,
                            int* bb_ll,
                            int* bb_ur,
                            Ath__array1D<uint>*** wireTable)
{
  uint cnt = 0;
  int ll[2] = {maxRect->xMin(), maxRect->yMin()};
  int ur[2] = {maxRect->xMax(), maxRect->yMax()};

  uint lo_bound = getBucketNum(bb_ll[dir], bb_ur[dir], nm_step[dir], ll[dir]);
  uint hi_bound = getBucketNum(bb_ll[dir], bb_ur[dir], nm_step[dir], ur[dir]);

  for (uint ii = lo_bound; ii <= hi_bound; ii++) {
    if (wireTable[dir][ii] == NULL)
      wireTable[dir][ii] = new Ath__array1D<uint>(8000);

    wireTable[dir][ii]->add(netId);
    cnt++;
  }
  return cnt;
}
uint extMain::getNetBbox(dbNet* net, Rect& maxRect)
{
  dbWire* wire = net->getWire();
  if (wire == NULL)
    return 0;

  maxRect.reset(MAX_INT, MAX_INT, MIN_INT, MIN_INT);
  uint cnt = 0;
  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    if (s.isVia())
      continue;

    Rect r = s.getBox();

    maxRect.merge(r);
    cnt++;
  }
  return cnt;
}
uint extMain::getNetBbox(dbNet* net, Rect* maxRect[2])
{
  dbWire* wire = net->getWire();
  if (wire == NULL)
    return 0;

  maxRect[0]->reset(MAX_INT, MAX_INT, MIN_INT, MIN_INT);
  maxRect[1]->reset(MAX_INT, MAX_INT, MIN_INT, MIN_INT);
  uint cnt = 0;
  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    if (s.isVia())
      continue;
    cnt++;
  }
  return cnt;
}
void extMain::getNetShapes(dbNet* net,
                           Rect** maxRectSdb,
                           Rect& maxRectGs,
                           bool* hasSdbWires,
                           bool& hasGsWires)
{
  dbWire* wire = net->getWire();
  if (wire == NULL)
    return;

  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    if (s.isVia())
      continue;

    Rect r = s.getBox();

    uint dd = 0;  // vertical
    if (r.dx() > r.dy())
      dd = 1;  // horizontal

    maxRectSdb[dd]->merge(r);
    hasSdbWires[dd] = true;

    maxRectGs.merge(r);
    hasGsWires = true;
  }
}
void extMain::getNetSboxes(dbNet* net,
                           Rect** maxRectSdb,
                           Rect& maxRectGs,
                           bool* hasSdbWires,
                           bool& hasGsWires)
{
  dbSet<dbSWire> swires = net->getSWires();
  dbSet<dbSWire>::iterator itr;

  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    dbSWire* swire = *itr;
    dbSet<dbSBox> wires = swire->getWires();
    dbSet<dbSBox>::iterator box_itr;

    for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
      dbSBox* s = *box_itr;

      if (s->isVia())
        continue;

      Rect r = s->getBox();

      uint dd = 0;  // vertical
      if (r.dx() > r.dy())
        dd = 1;  // horizontal

      maxRectSdb[dd]->merge(r);
      hasSdbWires[dd] = true;

      maxRectGs.merge(r);
      hasGsWires = true;
    }
  }
}
void extMain::freeSignalTables(bool rlog,
                               Ath__array1D<uint>*** sdbSignalTable,
                               Ath__array1D<uint>*** signalGsTable,
                               uint* bucketCnt)
{
  for (uint dd = 0; dd < 2; dd++) {
    uint n = bucketCnt[dd];

    for (uint jj = 0; jj < n; jj++) {
      if (sdbSignalTable[dd][jj] != NULL)
        delete sdbSignalTable[dd][jj];

      if ((signalGsTable != NULL) && (signalGsTable[dd][jj] != NULL))
        delete signalGsTable[dd][jj];
    }
    delete[] sdbSignalTable[dd];
    if (signalGsTable != NULL)
      delete[] signalGsTable[dd];
  }
}
extWireBin::extWireBin(uint d,
                       uint num,
                       int base,
                       AthPool<extWire>* wpool,
                       uint allocChunk)
{
  _dir = d;
  _num = num;
  _extWirePool = wpool;
  _base = base;
  _table = new Ath__array1D<extWire*>(allocChunk);
}

int extWireBin::addWire(uint netId, int sid, dbTechLayer* layer)
{
  extWire* w = _extWirePool->alloc();
  w->_layer = layer;
  w->_netId = netId;
  w->_shapeId = sid;
  return _table->add(w);
}

Ath__array1D<uint>*** extMain::mkInstBins(uint binSize,
                                          int* bb_ll,
                                          int* bb_ur,
                                          uint* bucketCnt)
{
  uint nm_step[2] = {binSize, binSize};
  Ath__array1D<uint>*** instTable = new Ath__array1D<uint>**[2];

  for (uint dd = 0; dd < 2; dd++) {
    bucketCnt[dd] = (bb_ur[dd] - bb_ll[dd]) / nm_step[dd] + 1;
    uint n = bucketCnt[dd];

    instTable[dd] = new Ath__array1D<uint>*[n];
    if (instTable[dd] == NULL) {
      logger_->error(
          RCX, 466, "cannot allocate <Ath__array1D<extWireBin*>*[layerCnt]>");
    }
    for (uint jj = 0; jj < n; jj++) {
      instTable[dd][jj] = NULL;
    }
  }
  dbSet<dbInst> insts = _block->getInsts();
  dbSet<dbInst>::iterator inst_itr;

  for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
    dbInst* inst = *inst_itr;
    dbBox* bb = inst->getBBox();

    Rect r = bb->getBox();

    for (uint dir = 0; dir < 2; dir++)
      addNetOnTable(inst->getId(), dir, &r, nm_step, bb_ll, bb_ur, instTable);
  }
  return instTable;
}

uint extMain::mkSignalTables2(uint* nm_step,
                              int* bb_ll,
                              int* bb_ur,
                              Ath__array1D<uint>*** sdbSignalTable,
                              Ath__array1D<uint>* sdbPowerTable,
                              Ath__array1D<uint>*** instTable,
                              uint* bucketCnt)
{
  for (uint dd = 0; dd < 2; dd++) {
    bucketCnt[dd] = (bb_ur[dd] - bb_ll[dd]) / nm_step[dd] + 2;
    uint n = bucketCnt[dd];

    sdbSignalTable[dd] = new Ath__array1D<uint>*[n];
    instTable[dd] = new Ath__array1D<uint>*[n];

    for (uint jj = 0; jj < n; jj++) {
      sdbSignalTable[dd][jj] = NULL;
      instTable[dd][jj] = NULL;
    }
  }
  uint cnt = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType().isSupply())) {
      sdbPowerTable->add(net->getId());
      continue;
    }

    Rect maxRect;
    uint cnt1 = getNetBbox(net, maxRect);
    if (cnt1 == 0)
      continue;

    net->setSpef(false);

    for (uint dir = 0; dir < 2; dir++) {
      addNetOnTable(
          net->getId(), dir, &maxRect, nm_step, bb_ll, bb_ur, sdbSignalTable);
    }
    cnt += cnt1;
  }
  return cnt;
}
uint extMain::mkSignalTables(uint* nm_step,
                             int* bb_ll,
                             int* bb_ur,
                             Ath__array1D<uint>*** sdbSignalTable,
                             Ath__array1D<uint>*** signalGsTable,
                             Ath__array1D<uint>*** instTable,
                             uint* bucketCnt)
{
  for (uint dd = 0; dd < 2; dd++) {
    bucketCnt[dd] = (bb_ur[dd] - bb_ll[dd]) / nm_step[dd] + 2;
    uint n = bucketCnt[dd];

    sdbSignalTable[dd] = new Ath__array1D<uint>*[n];
    signalGsTable[dd] = new Ath__array1D<uint>*[n];
    instTable[dd] = new Ath__array1D<uint>*[n];

    for (uint jj = 0; jj < n; jj++) {
      sdbSignalTable[dd][jj] = NULL;
      signalGsTable[dd][jj] = NULL;
      instTable[dd][jj] = NULL;
    }
    // bb_ur of chip has to be relace with one based on gs

    bb_ur[dd] = (bb_ur[dd] - bb_ll[dd]) / nm_step[dd];
    bb_ur[dd] *= nm_step[dd];
  }

  uint cnt = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    Rect maxRectGs;
    maxRectGs.reset(MAX_INT, MAX_INT, MIN_INT, MIN_INT);
    bool hasGsWires = false;

    Rect a;
    Rect b;
    Rect* maxRectSdb[2] = {&a, &b};
    maxRectSdb[0]->reset(MAX_INT, MAX_INT, MIN_INT, MIN_INT);
    maxRectSdb[1]->reset(MAX_INT, MAX_INT, MIN_INT, MIN_INT);
    bool hasSdbWires[2] = {false, false};

    if ((net->getSigType().isSupply()))
      getNetSboxes(net, maxRectSdb, maxRectGs, hasSdbWires, hasGsWires);
    else
      getNetShapes(net, maxRectSdb, maxRectGs, hasSdbWires, hasGsWires);

    for (uint dir = 0; dir < 2; dir++) {
      if (hasSdbWires[dir])
        addNetOnTable(net->getId(),
                      dir,
                      maxRectSdb[dir],
                      nm_step,
                      bb_ll,
                      bb_ur,
                      sdbSignalTable);
      if (hasGsWires)
        addNetOnTable(net->getId(),
                      dir,
                      &maxRectGs,
                      nm_step,
                      bb_ll,
                      bb_ur,
                      signalGsTable);
    }

    cnt++;
  }
  dbSet<dbInst> insts = _block->getInsts();
  dbSet<dbInst>::iterator inst_itr;

  for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
    dbInst* inst = *inst_itr;
    dbBox* bb = inst->getBBox();

    Rect s = bb->getBox();

    for (uint dir = 0; dir < 2; dir++)
      addNetOnTable(inst->getId(), dir, &s, nm_step, bb_ll, bb_ur, instTable);
  }
  return cnt;
}

bool extMain::matchDir(uint dir, Rect& r)
{
  uint dd = 0;  // vertical
  if (r.dx() >= r.dy())
    dd = 1;  // horizontal

  if (dir != dd)
    return false;
  else
    return true;
}
bool extMain::isIncludedInsearch(Rect& r, uint dir, int* bb_ll, int* bb_ur)
{
  if (!matchDir(dir, r))
    return false;

  int ll[2] = {r.xMin(), r.yMin()};

  if (ll[dir] >= bb_ur[dir])
    return false;

  if (ll[dir] < bb_ll[dir])
    return false;

  return true;
}
bool extMain::isIncluded(Rect& r, uint dir, int* ll, int* ur)
{
  uint dd = 0;  // vertical
  if (r.dx() > r.dy())
    dd = 1;  // horizontal

  if (dir != dd)
    return false;

  int rLL[2] = {r.xMin(), r.yMin()};
  int rUR[2] = {r.xMax(), r.yMax()};

  if ((rUR[dir] < ll[dir]) || (rLL[dir] > ur[dir]))
    return false;

  return true;
}
void extMain::GetDBcoords2(Rect& r)
{
  int x1 = r.xMin();
  int x2 = r.xMax();
  int y1 = r.yMin();
  int y2 = r.yMax();
  x1 = GetDBcoords2(x1);
  x2 = GetDBcoords2(x2);
  y1 = GetDBcoords2(y1);
  y2 = GetDBcoords2(y2);
  r.set_xlo(x1);
  r.set_ylo(y1);
  r.set_xhi(x2);
  r.set_yhi(y2);
}
uint extMain::initSearchForNets(int* X1,
                                int* Y1,
                                uint* pitchTable,
                                uint* widthTable,
                                uint* dirTable,
                                Rect& extRect,
                                bool skipBaseCalc)
{
  bool USE_DB_UNITS = false;
  uint W[32];
  uint S[32];

  dbSet<dbTechLayer> layers = _tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;
  dbTrackGrid* tg = NULL;

  Rect maxRect;
  if ((extRect.dx() > 0) && (extRect.dy() > 0)) {
    maxRect = extRect;
  } else {
    maxRect = _block->getDieArea();
    if (!((maxRect.dx() > 0) && (maxRect.dy() > 0)))
      logger_->error(
          RCX, 81, "Die Area for the block has 0 size, or is undefined!");
  }

  if (USE_DB_UNITS) {
    GetDBcoords2(maxRect);
    GetDBcoords2(extRect);
  }

  std::vector<int> trackXY(32000);
  uint n = 0;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;

    if (layer->getRoutingLevel() == 0)
      continue;

    n = layer->getRoutingLevel();
    int w = GetDBcoords2(layer->getWidth());
    widthTable[n] = layer->getWidth();

    if (USE_DB_UNITS)
      widthTable[n] = w;

    W[n] = 1;
    int s = GetDBcoords2(layer->getSpacing());
    S[n] = layer->getSpacing();

    if (USE_DB_UNITS)
      S[n] = s;

    int p = GetDBcoords2(layer->getPitch());
    pitchTable[n] = layer->getPitch();

    if (USE_DB_UNITS)
      pitchTable[n] = p;
    if (pitchTable[n] <= 0)
      logger_->error(RCX,
                     82,
                     "Layer {}, routing level {}, has pitch {}!!",
                     layer->getConstName(),
                     n,
                     pitchTable[n]);

    dirTable[n] = 0;
    if (layer->getDirection() == dbTechLayerDir::HORIZONTAL)
      dirTable[n] = 1;

    if (skipBaseCalc)
      continue;

    tg = _block->findTrackGrid(layer);
    if (tg) {
      tg->getGridX(trackXY);
      X1[n] = trackXY[0] - layer->getWidth() / 2;
      tg->getGridY(trackXY);
      Y1[n] = trackXY[0] - layer->getWidth() / 2;
    } else {
      X1[n] = maxRect.xMin();
      Y1[n] = maxRect.yMin();
    }
  }
  uint layerCnt = n + 1;

  _search = new odb::Ath__gridTable(
      &maxRect, 2, layerCnt, W, pitchTable, S, X1, Y1);
  _search->setBlock(_block);
  return layerCnt;
}

uint extMain::sBoxCounter(dbNet* net, uint& maxWidth)
{
  uint cnt = 0;
  dbSet<dbSWire> swires = net->getSWires();
  dbSet<dbSWire>::iterator itr;

  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    dbSWire* swire = *itr;
    dbSet<dbSBox> wires = swire->getWires();
    dbSet<dbSBox>::iterator box_itr;

    for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
      dbSBox* s = *box_itr;
      if (s->isVia())
        continue;

      uint x = s->getDX();
      uint y = s->getDY();
      uint w = y;
      if (w < x)
        w = x;

      if (maxWidth > w)
        maxWidth = w;

      cnt++;
    }
  }
  return cnt;
}
uint extMain::powerWireCounter(uint& maxWidth)
{
  uint cnt = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if (!((net->getSigType().isSupply())))
      continue;

    cnt += sBoxCounter(net, maxWidth);
  }
  return cnt;
}
uint extMain::addMultipleRectsOnSearch(Rect& r,
                                       uint level,
                                       uint dir,
                                       uint id,
                                       uint shapeId,
                                       uint wtype)
{
  if (_geoThickTable == NULL)
    return _search->addBox(
        r.xMin(), r.yMin(), r.xMax(), r.yMax(), level, id, shapeId, wtype);

  extGeoThickTable* thickTable = _geoThickTable[level];

  if (thickTable == NULL)
    return _search->addBox(
        r.xMin(), r.yMin(), r.xMax(), r.yMax(), level, id, shapeId, wtype);

  uint cnt = 0;

  int ll[2] = {r.xMin(), r.yMin()};
  int ur[2] = {r.xMax(), r.yMax()};

  uint startSquare[2];
  uint endSquare[2];

  extGeoVarTable* sq1 = thickTable->getSquare(r.xMin(), r.yMin(), startSquare);

  extGeoVarTable* sq2 = thickTable->getSquare(r.xMax(), r.yMax(), endSquare);

  if ((sq1 == NULL) || (sq2 == NULL))
    return _search->addBox(
        r.xMin(), r.yMin(), r.xMax(), r.yMax(), level, id, shapeId, wtype);

  dir = !dir;

  if (endSquare[dir] > startSquare[dir]) {
    ur[dir] = thickTable->getUpperBound(dir, startSquare);
    cnt += _search->addBox(
        ll[0], ll[1], ur[0], ur[1], level, id, shapeId, wtype);
  } else {  // assume equal
    return _search->addBox(
        r.xMin(), r.yMin(), r.xMax(), r.yMax(), level, id, shapeId, wtype);
  }

  uint sq[2] = {startSquare[0], startSquare[1]};
  for (sq[dir]++; sq[dir] < endSquare[dir]; sq[dir]++) {
    ll[dir] = ur[dir];  // prev bound
    ur[dir] = thickTable->getUpperBound(dir, sq);

    cnt += _search->addBox(
        ll[0], ll[1], ur[0], ur[1], level, id, shapeId, wtype);
  }
  ll[dir] = ur[dir];

  cnt += _search->addBox(
      ll[0], ll[1], r.xMax(), r.yMax(), level, id, shapeId, wtype);

  if (cnt > 0)
    return 1;
  else
    return 0;
}

uint extMain::addNetSBoxes(dbNet* net,
                           uint dir,
                           int* bb_ll,
                           int* bb_ur,
                           uint wtype,
                           dbCreateNetUtil* netUtil)
{
  uint cnt = 0;
  dbSet<dbSWire> swires = net->getSWires();
  dbSet<dbSWire>::iterator itr;

  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    dbSWire* swire = *itr;
    dbSet<dbSBox> wires = swire->getWires();
    dbSet<dbSBox>::iterator box_itr;

    for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
      dbSBox* s = *box_itr;
      if (s->isVia())
        continue;

      Rect r = s->getBox();
      if (isIncludedInsearch(r, dir, bb_ll, bb_ur)) {
        uint level = s->getTechLayer()->getRoutingLevel();

        int trackNum = -1;
        if (_geoThickTable != NULL) {
          cnt += addMultipleRectsOnSearch(r, level, dir, s->getId(), 0, wtype);
          continue;
        }
        if (netUtil != NULL) {
          netUtil->createSpecialWire(NULL, r, s->getTechLayer(), s->getId());
        } else {
          trackNum = _search->addBox(r.xMin(),
                                     r.yMin(),
                                     r.xMax(),
                                     r.yMax(),
                                     level,
                                     s->getId(),
                                     0,
                                     wtype);

          if (_searchFP != NULL) {
            fprintf(_searchFP,
                    "%d  %d %d  %d %d %d\n",
                    level,
                    r.xMin(),
                    r.yMin(),
                    r.xMax(),
                    r.yMax(),
                    trackNum);
          }
        }

        cnt++;
      }
    }
  }
  return cnt;
}
uint extMain::addNetSBoxes2(dbNet* net,
                            uint dir,
                            int* bb_ll,
                            int* bb_ur,
                            uint wtype,
                            uint step)
{
  uint cnt = 0;
  dbSet<dbSWire> swires = net->getSWires();
  dbSet<dbSWire>::iterator itr;

  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    dbSWire* swire = *itr;
    dbSet<dbSBox> wires = swire->getWires();
    dbSet<dbSBox>::iterator box_itr;

    for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
      dbSBox* s = *box_itr;
      if (s->isVia())
        continue;

      Rect r = s->getBox();
      if (isIncludedInsearch(r, dir, bb_ll, bb_ur)) {
        uint level = s->getTechLayer()->getRoutingLevel();

        if (step > 0) {
          uint len = r.dx();
          if (len < r.dy())
            len = r.dy();

          if (len <= step) {
            _search->addBox(r.xMin(),
                            r.yMin(),
                            r.xMax(),
                            r.yMax(),
                            level,
                            s->getId(),
                            0,
                            wtype);
          } else {
            if (r.dx() < r.dy()) {  // vertical
              for (int y1 = r.yMin(); y1 < r.yMax();) {
                int y2 = y1 + step;
                if (y2 > r.yMax())
                  y2 = r.yMax();

                _search->addBox(
                    r.xMin(), y1, r.xMax(), y2, level, s->getId(), 0, wtype);
                y1 = y2;
              }
            } else {  // horizontal
              for (int x1 = r.xMin(); x1 < r.xMax();) {
                int x2 = x1 + step;
                if (x2 > r.xMax())
                  x2 = r.xMax();

                _search->addBox(
                    x1, r.yMin(), x2, r.yMax(), level, s->getId(), 0, wtype);
                x1 = x2;
              }
            }
          }
        } else {
          _search->addBox(r.xMin(),
                          r.yMin(),
                          r.xMax(),
                          r.yMax(),
                          level,
                          s->getId(),
                          0,
                          wtype);
        }

        cnt++;
      }
    }
  }
  return cnt;
}
uint extMain::signalWireCounter(uint& maxWidth)
{
  uint cnt = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType().isSupply()))
      continue;

    dbWire* wire = net->getWire();
    if (wire == NULL)
      continue;

    dbWireShapeItr shapes;
    dbShape s;
    for (shapes.begin(wire); shapes.next(s);) {
      if (s.isVia())
        continue;

      uint x = s.getDX();
      uint y = s.getDY();
      uint w = y;
      if (w > x)
        w = x;

      if (maxWidth < w)
        maxWidth = w;
    }

    uint wireCnt = 0;
    uint viaCnt = 0;
    net->getSignalWireCount(wireCnt, viaCnt);

    cnt += wireCnt;
  }
  return cnt;
}
uint extMain::addPowerNets(uint dir,
                           int* bb_ll,
                           int* bb_ur,
                           uint wtype,
                           dbCreateNetUtil* netUtil)
{
  uint cnt = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if (!((net->getSigType().isSupply())))
      continue;

    cnt += addNetSBoxes(net, dir, bb_ll, bb_ur, wtype, netUtil);
  }
  return cnt;
}

double extMain::GetDBcoords1(int coord)
{
  int db_factor = _block->getDbUnitsPerMicron();
  return 1.0 * coord / db_factor;
}

int extMain::GetDBcoords2(int coord)
{
  int db_factor = _block->getDbUnitsPerMicron();
  double d = (1.0 * coord) / db_factor;
  int n = (int) ceil(1000 * d);
  return n;
}

uint extMain::addNetShapesOnSearch(dbNet* net,
                                   uint dir,
                                   int* bb_ll,
                                   int* bb_ur,
                                   uint wtype,
                                   FILE* fp,
                                   dbCreateNetUtil* netUtil)
{
  bool USE_DB_UNITS = false;

  dbWire* wire = net->getWire();

  if (wire == NULL)
    return 0;

  if (netUtil != NULL)
    netUtil->setCurrentNet(NULL);

  uint cnt = 0;

  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    int shapeId = shapes.getShapeId();

    if (s.isVia()) {
      if (!_skip_via_wires)
        addViaBoxes(s, net, shapeId, wtype);

      continue;
    }

    Rect r = s.getBox();
    if (isIncludedInsearch(r, dir, bb_ll, bb_ur)) {
      uint level = s.getTechLayer()->getRoutingLevel();

      if (_geoThickTable != NULL) {
        cnt += addMultipleRectsOnSearch(
            r, level, dir, net->getId(), shapeId, wtype);
        continue;
      }
      if (netUtil != NULL) {
        netUtil->createNetSingleWire(r, level, net->getId(), shapeId);
      } else {
        int dx = r.xMax() - r.xMin();
        int dy = r.yMax() - r.yMin();
        int via_ext = 32;

        uint trackNum = 0;
        if (trackNum > 0) {
          if (dy > dx) {
            trackNum = _search->addBox(r.xMin(),
                                       r.yMin() - via_ext,
                                       r.xMax(),
                                       r.yMax() + via_ext,
                                       level,
                                       net->getId(),
                                       shapeId,
                                       wtype);
          } else {
            trackNum = _search->addBox(r.xMin() - via_ext,
                                       r.yMin(),
                                       r.xMax() + via_ext,
                                       r.yMax(),
                                       level,
                                       net->getId(),
                                       shapeId,
                                       wtype);
          }
        } else {
          if (USE_DB_UNITS) {
            trackNum = _search->addBox(GetDBcoords2(r.xMin()),
                                       GetDBcoords2(r.yMin()),
                                       GetDBcoords2(r.xMax()),
                                       GetDBcoords2(r.yMax()),
                                       level,
                                       net->getId(),
                                       shapeId,
                                       wtype);
          } else {
            trackNum = _search->addBox(r.xMin(),
                                       r.yMin(),
                                       r.xMax(),
                                       r.yMax(),
                                       level,
                                       net->getId(),
                                       shapeId,
                                       wtype);
            if (net->getId() == _debug_net_id) {
              debugPrint(
                  logger_,
                  RCX,
                  "debug_net",
                  1,
                  "\t[Search:W]"
                  "\tonSearch: tr={} L{}  DX={} DY={} {} {}  {} {} -- {:.3f} "
                  "{:.3f}  {:.3f} {:.3f} net {}",
                  trackNum,
                  level,
                  dx,
                  dy,
                  r.xMin(),
                  r.yMin(),
                  r.xMax(),
                  r.yMax(),
                  GetDBcoords1(r.xMin()),
                  GetDBcoords1(r.yMin()),
                  GetDBcoords1(r.xMax()),
                  GetDBcoords1(r.yMax()),
                  net->getId());
            }
          }
        }

        if (_searchFP != NULL) {
          fprintf(_searchFP,
                  "%d  %d %d  %d %d %d\n",
                  level,
                  r.xMin(),
                  r.yMin(),
                  r.xMax(),
                  r.yMax(),
                  trackNum);
        }
      }

      cnt++;
    }
  }
  return cnt;
}

uint extMain::addViaBoxes(dbShape& sVia, dbNet* net, uint shapeId, uint wtype)
{
  wtype = 5;  // Via Type

  bool USE_DB_UNITS = false;
  uint cnt = 0;

  std::vector<dbShape> shapes;
  dbShape::getViaBoxes(sVia, shapes);

  std::vector<dbShape>::iterator shape_itr;
  for (shape_itr = shapes.begin(); shape_itr != shapes.end(); ++shape_itr) {
    dbShape s = *shape_itr;

    if (s.getTechLayer()->getType() == dbTechLayerType::CUT)
      continue;

    int x1 = s.xMin();
    int y1 = s.yMin();
    int x2 = s.xMax();
    int y2 = s.yMax();
    int dx = x2 - x1;
    int dy = y2 - y1;

    uint level = s.getTechLayer()->getRoutingLevel();
    uint width = s.getTechLayer()->getWidth();

    if (s.getTechLayer()->getDirection() == dbTechLayerDir::VERTICAL) {
      if (width != dx)
        continue;
    } else {
      if (width != dy)
        continue;
    }

    if (USE_DB_UNITS) {
      _search->addBox(GetDBcoords2(x1),
                      GetDBcoords2(y1),
                      GetDBcoords2(x2),
                      GetDBcoords2(y2),
                      level,
                      net->getId(),
                      shapeId,
                      wtype);
    } else {
      _search->addBox(x1, y1, x2, y2, level, net->getId(), shapeId, wtype);
    }
  }
  return cnt;
}

uint extMain::addSignalNets(uint dir,
                            int* bb_ll,
                            int* bb_ur,
                            uint wtype,
                            dbCreateNetUtil* createDbNet)
{
  uint cnt = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  FILE* fp = NULL;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType().isSupply()))
      continue;

    cnt += addNetShapesOnSearch(net, dir, bb_ll, bb_ur, wtype, fp, createDbNet);
  }
  if (createDbNet == NULL)
    _search->adjustOverlapMakerEnd();

  return cnt;
}
uint extMain::addPowerNets2(uint dir,
                            int* bb_ll,
                            int* bb_ur,
                            uint wtype,
                            Ath__array1D<uint>* sdbPowerTable,
                            dbCreateNetUtil* createDbNet)
{
  uint cnt = 0;
  uint netCnt = sdbPowerTable->getCnt();

  if (createDbNet != NULL)
    createDbNet->createSpecialNet(NULL, "POWER_SDB");

  for (uint ii = 0; ii < netCnt; ii++) {
    dbNet* net = dbNet::getNet(_block, sdbPowerTable->get(ii));

    cnt += addNetSBoxes(net, dir, bb_ll, bb_ur, wtype, createDbNet);
  }
  return cnt;
}
void extMain::resetNetSpefFlag(Ath__array1D<uint>* tmpNetIdTable)
{
  for (uint ii = 0; ii < tmpNetIdTable->getCnt(); ii++) {
    uint netId = tmpNetIdTable->get(ii);
    dbNet* net = dbNet::getNet(_block, netId);
    net->setSpef(false);
  }
}
uint extWireBin::createDbNetsGS(dbBlock* block, dbCreateNetUtil* createDbNet)
{
  uint cnt = 0;

  uint prevNetId = 0;
  dbNet* net = NULL;
  dbWire* wire = NULL;

  char netName[128];
  sprintf(netName, "GS_%d_%d", _dir, _num);
  createDbNet->setCurrentNet(NULL);

  createDbNet->createSpecialNet(NULL, netName);

  for (uint ii = 0; ii < _table->getCnt(); ii++) {
    extWire* w = _table->get(ii);

    if (w->_netId != prevNetId) {
      prevNetId = w->_netId;
      net = dbNet::getNet(block, w->_netId);
      wire = net->getWire();
    }

    Rect r;
    if (w->_shapeId < 0) {
      dbSBox* s = dbSBox::getSBox(block, -w->_shapeId);
      r = s->getBox();
    } else {
      dbShape s;
      wire->getSegment(w->_shapeId, w->_layer, s);

      r = s.getBox();
    }
    createDbNet->createSpecialWire(NULL, r, w->_layer, 0);
    cnt++;
  }
  return cnt;
}

uint extWireBin::createDbNets(dbBlock* block, dbCreateNetUtil* createDbNet)
{
  uint cnt = 0;

  uint prevNetId = 0;
  dbNet* net = NULL;
  dbWire* wire = NULL;

  for (uint ii = 0; ii < _table->getCnt(); ii++) {
    extWire* w = _table->get(ii);

    if (w->_netId != prevNetId) {
      prevNetId = w->_netId;
      net = dbNet::getNet(block, w->_netId);
      createDbNet->checkAndSet(w->_netId);

      if (w->_shapeId < 0)
        createDbNet->createSpecialNet(net, NULL);
      else
        wire = net->getWire();
    }
    if (w->_shapeId < 0) {
      dbSBox* s = dbSBox::getSBox(block, -w->_shapeId);

      Rect r = s->getBox();

      createDbNet->createSpecialWire(NULL, r, w->_layer, -w->_shapeId);
    } else {
      dbShape s;
      wire->getSegment(w->_shapeId, w->_layer, s);
      Rect r = s.getBox();

      createDbNet->createNetSingleWire(
          r, w->_layer->getRoutingLevel(), w->_netId, w->_shapeId);
    }
    cnt++;
  }
  return cnt;
}

uint extMain::addInsts(uint dir,
                       int* lo_gs,
                       int* hi_gs,
                       int* bb_ll,
                       int* bb_ur,
                       uint bucketSize,
                       Ath__array1D<uint>*** wireBinTable,
                       dbCreateNetUtil* createDbNet)
{
  uint lo_index = getBucketNum(bb_ll[dir], bb_ur[dir], bucketSize, lo_gs[dir]);
  uint hi_index = getBucketNum(bb_ll[dir], bb_ur[dir], bucketSize, hi_gs[dir]);

  uint cnt = 0;
  uint bucket;
  for (bucket = lo_index; bucket <= hi_index; bucket++) {
    if (wireBinTable[dir][bucket] == NULL)
      continue;

    uint instCnt = wireBinTable[dir][bucket]->getCnt();
    for (uint ii = 0; ii < instCnt; ii++) {
      uint instId = wireBinTable[dir][bucket]->get(ii);
      dbInst* inst0 = dbInst::getInst(_block, instId);

      if (inst0->getUserFlag1())
        continue;

      inst0->setUserFlag1();

      createDbNet->createInst(inst0);
      cnt++;
    }
  }
  for (bucket = lo_index; bucket <= hi_index; bucket++) {
    if (wireBinTable[dir][bucket] == NULL)
      continue;

    uint instCnt = wireBinTable[dir][bucket]->getCnt();
    for (uint ii = 0; ii < instCnt; ii++) {
      uint instId = wireBinTable[dir][bucket]->get(ii);
      dbInst* inst0 = dbInst::getInst(_block, instId);

      inst0->clearUserFlag1();
    }
  }
  return cnt;
}
uint extMain::addSignalNets2(uint dir,
                             int* lo_sdb,
                             int* hi_sdb,
                             int* bb_ll,
                             int* bb_ur,
                             uint* bucketSize,
                             uint wtype,
                             Ath__array1D<uint>*** sdbSignalTable,
                             Ath__array1D<uint>* tmpNetIdTable,
                             dbCreateNetUtil* createDbNet)
{
  if (sdbSignalTable == NULL)
    return 0;

  uint lo_index
      = getBucketNum(bb_ll[dir], bb_ur[dir], bucketSize[dir], lo_sdb[dir]);
  uint hi_index
      = getBucketNum(bb_ll[dir], bb_ur[dir], bucketSize[dir], hi_sdb[dir]);

  FILE* fp = NULL;

  uint cnt = 0;
  for (uint bucket = lo_index; bucket <= hi_index; bucket++) {
    if (sdbSignalTable[dir][bucket] == NULL)
      continue;

    uint netCnt = sdbSignalTable[dir][bucket]->getCnt();
    for (uint ii = 0; ii < netCnt; ii++) {
      uint netId = sdbSignalTable[dir][bucket]->get(ii);
      dbNet* net = dbNet::getNet(_block, netId);

      if (net->isSpef())
        continue;
      net->setSpef(true);
      tmpNetIdTable->add(netId);

      cnt += addNetShapesOnSearch(
          net, dir, lo_sdb, hi_sdb, wtype, fp, createDbNet);
    }
  }
  _search->adjustOverlapMakerEnd();

  resetNetSpefFlag(tmpNetIdTable);

  return cnt;
}
void extMain::reportTableNetCnt(uint* sdbBucketCnt,
                                Ath__array1D<uint>*** sdbSignalTable)
{
  for (uint dir = 0; dir < 2; dir++) {
    logger_->info(RCX,
                  83,
                  "DIR= {} -----------------------------------------------",
                  dir);

    for (uint bucket = 0; bucket < sdbBucketCnt[dir]; bucket++) {
      if (sdbSignalTable[dir][bucket] == NULL)
        continue;

      uint netCnt = sdbSignalTable[dir][bucket]->getCnt();
      logger_->info(RCX, 84, "\tbucket= {} -- {} nets", bucket, netCnt);
    }
  }
}
uint extMain::addNets(uint dir,
                      int* bb_ll,
                      int* bb_ur,
                      uint wtype,
                      uint ptype,
                      Ath__array1D<uint>* sdbSignalTable)
{
  if (sdbSignalTable == NULL)
    return 0;

  uint cnt = 0;
  uint netCnt = sdbSignalTable->getCnt();
  for (uint ii = 0; ii < netCnt; ii++) {
    dbNet* net = dbNet::getNet(_block, sdbSignalTable->get(ii));

    if ((net->getSigType().isSupply()))
      cnt += addNetSBoxes(net, dir, bb_ll, bb_ur, ptype);
    else
      cnt += addNetShapesOnSearch(net, dir, bb_ll, bb_ur, wtype, NULL);
  }
  return cnt;
}

uint extMain::getDir(Rect& r)
{
  return getDir(r.xMin(), r.yMin(), r.xMax(), r.yMax());
}
uint extMain::getDir(int x1, int y1, int x2, int y2)
{
  uint dx = x2 - x1;
  uint dy = y2 - y1;
  uint dd = 0;  // vertical
  if (dx > dy)
    dd = 1;  // horizontal

  return dd;
}
uint extMain::addShapeOnGS(dbNet* net,
                           uint sId,
                           Rect& r,
                           bool plane,
                           dbTechLayer* layer,
                           bool gsRotated,
                           bool swap_coords,
                           int dir,
                           bool specialWire,
                           dbCreateNetUtil* createDbNet)
{
  if (dir >= 0) {
    if (!plane) {
      if (matchDir(dir, r))
        return 0;
    }
  }
  bool checkFlag = false;
  if (createDbNet != NULL)
    checkFlag = true;

  uint level = layer->getRoutingLevel();
  int n = 0;
  if (!gsRotated) {
    n = _geomSeq->box(r.xMin(), r.yMin(), r.xMax(), r.yMax(), level, checkFlag);
  } else {
    if (!swap_coords)  // horizontal
      n = _geomSeq->box(
          r.xMin(), r.yMin(), r.xMax(), r.yMax(), level, checkFlag);
    else
      n = _geomSeq->box(
          r.yMin(), r.xMin(), r.yMax(), r.xMax(), level, checkFlag);
  }
  if (n == 0) {
    if (createDbNet != NULL) {
      if (specialWire)
        createDbNet->createSpecialWire(NULL, r, layer, sId);
      else
        createDbNet->createNetSingleWire(r, level, net->getId(), sId);
    }
    return 1;
  }
  return 0;
}
uint extMain::addNetShapesGs(dbNet* net,
                             bool gsRotated,
                             bool swap_coords,
                             int dir,
                             dbCreateNetUtil* createDbNet)
{
  bool USE_DB_UNITS = false;
  uint cnt = 0;
  dbWire* wire = net->getWire();
  if (wire == NULL)
    return 0;

  bool plane = false;
  if (net->getSigType() == dbSigType::ANALOG)
    plane = true;

  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    if (s.isVia())
      continue;

    int shapeId = shapes.getShapeId();

    Rect r = s.getBox();

    if (USE_DB_UNITS)
      this->GetDBcoords2(r);

    cnt += addShapeOnGS(net,
                        shapeId,
                        r,
                        plane,
                        s.getTechLayer(),
                        gsRotated,
                        swap_coords,
                        dir,
                        true,
                        createDbNet);
  }
  return cnt;
}
uint extMain::addNetSboxesGs(dbNet* net,
                             bool gsRotated,
                             bool swap_coords,
                             int dir,
                             dbCreateNetUtil* createDbNet)
{
  uint cnt = 0;

  dbSet<dbSWire> swires = net->getSWires();
  dbSet<dbSWire>::iterator itr;

  for (itr = swires.begin(); itr != swires.end(); ++itr) {
    dbSWire* swire = *itr;
    dbSet<dbSBox> wires = swire->getWires();
    dbSet<dbSBox>::iterator box_itr;

    for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
      dbSBox* s = *box_itr;

      if (s->isVia())
        continue;

      Rect r = s->getBox();
      cnt += addShapeOnGS(NULL,
                          s->getId(),
                          r,
                          true,
                          s->getTechLayer(),
                          gsRotated,
                          swap_coords,
                          dir,
                          true,
                          createDbNet);
    }
  }
  return cnt;
}
int extMain::getXY_gs(int base, int XY, uint minRes)
{
  uint maxRow = (XY - base) / minRes;
  int v = base + maxRow * minRes;
  return v;
}
uint extMain::initPlanes(uint dir,
                         int* wLL,
                         int* wUR,
                         uint layerCnt,
                         uint* pitchTable,
                         uint* widthTable,
                         uint* dirTable,
                         int* bb_ll,
                         bool skipMemAlloc)
{
  bool rotatedFlag = getRotatedFlag();

  if (_geomSeq != NULL)
    delete _geomSeq;
  _geomSeq = new gs(_seqPool);

  _geomSeq->setSlices(layerCnt);

  for (uint ii = 1; ii < layerCnt; ii++) {
    uint layerDir = dirTable[ii];

    uint res[2] = {pitchTable[ii], widthTable[ii]};  // vertical
    if (layerDir > 0) {                              // horizontal
      res[0] = widthTable[ii];
      res[1] = pitchTable[ii];
    }
    if (res[dir] == 0)
      continue;
    int ll[2];
    ll[!dir] = bb_ll[!dir];
    ll[dir] = getXY_gs(bb_ll[dir], wLL[dir], res[dir]);

    int ur[2];
    ur[!dir] = wUR[!dir];
    ur[dir] = getXY_gs(bb_ll[dir], wUR[dir], res[dir]);

    if (!rotatedFlag) {
      _geomSeq->configureSlice(
          ii, res[0], res[1], ll[0], ll[1], ur[0], ur[1], skipMemAlloc);
    } else {
      if (dir > 0) {  // horizontal segment extraction
        _geomSeq->configureSlice(
            ii, res[0], res[1], ll[0], ll[1], ur[0], ur[1], skipMemAlloc);
      } else {
        if (layerDir > 0) {
          _geomSeq->configureSlice(ii,
                                   pitchTable[ii],
                                   widthTable[ii],
                                   ll[1],
                                   ll[0],
                                   ur[1],
                                   ur[0],
                                   skipMemAlloc);

        } else {
          _geomSeq->configureSlice(ii,
                                   widthTable[ii],
                                   pitchTable[ii],
                                   ll[1],
                                   ll[0],
                                   ur[1],
                                   ur[0],
                                   skipMemAlloc);
        }
      }
    }
  }
  return layerCnt;
}
uint extMain::addInstsGs(Ath__array1D<uint>* instTable,
                         Ath__array1D<uint>* tmpInstIdTable,
                         uint dir)
{
  if (instTable == NULL)
    return 0;

  bool rotatedGs = getRotatedFlag();

  uint cnt = 0;
  uint instCnt = instTable->getCnt();

  for (uint ii = 0; ii < instCnt; ii++) {
    uint instId = instTable->get(ii);
    dbInst* inst = dbInst::getInst(_block, instId);

    if (tmpInstIdTable != NULL) {
      if (inst->getUserFlag1())
        continue;

      inst->setUserFlag1();
      tmpInstIdTable->add(instId);
    }

    cnt += addItermShapesOnPlanes(inst, rotatedGs, !dir);
    cnt += addObsShapesOnPlanes(inst, rotatedGs, !dir);
  }
  if (tmpInstIdTable != NULL) {
    for (uint jj = 0; jj < tmpInstIdTable->getCnt(); jj++) {
      uint instId = instTable->get(jj);
      dbInst* inst = dbInst::getInst(_block, instId);

      inst->clearUserFlag1();
    }
  }
  return cnt;
}
bool extMain::getRotatedFlag()
{
  return _rotatedGs;
}
void extMain::disableRotatedFlag()
{
  _rotatedGs = false;
}
bool extMain::enableRotatedFlag()
{
  _rotatedGs = (_use_signal_tables > 1) ? true : false;

  return _rotatedGs;
}
uint extMain::fill_gs4(int dir,
                       int* ll,
                       int* ur,
                       int* lo_gs,
                       int* hi_gs,
                       uint layerCnt,
                       uint* dirTable,
                       uint* pitchTable,
                       uint* widthTable,
                       dbCreateNetUtil* createDbNet)
{
  bool rotatedGs = getRotatedFlag();

  bool skipMemAlloc = false;
  if (createDbNet != NULL)
    skipMemAlloc = true;

  initPlanes(dir,
             lo_gs,
             hi_gs,
             layerCnt,
             pitchTable,
             widthTable,
             dirTable,
             ll,
             skipMemAlloc);

  const int gs_dir = dir;

  uint pcnt = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if (!((net->getSigType().isSupply())))
      continue;

    if (createDbNet != NULL)
      createDbNet->createSpecialNet(net, NULL);

    pcnt += addNetSboxesGs(net, rotatedGs, !dir, gs_dir, createDbNet);
  }

  uint scnt = 0;

  if (createDbNet != NULL)
    createDbNet->createSpecialNet(NULL, "SIGNALS_GS");

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType().isSupply()))
      continue;

    scnt += addNetShapesGs(net, rotatedGs, !dir, gs_dir, createDbNet);
  }

  return pcnt + scnt;
}

int extMain::fill_gs3(int dir,
                      int* ll,
                      int* ur,
                      int* lo_gs,
                      int* hi_gs,
                      uint layerCnt,
                      uint* dirTable,
                      uint* pitchTable,
                      uint* widthTable,
                      int* sdbTable_ll,
                      int* sdbTable_ur,
                      uint* bucketSize,
                      Ath__array1D<uint>* powerNetTable,
                      Ath__array1D<uint>* tmpNetIdTable,
                      Ath__array1D<uint>*** sdbSignalTable,
                      Ath__array1D<uint>*** instGsTable,
                      dbCreateNetUtil* createDbNet)
{
  if (sdbSignalTable == NULL)
    return 0;

  bool rotatedGs = getRotatedFlag();

  bool skipMemAlloc = false;
  if (createDbNet != NULL)
    skipMemAlloc = true;

  initPlanes(dir,
             lo_gs,
             hi_gs,
             layerCnt,
             pitchTable,
             widthTable,
             dirTable,
             ll,
             skipMemAlloc);

  const int gs_dir = dir;

  uint netCnt = powerNetTable->getCnt();
  for (uint ii = 0; ii < netCnt; ii++) {
    dbNet* net = dbNet::getNet(_block, powerNetTable->get(ii));

    if (createDbNet != NULL)
      createDbNet->createSpecialNet(net, NULL);

    addNetSboxesGs(net, rotatedGs, !dir, gs_dir, createDbNet);
  }
  uint lo_index = getBucketNum(
      sdbTable_ll[dir], sdbTable_ur[dir], bucketSize[dir], lo_gs[dir]);
  uint hi_index = getBucketNum(
      sdbTable_ll[dir], sdbTable_ur[dir], bucketSize[dir], hi_gs[dir]);

  if (createDbNet != NULL)
    createDbNet->createSpecialNet(NULL, "SIGNALS_GS");

  for (uint bucket = lo_index; bucket <= hi_index; bucket++) {
    if (sdbSignalTable[dir][bucket] == NULL)
      continue;

    uint netCnt = sdbSignalTable[dir][bucket]->getCnt();

    for (uint ii = 0; ii < netCnt; ii++) {
      uint netId = sdbSignalTable[dir][bucket]->get(ii);
      dbNet* net = dbNet::getNet(_block, netId);

      if (net->isSpef())  // tmp flag
        continue;
      net->setSpef(true);
      tmpNetIdTable->add(netId);

      addNetShapesGs(net, rotatedGs, !dir, gs_dir, createDbNet);
    }
  }
  resetNetSpefFlag(tmpNetIdTable);

  return lo_gs[dir];
}

void extMain::resetGndCaps()
{
  uint cornerCnt = _block->getCornerCount();

  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType().isSupply()))
      continue;

    dbSet<dbRSeg> rcSet = net->getRSegs();

    dbSet<dbRSeg>::iterator rc_itr;

    for (rc_itr = rcSet.begin(); rc_itr != rcSet.end(); ++rc_itr) {
      dbRSeg* rc = *rc_itr;

      for (uint ii = 0; ii < cornerCnt; ii++)
        rc->setCapacitance(0.0, ii);

      uint n = rc->getShapeId();
      uint n1 = rc->getTargetCapNode()->getShapeId();
      uint n2 = rc->getSourceCapNode()->getShapeId();

      if (n != n1) {
        logger_->info(RCX,
                      85,
                      "shapeIds {}: rc= {} tgt= {} src {}",
                      net->getConstName(),
                      n,
                      n1,
                      n2);
      }
    }
  }
}
uint extMain::couplingFlow(Rect& extRect,
                           uint ccFlag,
                           extMeasure* m,
                           CoupleAndCompute coupleAndCompute)
{
  uint ccDist = ccFlag;

  uint sigtype = 9;
  uint pwrtype = 11;

  uint pitchTable[32];
  uint widthTable[32];
  for (uint ii = 0; ii < 32; ii++) {
    pitchTable[ii] = 0;
    widthTable[ii] = 0;
  }
  uint dirTable[16];
  int baseX[32];
  int baseY[32];
  uint layerCnt = initSearchForNets(
      baseX, baseY, pitchTable, widthTable, dirTable, extRect, false);
  for (uint i = 0; i < layerCnt + 1; i++)
    m->_dirTable[i] = dirTable[i];

  uint maxPitch = pitchTable[layerCnt - 1];

  layerCnt = (int) layerCnt > _currentModel->getLayerCnt()
                 ? layerCnt
                 : _currentModel->getLayerCnt();
  int ll[2];
  int ur[2];
  ll[0] = extRect.xMin();
  ll[1] = extRect.yMin();
  ur[0] = extRect.xMax();
  ur[1] = extRect.yMax();

  int lo_gs[2];
  int hi_gs[2];
  int lo_sdb[2];
  int hi_sdb[2];

  Ath__overlapAdjust overlapAdj = Z_noAdjust;
  _useDbSdb = true;
  _search->setExtControl(_block,
                         _useDbSdb,
                         (uint) overlapAdj,
                         _CCnoPowerSource,
                         _CCnoPowerTarget,
                         _ccUp,
                         _allNet,
                         _ccContextDepth,
                         _ccContextArray,
                         _ccContextLength,
                         _dgContextArray,
                         &_dgContextDepth,
                         &_dgContextPlanes,
                         &_dgContextTracks,
                         &_dgContextBaseLvl,
                         &_dgContextLowLvl,
                         &_dgContextHiLvl,
                         _dgContextBaseTrack,
                         _dgContextLowTrack,
                         _dgContextHiTrack,
                         _dgContextTrackBase,
                         m->_seqPool);

  _seqPool = m->_seqPool;

  uint maxWidth = 0;
  uint totPowerWireCnt = powerWireCounter(maxWidth);
  uint totWireCnt = signalWireCounter(maxWidth);
  totWireCnt += totPowerWireCnt;

  logger_->info(RCX, 43, "{} wires to be extracted", totWireCnt);

  uint minRes[2];
  minRes[1] = pitchTable[1];
  minRes[0] = widthTable[1];

  const uint trackStep = 1000;
  uint step_nm[2];
  step_nm[1] = trackStep * minRes[1];
  step_nm[0] = trackStep * minRes[1];
  if (maxWidth > ccDist * maxPitch) {
    step_nm[1] = ur[1] - ll[1];
    step_nm[0] = ur[0] - ll[0];
  }
  // _use_signal_tables
  Ath__array1D<uint>** sdbSignalTable[2];
  Ath__array1D<uint>** gsInstTable[2];
  Ath__array1D<uint> sdbPowerTable;
  Ath__array1D<uint> tmpNetIdTable(64000);
  uint sdbBucketCnt[2];
  uint sdbBucketSize[2];
  int sdbTable_ll[2];
  int sdbTable_ur[2];

  bool use_signal_tables = false;
  if ((_use_signal_tables == 1) || (_use_signal_tables == 2)) {
    use_signal_tables = true;

    logger_->info(RCX,
                  467,
                  "Signal_table= {} ----------------------------- ",
                  _use_signal_tables);

    for (uint ii = 0; ii < 2; ii++) {
      sdbBucketCnt[ii] = 0;
      sdbBucketSize[ii] = 2 * step_nm[ii];
      sdbTable_ll[ii] = ll[ii] - step_nm[ii] / 10;
      sdbTable_ur[ii] = ur[ii] + step_nm[ii] / 10;
    }

    mkSignalTables2(sdbBucketSize,
                    sdbTable_ll,
                    sdbTable_ur,
                    sdbSignalTable,
                    &sdbPowerTable,
                    gsInstTable,
                    sdbBucketCnt);

    reportTableNetCnt(sdbBucketCnt, sdbSignalTable);
  }
  uint totalWiresExtracted = 0;

  int** limitArray;
  limitArray = new int*[layerCnt];
  for (uint jj = 0; jj < layerCnt; jj++)
    limitArray[jj] = new int[10];

  FILE* bandinfo = NULL;
  if (_printBandInfo) {
    if (_getBandWire)
      bandinfo = fopen("bandInfo.getWire", "w");
    else
      bandinfo = fopen("bandInfo.extract", "w");
  }
  for (int dir = 1; dir >= 0; dir--) {
    if (_printBandInfo)
      fprintf(bandinfo, "dir = %d\n", dir);
    if (dir == 0)
      enableRotatedFlag();

    lo_gs[!dir] = ll[!dir];
    hi_gs[!dir] = ur[!dir];
    lo_sdb[!dir] = ll[!dir];
    hi_sdb[!dir] = ur[!dir];

    int minExtracted = ll[dir];
    int gs_limit = ll[dir];

    _search->initCouplingCapLoops(dir, ccFlag, coupleAndCompute, m);

    lo_sdb[dir] = ll[dir] - step_nm[dir];
    int hiXY = ll[dir] + step_nm[dir];
    if (hiXY > ur[dir])
      hiXY = ur[dir];

    uint stepNum = 0;
    for (; hiXY <= ur[dir]; hiXY += step_nm[dir]) {
      if (ur[dir] - hiXY <= (int) step_nm[dir])
        hiXY = ur[dir] + 5 * ccDist * maxPitch;

      lo_gs[dir] = gs_limit;
      hi_gs[dir] = hiXY;

      if (use_signal_tables) {
        tmpNetIdTable.resetCnt();
        fill_gs3(dir,
                 ll,
                 ur,
                 lo_gs,
                 hi_gs,
                 layerCnt,
                 dirTable,
                 pitchTable,
                 widthTable,
                 sdbTable_ll,
                 sdbTable_ur,
                 sdbBucketSize,
                 &sdbPowerTable,
                 &tmpNetIdTable,
                 sdbSignalTable,
                 gsInstTable);
      } else
        fill_gs4(dir,
                 ll,
                 ur,
                 lo_gs,
                 hi_gs,
                 layerCnt,
                 dirTable,
                 pitchTable,
                 widthTable,
                 NULL);

      m->_rotatedGs = getRotatedFlag();
      m->_pixelTable = _geomSeq;

      // add wires onto search such that    loX<=loX<=hiX
      hi_sdb[dir] = hiXY;

      uint processWireCnt = 0;
      if (use_signal_tables) {
        processWireCnt
            += addPowerNets2(dir, lo_sdb, hi_sdb, pwrtype, &sdbPowerTable);
        tmpNetIdTable.resetCnt();
        processWireCnt += addSignalNets2(dir,
                                         lo_sdb,
                                         hi_sdb,
                                         sdbTable_ll,
                                         sdbTable_ur,
                                         sdbBucketSize,
                                         sigtype,
                                         sdbSignalTable,
                                         &tmpNetIdTable);
      } else {
        processWireCnt += addPowerNets(dir, lo_sdb, hi_sdb, pwrtype);
        processWireCnt += addSignalNets(dir, lo_sdb, hi_sdb, sigtype);
      }

      uint extractedWireCnt = 0;
      int extractLimit = hiXY - ccDist * maxPitch;
      minExtracted = _search->couplingCaps(extractLimit,
                                           ccFlag,
                                           dir,
                                           extractedWireCnt,
                                           coupleAndCompute,
                                           m,
                                           _getBandWire,
                                           limitArray);

      int deallocLimit = minExtracted - (ccDist + 1) * maxPitch;
      if (_printBandInfo)
        fprintf(bandinfo,
                "    step %d  hiXY=%d extLimit=%d minExtracted=%d "
                "deallocLimit=%d\n",
                stepNum,
                hiXY,
                extractLimit,
                minExtracted,
                deallocLimit);
      _search->dealloc(dir, deallocLimit);

      lo_sdb[dir] = hiXY;
      gs_limit = minExtracted - (ccDist + 2) * maxPitch;

      stepNum++;
      totalWiresExtracted += processWireCnt;
      float percent_extracted
          = Ath__double2int(100.0 * (1.0 * totalWiresExtracted / totWireCnt));

      if ((totWireCnt > 0) && (totalWiresExtracted > 0)
          && (percent_extracted - _previous_percent_extracted >= 5.0)) {
        logger_->info(RCX,
                      442,
                      "{:d}% completion -- {:d} wires have been extracted",
                      (int) (100.0 * (1.0 * totalWiresExtracted / totWireCnt)),
                      totalWiresExtracted);

        _previous_percent_extracted = percent_extracted;
      }
    }
  }
  if (_printBandInfo)
    fclose(bandinfo);
  if (use_signal_tables) {
    freeSignalTables(true, sdbSignalTable, NULL, sdbBucketCnt);
  }

  if (_geomSeq != NULL) {
    delete _geomSeq;
    _geomSeq = NULL;
  }

  return 0;
}

uint extMain::createNetShapePropertires(dbBlock* blk)
{
  dbSet<dbNet> nets = blk->getNets();
  dbSet<dbNet>::iterator net_itr;

  uint cnt = 0;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType().isSupply()))
      continue;

    dbWirePath path;
    dbWirePathShape pshape;

    dbWirePathItr pitr;
    dbWire* wire = net->getWire();

    for (pitr.begin(wire); pitr.getNextPath(path);) {
      pitr.getNextShape(pshape);
      uint n = pshape.junction_id;

      int map = 0;
      wire->getProperty(n, map);
      if (map > 0) {
        char bufName[16];
        sprintf(bufName, "J%d", n);
        dbIntProperty::create(net, bufName, map);
        wire->setProperty(n, 0);
      }
      cnt++;
    }
  }
  return cnt;
}
uint extMain::rcGenTile(dbBlock* blk)
{
  Ath__parser parser;

  dbSet<dbNet> nets = blk->getNets();
  dbSet<dbNet>::iterator net_itr;

  uint cnt = 0;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType().isSupply()))
      continue;

    dbNet* mainNet = getDstNet(net, _block, &parser);

    if (mainNet->isRCgraph())
      continue;

    cnt += rcNetGen(mainNet);
  }
  return cnt;
}

dbNet* extMain::getDstNet(dbNet* net, dbBlock* dstBlock, Ath__parser* parser)
{
  parser->mkWords(net->getConstName());
  int dstNetId = parser->getInt(0, 1);
  dbNet* dstNet = dbNet::getNet(dstBlock, dstNetId);
  return dstNet;
}
dbRSeg* extMain::getMainRSeg3(dbNet* srcNet, int srcShapeId, dbNet* dstNet)
{
  char bufName[16];
  sprintf(bufName, "J%d", srcShapeId);
  dbProperty* p = dbProperty::find(srcNet, bufName);
  if (p == NULL)
    return NULL;

  dbIntProperty* ip = (dbIntProperty*) p;
  uint rsegId2 = ip->getValue();

  if (rsegId2 == 0)
    return NULL;

  dbRSeg* rseg2 = dbRSeg::getRSeg(dstNet->getBlock(), rsegId2);

  return rseg2;
}
dbRSeg* extMain::getMainRSeg(dbNet* srcNet, int srcShapeId, dbNet* dstNet)
{
  char bufName[16];
  sprintf(bufName, "J%d", srcShapeId);
  dbProperty* p = dbProperty::find(srcNet, bufName);
  if (p == NULL)
    return NULL;

  dbIntProperty* ip = (dbIntProperty*) p;
  uint jid2 = ip->getValue();

  int rsegId2 = 0;
  dstNet->getWire()->getProperty(jid2, rsegId2);
  if (rsegId2 == 0)
    return NULL;

  dbRSeg* rseg2 = dbRSeg::getRSeg(dstNet->getBlock(), rsegId2);

  return rseg2;
}
dbRSeg* extMain::getMainRSeg2(dbNet* srcNet, int srcShapeId, dbNet* dstNet)
{
  int rsegId2 = 0;
  dstNet->getWire()->getProperty(srcShapeId, rsegId2);
  if (rsegId2 == 0)
    return NULL;

  dbRSeg* rseg2 = dbRSeg::getRSeg(dstNet->getBlock(), rsegId2);

  return rseg2;
}
dbRSeg* extMain::getRseg(dbNet* net, uint shapeId, Logger* logger)
{
  int rsegId2 = 0;
  net->getWire()->getProperty(shapeId, rsegId2);
  if (rsegId2 == 0) {
    logger->warn(RCX,
                 239,
                 "Zero rseg wire property {} on main net {} {}",
                 shapeId,
                 net->getId(),
                 net->getConstName());
    return NULL;
  }
  dbRSeg* rseg2 = dbRSeg::getRSeg(net->getBlock(), rsegId2);

  if (rseg2 == NULL) {
    logger->warn(RCX,
                 240,
                 "GndCap: cannot find rseg for rsegId {} on net {} {}",
                 rsegId2,
                 net->getId(),
                 net->getConstName());
    return NULL;
  }
  return rseg2;
}

dbRSeg* extMain::getMainRseg(dbCapNode* node,
                             dbBlock* blk,
                             Ath__parser* parser,
                             Logger* logger)
{
  if (parser != NULL) {
    dbNet* mainNet = getDstNet(node->getNet(), blk, parser);

    if (mainNet == NULL) {
      logger->warn(RCX,
                   479,
                   "CCap: cannot find main net for {}",
                   node->getNet()->getConstName());
      return NULL;
    }
    dbRSeg* rseg = getMainRSeg3(node->getNet(), node->getShapeId(), mainNet);
    if (rseg == NULL) {
      logger->warn(RCX,
                   246,
                   "CCap: cannot find rseg for net for {}",
                   node->getNet()->getConstName());
      return NULL;
    }
    return rseg;
  } else {
    uint rsegId = node->getShapeId();
    if (rsegId == 0) {
      logger->warn(
          RCX, 247, "CCap: cannot find rseg for capNode {}", node->getId());
      return NULL;
    }
    dbRSeg* rseg = dbRSeg::getRSeg(blk, rsegId);
    if (rseg == NULL) {
      logger->warn(RCX, 248, "CCap: cannot find rseg {}", rsegId);
      return NULL;
    }
    return rseg;
  }
}
void extMain::updateRseg(dbRSeg* rseg1, dbRSeg* rseg2, uint cornerCnt)
{
  double gndCapTable1[10];
  rseg1->getCapTable(gndCapTable1);

  double gndCapTable2[10];
  rseg2->getCapTable(gndCapTable2);

  for (uint ii = 0; ii < cornerCnt; ii++) {
    double cap = gndCapTable1[ii] + gndCapTable2[ii];
    rseg2->setCapacitance(cap, ii);
  }
}
uint extMain::assembly_RCs(dbBlock* mainBlock,
                           dbBlock* blk,
                           uint cornerCnt,
                           Logger* logger)
{
  uint rcCnt = 0;

  Ath__parser p;
  Ath__parser* parser = NULL;

  dbSet<dbRSeg> rcSegs = blk->getRSegs();
  dbSet<dbRSeg>::iterator rcitr;

  for (rcitr = rcSegs.begin(); rcitr != rcSegs.end(); ++rcitr) {
    dbRSeg* rseg1 = *rcitr;
    if (!rseg1->updatedCap())
      continue;

    dbRSeg* rseg2
        = getMainRseg(rseg1->getTargetCapNode(), mainBlock, parser, logger);

    if (rseg2 == NULL)
      continue;

    updateRseg(rseg1, rseg2, cornerCnt);

    rcCnt++;
  }
  return rcCnt;
}

uint extMain::assemblyCCs(dbBlock* mainBlock,
                          dbBlock* blk,
                          uint cornerCnt,
                          uint& missCCcnt,
                          Logger* logger)
{
  uint ccCnt = 0;

  Ath__parser p;
  Ath__parser* parser = NULL;

  dbSet<dbCCSeg> ccSegs = blk->getCCSegs();
  dbSet<dbCCSeg>::iterator ccitr;

  for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
    dbCCSeg* cc = *ccitr;

    dbRSeg* srcRC
        = getMainRseg(cc->getSourceCapNode(), mainBlock, parser, logger);

    if (srcRC == NULL) {
      missCCcnt++;
      continue;
    }
    dbRSeg* dstRC
        = getMainRseg(cc->getTargetCapNode(), mainBlock, parser, logger);
    if (dstRC == NULL) {
      missCCcnt++;
      continue;
    }

    dbCCSeg* ccap = dbCCSeg::create(
        srcRC->getTargetCapNode(), dstRC->getTargetCapNode(), false);

    for (uint ii = 0; ii < cornerCnt; ii++) {
      double cap = cc->getCapacitance(ii);
      ;
      ccap->setCapacitance(cap, ii);
    }
    ccCnt++;
  }
  return ccCnt;
}

extTileSystem::extTileSystem(Rect& extRect, uint* size)
{
  _ll[0] = extRect.xMin();
  _ll[1] = extRect.yMin();
  _ur[0] = extRect.xMax();
  _ur[1] = extRect.yMax();

  for (uint dd = 0; dd < 2; dd++) {
    _tileSize[dd] = size[dd] / 4;

    _tileCnt[dd] = (_ur[dd] - _ll[dd]) / _tileSize[dd] + 2;

    uint n = _tileCnt[dd];

    _signalTable[dd] = new Ath__array1D<uint>*[n];
    _instTable[dd] = new Ath__array1D<uint>*[n];

    for (uint jj = 0; jj < n; jj++) {
      _signalTable[dd][jj] = NULL;
      _instTable[dd][jj] = NULL;
    }
  }

  _powerTable = new Ath__array1D<uint>(512);
  _tmpIdTable = new Ath__array1D<uint>(64000);
}

uint extMain::mkTileNets(uint dir,
                         int* lo_sdb,
                         int* hi_sdb,
                         bool powerNets,
                         dbCreateNetUtil* createDbNet,
                         uint& rcCnt,
                         bool countOnly)
{
  if (_tiles == NULL)
    return 0;

  if (powerNets) {
    uint pCnt = mkTilePowerNets(dir, lo_sdb, hi_sdb, createDbNet);
    logger_->info(RCX,
                  97,
                  "created {} power wires for block {}",
                  pCnt,
                  _block->getConstName());
  }

  uint lo_index = getBucketNum(
      _tiles->_ll[dir], _tiles->_ur[dir], _tiles->_tileSize[dir], lo_sdb[dir]);
  uint hi_index = getBucketNum(
      _tiles->_ll[dir], _tiles->_ur[dir], _tiles->_tileSize[dir], hi_sdb[dir]);

  uint tot = 0;
  uint cnt1 = 0;
  uint local = 0;
  for (uint bucket = lo_index; bucket <= hi_index; bucket++) {
    if (_tiles->_signalTable[dir][bucket] == NULL)
      continue;

    uint netCnt = _tiles->_signalTable[dir][bucket]->getCnt();

    for (uint ii = 0; ii < netCnt; ii++) {
      uint netId = _tiles->_signalTable[dir][bucket]->get(ii);
      dbNet* net = dbNet::getNet(_block, netId);

      if (net->isSpef())  // tmp flag
        continue;
      net->setSpef(true);
      _tiles->_tmpIdTable->add(netId);

      dbWire* wire = net->getWire();
      if (wire == NULL)
        continue;

      Rect R;
      uint cnt = getNetBbox(net, R);

      int ll[2] = {R.xMin(), R.yMin()};
      int ur[2] = {R.xMax(), R.yMax()};

      tot += cnt;

      if ((ur[dir] < lo_sdb[dir]) || (ll[dir] > hi_sdb[dir]))
        continue;

      cnt1 += cnt;

      if ((ur[dir] <= hi_sdb[dir]) && (ll[dir] >= lo_sdb[dir])) {
        local += cnt;
      }

      cnt += wire->length();

      if (countOnly)
        continue;

      if (!net->isRCgraph())
        rcCnt += rcNetGen(net);

      createDbNet->copyNet(net, false);
    }
  }
  logger_->info(RCX,
                98,
                "{} local out of {} tile wires out of {} total",
                local,
                cnt1,
                tot);

  resetNetSpefFlag(_tiles->_tmpIdTable);

  return cnt1;
}
uint extMain::mkTilePowerNets(uint dir,
                              int* lo_sdb,
                              int* hi_sdb,
                              dbCreateNetUtil* createDbNet)
{
  Rect tileRect;
  tileRect.reset(lo_sdb[0], lo_sdb[1], hi_sdb[0], hi_sdb[1]);

  uint netCnt = _tiles->_powerTable->getCnt();

  createDbNet->setCurrentNet(NULL);
  uint cnt = 0;
  for (uint ii = 0; ii < netCnt; ii++) {
    uint netId = _tiles->_powerTable->get(ii);

    dbNet* net = dbNet::getNet(_block, netId);
    createDbNet->createSpecialNet(net, NULL);

    dbSet<dbSWire> swires = net->getSWires();
    dbSet<dbSWire>::iterator itr;
    for (itr = swires.begin(); itr != swires.end(); ++itr) {
      dbSWire* swire = *itr;

      dbSet<dbSBox> wires = swire->getWires();
      dbSet<dbSBox>::iterator box_itr;

      for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
        dbSBox* s = *box_itr;
        if (s->isVia())
          continue;

        Rect r = s->getBox();

        if (!tileRect.intersects(r))
          continue;

        createDbNet->createSpecialWire(NULL, r, s->getTechLayer(), 0);
        cnt++;
      }
    }
  }
  return cnt;
}

uint extMain::rcNetGen(dbNet* net)
{
  dbSigType type = net->getSigType();
  if ((type == dbSigType::POWER) || (type == dbSigType::GROUND))
    return 0;
  if (!_allNet && !net->isMarked())
    return 0;

  _connectedBTerm.clear();
  _connectedITerm.clear();

  uint cnt = makeNetRCsegs(net);

  uint tt;
  for (tt = 0; tt < _connectedBTerm.size(); tt++)
    ((dbBTerm*) _connectedBTerm[tt])->setMark(0);
  for (tt = 0; tt < _connectedITerm.size(); tt++)
    ((dbITerm*) _connectedITerm[tt])->setMark(0);

  return cnt;
}
uint extMain::rcGen(const char* netNames,
                    double resBound,
                    bool mergeViaRes,
                    uint debug,
                    bool rlog,
                    ZInterface* Interface)
{
  if (debug != 77)
    logger_->info(RCX,
                  102,
                  "RC segment generation {} ...",
                  getBlock()->getName().c_str());

  if (!_lefRC && (getRCmodel(0) == NULL)) {
    logger_->warn(RCX, 103, "No RC model was read with command <load_model>");
    logger_->warn(RCX, 104, "Can't perform RC generation!");
    return 0;
  }
  if (setMinTypMax(false, false, false, NULL, false, -1, -1, -1, 1) < 0) {
    logger_->warn(RCX, 105, "Wrong combination of corner related options");
    return 0;
  }
  _mergeViaRes = mergeViaRes;
  _mergeResBound = resBound;

  _foreign = false;  // extract after read_spef

  std::vector<dbNet*> inets;
  _allNet = !((dbBlock*) _block)->findSomeNet(netNames, inets);

  uint j;
  for (j = 0; j < inets.size(); j++) {
    dbNet* net = inets[j];
    net->setMark(true);
  }
  uint cnt = 0;

  getResCapTable(true);

  if (debug == 77) {
    logger_->info(RCX, 106, "Setup for RCgen done!");
    return 0;
  }
  dbSet<dbNet> bnets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;
  for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    cnt += rcNetGen(net);
  }
  logger_->info(RCX, 39, "Final {} rc segments", cnt);
  return cnt;
}

}  // namespace rcx
