// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetBTermItr.h"

#include "dbBTerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetBTermItr::reversible() const
{
  return true;
}

bool dbModuleModNetBTermItr::orderReversed() const
{
  return true;
}

void dbModuleModNetBTermItr::reverse(dbObject* parent)
{
}

uint dbModuleModNetBTermItr::sequential() const
{
  return 0;
}

uint dbModuleModNetBTermItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleModNetBTermItr::begin(parent);
       id != dbModuleModNetBTermItr::end(parent);
       id = dbModuleModNetBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleModNetBTermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->bterms_;
  // User Code End begin
}

uint dbModuleModNetBTermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbModuleModNetBTermItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbBTerm* _bterm = bterm_tbl_->getPtr(id);
  return _bterm->next_modnet_bterm_;
  // User Code End next
}

dbObject* dbModuleModNetBTermItr::getObject(uint id, ...)
{
  return bterm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
