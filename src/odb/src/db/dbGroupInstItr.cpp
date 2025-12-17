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
  uint id = _parent->insts_;
  uint list = 0;

  while (id != 0) {
    _dbInst* inst = inst_tbl_->getPtr(id);
    uint n = inst->group_next_;
    inst->group_next_ = list;
    list = id;
    id = n;
  }
  _parent->insts_ = list;
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
  return _parent->insts_;
  // User Code End begin
}

uint dbGroupInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbGroupInstItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbInst* inst = inst_tbl_->getPtr(id);
  return inst->group_next_;
  // User Code End next
}

dbObject* dbGroupInstItr::getObject(uint id, ...)
{
  return inst_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
