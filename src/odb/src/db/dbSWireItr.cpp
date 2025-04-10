// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbSWireItr.h"

#include "dbBlock.h"
#include "dbNet.h"
#include "dbSWire.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbSWireItr - Methods
//
////////////////////////////////////////////////////////////////////

//
// BTerms are ordered by io-type and cannot be reversed.
//
bool dbSWireItr::reversible()
{
  return true;
}

bool dbSWireItr::orderReversed()
{
  return true;
}

void dbSWireItr::reverse(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  uint id = net->_swires;
  uint list = 0;

  while (id != 0) {
    _dbSWire* swire = _swire_tbl->getPtr(id);
    uint n = swire->_next_swire;
    swire->_next_swire = list;
    list = id;
    id = n;
  }

  net->_swires = list;
}

uint dbSWireItr::sequential()
{
  return 0;
}

uint dbSWireItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbSWireItr::begin(parent); id != dbSWireItr::end(parent);
       id = dbSWireItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbSWireItr::begin(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  return net->_swires;
}

uint dbSWireItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbSWireItr::next(uint id, ...)
{
  _dbSWire* swire = _swire_tbl->getPtr(id);
  return swire->_next_swire;
}

dbObject* dbSWireItr::getObject(uint id, ...)
{
  return _swire_tbl->getPtr(id);
}

}  // namespace odb
