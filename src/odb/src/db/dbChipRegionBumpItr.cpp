// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipRegionBumpItr.h"

#include <cstdint>

#include "dbChipRegion.h"
#include "dbInst.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipRegionBumpItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipRegionBumpItr::reversible() const
{
  return true;
}

bool dbChipRegionBumpItr::orderReversed() const
{
  return true;
}

void dbChipRegionBumpItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbChipRegion* region = (_dbChipRegion*) parent;
  uint32_t id = region->bumps_;
  uint32_t list = 0;

  while (id != 0) {
    _dbInst* inst = inst_tbl_->getPtr(id);
    uint32_t n = inst->chip_region_next_;
    inst->chip_region_next_ = list;
    list = id;
    id = n;
  }
  region->bumps_ = list;
  // User Code End reverse
}

uint32_t dbChipRegionBumpItr::sequential() const
{
  return 0;
}

uint32_t dbChipRegionBumpItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbChipRegionBumpItr::begin(parent);
       id != dbChipRegionBumpItr::end(parent);
       id = dbChipRegionBumpItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbChipRegionBumpItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChipRegion* region = (_dbChipRegion*) parent;
  return region->bumps_;
  // User Code End begin
}

uint32_t dbChipRegionBumpItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbChipRegionBumpItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbInst* inst = inst_tbl_->getPtr(id);
  return inst->chip_region_next_;
  // User Code End next
}

dbObject* dbChipRegionBumpItr::getObject(uint32_t id, ...)
{
  return inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp