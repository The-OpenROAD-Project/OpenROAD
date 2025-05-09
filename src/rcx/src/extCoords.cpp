// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "grids.h"
#include "parse.h"
#include "rcx/extSpef.h"
#include "utl/Logger.h"

namespace rcx {

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
  delete _capNodeTable;
  _capNodeTable = nullptr;
  delete _xCoordTable;
  _xCoordTable = nullptr;
  delete _yCoordTable;
  _yCoordTable = nullptr;
  delete _x1CoordTable;
  _x1CoordTable = nullptr;
  delete _y1CoordTable;
  _y1CoordTable = nullptr;
  delete _x2CoordTable;
  _x2CoordTable = nullptr;
  delete _y2CoordTable;
  _y2CoordTable = nullptr;
  delete _levelTable;
  _levelTable = nullptr;
  delete _idTable;
  _idTable = nullptr;
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
  if (cpos + 3 > wCnt) {
    return false;
  }

  uint id1;
  uint tokenCnt = _nodeParser->mkWords(_parser->get(1));
  if (tokenCnt == 2 && _nodeParser->isDigit(1, 0)) {  // internal node
    id1 = _nodeParser->getInt(0, 1);
    if (id1 != _tmpNetSpefId) {
      return false;
    }
  }
  uint netId = 0;
  uint nodeId = getCapNodeId(_parser->get(1), nullptr, &netId);
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
    if (capId == targetCapNodeId) {
      break;
    }
  }
  if (ii == _capNodeTable->getCnt()) {
    return -1;
  }

  return ii;
}

}  // namespace rcx

namespace rcx {

void Grid::dealloc()
{
  for (uint ii = 0; ii <= _searchHiTrack; ii++) {
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
}

void GridTable::dealloc()
{
  for (uint dir = 0; dir < _rowCnt; dir++) {
    for (uint jj = 1; jj < _colCnt; jj++) {
      Grid* netGrid = _gridTable[dir][jj];
      if (netGrid == nullptr) {
        continue;
      }

      netGrid->dealloc();
    }
  }
}

}  // namespace rcx
