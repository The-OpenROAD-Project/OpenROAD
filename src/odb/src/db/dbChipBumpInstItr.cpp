// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipBumpInstItr.h"

#include "dbChipBumpInst.h"
#include "dbChipRegionInst.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

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
  uint id = chip_region_inst->chip_bump_insts_;
  uint list = 0;

  while (id != 0) {
    _dbChipBumpInst* chip_bump_inst = chip_bump_inst_tbl_->getPtr(id);
    uint n = chip_bump_inst->region_next_;
    chip_bump_inst->region_next_ = list;
    list = id;
    id = n;
  }
  chip_region_inst->chip_bump_insts_ = list;
  // User Code End reverse
}

uint dbChipBumpInstItr::sequential() const
{
  return 0;
}

uint dbChipBumpInstItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbChipBumpInstItr::begin(parent);
       id != dbChipBumpInstItr::end(parent);
       id = dbChipBumpInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbChipBumpInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChipRegionInst* chip_region_inst = (_dbChipRegionInst*) parent;
  return chip_region_inst->chip_bump_insts_;
  // User Code End begin
}

uint dbChipBumpInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbChipBumpInstItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbChipBumpInst* chip_bump_inst = chip_bump_inst_tbl_->getPtr(id);
  return chip_bump_inst->region_next_;
  // User Code End next
}

dbObject* dbChipBumpInstItr::getObject(uint id, ...)
{
  return chip_bump_inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
