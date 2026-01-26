// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModulePortItr.h"

#include <cstdint>

#include "dbBlock.h"
#include "dbBusPort.h"
#include "dbModBTerm.h"
#include "dbModule.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModulePortItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModulePortItr::reversible() const
{
  return true;
}

bool dbModulePortItr::orderReversed() const
{
  return true;
}

void dbModulePortItr::reverse(dbObject* parent)
{
}

uint32_t dbModulePortItr::sequential() const
{
  return 0;
}

uint32_t dbModulePortItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbModulePortItr::begin(parent); id != dbModulePortItr::end(parent);
       id = dbModulePortItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbModulePortItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModule* _module = (_dbModule*) parent;
  return _module->modbterms_;
  // User Code End begin
}

uint32_t dbModulePortItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbModulePortItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbModBTerm* modbterm = modbterm_tbl_->getPtr(id);
  if (modbterm->busPort_ != 0) {
    _dbBlock* block = (_dbBlock*) modbterm->getOwner();
    _dbBusPort* bus_port = block->busport_tbl_->getPtr(modbterm->busPort_);
    if (bus_port) {
      modbterm = modbterm_tbl_->getPtr(bus_port->last_);
    }
  }
  if (modbterm) {
    return modbterm->next_entry_;
  }
  return 0;
  // User Code End next
}

dbObject* dbModulePortItr::getObject(uint32_t id, ...)
{
  return modbterm_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp
