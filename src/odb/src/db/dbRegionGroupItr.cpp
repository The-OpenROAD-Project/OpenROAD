// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbRegionGroupItr.h"

#include <cstdint>

#include "dbGroup.h"
#include "dbTable.h"
// User Code Begin Includes
#include "dbRegion.h"
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbRegionGroupItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbRegionGroupItr::reversible() const
{
  return true;
}

bool dbRegionGroupItr::orderReversed() const
{
  return true;
}

void dbRegionGroupItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbRegion* _parent = (_dbRegion*) parent;
  uint32_t curr_id = _parent->groups_;
  uint32_t last_id = 0;

  while (curr_id != 0) {
    _dbGroup* _child = group_tbl_->getPtr(curr_id);
    uint32_t next_id = _child->region_next_;
    _child->region_next_ = _child->region_prev_;
    _child->region_prev_ = next_id;

    last_id = curr_id;
    curr_id = next_id;
  }
  _parent->groups_ = last_id;
  // User Code End reverse
}

uint32_t dbRegionGroupItr::sequential() const
{
  return 0;
}

uint32_t dbRegionGroupItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbRegionGroupItr::begin(parent);
       id != dbRegionGroupItr::end(parent);
       id = dbRegionGroupItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbRegionGroupItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbRegion* _parent = (_dbRegion*) parent;
  return _parent->groups_;
  // User Code End begin
}

uint32_t dbRegionGroupItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbRegionGroupItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbGroup* _child = group_tbl_->getPtr(id);
  return _child->region_next_;
  // User Code End next
}

dbObject* dbRegionGroupItr::getObject(uint32_t id, ...)
{
  return group_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
