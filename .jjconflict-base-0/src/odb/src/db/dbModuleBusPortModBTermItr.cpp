// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleBusPortModBTermItr.h"

#include "dbBusPort.h"
#include "dbModBTerm.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleBusPortModBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleBusPortModBTermItr::reversible()
{
  return true;
}

bool dbModuleBusPortModBTermItr::orderReversed()
{
  return true;
}

void dbModuleBusPortModBTermItr::reverse(dbObject* parent)
{
}

uint dbModuleBusPortModBTermItr::sequential()
{
  return 0;
}

uint dbModuleBusPortModBTermItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleBusPortModBTermItr::begin(parent);
       id != dbModuleBusPortModBTermItr::end(parent);
       id = dbModuleBusPortModBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleBusPortModBTermItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbBusPort* _busport = (_dbBusPort*) parent;
  _iter = _modbterm_tbl->getPtr(_busport->_members);
  _size = abs(_busport->_from - _busport->_to) + 1;
  _ix = 0;
  return _busport->_members;
  // User Code End begin
}

uint dbModuleBusPortModBTermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleBusPortModBTermItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModBTerm* lmodbterm = _modbterm_tbl->getPtr(id);
  _ix++;
  uint ret = lmodbterm->_next_entry;
  _iter = _modbterm_tbl->getPtr(ret);
  return ret;
  // User Code End next
}

dbObject* dbModuleBusPortModBTermItr::getObject(uint id, ...)
{
  return _modbterm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
