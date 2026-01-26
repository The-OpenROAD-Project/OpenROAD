// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModBTermItr.h"

#include <cstdint>

#include "dbBusPort.h"
#include "dbModBTerm.h"
#include "dbModule.h"
#include "dbTable.h"

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
  uint32_t id = module->modbterms_;
  uint32_t list = 0;
  while (id != 0) {
    _dbModBTerm* modbterm = modbterm_tbl_->getPtr(id);
    uint32_t n = modbterm->next_entry_;
    modbterm->next_entry_ = list;
    modbterm->prev_entry_ = n;
    list = id;
    id = n;
  }
  module->modbterms_ = list;
  // User Code End reverse
}

uint32_t dbModuleModBTermItr::sequential() const
{
  return 0;
}

uint32_t dbModuleModBTermItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbModuleModBTermItr::begin(parent);
       id != dbModuleModBTermItr::end(parent);
       id = dbModuleModBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbModuleModBTermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModule* _module = (_dbModule*) parent;
  return _module->modbterms_;
  // User Code End begin
}

uint32_t dbModuleModBTermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbModuleModBTermItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbModBTerm* modbterm = modbterm_tbl_->getPtr(id);
  return modbterm->next_entry_;
  // User Code End next
}

dbObject* dbModuleModBTermItr::getObject(uint32_t id, ...)
{
  return modbterm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
