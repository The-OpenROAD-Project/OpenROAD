// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModBTermItr.h"

#include "dbBusPort.h"
#include "dbModBTerm.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModBTermItr::reversible()
{
  return true;
}

bool dbModuleModBTermItr::orderReversed()
{
  return true;
}

void dbModuleModBTermItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbModule* module = (_dbModule*) parent;
  uint id = module->_modbterms;
  uint list = 0;
  while (id != 0) {
    _dbModBTerm* modbterm = _modbterm_tbl->getPtr(id);
    uint n = modbterm->_next_entry;
    modbterm->_next_entry = list;
    modbterm->_prev_entry = n;
    list = id;
    id = n;
  }
  module->_modbterms = list;
  // User Code End reverse
}

uint dbModuleModBTermItr::sequential()
{
  return 0;
}

uint dbModuleModBTermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleModBTermItr::begin(parent);
       id != dbModuleModBTermItr::end(parent);
       id = dbModuleModBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleModBTermItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModule* _module = (_dbModule*) parent;
  return _module->_modbterms;
  // User Code End begin
}

uint dbModuleModBTermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleModBTermItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModBTerm* modbterm = _modbterm_tbl->getPtr(id);
  return modbterm->_next_entry;
  // User Code End next
}

dbObject* dbModuleModBTermItr::getObject(uint id, ...)
{
  return _modbterm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
