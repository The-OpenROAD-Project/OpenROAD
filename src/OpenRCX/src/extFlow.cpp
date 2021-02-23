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
#include "OpenRCX/extRCap.h"
#include "utility/Logger.h"

//#define MAXINT 0x7FFFFFFF;

//#define DEBUG_NET_ID 10
//#define TEST_SIGNAL_TABLE 1
//#define TEST_POWER_LEN 1

namespace rcx {

using utl::RCX;

using odb::dbBlock;
using odb::dbBox;
using odb::dbBTerm;
using odb::dbCapNode;
using odb::dbCCSeg;
using odb::dbNet;
using odb::dbRSeg;
using odb::dbSet;
using odb::dbShape;
using odb::dbSigType;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbTechLayerType;
using odb::dbWire;
using odb::dbWirePath;
using odb::dbWirePathItr;
using odb::dbWirePathShape;
using odb::gs;
using odb::Rect;

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
FILE* extMain::openSearchFile(char* name)
{
  _searchFP = fopen(name, "w");
  return _searchFP;
}
void extMain::closeSearchFile()
{
  if (_searchFP != NULL)
    fclose(_searchFP);
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

    Rect r;
    s.getBox(r);

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

    Rect r;
    s.getBox(r);

    //		maxRect.merge(r);
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

    Rect r;
    s.getBox(r);

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

      Rect r;
      s->getBox(r);

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
void extMain::addExtWires(Rect& r,
                          extWireBin*** wireBinTable,
                          uint netId,
                          int shapeId,
                          dbTechLayer* layer,
                          uint* nm_step,
                          int* bb_ll,
                          int* bb_ur,
                          AthPool<extWire>* wpool,
                          bool cntxFlag)
{
  int ll[2] = {r.xMin(), r.yMin()};
  int ur[2] = {r.xMax(), r.yMax()};

  uint dir = getDir(r.xMin(), r.yMin(), r.xMax(), r.yMax());

  if (!cntxFlag) {
    uint binNum = getBucketNum(bb_ll[dir], bb_ur[dir], nm_step[dir], ll[dir]);
    if (wireBinTable[dir][binNum] == NULL)
      wireBinTable[dir][binNum]
          = new extWireBin(dir, binNum, bb_ll[dir], wpool, 1024);

    wireBinTable[dir][binNum]->addWire(netId, shapeId, layer);
  } else {
    uint not_dir = !dir;
    uint loBin = getBucketNum(
        bb_ll[not_dir], bb_ur[not_dir], nm_step[not_dir], ll[not_dir]);
    uint hiBin = getBucketNum(
        bb_ll[not_dir], bb_ur[not_dir], nm_step[not_dir], ur[not_dir]);

    for (uint ii = loBin; ii <= hiBin; ii++) {
      if (wireBinTable[not_dir][ii] == NULL)
        wireBinTable[not_dir][ii]
            = new extWireBin(dir, ii, bb_ll[not_dir], wpool, 1024);

      wireBinTable[not_dir][ii]->addWire(netId, shapeId, layer);
    }
  }
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
          RCX, 80, "cannot allocate <Ath__array1D<extWireBin*>*[layerCnt]>");
    }
    for (uint jj = 0; jj < n; jj++) {
      instTable[dd][jj] = NULL;
    }
  }
  // uint cnt= 0;
  dbSet<dbInst> insts = _block->getInsts();
  dbSet<dbInst>::iterator inst_itr;

  for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
    dbInst* inst = *inst_itr;
    dbBox* bb = inst->getBBox();

    Rect r;
    bb->getBox(r);

    for (uint dir = 0; dir < 2; dir++)
      addNetOnTable(inst->getId(), dir, &r, nm_step, bb_ll, bb_ur, instTable);
  }
  return instTable;
}

extWireBin*** extMain::mkSignalBins(uint binSize,
                                    int* bb_ll,
                                    int* bb_ur,
                                    uint* bucketCnt,
                                    AthPool<extWire>* wpool,
                                    bool cntxFlag)
{
  uint nm_step[2] = {binSize, binSize};

  extWireBin*** sdbWireTable = new extWireBin**[2];
  for (uint dd = 0; dd < 2; dd++) {
    bucketCnt[dd] = (bb_ur[dd] - bb_ll[dd]) / nm_step[dd] + 1;
    uint n = bucketCnt[dd];

    sdbWireTable[dd] = new extWireBin*[n];
    if (sdbWireTable[dd] == NULL) {
      logger_->error(
          RCX, 80, "cannot allocate <Ath__array1D<extWireBin*>*[layerCnt]>");
    }
    for (uint jj = 0; jj < n; jj++) {
      sdbWireTable[dd][jj] = NULL;
    }
  }
  // uint cnt= 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    uint netId = net->getId();

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND)) {
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

          Rect r;
          s->getBox(r);
          addExtWires(r,
                      sdbWireTable,
                      netId,
                      -s->getId(),
                      s->getTechLayer(),
                      nm_step,
                      bb_ll,
                      bb_ur,
                      wpool,
                      cntxFlag);
        }
      }

      continue;
    }
    dbWire* wire = net->getWire();
    if (wire == NULL)
      continue;

    dbWireShapeItr shapes;
    dbShape s;
    for (shapes.begin(wire); shapes.next(s);) {
      if (s.isVia())
        continue;

      Rect r;
      s.getBox(r);
      addExtWires(r,
                  sdbWireTable,
                  netId,
                  shapes.getShapeId(),
                  s.getTechLayer(),
                  nm_step,
                  bb_ll,
                  bb_ur,
                  wpool,
                  cntxFlag);

      /*
      int ll[2]= {s.xMin(), s.yMin()};
      int ur[2]= {s.xMax(), s.yMax()};

      uint dir= getDir(s.xMin(), s.yMin(), s.xMax(), s.yMax());

      if (!cntxFlag)
      {
              uint binNum= getBucketNum(bb_ll[dir], bb_ur[dir], nm_step[dir],
      ll[dir]); if (sdbWireTable[dir][binNum]==NULL) sdbWireTable[dir][binNum]=
      new extWireBin(dir, binNum, bb_ll[dir], wpool, 1024);

              uint shapeId= shapes.getShapeId();
              //uint l= s.getTechLayer()->getRoutingLevel();

              sdbWireTable[dir][binNum]->addWire(netId, shapeId,
      s.getTechLayer());
      }
      else {
              uint not_dir= !dir;
              uint loBin= getBucketNum(bb_ll[not_dir], bb_ur[not_dir],
      nm_step[not_dir], ll[not_dir]); uint hiBin= getBucketNum(bb_ll[not_dir],
      bb_ur[not_dir], nm_step[not_dir], ur[not_dir]);

              for (uint ii= loBin; ii<=hiBin; ii++)
              {
                      if (sdbWireTable[not_dir][ii]==NULL)
                              sdbWireTable[not_dir][ii]= new extWireBin(dir, ii,
      bb_ll[not_dir], wpool, 1024);

                      uint shapeId= shapes.getShapeId();
                      //uint l= s.getTechLayer()->getRoutingLevel();

                      sdbWireTable[not_dir][ii]->addWire(netId, shapeId,
      s.getTechLayer());
              }
      }
      */
    }
  }
  return sdbWireTable;
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

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND)) {
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
  if (_overCell) {
    dbSet<dbInst> insts = _block->getInsts();
    dbSet<dbInst>::iterator inst_itr;

    for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
      dbInst* inst = *inst_itr;
      dbBox* bb = inst->getBBox();

      Rect s;
      bb->getBox(s);

      for (uint dir = 0; dir < 2; dir++)
        addNetOnTable(inst->getId(), dir, &s, nm_step, bb_ll, bb_ur, instTable);

      inst->clearUserFlag1();
    }
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

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
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

    Rect s;
    bb->getBox(s);

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
    _block->getDieArea(maxRect);
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
    dbTechLayerType type = layer->getType();

    if (type.getValue() != dbTechLayerType::ROUTING)
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

  _search = new Ath__gridTable(&maxRect, 2, layerCnt, W, pitchTable, S, X1, Y1);
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

    if (!((net->getSigType() == dbSigType::POWER)
          || (net->getSigType() == dbSigType::GROUND)))
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

    // prevXY= thickTable->getUpperBound(dir, sq);
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

      Rect r;
      s->getBox(r);
      if (isIncludedInsearch(r, dir, bb_ll, bb_ur)) {
        uint level = s->getTechLayer()->getRoutingLevel();

        int trackNum = -1;
        if (_geoThickTable != NULL) {
          cnt += addMultipleRectsOnSearch(r, level, dir, s->getId(), 0, wtype);
          continue;
        }
        if (netUtil != NULL) {
          netUtil->createSpecialWire(NULL, r, s->getTechLayer(), s->getId());
          // netUtil->createSpecialNetSingleWire(r, s->getTechLayer(), net,
          // s->getId());
        } else {
          // int xmin= r.xMin();
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

      Rect r;
      s->getBox(r);
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

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
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

    if (!((net->getSigType() == dbSigType::POWER)
          || (net->getSigType() == dbSigType::GROUND)))
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
  //	uint wireId= wire->getId();

  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    //		uint level= 0;

    int shapeId = shapes.getShapeId();

    if (s.isVia()) {
      if (!_skip_via_wires)
        addViaBoxes(s, net, shapeId, wtype);

      continue;
    }

    Rect r;
    s.getBox(r);
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

        // int xmin= r.xMin();
        uint trackNum = 0;
        // if (net->getId()==2655) {
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

#ifdef TEST_SIGNAL_TABLE
      if (fp != NULL) {
        fprintf(fp,
                "%d %d  %d %d %d %d\n",
                net->getId(),
                level,
                r.xMin(),
                r.yMin(),
                r.xMax(),
                r.yMax());
      }
#endif
      cnt++;
    }
  }
  return cnt;
}

uint extMain::addViaBoxes(dbShape& sVia, dbNet* net, uint shapeId, uint wtype)
{
  int rcid = getShapeProperty(net, shapeId);
  wtype = 5;  // Via Type

  bool USE_DB_UNITS = false;
  uint cnt = 0;

  int X1[2] = {0, 0};
  int X2[2] = {0, 0};
  int Y1[2] = {0, 0};
  int Y2[2] = {0, 0};
  int id[2] = {0, 0};
  int LEN[2] = {0, 0};

  int botLevel = 0;
  int topLevel = 0;

  const char* tcut = "tcut";
  const char* bcut = "bcut";

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

    uint track_num;
    uint level = s.getTechLayer()->getRoutingLevel();
    uint width = s.getTechLayer()->getWidth();

    int len = dx;
    if (s.getTechLayer()->getDirection() == dbTechLayerDir::VERTICAL) {
      len = dy;
      if (width != dx)
        continue;
    } else {
      if (width != dy)
        continue;
    }

    if (USE_DB_UNITS) {
      track_num = _search->addBox(GetDBcoords2(x1),
                                  GetDBcoords2(y1),
                                  GetDBcoords2(x2),
                                  GetDBcoords2(y2),
                                  level,
                                  net->getId(),
                                  shapeId,
                                  wtype);
    } else {
      track_num = _search->addBox(
          x1, y1, x2, y2, level, net->getId(), shapeId, wtype);
    }
    // if (net->getId()==_debug_net_id) {
    //	debug("Search", "W", "addViaBoxes: L%d  DX=%3d DY=%d %d %d  %d %d --
    //%.3f %.3f  %.3f %.3f dx=%g dy=%g\n", 		level, dx, dy, x1, y1,
    // x2, y2, GetDBcoords1(x1), GetDBcoords1(y1), GetDBcoords1(x2),
    // GetDBcoords1(y2), 		GetDBcoords1(dx), GetDBcoords1(dy));
    //}
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

#ifdef TEST_SIGNAL_TABLE
  char filename[64];
  sprintf(filename, "old/%d.%d.%d.sig.sdb", dir, bb_ll[dir], bb_ur[dir]);
  fp = fopen(filename, "w");
#endif

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
      continue;

    cnt += addNetShapesOnSearch(net, dir, bb_ll, bb_ur, wtype, fp, createDbNet);
  }
  if (createDbNet == NULL)
    _search->adjustOverlapMakerEnd();

#ifdef TEST_SIGNAL_TABLE

  fprintf(fp,
          "dir= %d   %d %d -- %d %d sigCnt= %d\n",
          dir,
          bb_ll[0],
          bb_ur[0],
          bb_ll[1],
          bb_ur[1],
          cnt);

  if (fp != NULL)
    fclose(fp);
#endif
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
      s->getBox(r);
    } else {
      dbShape s;
      // wire->getShape(w->_shapeId, s);
      wire->getSegment(w->_shapeId, w->_layer, s);

      s.getBox(r);
    }
    createDbNet->createSpecialWire(NULL, r, w->_layer, 0);
    cnt++;
  }
  return cnt;
}
uint extMain::addNets3GS(uint dir,
                         int* lo_sdb,
                         int* hi_sdb,
                         int* bb_ll,
                         int* bb_ur,
                         uint bucketSize,
                         extWireBin*** wireBinTable,
                         dbCreateNetUtil* createDbNet)
{
  uint lo_index
      = getBucketNum(bb_ll[dir], bb_ur[dir], bucketSize, lo_sdb[dir] + 1);
  uint hi_index
      = getBucketNum(bb_ll[dir], bb_ur[dir], bucketSize, hi_sdb[dir] + 1);

  uint cnt = 0;
  for (uint bucket = lo_index; bucket <= hi_index; bucket++) {
    extWireBin* b = wireBinTable[dir][bucket];
    if (b == NULL)
      continue;

    if (createDbNet != NULL)
      b->createDbNetsGS(_block, createDbNet);
    else
      cnt += b->_table->getCnt();
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

      Rect r;
      s->getBox(r);

      createDbNet->createSpecialWire(NULL, r, w->_layer, -w->_shapeId);
    } else {
      dbShape s;
      // wire->getShape(w->_shapeId, s);
      wire->getSegment(w->_shapeId, w->_layer, s);
      Rect r;
      s.getBox(r);

      createDbNet->createNetSingleWire(
          r, w->_layer->getRoutingLevel(), w->_netId, w->_shapeId);
    }
    cnt++;
  }
  return cnt;
}
uint extMain::addNets3(uint dir,
                       int* lo_sdb,
                       int* hi_sdb,
                       int* bb_ll,
                       int* bb_ur,
                       uint bucketSize,
                       extWireBin*** wireBinTable,
                       dbCreateNetUtil* createDbNet)
{
  uint lo_index = getBucketNum(bb_ll[dir], bb_ur[dir], bucketSize, lo_sdb[dir]);
  uint hi_index = getBucketNum(bb_ll[dir], bb_ur[dir], bucketSize, hi_sdb[dir]);

  uint cnt = 0;
  for (uint bucket = lo_index; bucket <= hi_index; bucket++) {
    extWireBin* b = wireBinTable[dir][bucket];
    if (b == NULL)
      continue;

    if (createDbNet != NULL)
      b->createDbNets(_block, createDbNet);
    else
      cnt += b->_table->getCnt();
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
#ifdef TEST_SIGNAL_TABLE
  char filename[64];
  sprintf(filename, "new/%d.%d.%d.sig.sdb", dir, lo_sdb[dir], hi_sdb[dir]);
  fp = fopen(filename, "w");
#endif

  uint cnt = 0;
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

      cnt += addNetShapesOnSearch(
          net, dir, lo_sdb, hi_sdb, wtype, fp, createDbNet);
    }
  }
  _search->adjustOverlapMakerEnd();

  resetNetSpefFlag(tmpNetIdTable);

#ifdef TEST_SIGNAL_TABLE
  fprintf(fp,
          "dir= %d   %d %d -- %d %d sigCnt= %d\n",
          dir,
          lo_sdb[0],
          hi_sdb[0],
          lo_sdb[1],
          hi_sdb[1],
          cnt);

  if (fp != NULL)
    fclose(fp);
#endif

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

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
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
    if (plane) {
      uint rdir = 0;
      if (layer->getDirection() == dbTechLayerDir::HORIZONTAL)
        rdir = 1;
      // if (rdir==dir)
      // return 0;
    } else {
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

    Rect r;
    s.getBox(r);

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
    /*
                    if (dir>=0) {
                            Rect r;
                            s.getBox(r);
                            if (matchDir(dir, r))
                                    continue;
                    }
                    uint level= s.getTechLayer()->getRoutingLevel();

                    int n= 0;
                    if (!gsRotated) {
                            n= _geomSeq->box(s.xMin(), s.yMin(), s.xMax(),
       s.yMax(), level);
                    }
                    else {
                            if (!swap_coords) // horizontal
                                    n= _geomSeq->box(s.xMin(), s.yMin(),
       s.xMax(), s.yMax(), level); else n= _geomSeq->box(s.yMin(), s.xMin(),
       s.yMax(), s.xMax(), level);
                    }
                    if (n==0)
                            cnt ++;
    */
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

      Rect r;
      s->getBox(r);
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
      /*
                              if (dir>=0) {
                                      Rect r;
                                      s->getBox(r);
                                      if (matchDir(dir, r))
                                              continue;
                              }

                              uint level= s->getTechLayer()->getRoutingLevel();

                              int n= 0;
                              if (!gsRotated) {
                                      n= _geomSeq->box(s->xMin(), s->yMin(),
         s->xMax(), s->yMax(), level);
                              }
                              else {
                                      if (!swap_coords)
                                              n= _geomSeq->box(s->xMin(),
         s->yMin(), s->xMax(), s->yMax(), level); else n=
         _geomSeq->box(s->yMin(), s->xMin(), s->yMax(), s->xMax(), level);
                              }
                              if (n==0)
                                      cnt ++;
      */
    }
  }
  return cnt;
}
uint extMain::addNetsGs(Ath__array1D<uint>* gsTable, int dir)
{
  if (gsTable == NULL)
    return 0;

  uint cnt = 0;
  uint netCnt = gsTable->getCnt();
  for (uint ii = 0; ii < netCnt; ii++) {
    dbNet* net = dbNet::getNet(_block, gsTable->get(ii));

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
      cnt += addNetSboxesGs(net, false, false, dir);
    else
      cnt += addNetShapesGs(net, false, false, dir);
  }
  return cnt;
}
int extMain::getXY_gs(int base, int XY, uint minRes)
{
  uint maxRow = (XY - base) / minRes;
  int v = base + maxRow * minRes;
  return v;
}
int extMain::getXY_gs2(int base, int hiXY, int loXY, uint minRes)
{
  uint hiRow = (hiXY - base) / minRes;
  uint loRow = (loXY - base) / minRes;

  uint deltaRow = hiRow - loRow;

  int targetRow = hiRow - deltaRow;
  int v = base + targetRow * minRes;
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
      // logger_->info(RCX, 0, "GS: _slicenum={}, _xres={}, _yres={}, _x0={},
      // _y0={}, _x1={}, _y1={}", 	ii, res[0], res[1], ll[0], ll[1], ur[0],
      // ur[1]);

      _geomSeq->configureSlice(
          ii, res[0], res[1], ll[0], ll[1], ur[0], ur[1], skipMemAlloc);
    } else {
      if (dir > 0) {  // horizontal segment extraction
        _geomSeq->configureSlice(
            ii, res[0], res[1], ll[0], ll[1], ur[0], ur[1], skipMemAlloc);

        // logger_->info(RCX, 0, "HSE_GS: _slicenum={}, _xres={}, _yres={},
        // _x0={}, _y0={}, _x1={}, _y1={}", 	ii, res[0], res[1], ll[0],
        // ll[1], ur[0], ur[1]);
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

          // logger_->info(RCX, 0, "GS: _slicenum={}, _xres={}, _yres={},
          // _x0={}, _y0={}, _x1={}, _y1={}", 	ii, pitchTable[ii],
          // widthTable[ii], ll[1], ll[0], ur[1], ur[0]);
        } else {
          _geomSeq->configureSlice(ii,
                                   widthTable[ii],
                                   pitchTable[ii],
                                   ll[1],
                                   ll[0],
                                   ur[1],
                                   ur[0],
                                   skipMemAlloc);
          // logger_->info(RCX, 0, "H_GS: _slicenum={}, _xres={}, _yres={},
          // _x0={}, _y0={}, _x1={}, _y1={}", 	ii, widthTable[ii],
          // pitchTable[ii], ll[1], ll[0], ur[1], ur[0]);
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

    /*
                    Rect r;
                    dbBox *bb= inst->getBBox();
                    bb->getBox(r);
                    if ( !isIncludedInsearch(r, dir, bb_ll, bb_ur) )
                            continue;
    */
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

  int gs_dir = dir;
#ifndef GS_CROSS_LINES_ONLY
  gs_dir = -1;
#endif

  uint pcnt = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if (!((net->getSigType() == dbSigType::POWER)
          || (net->getSigType() == dbSigType::GROUND)))
      continue;

    if (createDbNet != NULL)
      createDbNet->createSpecialNet(net, NULL);

    pcnt += addNetSboxesGs(net, rotatedGs, !dir, gs_dir, createDbNet);
  }

  /*
          FILE *fp= NULL;

  #ifdef TEST_SIGNAL_TABLE
          char filename[64];
          sprintf(filename, "old/%d.%d.%d.sig.sdb", dir, bb_ll[dir],
  bb_ur[dir]); fp= fopen(filename, "w"); #endif
  */
  uint scnt = 0;

  if (createDbNet != NULL)
    createDbNet->createSpecialNet(NULL, "SIGNALS_GS");

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
      continue;

    scnt += addNetShapesGs(net, rotatedGs, !dir, gs_dir, createDbNet);
  }
  if (_overCell) {
    Ath__array1D<uint> instGsTable(nets.size());
    Ath__array1D<uint> tmpNetIdTable(nets.size());

    dbSet<dbInst> insts = _block->getInsts();
    dbSet<dbInst>::iterator inst_itr;
    for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
      dbInst* inst = *inst_itr;

      dbBox* R = inst->getBBox();

      int R_ll[2] = {R->xMin(), R->yMin()};
      int R_ur[2] = {R->xMax(), R->yMax()};

      if ((R_ur[dir] < lo_gs[dir]) || (R_ll[dir] > hi_gs[dir]))
        continue;

      instGsTable.add(inst->getId());
    }
    addInstsGs(&instGsTable, &tmpNetIdTable, dir);
  }

  /*
  #ifdef TEST_SIGNAL_TABLE
          FILE *fp= NULL;
          char filename[64];
          sprintf(filename, "new/%d.%d.%d.fill", dir, minExt, hiXY);
          fp= fopen(filename, "w");

          fprintf(fp, "dir= %d   minExt= %d   hiXY= %d  %d %d -- %d %d pwrCnt=
  %d  sigCnt= %d\n", dir, minExt, hiXY, lo_gs[0], hi_gs[0], lo_gs[1], hi_gs[1],
  pcnt, scnt);

          if (fp!=NULL)
                  fclose(fp);
  #endif
          // logger_->info(RCX, 0, "Extracting {} {}   {} {} ....", lo_gs[0],
  lo_gs[1], hi_gs[0], hi_gs[1]);
  */
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

  //	uint cnt= 0;

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

  int gs_dir = dir;
#ifndef GS_CROSS_LINES_ONLY
  gs_dir = -1;
#endif

  uint pcnt = 0;
  uint netCnt = powerNetTable->getCnt();
  for (uint ii = 0; ii < netCnt; ii++) {
    dbNet* net = dbNet::getNet(_block, powerNetTable->get(ii));

    if (createDbNet != NULL)
      createDbNet->createSpecialNet(net, NULL);

    pcnt += addNetSboxesGs(net, rotatedGs, !dir, gs_dir, createDbNet);
  }
  uint scnt = 0;

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

      scnt += addNetShapesGs(net, rotatedGs, !dir, gs_dir, createDbNet);
    }
  }
  resetNetSpefFlag(tmpNetIdTable);

  if (_overCell) {
    for (uint bucket = lo_index; bucket <= hi_index; bucket++) {
      addInstsGs(instGsTable[dir][bucket], tmpNetIdTable, dir);
    }
  }

#ifdef TEST_SIGNAL_TABLE
  FILE* fp = NULL;
  char filename[64];
  sprintf(filename, "new/%d.%d.%d.fill", dir, minExt, hiXY);
  fp = fopen(filename, "w");

  fprintf(fp,
          "dir= %d   minExt= %d   hiXY= %d  %d %d -- %d %d pwrCnt= %d  sigCnt= "
          "%d\n",
          dir,
          minExt,
          hiXY,
          lo_gs[0],
          hi_gs[0],
          lo_gs[1],
          hi_gs[1],
          pcnt,
          scnt);

  if (fp != NULL)
    fclose(fp);
#endif
  // logger_->info(RCX, 0, "Extracting {} {}   {} {} ....", lo_gs[0], lo_gs[1],
  // hi_gs[0], hi_gs[1]);
  return lo_gs[dir];
}

int extMain::fill_gs2(int dir,
                      int* ll,
                      int* ur,
                      int* lo_gs,
                      int* hi_gs,
                      uint layerCnt,
                      uint* dirTable,
                      uint* pitchTable,
                      uint* widthTable,
                      uint bucket,
                      Ath__array1D<uint>*** gsTable,
                      Ath__array1D<uint>*** instGsTable)
{
  /*
  int lo_gs[2];
  int hi_gs[2];

  lo_gs[!dir]= ll[!dir];
  hi_gs[!dir]= ur[!dir];

  lo_gs[dir]= minExt;
  hi_gs[dir]= hiXY;
*/
  initPlanes(dir, lo_gs, hi_gs, layerCnt, pitchTable, widthTable, dirTable, ll);

  int gs_dir = dir;
#ifndef GS_CROSS_LINES_ONLY
  gs_dir = -1;
#endif

  if (gsTable != NULL) {
    uint cnt = addNetsGs(gsTable[dir][bucket], gs_dir);
    if (bucket > 0)
      cnt += addNetsGs(gsTable[dir][bucket - 1], gs_dir);

    if (_overCell) {
      addInstsGs(instGsTable[dir][bucket], NULL, 0);
      if (bucket > 0)
        addInstsGs(instGsTable[dir][bucket], NULL, 0);
    }
  } else {
    addPowerGs(gs_dir);
    addSignalGs(gs_dir);

    if (_overCell)
      addInstShapesOnPlanes(dir);

#ifdef TEST_SIGNAL_TABLE
    FILE* fp = NULL;
    char filename[64];
    sprintf(filename, "old/%d.%d.%d.fill", dir, minExt, hiXY);
    fp = fopen(filename, "w");

    fprintf(fp,
            "dir= %d   minExt= %d   hiXY= %d  %d %d -- %d %d pwrCnt= %d  "
            "sigCnt= %d\n",
            dir,
            minExt,
            hiXY,
            lo_gs[0],
            hi_gs[0],
            lo_gs[1],
            hi_gs[1],
            gsPcnt,
            gsCnt);

    if (fp != NULL)
      fclose(fp);
#endif
  }

  return lo_gs[dir];
}

void extMain::resetGndCaps()
{
  uint cornerCnt = _block->getCornerCount();

  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  // uint cnt= 0;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
      continue;

    // uint netId= net->getId();

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
uint extMain::couplingFlow(bool rlog,
                           Rect& extRect,
                           uint trackStep,
                           uint ccFlag,
                           extMeasure* m,
                           void (*coupleAndCompute)(int*, void*))
{
  dbIntProperty* p = (dbIntProperty*) dbProperty::find(_block, "_currentDir");
  if (p != NULL) {
    _use_signal_tables = 3;
    int DIR = p->getValue();

    extWindow* W = new extWindow(20, logger_);
    W->getExtProperties(_block);

    uint propCnt = mkNetPropertiesForRsegs(_block, DIR);
    logger_->info(RCX, 86, "Created {} Net Properties", propCnt);

    _ignoreWarning_1st = true;
    rcGenBlock();
    _ignoreWarning_1st = false;
    if (m->_netId > 0) {
      writeMapping();
      writeMapping(_block->getParent());
    } else
      resetGndCaps();

    extractWindow(rlog, W, extRect, false, m, coupleAndCompute);

    uint rsegCnt = invalidateNonDirShapes(_block, (uint) DIR, true);
    logger_->info(RCX, 87, "Extracted {} valid rsegs", rsegCnt);

    return 0;
    // return tileFlow(rlog, extRect, trackStep, ccFlag, m, coupleAndCompute);
  }

  uint ccDist = ccFlag;
  //	if (ccFlag>20) {
  //		ccDist= ccFlag % 10;
  //	}

  // trackStep *= 2;
  bool single_gs = false;

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
  //	uint minPitch= pitchTable[1];

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

  if (single_gs) {
    initPlanes(layerCnt);
    m->_pixelTable = _geomSeq;
    addPowerGs();
    addSignalGs();
    if (rlog)
      AthResourceLog("NewFlow single GS ", 0);
  }
  uint minRes[2];
  minRes[1] = pitchTable[1];
  minRes[0] = widthTable[1];

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
                  88,
                  "Signal_table= {} ----------------------------- ",
                  _use_signal_tables);

    for (uint ii = 0; ii < 2; ii++) {
      sdbBucketCnt[ii] = 0;
      sdbBucketSize[ii] = 2 * step_nm[ii];
      sdbTable_ll[ii] = ll[ii] - step_nm[ii] / 10;
      sdbTable_ur[ii] = ur[ii] + step_nm[ii] / 10;
    }

    // mkSignalTables(sdbBucketSize, sdbTable_ll, sdbTable_ur, sdbSignalTable,
    // NULL, gsInstTable, sdbBucketCnt);
    mkSignalTables2(sdbBucketSize,
                    sdbTable_ll,
                    sdbTable_ur,
                    sdbSignalTable,
                    &sdbPowerTable,
                    gsInstTable,
                    sdbBucketCnt);

    reportTableNetCnt(sdbBucketCnt, sdbSignalTable);
#ifdef TEST_SIGNAL_TABLE
    reportTableNetCnt(sdbBucketCnt, sdbSignalTable);
#endif

    if (rlog)
      AthResourceLog("Making net tables ", 0);
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

    // if (getRotatedFlag())
    //   logger_->info(RCX, 0, "======> Fast Mode enabled for d= {} <======",
    //   dir);

    lo_gs[!dir] = ll[!dir];
    hi_gs[!dir] = ur[!dir];
    lo_sdb[!dir] = ll[!dir];
    hi_sdb[!dir] = ur[!dir];

    int minExtracted = ll[dir];
    int gs_limit = ll[dir];

    _search->initCouplingCapLoops(dir, ccFlag, coupleAndCompute, m);
    if (rlog)
      AthResourceLog("initCouplingCapLoops", 0);

    lo_sdb[dir] = ll[dir] - step_nm[dir];
    //		int loXY= ll[dir];
    int hiXY = ll[dir] + step_nm[dir];
    if (hiXY > ur[dir])
      hiXY = ur[dir];

    uint stepNum = 0;
    for (; hiXY <= ur[dir]; hiXY += step_nm[dir]) {
      if (ur[dir] - hiXY <= (int) step_nm[dir])
        hiXY = ur[dir] + 5 * ccDist * maxPitch;

      if (!single_gs) {
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
        // fill_gs2(dir, ll, ur, lo_gs, hi_gs, layerCnt, dirTable, pitchTable,
        // widthTable, 0, NULL, NULL);

        m->_rotatedGs = getRotatedFlag();
        m->_pixelTable = _geomSeq;
        if (rlog)
          AthResourceLog("Fill GS", 0);
      }
      // add wires onto search such that    loX<=loX<=hiX
      hi_sdb[dir] = hiXY;

      uint processWireCnt = 0;
      if (use_signal_tables) {
        // addNets(dir, lo_sdb, hi_sdb, sigtype, pwrtype,
        // sdbSignalTable[dir][stepNum]);
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

      if (rlog)
        AthResourceLog("Fill Sdb", 0);

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

      // printLimitArray(limitArray, layerCnt);

      if (rlog) {
        char buff[64];
        sprintf(buff,
                "CCext %d wires xy[%d]= %d ",
                processWireCnt,
                dir,
                minExtracted);

        AthResourceLog(buff, 0);
      }

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
      // totalWiresExtracted += extractedWireCnt;
      totalWiresExtracted += processWireCnt;
      float percent_extracted
          = Ath__double2int(100.0 * (1.0 * totalWiresExtracted / totWireCnt));

      if ((totWireCnt > 0) && (totalWiresExtracted > 0)
          && (percent_extracted - _previous_percent_extracted >= 5.0)) {
        logger_->info(RCX,
                      44,
                      "{:d}% completion -- {:d} wires have been extracted",
                      (int) (100.0 * (1.0 * totalWiresExtracted / totWireCnt)),
                      totalWiresExtracted);

        _previous_percent_extracted = percent_extracted;
      }
      // break;
    }
    // break;
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

//=============================================== WINDOW BASED ==============
void extWindow::initWindowStep(Rect& extRect,
                               uint trackStep,
                               uint layerCnt,
                               uint modelLevelCnt)
{
  _maxPitch = _pitchTable[layerCnt - 1];

  _layerCnt = layerCnt > modelLevelCnt ? layerCnt : modelLevelCnt;

  _ll[0] = extRect.xMin();
  _ll[1] = extRect.yMin();
  _ur[0] = extRect.xMax();
  _ur[1] = extRect.yMax();

  uint minRes[2];
  minRes[1] = _pitchTable[1];
  minRes[0] = _widthTable[1];

  _step_nm[1] = trackStep * minRes[1];
  _step_nm[0] = trackStep * minRes[1];
  if (_maxWidth > _ccDist * _maxPitch) {
    _step_nm[1] = _ur[1] - _ll[1];
    _step_nm[0] = _ur[0] - _ll[0];
  }
}
extWindow::extWindow(extWindow* e, uint num, Logger* logger)
{
  logger_ = logger;
  _num = num;

  init(20);

  _ccDist = e->_ccDist;

  _maxPitch = e->_maxPitch;

  _maxWidth = e->_maxWidth;
  _layerCnt = e->_layerCnt;

  for (uint ii = 0; ii < 2; ii++) {
    _ll[ii] = e->_ll[ii];
    _ur[ii] = e->_ur[ii];
    _step_nm[ii] = e->_step_nm[ii];
    _lo_gs[ii] = e->_lo_gs[ii];
    _hi_gs[ii] = e->_hi_gs[ii];
    _lo_sdb[ii] = e->_lo_sdb[ii];
    _hi_sdb[ii] = e->_hi_sdb[ii];
  }
  _gs_limit = e->_gs_limit;
  _minExtracted = e->_minExtracted;

  for (uint i = 0; i < _layerCnt + 1; i++) {
    _dirTable[i] = e->_dirTable[i];
    _widthTable[i] = e->_widthTable[i];
    _pitchTable[i] = e->_pitchTable[i];
    //_extractLimit[i]= e->_extractLimit[i];
    for (uint j = 0; j < 2; j++) {
      _extTrack[j][i] = e->_extTrack[j][i];
      _extLimit[j][i] = e->_extLimit[j][i];
      _cntxTrack[j][i] = e->_cntxTrack[j][i];
      _cntxLimit[j][i] = e->_cntxLimit[j][i];
      _sdbBase[j][i] = e->_sdbBase[j][i];
    }
  }
  _extractLimit = e->_extractLimit;
  _totPowerWireCnt = e->_totPowerWireCnt;
  _totWireCnt = e->_totPowerWireCnt;

  _currentDir = e->_currentDir;
  _hiXY = e->_hiXY;
  _gsRotatedFlag = e->_gsRotatedFlag;
  _deallocLimit = e->_deallocLimit;
}
extWindow* extMain::initWindowSearch(Rect& extRect,
                                     uint trackStep,
                                     uint ccFlag,
                                     uint modelLevelCnt,
                                     extMeasure* m)
{
  extWindow* W = new extWindow(20, logger_);
  W->_ccDist = ccFlag;

  uint layerCnt = initSearchForNets(W->_sdbBase[0],
                                    W->_sdbBase[1],
                                    W->_pitchTable,
                                    W->_widthTable,
                                    W->_dirTable,
                                    extRect,
                                    false);
  if (m != NULL) {
    for (uint i = 0; i < layerCnt + 1; i++)
      m->_dirTable[i] = W->_dirTable[i];
  }
  uint maxWidth = 0;
  for (uint i = 1; i < layerCnt + 1; i++) {
    if (W->_widthTable[i] > maxWidth)
      maxWidth = W->_widthTable[i];
  }
  // W->_totPowerWireCnt= powerWireCounter(maxWidth);
  // W->_totWireCnt= signalWireCounter(maxWidth);
  // W->_totWireCnt += W->_totPowerWireCnt;

  W->_maxWidth = maxWidth;

  W->initWindowStep(extRect, trackStep, layerCnt, modelLevelCnt);

  // logger_->info(RCX, 0, "{} wires to be extracted", W->_totWireCnt);

  return W;
}
void extWindow::init(uint maxLayerCnt)
{
  _sigtype = 9;
  _pwrtype = 11;

  _maxLayerCnt = maxLayerCnt;

  _dirTable = new uint[maxLayerCnt];
  _pitchTable = new uint[maxLayerCnt];
  _widthTable = new uint[maxLayerCnt];
  //_extractLimit= new int[maxLayerCnt];
  for (uint i = 0; i < 2; i++) {
    _extTrack[i] = new int[maxLayerCnt];
    _extLimit[i] = new int[maxLayerCnt];
    _cntxTrack[i] = new int[maxLayerCnt];
    _cntxLimit[i] = new int[maxLayerCnt];
    _sdbBase[i] = new int[maxLayerCnt];
  }

  for (uint ii = 0; ii < maxLayerCnt; ii++) {
    _dirTable[ii] = 0;
    _pitchTable[ii] = 0;
    _widthTable[ii] = 0;
  }
  _ccDist = 0;
  _layerCnt = 0;
  _maxPitch = 0;
  _gsRotatedFlag = false;
  _totalWiresExtracted = 0;
  _totWireCnt = 0;
  _prev_percent_extracted = 0;
}
void extWindow::updateExtLimits(int** limitArray)
{
  for (uint ii = 0; ii < _layerCnt; ii++) {
    _cntxTrack[0][ii] = limitArray[ii][0];
    _cntxLimit[0][ii] = limitArray[ii][1];

    _extTrack[0][ii] = limitArray[ii][2];
    _extLimit[0][ii] = limitArray[ii][3];

    _extTrack[1][ii] = limitArray[ii][4];
    _extLimit[1][ii] = limitArray[ii][5];
  }
  _lo_sdb[_currentDir] = _cntxLimit[0][1];
  _hi_sdb[_currentDir] = _extLimit[1][1] + _maxPitch * _ccDist;
}
void extMain::printLimitArray(int** limitArray, uint layerCnt)
{
  logger_->info(RCX, 89, " ------------------------ Context Lower Limits");
  uint ii;
  for (ii = 1; ii < layerCnt; ii++)
    logger_->info(
        RCX, 90, "L={} {}    {}", ii, limitArray[ii][0], limitArray[ii][1]);

  logger_->info(RCX, 91, "--------------------------- EXT Lower Limits");
  for (ii = 1; ii < layerCnt; ii++)
    logger_->info(
        RCX, 90, "L={} {}    {}", ii, limitArray[ii][2], limitArray[ii][3]);

  logger_->info(RCX, 92, " ------------------------ EXT Upper Limits");
  for (ii = 1; ii < layerCnt; ii++)
    logger_->info(
        RCX, 90, "L={} {}    {}", ii, limitArray[ii][4], limitArray[ii][5]);
}
int extWindow::getIntProperty(dbBlock* block, const char* name)
{
  dbProperty* p = dbProperty::find(block, name);

  if (p == NULL)
    return 0;
  dbIntProperty* ip = (dbIntProperty*) p;

  return ip->getValue();
}

void extWindow::getExtProperties(dbBlock* block)
{
  _num = getIntProperty(block, "_num");
  _currentDir = getIntProperty(block, "_currentDir");
  _extractLimit = getIntProperty(block, "_extractLimit");
  _ccDist = getIntProperty(block, "_ccDist");
  _maxPitch = getIntProperty(block, "_maxPitch");
  _lo_gs[0] = getIntProperty(block, "_lo_gs[0]");
  _lo_gs[1] = getIntProperty(block, "_lo_gs[1]");
  _hi_gs[0] = getIntProperty(block, "_hi_gs[0]");
  _hi_gs[1] = getIntProperty(block, "_hi_gs[1]");
  _gs_limit = getIntProperty(block, "_gs_limit");
  _layerCnt = getIntProperty(block, "_layerCnt");

  _ll[0] = getIntProperty(block, "_ll[0]");
  _ll[1] = getIntProperty(block, "_ll[1]");
  _ur[0] = getIntProperty(block, "_ur[0]");
  _ur[1] = getIntProperty(block, "_ur[1]");
  _hiXY = getIntProperty(block, "_hiXY");
  _step_nm[0] = getIntProperty(block, "_step_nm[0]");
  _step_nm[1] = getIntProperty(block, "_step_nm[1]");

  for (uint ii = 0; ii < _layerCnt; ii++) {
    char bufName[64];
    sprintf(bufName, "_extLimit[0]_%d", ii);

    _extLimit[0][ii] = getIntProperty(block, bufName);

    sprintf(bufName, "_sdbBase[0]_%d", ii);

    _sdbBase[0][ii] = getIntProperty(block, bufName);

    sprintf(bufName, "_sdbBase[1]_%d", ii);

    _sdbBase[1][ii] = getIntProperty(block, bufName);
  }
  for (uint j = 0; j < 2; j++) {
    _lo_sdb[j] = getIntArrayProperty(block, j, "_lo_sdb");
    _hi_sdb[j] = getIntArrayProperty(block, j, "_hi_sdb");
  }
}
void extWindow::makeIntArrayProperty(dbBlock* block,
                                     uint ii,
                                     int* A,
                                     const char* name)
{
  char bufName[64];

  sprintf(bufName, "%s[%d]", name, ii);

  dbIntProperty::create(block, bufName, A[ii]);
}
int extWindow::getIntArrayProperty(dbBlock* block, uint ii, const char* name)
{
  char bufName[64];

  sprintf(bufName, "%s[%d]", name, ii);

  return getIntProperty(block, bufName);
}

dbBlock* extWindow::createExtBlock(extMeasure* m,
                                   dbBlock* mainBlock,
                                   Rect& extRect)
{
  uint nm = 1000;

  char design[128];
  char DIR[2] = {'V', 'H'};

  sprintf(design, "%c_%d_%d", DIR[_currentDir], _currentDir, _hiXY);

  dbBlock* block = dbBlock::create(mainBlock, design, '/');
  assert(block);
  block->setBusDelimeters('[', ']');
  block->setDefUnits(nm);

  block->setDieArea(extRect);

  dbIntProperty::create(block, "_num", _num);
  dbIntProperty::create(block, "_currentDir", _currentDir);
  dbIntProperty::create(block, "_extractLimit", _extractLimit);
  dbIntProperty::create(block, "_ccDist", _ccDist);
  dbIntProperty::create(block, "_maxPitch", _maxPitch);
  dbIntProperty::create(block, "_step_nm[0]", _step_nm[0]);
  dbIntProperty::create(block, "_step_nm[1]", _step_nm[1]);
  dbIntProperty::create(block, "_lo_gs[0]", _lo_gs[0]);
  dbIntProperty::create(block, "_lo_gs[1]", _lo_gs[1]);
  dbIntProperty::create(block, "_hi_gs[0]", _hi_gs[0]);
  dbIntProperty::create(block, "_hi_gs[1]", _hi_gs[1]);
  dbIntProperty::create(block, "_gs_limit", _gs_limit);
  dbIntProperty::create(block, "_ccDist", _ccDist);
  dbIntProperty::create(block, "_layerCnt", _layerCnt);
  dbIntProperty::create(block, "_hiXY", _hiXY);

  for (uint i = 0; i < 2; i++) {
    char bufName[64];

    sprintf(bufName, "_ll[%d]", i);
    dbIntProperty::create(block, bufName, _ll[i]);

    sprintf(bufName, "_ur[%d]", i);
    dbIntProperty::create(block, bufName, _ur[i]);
  }

  for (uint ii = 0; ii < _layerCnt; ii++) {
    char bufName[64];
    sprintf(bufName, "_extLimit[0]_%d", ii);

    dbIntProperty::create(block, bufName, _extLimit[0][ii]);

    sprintf(bufName, "_sdbBase[0]_%d", ii);

    dbIntProperty::create(block, bufName, _sdbBase[0][ii]);

    sprintf(bufName, "_sdbBase[1]_%d", ii);

    dbIntProperty::create(block, bufName, _sdbBase[1][ii]);
  }
  for (uint j = 0; j < 2; j++) {
    makeIntArrayProperty(block, j, _lo_sdb, "_lo_sdb");
    makeIntArrayProperty(block, j, _hi_sdb, "_hi_sdb");
  }

  return block;
}
void extWindow::printLimits(FILE* fp,
                            const char* header,
                            uint maxLayer,
                            int** limitArray,
                            int** trackArray)
{
  fprintf(fp, "%s\n", header);
  for (uint ii = 1; ii < _layerCnt; ii++) {
    fprintf(fp,
            "\t%d - %d %d  - %d %d\n",
            ii,
            trackArray[0][ii],
            trackArray[1][ii],
            limitArray[0][ii],
            limitArray[1][ii]);
  }
}
void extWindow::printExtLimits(FILE* fp)
{
  fprintf(fp, "Window[%d]: %d\n", _currentDir, _hiXY);
  printLimits(fp, "\nExt limits", _maxLayerCnt, _extLimit, _extTrack);
  printLimits(fp, "\nContext limits", _maxLayerCnt, _cntxLimit, _cntxTrack);
  fprintf(fp, "\n");
}
/*
void extWindow::get_extractLimit(extWindow *e)
{
        for (uint ii = 1; ii <_maxLayerCnt; ii++)
                _extractLimit[ii]= e->_extractLimit[ii];
}
*/
extWindow::extWindow(uint maxLayerCnt, Logger* logger)
{
  logger_ = logger;
  init(maxLayerCnt);
}
void extWindow::makeSdbBuckets(uint sdbBucketCnt[2],
                               uint sdbBucketSize[2],
                               int sdbTable_ll[2],
                               int sdbTable_ur[2])
{
  for (uint ii = 0; ii < 2; ii++) {
    sdbBucketCnt[ii] = 0;
    sdbBucketSize[ii] = 2 * _step_nm[ii];
    sdbTable_ll[ii] = _ll[ii] - _step_nm[ii] / 10;
    sdbTable_ur[ii] = _ur[ii] + _step_nm[ii] / 10;
  }
}
void extWindow::printBoundaries(FILE* fp, bool flag)
{
  if (!flag && fp == NULL)
    return;

  if (fp == NULL) {
    logger_->info(RCX,
                  238,
                  "{:15s}= {} \t_currentDir\n"
                  "{:15s}= {} \t_hiXY\n"
                  "{:15s}= {} \t_lo_gs\n"
                  "{:15s}= {} \t_hi_gs\n"
                  "{:15s}= {} \t_lo_sdb\n"
                  "{:15s}= {} \t_hi_sdb\n"
                  "{:15s}= {} \t_gs_limit\n"
                  "{:15s}= {} \t_minExtracted\n"
                  "{:15s}= {} \t_deallocLimit",
                  _currentDir,
                  _hiXY,
                  _lo_gs[_currentDir],
                  _hi_gs[_currentDir],
                  _lo_sdb[_currentDir],
                  _hi_sdb[_currentDir],
                  _gs_limit,
                  _minExtracted,
                  _deallocLimit);
  } else {
    fprintf(fp, "\n%15s= %d\n", "_currentDir", _currentDir);
    fprintf(fp, "%15s= %d\n", "_hiXY", _hiXY);
    fprintf(fp, "%15s= %d\n", "_lo_gs", _lo_gs[_currentDir]);
    fprintf(fp, "%15s= %d\n", "_hi_gs", _hi_gs[_currentDir]);
    fprintf(fp, "%15s= %d\n", "_lo_sdb", _lo_sdb[_currentDir]);
    fprintf(fp, "%15s= %d\n", "_hi_sdb", _hi_sdb[_currentDir]);
    fprintf(fp, "%15s= %d\n", "_gs_limit", _gs_limit);
    fprintf(fp, "%15s= %d\n", "_minExtracted", _minExtracted);
    fprintf(fp, "%15s= %d\n\n", "_deallocLimit", _deallocLimit);
  }
}

int extWindow::setExtBoundaries(uint dir)
{
  _currentDir = dir;

  _lo_gs[!dir] = _ll[!dir];
  _hi_gs[!dir] = _ur[!dir];
  _lo_sdb[!dir] = _ll[!dir];
  _hi_sdb[!dir] = _ur[!dir];

  _minExtracted = _ll[dir];
  _gs_limit = _ll[dir];

  _lo_sdb[dir] = _ll[dir] - _step_nm[dir];

  _hiXY = _ll[dir] + _step_nm[dir];
  if (_hiXY > _ur[dir])
    _hiXY = _ur[dir];

  if (dir == 0)
    _gsRotatedFlag = true;

  if (_gsRotatedFlag)
    logger_->info(RCX, 93, "======> Fast Mode enabled for d= {} <======", dir);

  return _hiXY;
}
int extWindow::adjust_hiXY(int hiXY)
{
  _hiXY = hiXY;

  if (_ur[_currentDir] - _hiXY <= (int) _step_nm[_currentDir])
    _hiXY = _ur[_currentDir] + 5 * _ccDist * _maxPitch;

  _hi_sdb[_currentDir] = _hiXY;

  _lo_gs[_currentDir] = _gs_limit;
  _hi_gs[_currentDir] = _hiXY;

  return _hiXY;
}
void extMain::fillWindowGs(extWindow* W,
                           int* sdbTable_ll,
                           int* sdbTable_ur,
                           uint* bucketSize,
                           Ath__array1D<uint>* powerNetTable,
                           Ath__array1D<uint>* tmpNetIdTable,
                           Ath__array1D<uint>*** sdbSignalTable,
                           Ath__array1D<uint>*** instGsTable,
                           dbCreateNetUtil* createDbNet)
{
  if (sdbSignalTable != NULL) {
    tmpNetIdTable->resetCnt();
    fill_gs3(W->_currentDir,
             W->_ll,
             W->_ur,
             W->_ll,
             W->_ur,
             W->_layerCnt,
             //		fill_gs3(W->_currentDir, W->_ll, W->_ur, W->_lo_gs,
             // W->_hi_gs, W->_layerCnt,
             W->_dirTable,
             W->_pitchTable,
             W->_widthTable,
             sdbTable_ll,
             sdbTable_ur,
             bucketSize,
             powerNetTable,
             tmpNetIdTable,
             sdbSignalTable,
             instGsTable,
             createDbNet);
  } else {
    fill_gs4(W->_currentDir,
             W->_ll,
             W->_ur,
             W->_lo_gs,
             W->_hi_gs,
             W->_layerCnt,
             W->_dirTable,
             W->_pitchTable,
             W->_widthTable,
             createDbNet);
  }
}
uint extMain::fillWindowSearch(extWindow* W,
                               int* lo_sdb,
                               int* hi_sdb,
                               int* sdbTable_ll,
                               int* sdbTable_ur,
                               uint* bucketSize,
                               Ath__array1D<uint>* powerNetTable,
                               Ath__array1D<uint>* tmpNetIdTable,
                               Ath__array1D<uint>*** sdbSignalTable,
                               dbCreateNetUtil* createDbNet)
{
  int ll[2] = {lo_sdb[0], lo_sdb[1]};
  int ur[2] = {hi_sdb[0], hi_sdb[1]};
  //	ll[W->_currentDir]= W->_gs_limit;
  //	ur[W->_currentDir]= W->_hiXY;

  /*
  for (uint i= 0; i<2; i++) {
          ll[i]= W->_ll[i];
          ur[i]= W->_ur[i];
  }
  */
  char bufname[32];
  sprintf(bufname, "BOUNDS/%d_%d.sdb", W->_currentDir, W->_hiXY);
  openSearchFile(bufname);

  if (sdbSignalTable != NULL) {
    W->_processPowerWireCnt = addPowerNets2(
        W->_currentDir, ll, ur, W->_pwrtype, powerNetTable, createDbNet);
    tmpNetIdTable->resetCnt();
    W->_processSignalWireCnt = addSignalNets2(W->_currentDir,
                                              ll,
                                              ur,
                                              sdbTable_ll,
                                              sdbTable_ur,
                                              bucketSize,
                                              W->_sigtype,
                                              sdbSignalTable,
                                              tmpNetIdTable,
                                              createDbNet);
  } else {
    uint processWireCnt
        = addPowerNets(W->_currentDir, ll, ur, W->_pwrtype, createDbNet);
    processWireCnt
        += addSignalNets(W->_currentDir, ll, ur, W->_sigtype, createDbNet);
    // uint processWireCnt += addPowerNets(dir, lo_sdb, hi_sdb, pwrtype);
    // processWireCnt += addSignalNets(dir, lo_sdb, hi_sdb, sigtype);
  }
  closeSearchFile();

  return W->_processPowerWireCnt + W->_processSignalWireCnt;
}
int extWindow::set_extractLimit()
{
  _extractLimit = _hiXY - _ccDist * _maxPitch;

  return _extractLimit;
}
void extWindow::reportProcessedWires(bool rlog)
{
  if (rlog) {
    char buff[64];
    sprintf(buff,
            "CCext %d wires xy[%d]= %d ",
            _processPowerWireCnt + _processSignalWireCnt,
            _currentDir,
            _minExtracted);
#ifdef _WIN32
    logger_->info(RCX, 214, "{}", buff);
#endif
    AthResourceLog(buff, 0);
  }
}
int extWindow::getDeallocLimit()
{
  _deallocLimit = _minExtracted - (_ccDist + 1) * _maxPitch;
  return _deallocLimit;
}
void extWindow::updateLoBounds(bool reportFlag)
{
  _lo_sdb[_currentDir] = _hiXY;
  _gs_limit = _minExtracted - (_ccDist + 2) * _maxPitch;

  _totalWiresExtracted += _processPowerWireCnt + _processSignalWireCnt;
  if (!reportFlag)
    return;

  double percent_extracted
      = (int) (100.0 * (1.0 * _totalWiresExtracted / _totWireCnt));

  if ((_totWireCnt > 0) && (_totalWiresExtracted > 0)) {
    logger_->info(RCX,
                  44,
                  "{:d}% completion -- {:d} wires have been extracted",
                  (int) (100.0 * (1.0 * _totalWiresExtracted / _totWireCnt)),
                  _totalWiresExtracted);

    _prev_percent_extracted = percent_extracted;
  }
}
uint extMain::mkNetPropertiesForRsegs(dbBlock* blk, uint dir)
{
  dbSet<dbNet> nets = blk->getNets();
  dbSet<dbNet>::iterator net_itr;

  uint cnt = 0;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
      continue;

    dbWire* wire = net->getWire();
    dbWireShapeItr shapes;
    dbShape s;
    for (shapes.begin(wire); shapes.next(s);) {
      if (s.isVia())
        continue;

      int n = shapes.getShapeId();
      if (n == 0)
        continue;

      Rect r;
      s.getBox(r);

      if (!matchDir(dir, r)) {
        wire->setProperty(n, 0);
        continue;
      }

      int rsegId1 = 0;
      wire->getProperty(n, rsegId1);
      if (rsegId1 == 0)
        continue;

      char bufName[16];
      sprintf(bufName, "J%d", n);
      dbIntProperty::create(net, bufName, rsegId1);
      wire->setProperty(n, 0);
      cnt++;
    }
  }
  return cnt;
}
uint extMain::invalidateNonDirShapes(dbBlock* blk, uint dir, bool setMainNet)
{
  Ath__parser parser;

  Ath__array1D<dbRSeg*> rsegTable;

  dbSet<dbNet> nets = blk->getNets();
  dbSet<dbNet>::iterator net_itr;

  uint cnt = 0;
  uint tot = 0;
  uint dCnt = 0;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
      continue;

    int dstNetId = 0;
    if (setMainNet) {
      parser.mkWords(net->getConstName());
      dstNetId = parser.getInt(0, 1);
    }

    dbWire* wire = net->getWire();
    dbWireShapeItr shapes;
    dbShape s;
    for (shapes.begin(wire); shapes.next(s);) {
      if (s.isVia())
        continue;

      int shapeId = shapes.getShapeId();
      if (shapeId == 0)
        continue;

      Rect r;
      s.getBox(r);

      int rsegId1 = 0;
      wire->getProperty(shapeId, rsegId1);
      if (rsegId1 == 0)
        continue;

      dbRSeg* rseg1 = dbRSeg::getRSeg(blk, rsegId1);
      if (rseg1 == NULL) {
        logger_->warn(RCX,
                      94,
                      "GndCap: cannot find rseg for net [{}] {} and shapeId {}",
                      net->getId(),
                      net->getConstName(),
                      shapeId);
        continue;
      }
      tot++;
      if (!matchDir(dir, r)) {
        wire->setProperty(shapeId, 0);
        rsegTable.add(rseg1);
        continue;
      }

      if (!rseg1->updatedCap()) {  // context
        wire->setProperty(shapeId, 0);
        rsegTable.add(rseg1);
      } else {
        if (setMainNet) {
          dbCapNode* node = rseg1->getTargetCapNode();
          // node->setNetId(dstNetId);

          char bufName[16];
          sprintf(bufName, "J%d", shapeId);
          dbProperty* p = dbProperty::find(net, bufName);
          uint rsegId2 = 0;
          if (p == NULL) {
            rsegId2 = 0;
          } else {
            dbIntProperty* ip = (dbIntProperty*) p;
            rsegId2 = ip->getValue();
          }
          node->setNode(rsegId2);
        }
        cnt++;
      }
    }
    uint destroySegCnt = rsegTable.getCnt();
    dCnt += destroySegCnt;
    for (uint ii = 0; ii < destroySegCnt; ii++) {
      dbRSeg* rc = rsegTable.get(ii);
      dbCapNode* node = rc->getTargetCapNode();
      dbCapNode::destroy(node);
      dbRSeg::destroy(rc, net);
    }
    rsegTable.resetCnt();
  }
  logger_->info(RCX,
                95,
                "Deleted {} Rsegs/CapNodes from total {} with {} remaing",
                dCnt,
                tot,
                cnt);
  return cnt;
}
uint extMain::createNetShapePropertires(dbBlock* blk)
{
  dbSet<dbNet> nets = blk->getNets();
  dbSet<dbNet>::iterator net_itr;

  uint cnt = 0;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
      continue;

    // uint netId= net->getId();

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

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
      continue;

    // uint netId= net->getId();

    dbNet* mainNet = getDstNet(net, _block, &parser);

    if (mainNet->isRCgraph())
      continue;

    cnt += rcNetGen(mainNet);
  }
  return cnt;
}

uint extMain::couplingWindowFlow(bool rlog,
                                 Rect& extRect,
                                 uint trackStep,
                                 uint ccFlag,
                                 bool doExt,
                                 extMeasure* m,
                                 void (*coupleAndCompute)(int*, void*))
{
  bool single_gs = false;

  extWindow* W = initWindowSearch(
      extRect, trackStep, ccFlag, _currentModel->getLayerCnt(), m);

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

  // _use_signal_tables^M
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
                  88,
                  "Signal_table= {} ----------------------------- ",
                  _use_signal_tables);

    W->makeSdbBuckets(sdbBucketCnt, sdbBucketSize, sdbTable_ll, sdbTable_ur);

    // mkSignalTables(sdbBucketSize, sdbTable_ll, sdbTable_ur, sdbSignalTable,
    // NULL, gsInstTable, sdbBucketCnt);
    mkSignalTables2(sdbBucketSize,
                    sdbTable_ll,
                    sdbTable_ur,
                    sdbSignalTable,
                    &sdbPowerTable,
                    gsInstTable,
                    sdbBucketCnt);

    reportTableNetCnt(sdbBucketCnt, sdbSignalTable);
#ifdef TEST_SIGNAL_TABLE
    reportTableNetCnt(sdbBucketCnt, sdbSignalTable);
#endif

    if (rlog)
      AthResourceLog("Making net tables ", 0);
  }
  uint totalWiresExtracted = 0;

  FILE* boundFP = fopen("window.bounds", "w");
  FILE* limitFP = fopen("window.limits", "w");

  extWindow* windowTable[1000];
  // extWindow **windowTable= allocWindowTable();
  uint windowCnt = 0;

  int** limitArray;
  limitArray = new int*[W->_layerCnt];
  for (uint k = 0; k < W->_layerCnt; k++)
    limitArray[k] = new int[10];

  for (int dir = 1; dir >= 0; dir--) {
    if (dir == 0)
      enableRotatedFlag();
    else
      disableRotatedFlag();

    _search->initCouplingCapLoops(dir, ccFlag, coupleAndCompute, m);
    if (rlog)
      AthResourceLog("initCouplingCapLoops", 0);

    uint stepNum = 0;
    int hiXY = W->setExtBoundaries(dir);
    for (; hiXY <= W->_ur[dir]; hiXY += W->_step_nm[dir]) {
      hiXY = W->adjust_hiXY(hiXY);

      uint extractedWireCnt = 0;
      W->set_extractLimit();
      W->_minExtracted = _search->couplingCaps(W->_extractLimit,
                                               W->_ccDist,
                                               W->_currentDir,
                                               extractedWireCnt,
                                               coupleAndCompute,
                                               m,
                                               _getBandWire,
                                               limitArray);

      // W->reportProcessedWires(rlog);

      int deallocLimit = W->getDeallocLimit();
      _search->dealloc(W->_currentDir, deallocLimit);

      // W->printBoundaries(boundFP, true);

      stepNum++;

      W->updateExtLimits(limitArray);
      windowTable[windowCnt] = new extWindow(W, windowCnt, logger_);
      windowTable[windowCnt]->printBoundaries(boundFP, true);
      windowCnt++;

      W->updateLoBounds(false /*report*/);

      W->printExtLimits(limitFP);
    }
  }
  fclose(boundFP);
  fclose(limitFP);

  bool single_sdb = false;

  if (single_sdb) {
    uint layerCnt = initSearchForNets(W->_sdbBase[0],
                                      W->_sdbBase[1],
                                      W->_pitchTable,
                                      W->_widthTable,
                                      W->_dirTable,
                                      extRect,
                                      true);
    for (uint i = 0; i < layerCnt + 1; i++)
      m->_dirTable[i] = W->_dirTable[i];

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
  }

  // uint prevDir= 10;
  bool useSdbTable = true;

  FILE* bndFP = fopen("window.bounds.new", "w");
  FILE* limFP = fopen("window.limits.after", "w");

  bool extractionFlag = doExt;
  bool skipInit = false;
  for (int jj = 0; jj < windowCnt; jj++) {
    extWindow* w = windowTable[jj];

    // if (W->_currentDir==1)
    //	continue;

    w->printExtLimits(limFP);

    if (w->_currentDir == 0)
      enableRotatedFlag();
    else
      disableRotatedFlag();

    if (!extractionFlag) {
      dbBlock* extBlock = w->createExtBlock(m, _block, extRect);
      m->_create_net_util.setBlock(extBlock, skipInit);
      if (!skipInit)
        skipInit = true;

      fillWindowGs(w,
                   sdbTable_ll,
                   sdbTable_ur,
                   sdbBucketSize,
                   &sdbPowerTable,
                   &tmpNetIdTable,
                   sdbSignalTable,
                   gsInstTable,
                   &m->_create_net_util);

      uint processWireCnt = fillWindowSearch(w,
                                             w->_lo_sdb,
                                             w->_hi_sdb,
                                             sdbTable_ll,
                                             sdbTable_ur,
                                             sdbBucketSize,
                                             &sdbPowerTable,
                                             &tmpNetIdTable,
                                             sdbSignalTable,
                                             &m->_create_net_util);

      uint signalWireCnt = createNetShapePropertires(extBlock);
      logger_->info(RCX,
                    96,
                    "Block {} has {} signal wires",
                    extBlock->getConstName(),
                    processWireCnt);

      continue;
      // break;
    }

    extractWindow(rlog,
                  W,
                  extRect,
                  single_sdb,
                  m,
                  coupleAndCompute,
                  sdbTable_ll,
                  sdbTable_ur,
                  sdbBucketSize,
                  &sdbPowerTable,
                  &tmpNetIdTable,
                  sdbSignalTable,
                  gsInstTable);

    break;
  }
  fclose(bndFP);
  fclose(limFP);

  if (use_signal_tables) {
    freeSignalTables(true, sdbSignalTable, NULL, sdbBucketCnt);
  }

  if (_geomSeq != NULL) {
    delete _geomSeq;
    _geomSeq = NULL;
  }
  return 0;
}

uint extMain::extractWindow(bool rlog,
                            extWindow* W,
                            Rect& extRect,
                            bool single_sdb,
                            extMeasure* m,
                            void (*coupleAndCompute)(int*, void*),
                            int* sdbTable_ll,
                            int* sdbTable_ur,
                            uint* sdbBucketSize,
                            Ath__array1D<uint>* sdbPowerTable,
                            Ath__array1D<uint>* tmpNetIdTable,
                            Ath__array1D<uint>*** sdbSignalTable,
                            Ath__array1D<uint>*** gsInstTable)
{
  /*
  char bufname[128];
  sprintf(bufname, "BOUNDS/%d_%d.limits", W->_currentDir, W->_hiXY);
  FILE *limFP= fopen(bufname, "w");
  sprintf(bufname, "BOUNDS/%d_%d.bounds", W->_currentDir, W->_hiXY);
  FILE *bndFP= fopen(bufname, "w");
  */
  int** limitArray;
  limitArray = new int*[W->_layerCnt];
  for (uint k = 0; k < W->_layerCnt; k++)
    limitArray[k] = new int[10];

  uint prevDir = 10;

  //	if (W->_num!=0)
  //		return 1;

  disableRotatedFlag();
  if (W->_currentDir == 0)
    enableRotatedFlag();

  if (single_sdb) {
    uint layerCnt = initSearchForNets(W->_sdbBase[0],
                                      W->_sdbBase[1],
                                      W->_pitchTable,
                                      W->_widthTable,
                                      W->_dirTable,
                                      extRect,
                                      true);
    for (uint i = 0; i < layerCnt + 1; i++)
      m->_dirTable[i] = W->_dirTable[i];

    _search->initCouplingCapLoops(
        W->_currentDir, W->_ccDist, coupleAndCompute, m, W->_extLimit[0]);
    /*
    if (W->_currentDir!=prevDir) {
            _search->initCouplingCapLoops(W->_currentDir, W->_ccDist,
    coupleAndCompute, m); prevDir= W->_currentDir;
    }
    */
  } else {
    uint layerCnt = initSearchForNets(W->_sdbBase[0],
                                      W->_sdbBase[1],
                                      W->_pitchTable,
                                      W->_widthTable,
                                      W->_dirTable,
                                      extRect,
                                      true);
    for (uint i = 0; i < layerCnt + 1; i++)
      m->_dirTable[i] = W->_dirTable[i];

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
    /*
                    if (W->_currentDir!=prevDir) {
                            _search->initCouplingCapLoops(W->_currentDir,
       W->_ccDist, coupleAndCompute, m); prevDir= W->_currentDir;
                    }
                    else
    */
    _search->initCouplingCapLoops(
        W->_currentDir, W->_ccDist, coupleAndCompute, m, W->_extLimit[0]);
  }

  if (rlog)
    AthResourceLog("initCouplingCapLoops", 0);

  fillWindowGs(W,
               sdbTable_ll,
               sdbTable_ur,
               sdbBucketSize,
               sdbPowerTable,
               tmpNetIdTable,
               sdbSignalTable,
               gsInstTable);

  uint processWireCnt = fillWindowSearch(W,
                                         W->_lo_sdb,
                                         W->_hi_sdb,
                                         sdbTable_ll,
                                         sdbTable_ur,
                                         sdbBucketSize,
                                         sdbPowerTable,
                                         tmpNetIdTable,
                                         sdbSignalTable);

  if (rlog)
    AthResourceLog("Fill GS", 0);

  m->_rotatedGs = getRotatedFlag();
  m->_pixelTable = _geomSeq;

  if (rlog)
    AthResourceLog("Fill Sdb", 0);

  // W->printExtLimits(limFP);
  // W->printBoundaries(bndFP, true);

  uint extractedWireCnt = 0;
  _search->couplingCaps(W->_extractLimit,
                        W->_ccDist,
                        W->_currentDir,
                        extractedWireCnt,
                        coupleAndCompute,
                        m,
                        false,
                        limitArray);
  // printLimitArray(limitArray, W->_layerCnt);

  W->reportProcessedWires(true);

  _search->dealloc(W->_currentDir, W->_deallocLimit);

  // W->printBoundaries(bndFP, true);

  W->updateExtLimits(limitArray);
  //	W->printExtLimits(limFP);
  //	fclose(limFP);
  //	fclose(bndFP);

  return 0;
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

uint extMain::assemblyExt(dbBlock* mainBlock, dbBlock* blk, Logger* logger)
{
  if (mainBlock != NULL)
    return assemblyExt__2(mainBlock, blk, logger);
  else {  // block based
    dbSet<dbCapNode> capNodes = blk->getCapNodes();
    uint csize = capNodes.size();

    uint gndCnt = 0;
    dbSet<dbCapNode>::iterator cap_node_itr = capNodes.begin();
    for (; cap_node_itr != capNodes.end(); ++cap_node_itr) {
      dbCapNode* node = *cap_node_itr;
      node->addToNet();
      gndCnt++;
    }
    logger->info(RCX, 241, "{} nodes on block {}", gndCnt, blk->getConstName());
    if (csize != gndCnt)
      logger->info(RCX, 242, "\tdifferent from {} cap nodes read", csize);

    dbSet<dbRSeg> rsegs = blk->getRSegs();
    uint rsize = rsegs.size();

    dbNet* net = NULL;
    uint rCnt = 0;
    dbSet<dbRSeg>::iterator rseg_itr = rsegs.begin();
    for (; rseg_itr != rsegs.end(); ++rseg_itr) {
      rCnt++;
      dbRSeg* rseg = *rseg_itr;
      uint tgtId = rseg->getTargetNode();
      uint srcId = rseg->getSourceNode();
      if ((srcId == 0) && (tgtId == rseg->getId())) {  // new net
        if (net != NULL) {
          dbSet<dbRSeg> rSet = net->getRSegs();
          rSet.reverse();
        }
        net = rseg->getNet();
      }
      rseg->addToNet();
    }
    if (net != NULL) {
      dbSet<dbRSeg> rSet = net->getRSegs();
      rSet.reverse();
    }
    logger->info(RCX, 243, "{} rsegs on block {}", rCnt, blk->getConstName());
    if (rsize != rCnt)
      logger->info(RCX, 244, "Different from {} rsegs read", rsize);
    return rCnt;
  }
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
                   245,
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
    // uint netId= node->getNetId();
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

    uint rsgeId1 = rseg1->getId();
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

uint extMain::assemblyExt__2(dbBlock* mainBlock, dbBlock* blk, Logger* logger)
{
  bool flag = true;

  uint cornerCnt = mainBlock->getCornerCount();
  if (flag) {
    uint rcCnt = assembly_RCs(mainBlock, blk, cornerCnt, logger);
    uint missCCcnt = 0;
    uint ccCnt = assemblyCCs(mainBlock, blk, cornerCnt, missCCcnt, logger);

    int numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg;
    mainBlock->getExtCount(numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg);

    logger->info(RCX,
                 249,
                 "Updated {} rsegs and added {} ccsegs of {} from {}",
                 rcCnt,
                 ccCnt,
                 mainBlock->getConstName(),
                 blk->getConstName());

    return rcCnt;
  }

  Ath__parser parser;

  dbSet<dbNet> nets = blk->getNets();
  dbSet<dbNet>::iterator net_itr;

  uint missCCcnt = 0;
  uint gndCnt = 0;
  uint ccCnt = 0;
  uint cnt = 0;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND))
      continue;

    dbNet* dstNet = getDstNet(net, mainBlock, &parser);
    if (dstNet == NULL)
      continue;

    dbWire* wire = net->getWire();

    uint netId = dstNet->getId();

    if (wire == NULL)
      continue;

    dbWireShapeItr shapes;
    dbShape s;
    for (shapes.begin(wire); shapes.next(s);) {
      if (s.isVia())
        continue;

      int shapeId = shapes.getShapeId();
      if (shapeId == 0)
        continue;

      int rsegId1 = 0;
      wire->getProperty(shapeId, rsegId1);
      if (rsegId1 == 0) {
        continue;
      }
      dbRSeg* rseg1 = dbRSeg::getRSeg(blk, rsegId1);

      // if (rseg1->marked())
      // continue;

      // rseg1->setMark(true);

      cnt++;
      /*
                              int rsegId2= 0;
                              dstNet->getWire()->getProperty(shapeId, rsegId2);
                              if (rsegId2==0) {
                                      logger_->warn(RCX, 0, "zero rseg wire
         property {} on main net {} {}", shapeId, dstNet->getId(),
         dstNet->getConstName()); continue;
                              }
                              dbRSeg *rseg2= dbRSeg::getRSeg(dstNet->getBlock(),
         rsegId2);

      */
      dbRSeg* rseg2 = getMainRSeg3(net, shapeId, dstNet);

      if (rseg2 == NULL) {
        logger->warn(RCX,
                     240,
                     "GndCap: cannot find rseg for rsegId {} on net {} {}",
                     rsegId1,
                     dstNet->getId(),
                     dstNet->getConstName());
        continue;
      }

      double gndCapTable1[10];
      rseg1->getCapTable(gndCapTable1);

      double gndCapTable2[10];
      rseg2->getCapTable(gndCapTable2);

      for (uint ii = 0; ii < cornerCnt; ii++) {
        double cap = gndCapTable1[ii] + gndCapTable2[ii];
        rseg2->setCapacitance(cap, ii);
      }
      gndCnt++;

      continue;

      dbCapNode* srcCapNode1 = rseg1->getTargetCapNode();
      dbSet<dbCCSeg> ccSegs = srcCapNode1->getCCSegs();
      dbSet<dbCCSeg>::iterator ccitr;

      for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
        dbCCSeg* cc = *ccitr;

        if (cc->isMarked())
          continue;

        cc->setMark(true);

        dbCapNode* dstCapNode1 = cc->getTargetCapNode();
        if (cc->getSourceCapNode() != srcCapNode1) {
          dstCapNode1 = cc->getSourceCapNode();
          if (cc->getTargetCapNode() != srcCapNode1) {
            logger->warn(RCX,
                         250,
                         "Mismatch for CCap {} for net {}",
                         cc->getId(),
                         dstNet->getId());
          }
        }

        dbNet* dstTgtNet
            = extMain::getDstNet(dstCapNode1->getNet(), mainBlock, &parser);
        if (dstTgtNet == NULL) {
          logger->warn(RCX,
                       245,
                       "CCap: cannot find main net for {}",
                       dstCapNode1->getNet()->getConstName());
          continue;
        }

        // dbRSeg *tgtRseg2= extMain::getRseg(dstTgtNet,
        // dstCapNode1->getShapeId());

        dbRSeg* tgtRseg2 = getMainRSeg3(
            dstCapNode1->getNet(), dstCapNode1->getShapeId(), dstTgtNet);

        if (tgtRseg2 == NULL) {
          missCCcnt++;
          continue;
        }

        dbCCSeg* ccap = dbCCSeg::create(
            rseg2->getTargetCapNode(), tgtRseg2->getTargetCapNode(), true);

        for (uint ii = 0; ii < cornerCnt; ii++) {
          double cap = cc->getCapacitance(ii);
          ;
          ccap->setCapacitance(cap, ii);
        }
        ccCnt++;
      }
    }
  }
  ccCnt = assemblyCCs(mainBlock, blk, cornerCnt, missCCcnt, logger);

  int numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg;
  mainBlock->getExtCount(numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg);

  logger->info(
      RCX,
      251,
      "Updated {} nets, {} rsegs, and added {} ({}) ccsegs of {} from {}",
      cnt,
      gndCnt,
      ccCnt,
      numOfCCSeg,
      mainBlock->getConstName(),
      blk->getConstName());

  return cnt;
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
uint extMain::mkTileBoundaries(bool skipPower, bool skipInsts)
{
  uint cnt = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    uint netId = net->getId();

    if ((net->getSigType() == dbSigType::POWER)
        || (net->getSigType() == dbSigType::GROUND)) {
      if (!skipPower)
        _tiles->_powerTable->add(net->getId());
      continue;
    }

    Rect maxRect;
    uint cnt1 = getNetBbox(net, maxRect);
    if (cnt1 == 0)
      continue;

    net->setSpef(false);

    for (uint dir = 0; dir < 2; dir++) {
      addNetOnTable(net->getId(),
                    dir,
                    &maxRect,
                    _tiles->_tileSize,
                    _tiles->_ll,
                    _tiles->_ur,
                    _tiles->_signalTable);
    }
    cnt += cnt1;
  }
  if (_overCell || !skipInsts) {
    dbSet<dbInst> insts = _block->getInsts();
    dbSet<dbInst>::iterator inst_itr;

    for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
      dbInst* inst = *inst_itr;
      dbBox* bb = inst->getBBox();

      Rect s;
      bb->getBox(s);

      for (uint dir = 0; dir < 2; dir++)
        addNetOnTable(inst->getId(),
                      dir,
                      &s,
                      _tiles->_tileSize,
                      _tiles->_ll,
                      _tiles->_ur,
                      _tiles->_instTable);

      inst->clearUserFlag1();
    }
  }
  return cnt;
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

  FILE* fp = NULL;
#ifdef TEST_SIGNAL_TABLE
  char filename[64];
  sprintf(filename, "new/%d.%d.%d.sig.sdb", dir, lo_sdb[dir], hi_sdb[dir]);
  fp = fopen(filename, "w");
#endif

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

      bool inside = false;
      if ((ur[dir] <= hi_sdb[dir]) && (ll[dir] >= lo_sdb[dir])) {
        inside = true;
        local += cnt;
      }

      cnt += wire->length();

      if (countOnly)
        continue;

      if (!net->isRCgraph())
        rcCnt += rcNetGen(net);

      createDbNet->copyNet(net, false);

      /*
      if (local) {
              createDbNet->copyNet(net, false);
      }
      else {
              createDbNet->copyNet(net, false);
              continue;
      }

      Rect maxRectV;
      Rect maxRectH;
      Rect *maxRect[2];
      maxRect[0]= &maxRectH;
      maxRect[1]= &maxRectV;

      uint cnt1= getNetBbox(net, maxRect);
      */
    }
  }
  logger_->info(RCX,
                98,
                "{} local out of {} tile wires out of {} total",
                local,
                cnt1,
                tot);

  resetNetSpefFlag(_tiles->_tmpIdTable);

#ifdef TEST_SIGNAL_TABLE
  fprintf(fp,
          "dir= %d   %d %d -- %d %d sigCnt= %d\n",
          dir,
          lo_sdb[0],
          hi_sdb[0],
          lo_sdb[1],
          hi_sdb[1],
          cnt);

  if (fp != NULL)
    fclose(fp);
#endif

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

        Rect r;
        s->getBox(r);

        if (!tileRect.intersects(r))
          continue;

        createDbNet->createSpecialWire(NULL, r, s->getTechLayer(), 0);
        cnt++;
      }
    }
  }
  return cnt;
}

uint extMain::createWindowsDB(bool rlog,
                              Rect& extRect,
                              uint trackStep,
                              uint ccFlag,
                              uint use_bin_tables)
{
  bool single_gs = false;

  extWindow* W = initWindowSearch(extRect, trackStep, ccFlag, 0, NULL);

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
                         NULL);

  extWindow* windowTable[1000];
  uint windowCnt = 0;

  int** limitArray;
  limitArray = new int*[W->_layerCnt];
  for (uint k = 0; k < W->_layerCnt; k++)
    limitArray[k] = new int[10];

  _tiles = NULL;

  _wireBinTable = NULL;
  _cntxBinTable = NULL;
  _cntxInstTable = NULL;
  uint binCnt[2];
  AthPool<extWire>* wpool = NULL;

  bool use_signal_tables = false;
  if ((use_bin_tables == 1) || (use_bin_tables == 2)) {
    use_signal_tables = true;

    logger_->info(RCX,
                  88,
                  "Signal_table= {} ----------------------------- ",
                  _use_signal_tables);

    wpool = new AthPool<extWire>(false, 1024);

    _wireBinTable = mkSignalBins(
        W->_maxPitch * W->_ccDist, W->_ll, W->_ur, binCnt, wpool, false);
    //_cntxBinTable= mkSignalBins(W->_step_nm[0], W->_ll, W->_ur, binCnt, wpool,
    // true);
    _cntxInstTable
        = mkInstBins(W->_maxPitch * W->_ccDist, W->_ll, W->_ur, binCnt);

    if (rlog)
      AthResourceLog("Making net tables ", 0);
  } else if (use_bin_tables == 3) {  // signal_table based tiling
    _tiles = new extTileSystem(extRect, W->_step_nm);
    bool skipPower = false;
    bool skipInsts = false;
    uint wireCnt = mkTileBoundaries(skipPower, skipInsts);
  }

  for (int dir = 1; dir >= 0; dir--) {
    if (dir == 0)
      enableRotatedFlag();
    else
      disableRotatedFlag();

    //_search->initCouplingCapLoops(dir, ccFlag, coupleAndCompute, m);
    _search->initCouplingCapLoops(dir, ccFlag, NULL, NULL);

    uint stepNum = 0;
    int hiXY = W->setExtBoundaries(dir);
    // logger_->info(RCX, 0, "Dir={} hiXY= {}", dir, hiXY);
    for (; hiXY <= W->_ur[dir]; hiXY += W->_step_nm[dir]) {
      hiXY = W->adjust_hiXY(hiXY);

      uint extractedWireCnt = 0;
      int extractLimit = W->set_extractLimit();
      W->_minExtracted = _search->couplingCaps(W->_extractLimit,
                                               W->_ccDist,
                                               W->_currentDir,
                                               extractedWireCnt,
                                               NULL,
                                               NULL,
                                               _getBandWire,
                                               limitArray);

      // W->reportProcessedWires(rlog);

      int deallocLimit = W->getDeallocLimit();
      _search->dealloc(W->_currentDir, deallocLimit);

      // W->printBoundaries(boundFP, true);

      W->updateExtLimits(limitArray);

      extWindow* W1 = new extWindow(W, windowCnt, logger_);
      windowTable[windowCnt++] = W1;

      W->updateLoBounds(false /*report*/);

      // W->printExtLimits(limitFP);

      // dbBlock* extBlock= W->createExtBlock(m, _block, extRect);
      dbBlock* extBlock = W1->createExtBlock(NULL, _block, extRect);
      // logger_->info(RCX, 0, "Created dbBlock {} ", extBlock->getConstName());
    }
  }
  return 1;
}
uint extMain::fillWindowsDB(bool rlog, Rect& extRect, uint use_signal_tables)
{
  dbIntProperty* p = (dbIntProperty*) dbProperty::find(_block, "_currentDir");
  if (p == NULL) {
    logger_->warn(RCX,
                  99,
                  "Block {} has no defined extraction boundaries",
                  _block->getConstName());
    return 0;
  }
  bool rcgenFlag = use_signal_tables > 1 ? true : false;

  // logger_->info(RCX, 0, "D{}", p->getValue());

  extWindow* W = new extWindow(20, logger_);
  W->getExtProperties(_block);

  dbBlock* extBlock = _block;
  setBlock(_block->getParent());

  if (use_signal_tables == 2) {
    uint extWireCnt = 0;
    uint rcCnt = 0;
    if (_tiles != NULL)
      extWireCnt = mkTileNets(
          W->_currentDir, W->_lo_sdb, W->_hi_sdb, false, NULL, rcCnt, true);
    else
      extWireCnt = addNets3(W->_currentDir,
                            W->_lo_sdb,
                            W->_hi_sdb,
                            W->_ll,
                            W->_ur,
                            W->_maxPitch * W->_ccDist,
                            _wireBinTable,
                            NULL);

    dbIntProperty::create(extBlock, "_estimatedWireCnt", extWireCnt);
    setBlock(extBlock);
    return extWireCnt;
  }

  dbCreateNetUtil createNetUtil;

  createNetUtil.setBlock(extBlock, false);
  if (use_signal_tables == 3) {
    dbBlock::copyViaTable(extBlock, _block);
  }

  uint extWireCnt = 0;
  if (use_signal_tables == 0) {
    fillWindowGs(W, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &createNetUtil);

    uint processWireCnt = fillWindowSearch(W,
                                           W->_lo_sdb,
                                           W->_hi_sdb,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &createNetUtil);
  } else if (use_signal_tables == 3) {
    bool powerNets = true;

    uint rcCnt = 0;

    extWireCnt = mkTileNets(W->_currentDir,
                            W->_lo_sdb,
                            W->_hi_sdb,
                            powerNets,
                            &createNetUtil,
                            rcCnt);
    logger_->info(RCX,
                  100,
                  "BBlock {} has {} signal wires and {} rcSegs were generated",
                  extBlock->getConstName(),
                  extWireCnt,
                  rcCnt);
    rcgenFlag = false;
  } else {
    createNetUtil.allocMapArray(2000000);
    extWireCnt = addNets3(W->_currentDir,
                          W->_lo_sdb,
                          W->_hi_sdb,
                          W->_ll,
                          W->_ur,
                          W->_maxPitch * W->_ccDist,
                          _wireBinTable,
                          &createNetUtil);

    uint cntxCnt = addNets3GS(W->_currentDir,
                              W->_lo_gs,
                              W->_hi_gs,
                              W->_ll,
                              W->_ur,
                              W->_step_nm[0],
                              _cntxBinTable,
                              &createNetUtil);

    uint instCnt = addInsts(W->_currentDir,
                            W->_lo_gs,
                            W->_hi_gs,
                            W->_ll,
                            W->_ur,
                            W->_maxPitch * W->_ccDist,
                            _cntxInstTable,
                            &createNetUtil);
    uint signalWireCnt = createNetShapePropertires(extBlock);
    logger_->info(RCX,
                  96,
                  "Block {} has {} signal wires",
                  extBlock->getConstName(),
                  signalWireCnt);
  }

  if (rcgenFlag) {
    uint rcCnt = rcGenTile(extBlock);
    logger_->info(RCX, 101, "{} rsegs for {}", rcCnt, extBlock->getConstName());
  }
  return extWireCnt;
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
  if (setMinTypMax(false, false, false, NULL, false, false, -1, -1, -1, 1)
      < 0) {
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

  // if (_couplingFlag>1 && !_lefRC)
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
  logger_->info(RCX, 40, "Final {} rc segments", cnt);
  return cnt;
}
uint extMain::rcGenBlock(dbBlock* block)
{
  if (block == NULL)
    block = _block;

  uint cnt = 0;

  dbSet<dbNet> bnets = block->getNets();
  dbSet<dbNet>::iterator net_itr;
  for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    cnt += rcNetGen(net);
  }
  logger_->info(RCX, 40, "Final {} rc segments", cnt);
  return cnt;
}
void extMain::writeMapping(dbBlock* block)
{
  if (block == NULL)
    block = _block;

  char buf[1024];
  sprintf(buf, "%s.netMap", block->getConstName());
  FILE* fp = fopen(buf, "w");

  uint cnt = 0;

  dbSet<dbNet> bnets = block->getNets();
  dbSet<dbNet>::iterator net_itr;
  for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    net->printNetName(fp, true, true);

    dbWire* wire = net->getWire();

    if (wire == NULL)
      continue;

    dbWireShapeItr shapes;
    dbShape s;
    for (shapes.begin(wire); shapes.next(s);) {
      if (s.isVia())
        continue;

      Rect r;
      s.getBox(r);

      fprintf(fp,
              "\t\t%d  %d %d  %d %d  %d %d\n",
              shapes.getShapeId(),
              r.dx(),
              r.dy(),
              r.xMin(),
              r.yMin(),
              r.xMax(),
              r.yMax());
    }
  }
  fclose(fp);
}

}  // namespace rcx
