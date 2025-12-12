// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbRegionGroupItr.h"

#include "dbGroup.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"
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
  uint id = _parent->groups_;
  uint list = 0;

  while (id != 0) {
    _dbGroup* _child = group_tbl_->getPtr(id);
    uint n = _child->region_next_;
    _child->region_next_ = list;
    list = id;
    id = n;
  }
  _parent->groups_ = list;
  // User Code End reverse
}

uint dbRegionGroupItr::sequential() const
{
  return 0;
}

uint dbRegionGroupItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbRegionGroupItr::begin(parent);
       id != dbRegionGroupItr::end(parent);
       id = dbRegionGroupItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbRegionGroupItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbRegion* _parent = (_dbRegion*) parent;
  return _parent->groups_;
  // User Code End begin
}

uint dbRegionGroupItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbRegionGroupItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbGroup* _child = group_tbl_->getPtr(id);
  return _child->region_next_;
  // User Code End next
}

dbObject* dbRegionGroupItr::getObject(uint id, ...)
{
  return group_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
