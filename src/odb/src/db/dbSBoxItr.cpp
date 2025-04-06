// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbSBoxItr.h"

#include "dbBlock.h"
#include "dbSBox.h"
#include "dbSWire.h"
#include "dbTable.h"

namespace odb {

bool dbSBoxItr::reversible()
{
  return true;
}

bool dbSBoxItr::orderReversed()
{
  return true;
}

void dbSBoxItr::reverse(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbSWireObj: {
      _dbSWire* wire = (_dbSWire*) parent;
      uint id = wire->_wires;
      uint list = 0;

      while (id != 0) {
        _dbSBox* b = _box_tbl->getPtr(id);
        uint n = b->_next_box;
        b->_next_box = list;
        list = id;
        id = n;
      }

      wire->_wires = list;
      break;
    }

    default:
      break;
  }
}

uint dbSBoxItr::sequential()
{
  return 0;
}

uint dbSBoxItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbSBoxItr::begin(parent); id != dbSBoxItr::end(parent);
       id = dbSBoxItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbSBoxItr::begin(dbObject* parent)
{
  switch (parent->getImpl()->getType()) {
    case dbSWireObj: {
      _dbSWire* wire = (_dbSWire*) parent;
      return wire->_wires;
    }

    default:
      break;
  }

  return 0;
}

uint dbSBoxItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbSBoxItr::next(uint id, ...)
{
  _dbSBox* box = _box_tbl->getPtr(id);
  return box->_next_box;
}

dbObject* dbSBoxItr::getObject(uint id, ...)
{
  return _box_tbl->getPtr(id);
}

}  // namespace odb
