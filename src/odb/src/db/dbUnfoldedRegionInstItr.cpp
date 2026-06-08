// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedRegionInstItr.h"

#include <cstdint>

#include "dbTable.h"
#include "dbUnfoldedChipInst.h"
#include "dbUnfoldedRegionInst.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedRegionInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbUnfoldedRegionInstItr::reversible() const
{
  return true;
}

bool dbUnfoldedRegionInstItr::orderReversed() const
{
  return true;
}

void dbUnfoldedRegionInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbUnfoldedChipInst* chip = (_dbUnfoldedChipInst*) parent;
  uint32_t id = chip->region_;
  uint32_t list = 0;
  while (id != 0) {
    _dbUnfoldedRegionInst* region = unfolded_region_inst_tbl_->getPtr(id);
    uint32_t n = region->chip_next_;
    region->chip_next_ = list;
    list = id;
    id = n;
  }
  chip->region_ = list;
  // User Code End reverse
}

uint32_t dbUnfoldedRegionInstItr::sequential() const
{
  return 0;
}

uint32_t dbUnfoldedRegionInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbUnfoldedRegionInstItr::begin(parent);
       id != dbUnfoldedRegionInstItr::end(parent);
       id = dbUnfoldedRegionInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbUnfoldedRegionInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  return ((_dbUnfoldedChipInst*) parent)->region_;
  // User Code End begin
}

uint32_t dbUnfoldedRegionInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbUnfoldedRegionInstItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  return unfolded_region_inst_tbl_->getPtr(id)->chip_next_;
  // User Code End next
}

dbObject* dbUnfoldedRegionInstItr::getObject(uint32_t id, ...)
{
  return unfolded_region_inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp