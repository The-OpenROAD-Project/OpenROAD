// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGroupItr.h"

#include "dbGroup.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"
// User Code Begin Includes
#include "dbModule.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbGroupItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbGroupItr::reversible() const
{
  return true;
}

bool dbGroupItr::orderReversed() const
{
  return true;
}

void dbGroupItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbGroup* _parent = (_dbGroup*) parent;
  uint id = _parent->groups_;
  uint list = 0;

  while (id != 0) {
    _dbGroup* _child = group_tbl_->getPtr(id);
    uint n = _child->group_next_;
    _child->group_next_ = list;
    list = id;
    id = n;
  }
  _parent->groups_ = list;
  // User Code End reverse
}

uint dbGroupItr::sequential() const
{
  return 0;
}

uint dbGroupItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbGroupItr::begin(parent); id != dbGroupItr::end(parent);
       id = dbGroupItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbGroupItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbGroup* _parent = (_dbGroup*) parent;
  return _parent->groups_;
  // User Code End begin
}

uint dbGroupItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbGroupItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbGroup* _child = group_tbl_->getPtr(id);
  return _child->group_next_;
  // User Code End next
}

dbObject* dbGroupItr::getObject(uint id, ...)
{
  return group_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
