// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbMPinItr.h"

#include "dbMPin.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbMPinItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbMPinItr::reversible()
{
  return true;
}

bool dbMPinItr::orderReversed()
{
  return true;
}

void dbMPinItr::reverse(dbObject* parent)
{
  _dbMTerm* mterm = (_dbMTerm*) parent;
  uint id = mterm->_pins;
  uint list = 0;

  while (id != 0) {
    _dbMPin* pin = _mpin_tbl->getPtr(id);
    uint n = pin->_next_mpin;
    pin->_next_mpin = list;
    list = id;
    id = n;
  }

  mterm->_pins = list;
}

uint dbMPinItr::sequential()
{
  return 0;
}

uint dbMPinItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbMPinItr::begin(parent); id != dbMPinItr::end(parent);
       id = dbMPinItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbMPinItr::begin(dbObject* parent)
{
  _dbMTerm* mterm = (_dbMTerm*) parent;
  return mterm->_pins;
}

uint dbMPinItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbMPinItr::next(uint id, ...)
{
  _dbMPin* mpin = _mpin_tbl->getPtr(id);
  return mpin->_next_mpin;
}

dbObject* dbMPinItr::getObject(uint id, ...)
{
  return _mpin_tbl->getPtr(id);
}

}  // namespace odb
