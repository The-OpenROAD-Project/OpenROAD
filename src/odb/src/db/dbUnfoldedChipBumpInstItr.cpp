// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedChipBumpInstItr.h"

#include <cstdint>

#include "dbTable.h"
#include "dbUnfoldedChipBumpInst.h"
#include "dbUnfoldedChipRegionInst.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedChipBumpInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbUnfoldedChipBumpInstItr::reversible() const
{
  return true;
}

bool dbUnfoldedChipBumpInstItr::orderReversed() const
{
  return true;
}

void dbUnfoldedChipBumpInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbUnfoldedChipRegionInst* region = (_dbUnfoldedChipRegionInst*) parent;
  uint32_t id = region->bump_;
  uint32_t list = 0;
  while (id != 0) {
    _dbUnfoldedChipBumpInst* bump = unfolded_chip_bump_inst_tbl_->getPtr(id);
    uint32_t n = bump->region_next_;
    bump->region_next_ = list;
    list = id;
    id = n;
  }
  region->bump_ = list;
  // User Code End reverse
}

uint32_t dbUnfoldedChipBumpInstItr::sequential() const
{
  return 0;
}

uint32_t dbUnfoldedChipBumpInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbUnfoldedChipBumpInstItr::begin(parent);
       id != dbUnfoldedChipBumpInstItr::end(parent);
       id = dbUnfoldedChipBumpInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbUnfoldedChipBumpInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  return ((_dbUnfoldedChipRegionInst*) parent)->bump_;
  // User Code End begin
}

uint32_t dbUnfoldedChipBumpInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbUnfoldedChipBumpInstItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  return unfolded_chip_bump_inst_tbl_->getPtr(id)->region_next_;
  // User Code End next
}

dbObject* dbUnfoldedChipBumpInstItr::getObject(uint32_t id, ...)
{
  return unfolded_chip_bump_inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp