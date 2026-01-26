// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleBusPortModBTermItr.h"

#include <cstdint>

#include "dbBusPort.h"
#include "dbModBTerm.h"
#include "dbModule.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleBusPortModBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleBusPortModBTermItr::reversible() const
{
  return true;
}

bool dbModuleBusPortModBTermItr::orderReversed() const
{
  return true;
}

void dbModuleBusPortModBTermItr::reverse(dbObject* parent)
{
}

uint32_t dbModuleBusPortModBTermItr::sequential() const
{
  return 0;
}

uint32_t dbModuleBusPortModBTermItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbModuleBusPortModBTermItr::begin(parent);
       id != dbModuleBusPortModBTermItr::end(parent);
       id = dbModuleBusPortModBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbModuleBusPortModBTermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbBusPort* _busport = (_dbBusPort*) parent;
  return _busport->members_;
  // User Code End begin
}

uint32_t dbModuleBusPortModBTermItr::end(dbObject* parent) const
{
  // User Code Begin end
  _dbBusPort* _busport = (_dbBusPort*) parent;
  return next(_busport->last_);
  // User Code End end
}

uint32_t dbModuleBusPortModBTermItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbModBTerm* lmodbterm = modbterm_tbl_->getPtr(id);
  uint32_t ret = lmodbterm->next_entry_;
  return ret;
  // User Code End next
}

dbObject* dbModuleBusPortModBTermItr::getObject(uint32_t id, ...)
{
  return modbterm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
