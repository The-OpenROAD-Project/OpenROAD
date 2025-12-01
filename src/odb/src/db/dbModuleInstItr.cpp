// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleInstItr.h"

#include "dbInst.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

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
  uint id = module->_insts;
  uint list = 0;

  while (id != 0) {
    _dbInst* inst = _inst_tbl->getPtr(id);
    uint n = inst->_module_prev;
    inst->_module_prev = inst->_module_next;
    inst->_module_next = n;
    list = id;
    id = inst->_module_prev;
  }
  module->_insts = list;
  // User Code End reverse
}

uint dbModuleInstItr::sequential() const
{
  return 0;
}

uint dbModuleInstItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleInstItr::begin(parent); id != dbModuleInstItr::end(parent);
       id = dbModuleInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModule* module = (_dbModule*) parent;
  return module->_insts;
  // User Code End begin
}

uint dbModuleInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbModuleInstItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbInst* inst = _inst_tbl->getPtr(id);
  return inst->_module_next;
  // User Code End next
}

dbObject* dbModuleInstItr::getObject(uint id, ...)
{
  return _inst_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
