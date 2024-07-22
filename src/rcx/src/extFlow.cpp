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

#include <map>
#include <vector>

#include "rcx/dbUtil.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"
#include "wire.h"

namespace rcx {

using odb::dbInst;
using odb::dbNet;
using odb::dbRSeg;
using odb::dbSBox;
using odb::dbSet;
using odb::dbShape;
using odb::dbSigType;
using odb::dbSWire;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbTechLayerType;
using odb::dbTrackGrid;
using odb::dbWire;
using odb::dbWireShapeItr;
using odb::MAX_INT;
using odb::MIN_INT;
using odb::Rect;
using utl::RCX;

uint extMain::getBucketNum(int base, int max, uint step, int xy)
{
  if (xy >= max) {
    xy = max - 1;
  }

  int delta = xy - base;
  if (delta < 0) {
    return 0;
  }

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
    if (wireTable[dir][ii] == nullptr) {
      wireTable[dir][ii] = new Ath__array1D<uint>(8000);
    }

    wireTable[dir][ii]->add(netId);
    cnt++;
  }
  return cnt;
}

uint extMain::getNetBbox(dbNet* net, Rect& maxRect)
{
  dbWire* wire = net->getWire();
  if (wire == nullptr) {
    return 0;
  }

  maxRect.reset(MAX_INT, MAX_INT, MIN_INT, MIN_INT);
  uint cnt = 0;
  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    if (s.isVia()) {
      continue;
    }

    Rect r = s.getBox();

    maxRect.merge(r);
    cnt++;
  }
  return cnt;
}

uint extMain::getNetBbox(dbNet* net, Rect* maxRect[2])
{
  dbWire* wire = net->getWire();
  if (wire == nullptr) {
    return 0;
  }

  maxRect[0]->reset(MAX_INT, MAX_INT, MIN_INT, MIN_INT);
  maxRect[1]->reset(MAX_INT, MAX_INT, MIN_INT, MIN_INT);
  uint cnt = 0;
  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    if (s.isVia()) {
      continue;
    }
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
  if (wire == nullptr) {
    return;
  }

  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    if (s.isVia()) {
      continue;
    }

    Rect r = s.getBox();

    uint dd = 0;  // vertical
    if (r.dx() > r.dy()) {
      dd = 1;  // horizontal
    }

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

      if (s->isVia()) {
        continue;
      }

      Rect r = s->getBox();

      uint dd = 0;  // vertical
      if (r.dx() > r.dy()) {
        dd = 1;  // horizontal
      }

      maxRectSdb[dd]->merge(r);
      hasSdbWires[dd] = true;

      maxRectGs.merge(r);
      hasGsWires = true;
    }
  }
}

bool extMain::matchDir(uint dir, Rect& r)
{
  uint dd = 0;  // vertical
  if (r.dx() >= r.dy()) {
    dd = 1;  // horizontal
  }

  return dir == dd;
}

bool extMain::isIncludedInsearch(Rect& r,
                                 uint dir,
                                 const int* bb_ll,
                                 const int* bb_ur)
{
  if (!matchDir(dir, r)) {
    return false;
  }

  int ll[2] = {r.xMin(), r.yMin()};

  if (ll[dir] >= bb_ur[dir]) {
    return false;
  }

  if (ll[dir] < bb_ll[dir]) {
    return false;
  }

  return true;
}

bool extMain::isIncluded(Rect& r, uint dir, const int* ll, const int* ur)
{
  uint dd = 0;  // vertical
  if (r.dx() > r.dy()) {
    dd = 1;  // horizontal
  }

  if (dir != dd) {
    return false;
  }

  int rLL[2] = {r.xMin(), r.yMin()};
  int rUR[2] = {r.xMax(), r.yMax()};

  if ((rUR[dir] < ll[dir]) || (rLL[dir] > ur[dir])) {
    return false;
  }

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
  dbTrackGrid* tg = nullptr;

  Rect maxRect;
  if ((extRect.dx() > 0) && (extRect.dy() > 0)) {
    maxRect = extRect;
  } else {
    maxRect = _block->getDieArea();
    if (!((maxRect.dx() > 0) && (maxRect.dy() > 0))) {
      logger_->error(
          RCX, 81, "Die Area for the block has 0 size, or is undefined!");
    }
  }

  if (USE_DB_UNITS) {
    GetDBcoords2(maxRect);
    GetDBcoords2(extRect);
  }

  std::vector<int> trackXY(32000);
  uint n = 0;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;

    if (layer->getRoutingLevel() == 0) {
      continue;
    }

    n = layer->getRoutingLevel();
    int w = GetDBcoords2(layer->getWidth());
    widthTable[n] = layer->getWidth();

    if (USE_DB_UNITS) {
      widthTable[n] = w;
    }

    W[n] = 1;
    int s = GetDBcoords2(layer->getSpacing());
    S[n] = layer->getSpacing();

    if (USE_DB_UNITS) {
      S[n] = s;
    }

    int p = GetDBcoords2(layer->getPitch());
    pitchTable[n] = layer->getPitch();

    if (USE_DB_UNITS) {
      pitchTable[n] = p;
    }
    if (pitchTable[n] <= 0) {
      logger_->error(RCX,
                     82,
                     "Layer {}, routing level {}, has pitch {}!!",
                     layer->getConstName(),
                     n,
                     pitchTable[n]);
    }

    dirTable[n] = 0;
    if (layer->getDirection() == dbTechLayerDir::HORIZONTAL) {
      dirTable[n] = 1;
    }

    if (skipBaseCalc) {
      continue;
    }

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
      if (s->isVia()) {
        continue;
      }

      uint x = s->getDX();
      uint y = s->getDY();
      uint w = y;
      if (w < x) {
        w = x;
      }

      if (maxWidth > w) {
        maxWidth = w;
      }

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

    if (!((net->getSigType().isSupply()))) {
      continue;
    }

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
  return _search->addBox(
      r.xMin(), r.yMin(), r.xMax(), r.yMax(), level, id, shapeId, wtype);
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
      if (s->isVia()) {
        continue;
      }

      Rect r = s->getBox();
      if (isIncludedInsearch(r, dir, bb_ll, bb_ur)) {
        uint level = s->getTechLayer()->getRoutingLevel();

        if (netUtil != nullptr) {
          netUtil->createSpecialWire(nullptr, r, s->getTechLayer(), s->getId());
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
      if (s->isVia()) {
        continue;
      }

      Rect r = s->getBox();
      if (isIncludedInsearch(r, dir, bb_ll, bb_ur)) {
        uint level = s->getTechLayer()->getRoutingLevel();

        if (step > 0) {
          uint len = r.dx();
          if (len < r.dy()) {
            len = r.dy();
          }

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
                if (y2 > r.yMax()) {
                  y2 = r.yMax();
                }

                _search->addBox(
                    r.xMin(), y1, r.xMax(), y2, level, s->getId(), 0, wtype);
                y1 = y2;
              }
            } else {  // horizontal
              for (int x1 = r.xMin(); x1 < r.xMax();) {
                int x2 = x1 + step;
                if (x2 > r.xMax()) {
                  x2 = r.xMax();
                }

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

    if ((net->getSigType().isSupply())) {
      continue;
    }

    dbWire* wire = net->getWire();
    if (wire == nullptr) {
      continue;
    }

    dbWireShapeItr shapes;
    dbShape s;
    for (shapes.begin(wire); shapes.next(s);) {
      if (s.isVia()) {
        continue;
      }

      uint x = s.getDX();
      uint y = s.getDY();
      uint w = y;
      if (w > x) {
        w = x;
      }

      if (maxWidth < w) {
        maxWidth = w;
      }
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

    if (!((net->getSigType().isSupply()))) {
      continue;
    }

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

  if (wire == nullptr) {
    return 0;
  }

  if (netUtil != nullptr) {
    netUtil->setCurrentNet(nullptr);
  }

  uint cnt = 0;

  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    int shapeId = shapes.getShapeId();

    if (s.isVia()) {
      continue;
    }

    Rect r = s.getBox();
    if (isIncludedInsearch(r, dir, bb_ll, bb_ur)) {
      uint level = s.getTechLayer()->getRoutingLevel();

      if (netUtil != nullptr) {
        netUtil->createNetSingleWire(r, level, net->getId(), shapeId);
      } else {
        if (USE_DB_UNITS) {
          _search->addBox(GetDBcoords2(r.xMin()),
                          GetDBcoords2(r.yMin()),
                          GetDBcoords2(r.xMax()),
                          GetDBcoords2(r.yMax()),
                          level,
                          net->getId(),
                          shapeId,
                          wtype);
        } else {
          const uint trackNum = _search->addBox(r.xMin(),
                                                r.yMin(),
                                                r.xMax(),
                                                r.yMax(),
                                                level,
                                                net->getId(),
                                                shapeId,
                                                wtype);
          if (net->getId() == _debug_net_id) {
            const int dx = r.xMax() - r.xMin();
            const int dy = r.yMax() - r.yMin();
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

    if (s.getTechLayer()->getType() == dbTechLayerType::CUT) {
      continue;
    }

    int x1 = s.xMin();
    int y1 = s.yMin();
    int x2 = s.xMax();
    int y2 = s.yMax();
    int dx = x2 - x1;
    int dy = y2 - y1;

    uint level = s.getTechLayer()->getRoutingLevel();
    uint width = s.getTechLayer()->getWidth();

    if (s.getTechLayer()->getDirection() == dbTechLayerDir::VERTICAL) {
      if (width != dx) {
        continue;
      }
    } else if (width != dy) {
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

  FILE* fp = nullptr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType().isSupply())) {
      continue;
    }

    cnt += addNetShapesOnSearch(net, dir, bb_ll, bb_ur, wtype, fp, createDbNet);
  }
  if (createDbNet == nullptr) {
    _search->adjustOverlapMakerEnd();
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
    if (wireBinTable[dir][bucket] == nullptr) {
      continue;
    }

    uint instCnt = wireBinTable[dir][bucket]->getCnt();
    for (uint ii = 0; ii < instCnt; ii++) {
      uint instId = wireBinTable[dir][bucket]->get(ii);
      dbInst* inst0 = dbInst::getInst(_block, instId);

      if (inst0->getUserFlag1()) {
        continue;
      }

      inst0->setUserFlag1();

      createDbNet->createInst(inst0);
      cnt++;
    }
  }
  for (bucket = lo_index; bucket <= hi_index; bucket++) {
    if (wireBinTable[dir][bucket] == nullptr) {
      continue;
    }

    uint instCnt = wireBinTable[dir][bucket]->getCnt();
    for (uint ii = 0; ii < instCnt; ii++) {
      uint instId = wireBinTable[dir][bucket]->get(ii);
      dbInst* inst0 = dbInst::getInst(_block, instId);

      inst0->clearUserFlag1();
    }
  }
  return cnt;
}

uint extMain::addNets(uint dir,
                      int* bb_ll,
                      int* bb_ur,
                      uint wtype,
                      uint ptype,
                      Ath__array1D<uint>* sdbSignalTable)
{
  if (sdbSignalTable == nullptr) {
    return 0;
  }

  uint cnt = 0;
  uint netCnt = sdbSignalTable->getCnt();
  for (uint ii = 0; ii < netCnt; ii++) {
    dbNet* net = dbNet::getNet(_block, sdbSignalTable->get(ii));

    if ((net->getSigType().isSupply())) {
      cnt += addNetSBoxes(net, dir, bb_ll, bb_ur, ptype);
    } else {
      cnt += addNetShapesOnSearch(net, dir, bb_ll, bb_ur, wtype, nullptr);
    }
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
  if (dx > dy) {
    dd = 1;  // horizontal
  }

  return dd;
}

uint extMain::addShapeOnGS(dbNet* net,
                           uint sId,
                           Rect& r,
                           bool plane,
                           dbTechLayer* layer,
                           bool gsRotated,
                           bool swap_coords,
                           int dir)
{
  if (dir >= 0) {
    if (!plane) {
      if (matchDir(dir, r)) {
        return 0;
      }
    }
  }

  uint level = layer->getRoutingLevel();
  int n = 0;
  if (!gsRotated) {
    n = _geomSeq->box(r.xMin(), r.yMin(), r.xMax(), r.yMax(), level);
  } else {
    if (!swap_coords) {  // horizontal
      n = _geomSeq->box(r.xMin(), r.yMin(), r.xMax(), r.yMax(), level);
    } else {
      n = _geomSeq->box(r.yMin(), r.xMin(), r.yMax(), r.xMax(), level);
    }
  }
  if (n == 0) {
    return 1;
  }
  return 0;
}

uint extMain::addNetShapesGs(dbNet* net,
                             bool gsRotated,
                             bool swap_coords,
                             int dir)
{
  bool USE_DB_UNITS = false;
  uint cnt = 0;
  dbWire* wire = net->getWire();
  if (wire == nullptr) {
    return 0;
  }

  bool plane = false;
  if (net->getSigType() == dbSigType::ANALOG) {
    plane = true;
  }

  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    if (s.isVia()) {
      continue;
    }

    int shapeId = shapes.getShapeId();

    Rect r = s.getBox();

    if (USE_DB_UNITS) {
      this->GetDBcoords2(r);
    }

    cnt += addShapeOnGS(
        net, shapeId, r, plane, s.getTechLayer(), gsRotated, swap_coords, dir);
  }
  return cnt;
}

uint extMain::addNetSboxesGs(dbNet* net,
                             bool gsRotated,
                             bool swap_coords,
                             int dir)
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

      if (s->isVia()) {
        continue;
      }

      Rect r = s->getBox();
      cnt += addShapeOnGS(nullptr,
                          s->getId(),
                          r,
                          true,
                          s->getTechLayer(),
                          gsRotated,
                          swap_coords,
                          dir);
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
                         const uint* dirTable,
                         int* bb_ll)
{
  bool rotatedFlag = getRotatedFlag();

  {
    delete _geomSeq;
  }
  _geomSeq = new gs(_seqPool);

  _geomSeq->set_slices(layerCnt);

  for (uint ii = 1; ii < layerCnt; ii++) {
    uint layerDir = dirTable[ii];

    uint res[2] = {pitchTable[ii], widthTable[ii]};  // vertical
    if (layerDir > 0) {                              // horizontal
      res[0] = widthTable[ii];
      res[1] = pitchTable[ii];
    }
    if (res[dir] == 0) {
      continue;
    }
    int ll[2];
    ll[!dir] = bb_ll[!dir];
    ll[dir] = getXY_gs(bb_ll[dir], wLL[dir], res[dir]);

    int ur[2];
    ur[!dir] = wUR[!dir];
    ur[dir] = getXY_gs(bb_ll[dir], wUR[dir], res[dir]);

    if (!rotatedFlag) {
      _geomSeq->configureSlice(ii, res[0], res[1], ll[0], ll[1], ur[0], ur[1]);
    } else {
      if (dir > 0) {  // horizontal segment extraction
        _geomSeq->configureSlice(
            ii, res[0], res[1], ll[0], ll[1], ur[0], ur[1]);
      } else {
        if (layerDir > 0) {
          _geomSeq->configureSlice(
              ii, pitchTable[ii], widthTable[ii], ll[1], ll[0], ur[1], ur[0]);

        } else {
          _geomSeq->configureSlice(
              ii, widthTable[ii], pitchTable[ii], ll[1], ll[0], ur[1], ur[0]);
        }
      }
    }
  }
  return layerCnt;
}

bool extMain::getRotatedFlag()
{
  return _rotatedGs;
}

bool extMain::enableRotatedFlag()
{
  _rotatedGs = true;

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
                       uint* widthTable)
{
  bool rotatedGs = getRotatedFlag();

  initPlanes(dir, lo_gs, hi_gs, layerCnt, pitchTable, widthTable, dirTable, ll);

  const int gs_dir = dir;

  uint pcnt = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if (!((net->getSigType().isSupply()))) {
      continue;
    }

    pcnt += addNetSboxesGs(net, rotatedGs, !dir, gs_dir);
  }

  uint scnt = 0;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;

    if ((net->getSigType().isSupply())) {
      continue;
    }

    scnt += addNetShapesGs(net, rotatedGs, !dir, gs_dir);
  }

  return pcnt + scnt;
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
  Ath__array1D<uint> sdbPowerTable;
  Ath__array1D<uint> tmpNetIdTable(64000);

  uint totalWiresExtracted = 0;

  int** limitArray;
  limitArray = new int*[layerCnt];
  for (uint jj = 0; jj < layerCnt; jj++) {
    limitArray[jj] = new int[10];
  }

  FILE* bandinfo = nullptr;
  if (_printBandInfo) {
    if (_getBandWire) {
      bandinfo = fopen("bandInfo.getWire", "w");
    } else {
      bandinfo = fopen("bandInfo.extract", "w");
    }
  }
  for (int dir = 1; dir >= 0; dir--) {
    if (_printBandInfo) {
      fprintf(bandinfo, "dir = %d\n", dir);
    }
    if (dir == 0) {
      enableRotatedFlag();
    }

    lo_gs[!dir] = ll[!dir];
    hi_gs[!dir] = ur[!dir];
    lo_sdb[!dir] = ll[!dir];
    hi_sdb[!dir] = ur[!dir];

    int gs_limit = ll[dir];

    _search->initCouplingCapLoops(dir, ccFlag, coupleAndCompute, m);

    lo_sdb[dir] = ll[dir] - step_nm[dir];
    int hiXY = ll[dir] + step_nm[dir];
    if (hiXY > ur[dir]) {
      hiXY = ur[dir];
    }

    uint stepNum = 0;
    for (; hiXY <= ur[dir]; hiXY += step_nm[dir]) {
      if (ur[dir] - hiXY <= (int) step_nm[dir]) {
        hiXY = ur[dir] + 5 * ccDist * maxPitch;
      }

      lo_gs[dir] = gs_limit;
      hi_gs[dir] = hiXY;

      fill_gs4(dir,
               ll,
               ur,
               lo_gs,
               hi_gs,
               layerCnt,
               dirTable,
               pitchTable,
               widthTable);

      m->_rotatedGs = getRotatedFlag();
      m->_pixelTable = _geomSeq;

      // add wires onto search such that    loX<=loX<=hiX
      hi_sdb[dir] = hiXY;

      uint processWireCnt = 0;
      processWireCnt += addPowerNets(dir, lo_sdb, hi_sdb, pwrtype);
      processWireCnt += addSignalNets(dir, lo_sdb, hi_sdb, sigtype);

      uint extractedWireCnt = 0;
      int extractLimit = hiXY - ccDist * maxPitch;
      const int minExtracted = _search->couplingCaps(extractLimit,
                                                     ccFlag,
                                                     dir,
                                                     extractedWireCnt,
                                                     coupleAndCompute,
                                                     m,
                                                     _getBandWire,
                                                     limitArray);

      int deallocLimit = minExtracted - (ccDist + 1) * maxPitch;
      if (_printBandInfo) {
        fprintf(bandinfo,
                "    step %d  hiXY=%d extLimit=%d minExtracted=%d "
                "deallocLimit=%d\n",
                stepNum,
                hiXY,
                extractLimit,
                minExtracted,
                deallocLimit);
      }
      _search->dealloc(dir, deallocLimit);

      lo_sdb[dir] = hiXY;
      gs_limit = minExtracted - (ccDist + 2) * maxPitch;

      stepNum++;
      totalWiresExtracted += processWireCnt;
      float percent_extracted
          = lround(100.0 * (1.0 * totalWiresExtracted / totWireCnt));

      if ((totWireCnt > 0) && (totalWiresExtracted > 0)
          && (((totWireCnt == totalWiresExtracted)
               && (percent_extracted > _previous_percent_extracted))
              || (percent_extracted - _previous_percent_extracted >= 5.0))) {
        logger_->info(RCX,
                      442,
                      "{:d}% of {:d} wires extracted",
                      (int) (100.0 * (1.0 * totalWiresExtracted / totWireCnt)),
                      totWireCnt);

        _previous_percent_extracted = percent_extracted;
      }
    }
  }
  if (_printBandInfo) {
    fclose(bandinfo);
  }

  delete _geomSeq;
  _geomSeq = nullptr;

  for (uint jj = 0; jj < layerCnt; jj++) {
    delete[] limitArray[jj];
  }
  delete[] limitArray;

  return 0;
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
    return nullptr;
  }
  dbRSeg* rseg2 = dbRSeg::getRSeg(net->getBlock(), rsegId2);

  if (rseg2 == nullptr) {
    logger->warn(RCX,
                 240,
                 "GndCap: cannot find rseg for rsegId {} on net {} {}",
                 rsegId2,
                 net->getId(),
                 net->getConstName());
    return nullptr;
  }
  return rseg2;
}

}  // namespace rcx
