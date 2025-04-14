// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetModBTermItr.h"

#include "dbModBTerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetModBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetModBTermItr::reversible()
{
  return true;
}

bool dbModuleModNetModBTermItr::orderReversed()
{
  return true;
}

void dbModuleModNetModBTermItr::reverse(dbObject* parent)
{
}

uint dbModuleModNetModBTermItr::sequential()
{
  return 0;
}

uint dbModuleModNetModBTermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleModNetModBTermItr::begin(parent);
       id != dbModuleModNetModBTermItr::end(parent);
       id = dbModuleModNetModBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleModNetModBTermItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->_modbterms;
  // User Code End begin
}

uint dbModuleModNetModBTermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleModNetModBTermItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModBTerm* _modbterm = _modbterm_tbl->getPtr(id);
  return _modbterm->_next_net_modbterm;
  // User Code End next
}

dbObject* dbModuleModNetModBTermItr::getObject(uint id, ...)
{
  return _modbterm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
