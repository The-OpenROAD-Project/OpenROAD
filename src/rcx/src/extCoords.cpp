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

#include "parse.h"
#include "rcx/extSpef.h"
#include "wire.h"

namespace rcx {

using namespace odb;

void extSpef::initSearchForNets()
{
  uint W[16];
  uint S[16];
  uint P[16];
  int X1[16];
  int Y1[16];

  dbSet<dbTechLayer> layers = _tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;
  dbTrackGrid* tg = NULL;

  Rect maxRect = _block->getDieArea();

  std::vector<int> trackXY(32000);
  uint n = 0;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;

    if (layer->getRoutingLevel() == 0)
      continue;

    n = layer->getRoutingLevel();
    W[n] = 1;
    S[n] = layer->getSpacing();
    P[n] = layer->getPitch();
    if (P[n] <= 0)
      error(0,
            "Layer %s, routing level %d, has pitch %d !!\n",
            layer->getConstName(),
            n,
            P[n]);
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

  _search = new Ath__gridTable(&maxRect, 2, layerCnt, W, P, S, X1, Y1);
  _search->setBlock(_block);
}

uint extSpef::addNetShapesOnSearch(uint netId)
{
  dbNet* net = dbNet::getNet(_block, netId);

  dbWire* wire = net->getWire();
  uint wtype = 9;

  if (wire == NULL)
    return 0;

  uint cnt = 0;

  int minx = MAX_INT;
  int miny = MAX_INT;
  int maxx = -MAX_INT;
  int maxy = -MAX_INT;
  dbWireShapeItr shapes;
  odb::dbShape s;
  uint level1, level2;
  odb::dbVia* vv;
  odb::dbTechVia* tv;
  for (shapes.begin(wire); shapes.next(s);) {
    int shapeId = shapes.getShapeId();

    tv = s.getTechVia();
    vv = s.getVia();
    if (tv) {
      level1 = tv->getBottomLayer()->getRoutingLevel();
      level2 = tv->getTopLayer()->getRoutingLevel();
    } else if (vv) {
      level1 = vv->getBottomLayer()->getRoutingLevel();
      level2 = vv->getTopLayer()->getRoutingLevel();
    } else {
      level1 = s.getTechLayer()->getRoutingLevel();
      level2 = 0;
    }

    Rect r = s.getBox();

    _search->addBox(
        r.xMin(), r.yMin(), r.xMax(), r.yMax(), level1, netId, shapeId, wtype);
    if (level2)
      _search->addBox(r.xMin(),
                      r.yMin(),
                      r.xMax(),
                      r.yMax(),
                      level2,
                      netId,
                      shapeId,
                      wtype);
    cnt++;
    minx = std::min(r.xMin(), minx);
    miny = std::min(r.yMin(), miny);
    maxx = std::max(r.xMax(), maxx);
    maxy = std::max(r.yMax(), maxy);
  }
  _search->setMaxArea(minx, miny, maxx, maxy);
  for (uint dir = 0; dir < _search->getRowCnt(); dir++) {
    for (uint layer = 1; layer < _search->getColCnt(); layer++) {
      _search->getGrid(dir, layer)->setSearchDomain(0);
    }
  }
  return cnt;
}

void extSpef::initNodeCoordTables(uint memChunk)
{
  _capNodeTable = new Ath__array1D<uint>(memChunk);
  _xCoordTable = new Ath__array1D<double>(memChunk);
  _yCoordTable = new Ath__array1D<double>(memChunk);
  _x1CoordTable = new Ath__array1D<int>(memChunk);
  _x2CoordTable = new Ath__array1D<int>(memChunk);
  _y1CoordTable = new Ath__array1D<int>(memChunk);
  _y2CoordTable = new Ath__array1D<int>(memChunk);
  _levelTable = new Ath__array1D<uint>(memChunk);
  _idTable = new Ath__array1D<uint>(16);
  initSearchForNets();  // search DB
}

void extSpef::resetNodeCoordTables()
{
  _capNodeTable->resetCnt();
  _xCoordTable->resetCnt();
  _yCoordTable->resetCnt();
  _x1CoordTable->resetCnt();
  _x2CoordTable->resetCnt();
  _y1CoordTable->resetCnt();
  _y2CoordTable->resetCnt();
  _levelTable->resetCnt();
}

void extSpef::deleteNodeCoordTables()
{
  if (_capNodeTable)
    delete _capNodeTable;
  _capNodeTable = NULL;
  if (_xCoordTable)
    delete _xCoordTable;
  _xCoordTable = NULL;
  if (_yCoordTable)
    delete _yCoordTable;
  _yCoordTable = NULL;
  if (_x1CoordTable)
    delete _x1CoordTable;
  _x1CoordTable = NULL;
  if (_y1CoordTable)
    delete _y1CoordTable;
  _y1CoordTable = NULL;
  if (_x2CoordTable)
    delete _x2CoordTable;
  _x2CoordTable = NULL;
  if (_y2CoordTable)
    delete _y2CoordTable;
  _y2CoordTable = NULL;
  if (_levelTable)
    delete _levelTable;
  _levelTable = NULL;
  if (_search)
    delete _search;
  _search = NULL;
  if (_idTable)
    delete _idTable;
  _idTable = NULL;
}

bool extSpef::readNodeCoords(uint cpos)
{
  //*CONN
  //*I *877470:SI_x5000y1770 I *C 3.54000 125.970 *L 2.41675 *D R00SPX00HA0
  //*I *875052:SO_x60y3190 O *C 2.66000 120.190 *L 1.54573 *D R00MSX42HD0
  //*N *1:2 *C 3.06500 125.815 M1
  //*N *1:3 *C 3.03000 120.555 M5
  //*N *2:4 *C 3.07000 120.190 M4
  //*N *2:5 *C 3.07000 120.190 M1

  uint wCnt = _parser->getWordCnt();
  if (cpos + 3 > wCnt)
    return false;

  uint id1;
  uint tokenCnt = _nodeParser->mkWords(_parser->get(1));
  if (tokenCnt == 2 && _nodeParser->isDigit(1, 0)) {  // internal node
    id1 = _nodeParser->getInt(0, 1);
    if (id1 != _tmpNetSpefId)
      return false;
  }
  uint netId = 0;
  uint nodeId = getCapNodeId(_parser->get(1), NULL, &netId);
  double x = _parser->getDouble(cpos + 1);
  double y = _parser->getDouble(cpos + 2);

  _capNodeTable->add(nodeId);
  _xCoordTable->add(x);
  _yCoordTable->add(y);
  uint level = 0;
  _levelTable->add(level);
  return true;
}

int extSpef::findNodeIndexFromNodeCoords(uint targetCapNodeId)  // TO OPTIMIZE
{
  uint ii;
  for (ii = 0; ii < _capNodeTable->getCnt(); ii++) {
    uint capId = _capNodeTable->get(ii);
    if (capId == targetCapNodeId)
      break;
  }
  if (ii == _capNodeTable->getCnt())
    return -1;

  return ii;
}

void extSpef::searchDealloc()
{
  _search->dealloc();
}

}  // namespace rcx

namespace odb {

void Ath__grid::dealloc()
{
  for (uint ii = 0; ii <= _searchHiTrack; ii++) {
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
  }
}

void Ath__gridTable::dealloc()
{
  for (uint dir = 0; dir < _rowCnt; dir++) {
    for (uint jj = 1; jj < _colCnt; jj++) {
      Ath__grid* netGrid = _gridTable[dir][jj];
      if (netGrid == NULL)
        continue;

      netGrid->dealloc();
    }
  }
}

}  // namespace odb
