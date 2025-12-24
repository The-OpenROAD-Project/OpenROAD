// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbRegionGroupItr.h"

#include <cstdint>

#include "dbGroup.h"
#include "dbTable.h"
#include "dbTable.hpp"
// User Code Begin Includes
#include "dbRegion.h"
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
  uint32_t id = _parent->groups_;
  uint32_t list = 0;

  while (id != 0) {
    _dbGroup* _child = group_tbl_->getPtr(id);
    uint32_t n = _child->region_next_;
    _child->region_next_ = list;
    list = id;
    id = n;
  }
  _parent->groups_ = list;
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
