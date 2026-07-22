// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbRegionInstItr.h"

#include <cstdint>

#include "dbBlock.h"
#include "dbInst.h"
#include "dbRegion.h"
#include "dbTable.h"
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
  uint32_t id = region->insts_;
  uint32_t list = 0;

  while (id != 0) {
    _dbInst* inst = inst_tbl_->getPtr(id);
    uint32_t n = inst->region_next_;
    inst->region_next_ = list;
    list = id;
    id = n;
  }

  uint32_t prev = 0;
  id = list;

  while (id != 0) {
    _dbInst* inst = inst_tbl_->getPtr(id);
    inst->region_prev_ = prev;
    prev = id;
    id = inst->region_next_;
  }

  region->insts_ = list;
}

uint32_t dbRegionInstItr::sequential() const
{
  return 0;
}

uint32_t dbRegionInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbRegionInstItr::begin(parent); id != dbRegionInstItr::end(parent);
       id = dbRegionInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbRegionInstItr::begin(dbObject* parent) const
{
  _dbRegion* region = (_dbRegion*) parent;
  return (uint32_t) region->insts_;
}

uint32_t dbRegionInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbRegionInstItr::next(uint32_t id, ...) const
{
  _dbInst* inst = inst_tbl_->getPtr(id);
  return inst->region_next_;
}

dbObject* dbRegionInstItr::getObject(uint32_t id, ...)
{
  return inst_tbl_->getPtr(id);
}

}  // namespace odb
