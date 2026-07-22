// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBPinItr.h"

#include <cstdint>

#include "dbBPin.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbTable.h"
#include "odb/dbObject.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbBPinItr - Methods
//
////////////////////////////////////////////////////////////////////

//
// BPins are ordered by io-type and cannot be reversed.
//
bool dbBPinItr::reversible() const
{
  return true;
}

bool dbBPinItr::orderReversed() const
{
  return true;
}

void dbBPinItr::reverse(dbObject* parent)
{
  _dbBTerm* bterm = (_dbBTerm*) parent;
  uint32_t id = bterm->bpins_;
  uint32_t list = 0;

  while (id != 0) {
    _dbBPin* bpin = bpin_tbl_->getPtr(id);
    uint32_t n = bpin->next_bpin_;
    bpin->next_bpin_ = list;
    list = id;
    id = n;
  }

  bterm->bpins_ = list;
}

uint32_t dbBPinItr::sequential() const
{
  return 0;
}

uint32_t dbBPinItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbBPinItr::begin(parent); id != dbBPinItr::end(parent);
       id = dbBPinItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbBPinItr::begin(dbObject* parent) const
{
  _dbBTerm* bterm = (_dbBTerm*) parent;
  return bterm->bpins_;
}

uint32_t dbBPinItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbBPinItr::next(uint32_t id, ...) const
{
  _dbBPin* bpin = bpin_tbl_->getPtr(id);
  return bpin->next_bpin_;
}

dbObject* dbBPinItr::getObject(uint32_t id, ...)
{
  return bpin_tbl_->getPtr(id);
}

}  // namespace odb
