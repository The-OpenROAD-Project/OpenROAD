// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipRegionInstItr.h"

#include "dbChip.h"
#include "dbChipInst.h"
#include "dbChipRegionInst.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipRegionInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipRegionInstItr::reversible() const
{
  return true;
}

bool dbChipRegionInstItr::orderReversed() const
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
    _dbChipRegionInst* regioninst = chip_region_inst_tbl_->getPtr(id);
    uint n = regioninst->chip_region_inst_next_;
    regioninst->chip_region_inst_next_ = list;
    list = id;
    id = n;
  }
  chipinst->chip_region_insts_ = list;
  // User Code End reverse
}

uint dbChipRegionInstItr::sequential() const
{
  return 0;
}

uint dbChipRegionInstItr::size(dbObject* parent) const
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

uint dbChipRegionInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChipInst* chipinst = (_dbChipInst*) parent;
  return chipinst->chip_region_insts_;
  // User Code End begin
}

uint dbChipRegionInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbChipRegionInstItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbChipRegionInst* regioninst = chip_region_inst_tbl_->getPtr(id);
  return regioninst->chip_region_inst_next_;
  // User Code End next
}

dbObject* dbChipRegionInstItr::getObject(uint id, ...)
{
  return chip_region_inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
