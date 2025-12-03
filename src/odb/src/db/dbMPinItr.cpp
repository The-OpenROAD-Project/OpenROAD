// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbMPinItr.h"

#include "dbMPin.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/dbObject.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbMPinItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbMPinItr::reversible() const
{
  return true;
}

bool dbMPinItr::orderReversed() const
{
  return true;
}

void dbMPinItr::reverse(dbObject* parent)
{
  _dbMTerm* mterm = (_dbMTerm*) parent;
  uint id = mterm->pins_;
  uint list = 0;

  while (id != 0) {
    _dbMPin* pin = _mpin_tbl->getPtr(id);
    uint n = pin->next_mpin_;
    pin->next_mpin_ = list;
    list = id;
    id = n;
  }

  mterm->pins_ = list;
}

uint dbMPinItr::sequential() const
{
  return 0;
}

uint dbMPinItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbMPinItr::begin(parent); id != dbMPinItr::end(parent);
       id = dbMPinItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbMPinItr::begin(dbObject* parent) const
{
  _dbMTerm* mterm = (_dbMTerm*) parent;
  return mterm->pins_;
}

uint dbMPinItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbMPinItr::next(uint id, ...) const
{
  _dbMPin* mpin = _mpin_tbl->getPtr(id);
  return mpin->next_mpin_;
}

dbObject* dbMPinItr::getObject(uint id, ...)
{
  return _mpin_tbl->getPtr(id);
}

}  // namespace odb
