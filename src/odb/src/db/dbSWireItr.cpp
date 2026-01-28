// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbSWireItr.h"

#include <cstdint>

#include "dbBlock.h"
#include "dbNet.h"
#include "dbSWire.h"
#include "dbTable.h"
#include "odb/dbObject.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbSWireItr - Methods
//
////////////////////////////////////////////////////////////////////

//
// BTerms are ordered by io-type and cannot be reversed.
//
bool dbSWireItr::reversible() const
{
  return true;
}

bool dbSWireItr::orderReversed() const
{
  return true;
}

void dbSWireItr::reverse(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  uint32_t id = net->swires_;
  uint32_t list = 0;

  while (id != 0) {
    _dbSWire* swire = swire_tbl_->getPtr(id);
    uint32_t n = swire->next_swire_;
    swire->next_swire_ = list;
    list = id;
    id = n;
  }

  net->swires_ = list;
}

uint32_t dbSWireItr::sequential() const
{
  return 0;
}

uint32_t dbSWireItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbSWireItr::begin(parent); id != dbSWireItr::end(parent);
       id = dbSWireItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbSWireItr::begin(dbObject* parent) const
{
  _dbNet* net = (_dbNet*) parent;
  return net->swires_;
}

uint32_t dbSWireItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbSWireItr::next(uint32_t id, ...) const
{
  _dbSWire* swire = swire_tbl_->getPtr(id);
  return swire->next_swire_;
}

dbObject* dbSWireItr::getObject(uint32_t id, ...)
{
  return swire_tbl_->getPtr(id);
}

}  // namespace odb
