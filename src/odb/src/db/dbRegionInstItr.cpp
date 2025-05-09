// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbRegionInstItr.h"

#include "dbBlock.h"
#include "dbInst.h"
#include "dbRegion.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////
//
// dbRegionInstItr - Methods
//
////////////////////////////////////////////////

bool dbRegionInstItr::reversible()
{
  return true;
}

bool dbRegionInstItr::orderReversed()
{
  return true;
}

void dbRegionInstItr::reverse(dbObject* parent)
{
  _dbRegion* region = (_dbRegion*) parent;
  uint id = region->_insts;
  uint list = 0;

  while (id != 0) {
    _dbInst* inst = _inst_tbl->getPtr(id);
    uint n = inst->_region_next;
    inst->_region_next = list;
    list = id;
    id = n;
  }

  uint prev = 0;
  id = list;

  while (id != 0) {
    _dbInst* inst = _inst_tbl->getPtr(id);
    inst->_region_prev = prev;
    prev = id;
    id = inst->_region_next;
  }

  region->_insts = list;
}

uint dbRegionInstItr::sequential()
{
  return 0;
}

uint dbRegionInstItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbRegionInstItr::begin(parent); id != dbRegionInstItr::end(parent);
       id = dbRegionInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbRegionInstItr::begin(dbObject* parent)
{
  _dbRegion* region = (_dbRegion*) parent;
  return (uint) region->_insts;
}

uint dbRegionInstItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbRegionInstItr::next(uint id, ...)
{
  _dbInst* inst = _inst_tbl->getPtr(id);
  return inst->_region_next;
}

dbObject* dbRegionInstItr::getObject(uint id, ...)
{
  return _inst_tbl->getPtr(id);
}

}  // namespace odb
