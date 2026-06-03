// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedRegionItr.h"

#include <cstdint>

#include "dbTable.h"
#include "dbUnfoldedChip.h"
#include "dbUnfoldedRegion.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedRegionItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbUnfoldedRegionItr::reversible() const
{
  return true;
}

bool dbUnfoldedRegionItr::orderReversed() const
{
  return true;
}

void dbUnfoldedRegionItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbUnfoldedChip* chip = (_dbUnfoldedChip*) parent;
  uint32_t id = chip->region_;
  uint32_t list = 0;
  while (id != 0) {
    _dbUnfoldedRegion* region = unfolded_region_tbl_->getPtr(id);
    uint32_t n = region->chip_next_;
    region->chip_next_ = list;
    list = id;
    id = n;
  }
  chip->region_ = list;
  // User Code End reverse
}

uint32_t dbUnfoldedRegionItr::sequential() const
{
  return 0;
}

uint32_t dbUnfoldedRegionItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbUnfoldedRegionItr::begin(parent);
       id != dbUnfoldedRegionItr::end(parent);
       id = dbUnfoldedRegionItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbUnfoldedRegionItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  return ((_dbUnfoldedChip*) parent)->region_;
  // User Code End begin
}

uint32_t dbUnfoldedRegionItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbUnfoldedRegionItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  return unfolded_region_tbl_->getPtr(id)->chip_next_;
  // User Code End next
}

dbObject* dbUnfoldedRegionItr::getObject(uint32_t id, ...)
{
  return unfolded_region_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp