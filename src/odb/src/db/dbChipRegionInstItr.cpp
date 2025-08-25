// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipRegionInstItr.h"

#include "dbChip.h"
#include "dbChipInst.h"
#include "dbChipRegionInst.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipRegionInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipRegionInstItr::reversible()
{
  return true;
}

bool dbChipRegionInstItr::orderReversed()
{
  return true;
}

void dbChipRegionInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbChipInst* chipinst = (_dbChipInst*) parent;
  uint id = chipinst->chip_region_insts_;
  uint list = 0;

  while (id != 0) {
    _dbChipRegionInst* regioninst = _chip_region_inst_tbl->getPtr(id);
    uint n = regioninst->chip_region_inst_next_;
    regioninst->chip_region_inst_next_ = list;
    list = id;
    id = n;
  }
  chipinst->chip_region_insts_ = list;
  // User Code End reverse
}

uint dbChipRegionInstItr::sequential()
{
  return 0;
}

uint dbChipRegionInstItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbChipRegionInstItr::begin(parent);
       id != dbChipRegionInstItr::end(parent);
       id = dbChipRegionInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbChipRegionInstItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbChipInst* chipinst = (_dbChipInst*) parent;
  return chipinst->chip_region_insts_;
  // User Code End begin
}

uint dbChipRegionInstItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbChipRegionInstItr::next(uint id, ...)
{
  // User Code Begin next
  _dbChipRegionInst* regioninst = _chip_region_inst_tbl->getPtr(id);
  return regioninst->chip_region_inst_next_;
  // User Code End next
}

dbObject* dbChipRegionInstItr::getObject(uint id, ...)
{
  return _chip_region_inst_tbl->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp