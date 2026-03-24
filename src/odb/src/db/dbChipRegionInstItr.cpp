// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipRegionInstItr.h"

#include <cstdint>

#include "dbChip.h"
#include "dbChipInst.h"
#include "dbChipRegionInst.h"
#include "dbTable.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

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
  uint32_t id = chipinst->chip_region_insts_;
  uint32_t list = 0;

  while (id != 0) {
    _dbChipRegionInst* regioninst = chip_region_inst_tbl_->getPtr(id);
    uint32_t n = regioninst->chip_region_inst_next_;
    regioninst->chip_region_inst_next_ = list;
    list = id;
    id = n;
  }
  chipinst->chip_region_insts_ = list;
  // User Code End reverse
}

uint32_t dbChipRegionInstItr::sequential() const
{
  return 0;
}

uint32_t dbChipRegionInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbChipRegionInstItr::begin(parent);
       id != dbChipRegionInstItr::end(parent);
       id = dbChipRegionInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbChipRegionInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChipInst* chipinst = (_dbChipInst*) parent;
  return chipinst->chip_region_insts_;
  // User Code End begin
}

uint32_t dbChipRegionInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbChipRegionInstItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbChipRegionInst* regioninst = chip_region_inst_tbl_->getPtr(id);
  return regioninst->chip_region_inst_next_;
  // User Code End next
}

dbObject* dbChipRegionInstItr::getObject(uint32_t id, ...)
{
  return chip_region_inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
