// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedBumpItr.h"

#include <cstdint>

#include "dbTable.h"
#include "dbUnfoldedBump.h"
#include "dbUnfoldedRegion.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedBumpItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbUnfoldedBumpItr::reversible() const
{
  return true;
}

bool dbUnfoldedBumpItr::orderReversed() const
{
  return true;
}

void dbUnfoldedBumpItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbUnfoldedRegion* region = (_dbUnfoldedRegion*) parent;
  uint32_t id = region->bump_;
  uint32_t list = 0;
  while (id != 0) {
    _dbUnfoldedBump* bump = unfolded_bump_tbl_->getPtr(id);
    uint32_t n = bump->region_next_;
    bump->region_next_ = list;
    list = id;
    id = n;
  }
  region->bump_ = list;
  // User Code End reverse
}

uint32_t dbUnfoldedBumpItr::sequential() const
{
  return 0;
}

uint32_t dbUnfoldedBumpItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbUnfoldedBumpItr::begin(parent);
       id != dbUnfoldedBumpItr::end(parent);
       id = dbUnfoldedBumpItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbUnfoldedBumpItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  return ((_dbUnfoldedRegion*) parent)->bump_;
  // User Code End begin
}

uint32_t dbUnfoldedBumpItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbUnfoldedBumpItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  return unfolded_bump_tbl_->getPtr(id)->region_next_;
  // User Code End next
}

dbObject* dbUnfoldedBumpItr::getObject(uint32_t id, ...)
{
  return unfolded_bump_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp