// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModBTermItr.h"

#include "dbBusPort.h"
#include "dbModBTerm.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModBTermItr::reversible() const
{
  return true;
}

bool dbModuleModBTermItr::orderReversed() const
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
    uint n = modbterm->next_entry_;
    modbterm->next_entry_ = list;
    modbterm->_prev_entry = n;
    list = id;
    id = n;
  }
  module->_modbterms = list;
  // User Code End reverse
}

uint dbModuleModBTermItr::sequential() const
{
  return 0;
}

uint dbModuleModBTermItr::size(dbObject* parent) const
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

uint dbModuleModBTermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModule* _module = (_dbModule*) parent;
  return _module->_modbterms;
  // User Code End begin
}

uint dbModuleModBTermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbModuleModBTermItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbModBTerm* modbterm = _modbterm_tbl->getPtr(id);
  return modbterm->next_entry_;
  // User Code End next
}

dbObject* dbModuleModBTermItr::getObject(uint id, ...)
{
  return _modbterm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
