// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGroupItr.h"

#include "dbGroup.h"
#include "dbTable.h"
#include "dbTable.hpp"
// User Code Begin Includes
#include "dbModule.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbGroupItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbGroupItr::reversible()
{
  return true;
}

bool dbGroupItr::orderReversed()
{
  return true;
}

void dbGroupItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbGroup* _parent = (_dbGroup*) parent;
  uint id = _parent->_groups;
  uint list = 0;

  while (id != 0) {
    _dbGroup* _child = _group_tbl->getPtr(id);
    uint n = _child->_group_next;
    _child->_group_next = list;
    list = id;
    id = n;
  }
  _parent->_groups = list;
  // User Code End reverse
}

uint dbGroupItr::sequential()
{
  return 0;
}

uint dbGroupItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbGroupItr::begin(parent); id != dbGroupItr::end(parent);
       id = dbGroupItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbGroupItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbGroup* _parent = (_dbGroup*) parent;
  return _parent->_groups;
  // User Code End begin
}

uint dbGroupItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbGroupItr::next(uint id, ...)
{
  // User Code Begin next
  _dbGroup* _child = _group_tbl->getPtr(id);
  return _child->_group_next;
  // User Code End next
}

dbObject* dbGroupItr::getObject(uint id, ...)
{
  return _group_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
