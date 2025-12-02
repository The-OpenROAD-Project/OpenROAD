// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGroupInstItr.h"

#include "dbGroup.h"
#include "dbInst.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbGroupInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbGroupInstItr::reversible() const
{
  return true;
}

bool dbGroupInstItr::orderReversed() const
{
  return true;
}

void dbGroupInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbGroup* _parent = (_dbGroup*) parent;
  uint id = _parent->_insts;
  uint list = 0;

  while (id != 0) {
    _dbInst* inst = _inst_tbl->getPtr(id);
    uint n = inst->_group_next;
    inst->_group_next = list;
    list = id;
    id = n;
  }
  _parent->_insts = list;
  // User Code End reverse
}

uint dbGroupInstItr::sequential() const
{
  return 0;
}

uint dbGroupInstItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbGroupInstItr::begin(parent); id != dbGroupInstItr::end(parent);
       id = dbGroupInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbGroupInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbGroup* _parent = (_dbGroup*) parent;
  return _parent->_insts;
  // User Code End begin
}

uint dbGroupInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbGroupInstItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbInst* inst = _inst_tbl->getPtr(id);
  return inst->_group_next;
  // User Code End next
}

dbObject* dbGroupInstItr::getObject(uint id, ...)
{
  return _inst_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
