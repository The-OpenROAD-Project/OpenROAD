// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleInstItr.h"

#include <cstdint>

#include "dbInst.h"
#include "dbModule.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleInstItr::reversible() const
{
  return true;
}

bool dbModuleInstItr::orderReversed() const
{
  return true;
}

void dbModuleInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbModule* module = (_dbModule*) parent;
  uint32_t id = module->insts_;
  uint32_t list = 0;

  while (id != 0) {
    _dbInst* inst = inst_tbl_->getPtr(id);
    uint32_t n = inst->module_prev_;
    inst->module_prev_ = inst->module_next_;
    inst->module_next_ = n;
    list = id;
    id = inst->module_prev_;
  }
  module->insts_ = list;
  // User Code End reverse
}

uint32_t dbModuleInstItr::sequential() const
{
  return 0;
}

uint32_t dbModuleInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbModuleInstItr::begin(parent); id != dbModuleInstItr::end(parent);
       id = dbModuleInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbModuleInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModule* module = (_dbModule*) parent;
  return module->insts_;
  // User Code End begin
}

uint32_t dbModuleInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbModuleInstItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbInst* inst = inst_tbl_->getPtr(id);
  return inst->module_next_;
  // User Code End next
}

dbObject* dbModuleInstItr::getObject(uint32_t id, ...)
{
  return inst_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
