// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbMPinItr.h"

#include <cstdint>

#include "dbMPin.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
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
  uint32_t id = mterm->pins_;
  uint32_t list = 0;

  while (id != 0) {
    _dbMPin* pin = _mpin_tbl->getPtr(id);
    uint32_t n = pin->next_mpin_;
    pin->next_mpin_ = list;
    list = id;
    id = n;
  }

  mterm->pins_ = list;
}

uint32_t dbMPinItr::sequential() const
{
  return 0;
}

uint32_t dbMPinItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbMPinItr::begin(parent); id != dbMPinItr::end(parent);
       id = dbMPinItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbMPinItr::begin(dbObject* parent) const
{
  _dbMTerm* mterm = (_dbMTerm*) parent;
  return mterm->pins_;
}

uint32_t dbMPinItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbMPinItr::next(uint32_t id, ...) const
{
  _dbMPin* mpin = _mpin_tbl->getPtr(id);
  return mpin->next_mpin_;
}

dbObject* dbMPinItr::getObject(uint32_t id, ...)
{
  return _mpin_tbl->getPtr(id);
}

}  // namespace odb
