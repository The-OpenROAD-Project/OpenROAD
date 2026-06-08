// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbUnfoldedBumpInstItr.h"

#include <cstdint>

#include "dbTable.h"
#include "dbUnfoldedBumpInst.h"
#include "dbUnfoldedRegionInst.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbUnfoldedBumpInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbUnfoldedBumpInstItr::reversible() const
{
  return true;
}

bool dbUnfoldedBumpInstItr::orderReversed() const
{
  return true;
}

void dbUnfoldedBumpInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbUnfoldedRegionInst* region = (_dbUnfoldedRegionInst*) parent;
  uint32_t id = region->bump_;
  uint32_t list = 0;
  while (id != 0) {
    _dbUnfoldedBumpInst* bump = unfolded_bump_inst_tbl_->getPtr(id);
    uint32_t n = bump->region_next_;
    bump->region_next_ = list;
    list = id;
    id = n;
  }
  region->bump_ = list;
  // User Code End reverse
}

uint32_t dbUnfoldedBumpInstItr::sequential() const
{
  return 0;
}

uint32_t dbUnfoldedBumpInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbUnfoldedBumpInstItr::begin(parent);
       id != dbUnfoldedBumpInstItr::end(parent);
       id = dbUnfoldedBumpInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbUnfoldedBumpInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  return ((_dbUnfoldedRegionInst*) parent)->bump_;
  // User Code End begin
}

uint32_t dbUnfoldedBumpInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbUnfoldedBumpInstItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  return unfolded_bump_inst_tbl_->getPtr(id)->region_next_;
  // User Code End next
}

dbObject* dbUnfoldedBumpInstItr::getObject(uint32_t id, ...)
{
  return unfolded_bump_inst_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp