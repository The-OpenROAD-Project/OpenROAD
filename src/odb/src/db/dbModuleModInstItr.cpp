// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModInstItr.h"

#include "dbModInst.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModInstItr::reversible()
{
  return true;
}

bool dbModuleModInstItr::orderReversed()
{
  return true;
}

void dbModuleModInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbModule* module = (_dbModule*) parent;
  uint id = module->_modinsts;
  uint list = 0;

  while (id != 0) {
    _dbModInst* modinst = _modinst_tbl->getPtr(id);
    uint n = modinst->_module_next;
    modinst->_module_next = list;
    list = id;
    id = n;
  }
  module->_modinsts = list;
  // User Code End reverse
}

uint dbModuleModInstItr::sequential()
{
  return 0;
}

uint dbModuleModInstItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleModInstItr::begin(parent);
       id != dbModuleModInstItr::end(parent);
       id = dbModuleModInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleModInstItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModule* module = (_dbModule*) parent;
  return module->_modinsts;
  // User Code End begin
}

uint dbModuleModInstItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleModInstItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModInst* modinst = _modinst_tbl->getPtr(id);
  return modinst->_module_next;
  // User Code End next
}

dbObject* dbModuleModInstItr::getObject(uint id, ...)
{
  return _modinst_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
