// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetModITermItr.h"

#include "dbModITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetModITermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetModITermItr::reversible() const
{
  return true;
}

bool dbModuleModNetModITermItr::orderReversed() const
{
  return true;
}

void dbModuleModNetModITermItr::reverse(dbObject* parent)
{
}

uint dbModuleModNetModITermItr::sequential() const
{
  return 0;
}

uint dbModuleModNetModITermItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleModNetModITermItr::begin(parent);
       id != dbModuleModNetModITermItr::end(parent);
       id = dbModuleModNetModITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleModNetModITermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->moditerms_;
  // User Code End begin
}

uint dbModuleModNetModITermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbModuleModNetModITermItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbModITerm* _moditerm = moditerm_tbl_->getPtr(id);
  return _moditerm->next_net_moditerm_;
  // User Code End next
}

dbObject* dbModuleModNetModITermItr::getObject(uint id, ...)
{
  return moditerm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
