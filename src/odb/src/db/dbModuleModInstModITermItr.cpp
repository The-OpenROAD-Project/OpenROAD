// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModInstModITermItr.h"

#include "dbModITerm.h"
#include "dbModInst.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModInstModITermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModInstModITermItr::reversible()
{
  return true;
}

bool dbModuleModInstModITermItr::orderReversed()
{
  return true;
}

void dbModuleModInstModITermItr::reverse(dbObject* parent)
{
}

uint dbModuleModInstModITermItr::sequential()
{
  return 0;
}

uint dbModuleModInstModITermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleModInstModITermItr::begin(parent);
       id != dbModuleModInstModITermItr::end(parent);
       id = dbModuleModInstModITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleModInstModITermItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModInst* mod_inst = (_dbModInst*) parent;
  return mod_inst->_moditerms;
  // User Code End begin
}

uint dbModuleModInstModITermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleModInstModITermItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModITerm* moditerm = _moditerm_tbl->getPtr(id);
  return moditerm->_next_entry;
  // User Code End next
}

dbObject* dbModuleModInstModITermItr::getObject(uint id, ...)
{
  return _moditerm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
