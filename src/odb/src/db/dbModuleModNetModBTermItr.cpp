// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetModBTermItr.h"

#include "dbModBTerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetModBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetModBTermItr::reversible() const
{
  return true;
}

bool dbModuleModNetModBTermItr::orderReversed() const
{
  return true;
}

void dbModuleModNetModBTermItr::reverse(dbObject* parent)
{
}

uint dbModuleModNetModBTermItr::sequential() const
{
  return 0;
}

uint dbModuleModNetModBTermItr::size(dbObject* parent) const
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

uint dbModuleModNetModBTermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->modbterms_;
  // User Code End begin
}

uint dbModuleModNetModBTermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbModuleModNetModBTermItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbModBTerm* _modbterm = modbterm_tbl_->getPtr(id);
  return _modbterm->next_net_modbterm_;
  // User Code End next
}

dbObject* dbModuleModNetModBTermItr::getObject(uint id, ...)
{
  return modbterm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
