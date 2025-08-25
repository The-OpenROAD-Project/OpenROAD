// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipBumpInstItr.h"

#include "dbChipBumpInst.h"
#include "dbChipRegionInst.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipBumpInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipBumpInstItr::reversible()
{
  return true;
}

bool dbChipBumpInstItr::orderReversed()
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
    _dbChipBumpInst* chip_bump_inst = _chip_bump_inst_tbl->getPtr(id);
    uint n = chip_bump_inst->region_next_;
    chip_bump_inst->region_next_ = list;
    list = id;
    id = n;
  }
  chip_region_inst->chip_bump_insts_ = list;
  // User Code End reverse
}

uint dbChipBumpInstItr::sequential()
{
  return 0;
}

uint dbChipBumpInstItr::size(dbObject* parent)
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

uint dbChipBumpInstItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbChipRegionInst* chip_region_inst = (_dbChipRegionInst*) parent;
  return chip_region_inst->chip_bump_insts_;
  // User Code End begin
}

uint dbChipBumpInstItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbChipBumpInstItr::next(uint id, ...)
{
  // User Code Begin next
  _dbChipBumpInst* chip_bump_inst = _chip_bump_inst_tbl->getPtr(id);
  return chip_bump_inst->region_next_;
  // User Code End next
}

dbObject* dbChipBumpInstItr::getObject(uint id, ...)
{
  return _chip_bump_inst_tbl->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp