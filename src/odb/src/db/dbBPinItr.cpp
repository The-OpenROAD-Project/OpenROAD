// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBPinItr.h"

#include "dbBPin.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbBPinItr - Methods
//
////////////////////////////////////////////////////////////////////

//
// BPins are ordered by io-type and cannot be reversed.
//
bool dbBPinItr::reversible()
{
  return true;
}

bool dbBPinItr::orderReversed()
{
  return true;
}

void dbBPinItr::reverse(dbObject* parent)
{
  _dbBTerm* bterm = (_dbBTerm*) parent;
  uint id = bterm->_bpins;
  uint list = 0;

  while (id != 0) {
    _dbBPin* bpin = _bpin_tbl->getPtr(id);
    uint n = bpin->_next_bpin;
    bpin->_next_bpin = list;
    list = id;
    id = n;
  }

  bterm->_bpins = list;
}

uint dbBPinItr::sequential()
{
  return 0;
}

uint dbBPinItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbBPinItr::begin(parent); id != dbBPinItr::end(parent);
       id = dbBPinItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbBPinItr::begin(dbObject* parent)
{
  _dbBTerm* bterm = (_dbBTerm*) parent;
  return bterm->_bpins;
}

uint dbBPinItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbBPinItr::next(uint id, ...)
{
  _dbBPin* bpin = _bpin_tbl->getPtr(id);
  return bpin->_next_bpin;
}

dbObject* dbBPinItr::getObject(uint id, ...)
{
  return _bpin_tbl->getPtr(id);
}

}  // namespace odb
