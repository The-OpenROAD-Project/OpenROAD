// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbRegionInstItr.h"

#include "dbBlock.h"
#include "dbInst.h"
#include "dbRegion.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/dbObject.h"

namespace odb {

////////////////////////////////////////////////
//
// dbRegionInstItr - Methods
//
////////////////////////////////////////////////

bool dbRegionInstItr::reversible() const
{
  return true;
}

bool dbRegionInstItr::orderReversed() const
{
  return true;
}

void dbRegionInstItr::reverse(dbObject* parent)
{
  _dbRegion* region = (_dbRegion*) parent;
  uint id = region->insts_;
  uint list = 0;

  while (id != 0) {
    _dbInst* inst = _inst_tbl->getPtr(id);
    uint n = inst->region_next_;
    inst->region_next_ = list;
    list = id;
    id = n;
  }

  uint prev = 0;
  id = list;

  while (id != 0) {
    _dbInst* inst = _inst_tbl->getPtr(id);
    inst->region_prev_ = prev;
    prev = id;
    id = inst->region_next_;
  }

  region->insts_ = list;
}

uint dbRegionInstItr::sequential() const
{
  return 0;
}

uint dbRegionInstItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbRegionInstItr::begin(parent); id != dbRegionInstItr::end(parent);
       id = dbRegionInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbRegionInstItr::begin(dbObject* parent) const
{
  _dbRegion* region = (_dbRegion*) parent;
  return (uint) region->insts_;
}

uint dbRegionInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbRegionInstItr::next(uint id, ...) const
{
  _dbInst* inst = _inst_tbl->getPtr(id);
  return inst->region_next_;
}

dbObject* dbRegionInstItr::getObject(uint id, ...)
{
  return _inst_tbl->getPtr(id);
}

}  // namespace odb
