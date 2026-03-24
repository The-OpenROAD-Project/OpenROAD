// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGroupInstItr.h"

#include <cstdint>

#include "dbGroup.h"
#include "dbInst.h"
#include "dbTable.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

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
  uint32_t id = _parent->insts_;
  uint32_t list = 0;

  while (id != 0) {
    _dbInst* inst = inst_tbl_->getPtr(id);
    uint32_t n = inst->group_next_;
    inst->group_next_ = list;
    list = id;
    id = n;
  }
  _parent->insts_ = list;
  // User Code End reverse
}

uint32_t dbGroupInstItr::sequential() const
{
  return 0;
}

uint32_t dbGroupInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbGroupInstItr::begin(parent); id != dbGroupInstItr::end(parent);
       id = dbGroupInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbGroupInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbGroup* _parent = (_dbGroup*) parent;
  return _parent->insts_;
  // User Code End begin
}

uint32_t dbGroupInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbGroupInstItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbInst* inst = inst_tbl_->getPtr(id);
  return inst->group_next_;
  // User Code End next
}

dbObject* dbGroupInstItr::getObject(uint32_t id, ...)
{
  return inst_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
