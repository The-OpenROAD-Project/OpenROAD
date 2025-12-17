// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleBusPortModBTermItr.h"

#include "dbBusPort.h"
#include "dbModBTerm.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

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

uint dbModuleBusPortModBTermItr::sequential() const
{
  return 0;
}

uint dbModuleBusPortModBTermItr::size(dbObject* parent) const
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

uint dbModuleBusPortModBTermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbBusPort* _busport = (_dbBusPort*) parent;
  return _busport->members_;
  // User Code End begin
}

uint dbModuleBusPortModBTermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbModuleBusPortModBTermItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbModBTerm* lmodbterm = modbterm_tbl_->getPtr(id);
  uint ret = lmodbterm->next_entry_;
  return ret;
  // User Code End next
}

dbObject* dbModuleBusPortModBTermItr::getObject(uint id, ...)
{
  return modbterm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
