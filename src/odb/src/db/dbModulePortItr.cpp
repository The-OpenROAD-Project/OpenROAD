// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModulePortItr.h"

#include "dbBlock.h"
#include "dbBusPort.h"
#include "dbModBTerm.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModulePortItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModulePortItr::reversible()
{
  return true;
}

bool dbModulePortItr::orderReversed()
{
  return true;
}

void dbModulePortItr::reverse(dbObject* parent)
{
}

uint dbModulePortItr::sequential()
{
  return 0;
}

uint dbModulePortItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbModulePortItr::begin(parent); id != dbModulePortItr::end(parent);
       id = dbModulePortItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModulePortItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModule* _module = (_dbModule*) parent;
  return _module->_modbterms;
  // User Code End begin
}

uint dbModulePortItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModulePortItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModBTerm* modbterm = _modbterm_tbl->getPtr(id);
  if (modbterm->_busPort != 0) {
    _dbBlock* block = (_dbBlock*) modbterm->getOwner();
    _dbBusPort* bus_port = block->_busport_tbl->getPtr(modbterm->_busPort);
    if (bus_port) {
      modbterm = _modbterm_tbl->getPtr(bus_port->_last);
    }
  }
  if (modbterm) {
    return modbterm->_next_entry;
  }
  return 0;
  // User Code End next
}

dbObject* dbModulePortItr::getObject(uint id, ...)
{
  return _modbterm_tbl->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
