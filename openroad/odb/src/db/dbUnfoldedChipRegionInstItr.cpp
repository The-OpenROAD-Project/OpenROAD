// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedChipRegionInstItr.h"

#include <cstdint>

#include "dbTable.h"
#include "dbUnfoldedChipInst.h"
#include "dbUnfoldedChipRegionInst.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedChipRegionInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbUnfoldedChipRegionInstItr::reversible() const
{
  return true;
}

bool dbUnfoldedChipRegionInstItr::orderReversed() const
{
  return true;
}

void dbUnfoldedChipRegionInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbUnfoldedChipInst* chip = (_dbUnfoldedChipInst*) parent;
  uint32_t id = chip->region_;
  uint32_t list = 0;
  while (id != 0) {
    _dbUnfoldedChipRegionInst* region
        = unfolded_chip_region_inst_tbl_->getPtr(id);
    uint32_t n = region->chip_next_;
    region->chip_next_ = list;
    list = id;
    id = n;
  }
  chip->region_ = list;
  // User Code End reverse
}

uint32_t dbUnfoldedChipRegionInstItr::sequential() const
{
  return 0;
}

uint32_t dbUnfoldedChipRegionInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbUnfoldedChipRegionInstItr::begin(parent);
       id != dbUnfoldedChipRegionInstItr::end(parent);
       id = dbUnfoldedChipRegionInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbUnfoldedChipRegionInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  return ((_dbUnfoldedChipInst*) parent)->region_;
  // User Code End begin
}

uint32_t dbUnfoldedChipRegionInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbUnfoldedChipRegionInstItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  return unfolded_chip_region_inst_tbl_->getPtr(id)->chip_next_;
  // User Code End next
}

dbObject* dbUnfoldedChipRegionInstItr::getObject(uint32_t id, ...)
{
  return unfolded_chip_region_inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp