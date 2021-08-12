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

#include <math.h>

#include "parse.h"
#include "rcx/extSpef.h"
#include "wire.h"

namespace rcx {

void extSpef::initSearchForNets() {
  uint W[16];
  uint S[16];
  uint P[16];
  int X1[16];
  int Y1[16];

  dbSet<dbTechLayer> layers = _tech->getLayers();
  dbSet<dbTechLayer>::iterator itr;
  dbTrackGrid* tg = NULL;

  Rect maxRect;
  _block->getDieArea(maxRect);

  std::vector<int> trackXY(32000);
  uint n = 0;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    dbTechLayer* layer = *itr;
    dbTechLayerType type = layer->getType();

    if (type.getValue() != dbTechLayerType::ROUTING ||
        layer->getRoutingLevel() == 0)
      continue;

    n = layer->getRoutingLevel();
    // W[n]= layer->getWidth();
    W[n] = 1;
    S[n] = layer->getSpacing();
    P[n] = layer->getPitch();
    if (P[n] <= 0)
      error(0, "Layer %s, routing level %d, has pitch %d !!\n",
            layer->getConstName(), n, P[n]);
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
uint extSpef::addNetShapesOnSearch(uint netId) {
  dbNet* net = dbNet::getNet(_block, netId);

  dbWire* wire = net->getWire();
  uint wtype = 9;

  if (wire == NULL)
    return 0;

  uint cnt = 0;
  //	uint wireId= wire->getId();

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

    Rect r;
    s.getBox(r);

    _search->addBox(r.xMin(), r.yMin(), r.xMax(), r.yMax(), level1, netId,
                    shapeId, wtype);
    if (level2)
      _search->addBox(r.xMin(), r.yMin(), r.xMax(), r.yMax(), level2, netId,
                      shapeId, wtype);
    cnt++;
    minx = MIN(r.xMin(), minx);
    miny = MIN(r.yMin(), miny);
    maxx = MAX(r.xMax(), maxx);
    maxy = MAX(r.yMax(), maxy);
  }
  _search->setMaxArea(minx, miny, maxx, maxy);
  for (uint dir = 0; dir < _search->getRowCnt(); dir++) {
    for (uint layer = 1; layer < _search->getColCnt(); layer++) {
      _search->getGrid(dir, layer)->setSearchDomain(0);
    }
  }
  return cnt;
}
uint extSpef::findShapeId(uint netId, int x1, int y1, int x2, int y2,
                          uint level) {
  _idTable->resetCnt(0);

  if (level > 0) {
    for (uint dir = 0; dir < _search->getRowCnt(); dir++) {
      _search->search(x1, y1, x2, y2, dir, level, _idTable, true);
      if (_idTable->getCnt() > 0)
        break;
    }
  } else {
    for (uint dir = 0; dir < _search->getRowCnt(); dir++) {
      for (uint layer = 1; layer < _search->getColCnt(); layer++) {
        _search->search(x1, y1, x2, y2, dir, layer, _idTable, true);
        if (_idTable->getCnt() > 0)
          break;
      }
      if (_idTable->getCnt() > 0)
        break;
    }
  }
  if (_idTable->getCnt() <= 0) {
    return 0;
  }

  int loX, loY, hiX, hiY;
  uint l, id1, id2, wtype;

  uint id = _idTable->get(0);
  _search->getBox(id, &loX, &loY, &hiX, &hiY, &l, &id1, &id2, &wtype);

  if (netId != id1)
    return 0;

  return id2;
}

uint extSpef::findShapeId(uint netId, int x1, int y1, int x2, int y2,
                          char* layer, bool matchLayer) {
  _idTable->resetCnt(0);

  uint level = 0;
  if (layer != NULL) {
    dbTechLayer* dblayer = _tech->findLayer(layer);
    if (dblayer != NULL)
      level = dblayer->getRoutingLevel();
    else if (matchLayer)
      return 0;
  }
  return findShapeId(netId, x1, y1, x2, y2, level);
}

void extSpef::initNodeCoordTables(uint memChunk) {
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
void extSpef::resetNodeCoordTables() {
  _capNodeTable->resetCnt();
  _xCoordTable->resetCnt();
  _yCoordTable->resetCnt();
  _x1CoordTable->resetCnt();
  _x2CoordTable->resetCnt();
  _y1CoordTable->resetCnt();
  _y2CoordTable->resetCnt();
  _levelTable->resetCnt();
}
void extSpef::deleteNodeCoordTables() {
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
bool extSpef::readNodeCoords(uint cpos) {
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
  //	if (_NsLayer && _parser->getWordCnt() > cpos + 3)
  //		level = _tech->findLayer(_parser->get(cpos +
  // 3))->getRoutingLevel();
  _levelTable->add(level);
  return true;
}
void extSpef::adjustNodeCoords() {
  for (uint ii = 0; ii < _capNodeTable->getCnt(); ii++) {
    char buff[128];
    sprintf(buff, "*%d%s%d", _tmpNetSpefId, _delimiter, _capNodeTable->get(ii));
    uint capId = getCapIdFromCapTable(buff);
    _capNodeTable->set(ii, capId);
  }
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
uint extSpef::getITermShapeId(dbITerm* iterm) {
  if (iterm->getNet()->getWire() == NULL)
    return 0;
  return iterm->getNet()->getWire()->getTermJid(iterm->getId());
  /*
    uint shapeId = 0;
    dbMTerm *mterm = iterm->getMTerm();
    int px,py;
    iterm->getInst()->getOrigin(px,py);
    Point origin = adsPoint(px,py);
    dbOrientType orient = iterm->getInst()->getOrient();
    dbTransform transform( orient, origin );

    dbSet<dbMPin> mpins = mterm->getMPins();
    dbSet<dbMPin>::iterator mpin_itr;
    for (mpin_itr = mpins.begin(); mpin_itr != mpins.end(); mpin_itr++) {
      dbMPin *mpin = *mpin_itr;
      dbSet<dbBox> boxes = mpin->getGeometry();
      dbSet<dbBox>::iterator box_itr;
      int level, tlevel, blevel;
      for (box_itr = boxes.begin(); box_itr != boxes.end(); box_itr++) {
        dbBox *box = *box_itr;
        Rect rect;
        box->getBox( rect );
        transform.apply( rect );
        if (box->isVia()) {
          dbTechVia *tv = box->getTechVia();
          tlevel = tv->getTopLayer()->getRoutingLevel();
          shapeId = findShapeId(_d_net->getId(), rect.xMin(), rect.yMin(),
  rect.xMax(), rect.yMax(), tlevel); blevel =
  tv->getBottomLayer()->getRoutingLevel(); if (shapeId == 0) shapeId =
  findShapeId(_d_net->getId(), rect.xMin(), rect.yMin(), rect.xMax(),
  rect.yMax(), blevel);
  //        if (shapeId == 0)
  //          shapeId = findShapeId(_d_net->getId(), rect.xMin(), rect.yMin(),
  rect.xMax(), rect.yMax(), tlevel+1);
  //        if (shapeId == 0)
  //          shapeId = findShapeId(_d_net->getId(), rect.xMin(), rect.yMin(),
  rect.xMax(), rect.yMax(), blevel-1); } else { level =
  box->getTechLayer()->getRoutingLevel(); shapeId = findShapeId(_d_net->getId(),
  rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax(), level);
  //        if (shapeId == 0)
  //          shapeId = findShapeId(_d_net->getId(), rect.xMin(), rect.yMin(),
  rect.xMax(), rect.yMax(), level+1);
  //        if (shapeId == 0)
  //          shapeId = findShapeId(_d_net->getId(), rect.xMin(), rect.yMin(),
  rect.xMax(), rect.yMax(), level+2);
  //        if (shapeId == 0 && level > 1)
  //          shapeId = findShapeId(_d_net->getId(), rect.xMin(), rect.yMin(),
  rect.xMax(), rect.yMax(), level-1);
        }
        if (shapeId!= 0)
          return shapeId;
      }
    }
    // assert(shapeId);
    return shapeId;
  */
}
uint extSpef::getBTermShapeId(dbBTerm* bterm) {
  dbShape pin;
  if (!bterm->getFirstPin(pin))  // TWG: added bpins
    return 0;
  if (bterm->getNet()->getWire() == NULL)
    return 0;
  return bterm->getNet()->getWire()->getTermJid(-bterm->getId());
  /*
    Rect rect;
    pin.getBox(rect);
    uint shapeId = 0;
    int level;
    if (pin.isVia()) {
      // TODO
    } else {
      level = pin.getTechLayer()->getRoutingLevel();
      shapeId = findShapeId(_d_net->getId(), rect.xMin(), rect.yMin(),
  rect.xMax(), rect.yMax(), level);
  //    if (shapeId == 0)
  //        shapeId = findShapeId(_d_net->getId(), rect.xMin(), rect.yMin(),
  rect.xMax(), rect.yMax(), level+1);
  //    if (shapeId == 0)
  //        shapeId = findShapeId(_d_net->getId(), rect.xMin(), rect.yMin(),
  rect.xMax(), rect.yMax(), level+2);
  //    if (shapeId == 0 && level > 1)
  //        shapeId = findShapeId(_d_net->getId(), rect.xMin(), rect.yMin(),
  rect.xMax(), rect.yMax(), level-1);
      // assert(shapeId);
    }
    return shapeId;
  */
}
uint extSpef::getShapeIdFromNodeCoords(uint targetCapNodeId) {
  uint shapeId;
  int ii = findNodeIndexFromNodeCoords(targetCapNodeId);
  if (ii < 0)
    return 0;

  int halo = 20;
  int x1 = Ath__double2int(_nodeCoordFactor * _xCoordTable->get(ii));
  int y1 = Ath__double2int(_nodeCoordFactor * _yCoordTable->get(ii));
  uint level = _levelTable->get(ii);
  x1 -= halo;
  y1 -= halo;
  int x2 = x1 + halo;
  int y2 = y1 + halo;

  shapeId = findShapeId(_d_net->getId(), x1, y1, x2, y2, level);
  return shapeId;
}

/*
*RES
1 *1:1 *1:2 7.3792 // x=[782.74,791.7] y=[376.67,376.81] dx=8.96 dy=0.14
lyr=METAL3 2 *1:5 *1:6 23.1819 // x=[740.11,740.25] y=[340.83,358.11] dx=0.14
dy=17.28 lyr=METAL4 3 *1:7 *1:8 24.5655 // x=[779.31,779.45] y=[357.85,376.81]
dx=0.14 dy=18.96 lyr=METAL4
*/
uint extSpef::parseAndFindShapeId() {
  if (_parser->getWordCnt() < 10)
    return 0;

  _nodeCoordParser->mkWords(_parser->get(5));

  int x1 = Ath__double2int(_nodeCoordFactor * _nodeCoordParser->getDouble(1));
  int x2 = Ath__double2int(_nodeCoordFactor * _nodeCoordParser->getDouble(2));

  _nodeCoordParser->mkWords(_parser->get(6));

  int y1 = Ath__double2int(_nodeCoordFactor * _nodeCoordParser->getDouble(1));
  int y2 = Ath__double2int(_nodeCoordFactor * _nodeCoordParser->getDouble(2));

  _nodeCoordParser->mkWords(_parser->get(9));

  char* layer = NULL;
  if (strcmp(_nodeCoordParser->get(0), "lyr") == 0)
    layer = _nodeCoordParser->get(1);
  // bool matchLayer= true;
  bool matchLayer = false;

  uint shapeId =
      findShapeId(_d_net->getId(), x1, y1, x2, y2, layer, matchLayer);
  return shapeId;
}
void extSpef::readNmCoords() {
  if (_parser->getWordCnt() < 10)
    return;

  _nodeCoordParser->mkWords(_parser->get(5));

  int x1 = Ath__double2int(_nodeCoordFactor * _nodeCoordParser->getDouble(1));
  int x2 = Ath__double2int(_nodeCoordFactor * _nodeCoordParser->getDouble(2));

  _nodeCoordParser->mkWords(_parser->get(6));

  int y1 = Ath__double2int(_nodeCoordFactor * _nodeCoordParser->getDouble(1));
  int y2 = Ath__double2int(_nodeCoordFactor * _nodeCoordParser->getDouble(2));

  _x1CoordTable->add(x1);
  _y1CoordTable->add(y1);
  _x2CoordTable->add(x2);
  _y2CoordTable->add(y2);
}
void extSpef::searchDealloc() { _search->dealloc(); }

}  // namespace rcx

void Ath__grid::dealloc() {
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
void Ath__gridTable::dealloc() {
  for (uint dir = 0; dir < _rowCnt; dir++) {
    for (uint jj = 1; jj < _colCnt; jj++) {
      Ath__grid* netGrid = _gridTable[dir][jj];
      if (netGrid == NULL)
        continue;

      netGrid->dealloc();
    }
  }
}
