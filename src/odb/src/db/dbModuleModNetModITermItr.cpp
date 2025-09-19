// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetModITermItr.h"

#include "dbModITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetModITermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetModITermItr::reversible()
{
  return true;
}

bool dbModuleModNetModITermItr::orderReversed()
{
  return true;
}

void dbModuleModNetModITermItr::reverse(dbObject* parent)
{
}

uint dbModuleModNetModITermItr::sequential()
{
  return 0;
}

uint dbModuleModNetModITermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleModNetModITermItr::begin(parent);
       id != dbModuleModNetModITermItr::end(parent);
       id = dbModuleModNetModITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleModNetModITermItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->_moditerms;
  // User Code End begin
}

uint dbModuleModNetModITermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleModNetModITermItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModITerm* _moditerm = _moditerm_tbl->getPtr(id);
  return _moditerm->_next_net_moditerm;
  // User Code End next
}

dbObject* dbModuleModNetModITermItr::getObject(uint id, ...)
{
  return _moditerm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
