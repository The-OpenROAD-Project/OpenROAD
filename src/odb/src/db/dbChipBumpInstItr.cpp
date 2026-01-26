// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipBumpInstItr.h"

#include <cstdint>

#include "dbChipBumpInst.h"
#include "dbChipRegionInst.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipBumpInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipBumpInstItr::reversible() const
{
  return true;
}

bool dbChipBumpInstItr::orderReversed() const
{
  return true;
}

void dbChipBumpInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbChipRegionInst* chip_region_inst = (_dbChipRegionInst*) parent;
  uint32_t id = chip_region_inst->chip_bump_insts_;
  uint32_t list = 0;

  while (id != 0) {
    _dbChipBumpInst* chip_bump_inst = chip_bump_inst_tbl_->getPtr(id);
    uint32_t n = chip_bump_inst->region_next_;
    chip_bump_inst->region_next_ = list;
    list = id;
    id = n;
  }
  chip_region_inst->chip_bump_insts_ = list;
  // User Code End reverse
}

uint32_t dbChipBumpInstItr::sequential() const
{
  return 0;
}

uint32_t dbChipBumpInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbChipBumpInstItr::begin(parent);
       id != dbChipBumpInstItr::end(parent);
       id = dbChipBumpInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbChipBumpInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChipRegionInst* chip_region_inst = (_dbChipRegionInst*) parent;
  return chip_region_inst->chip_bump_insts_;
  // User Code End begin
}

uint32_t dbChipBumpInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbChipBumpInstItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbChipBumpInst* chip_bump_inst = chip_bump_inst_tbl_->getPtr(id);
  return chip_bump_inst->region_next_;
  // User Code End next
}

dbObject* dbChipBumpInstItr::getObject(uint32_t id, ...)
{
  return chip_bump_inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
