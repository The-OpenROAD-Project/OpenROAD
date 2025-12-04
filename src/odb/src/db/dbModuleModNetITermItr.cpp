// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetITermItr.h"

#include "dbITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetITermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetITermItr::reversible() const
{
  return true;
}

bool dbModuleModNetITermItr::orderReversed() const
{
  return true;
}

void dbModuleModNetITermItr::reverse(dbObject* parent)
{
}

uint dbModuleModNetITermItr::sequential() const
{
  return 0;
}

uint dbModuleModNetITermItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleModNetITermItr::begin(parent);
       id != dbModuleModNetITermItr::end(parent);
       id = dbModuleModNetITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleModNetITermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->iterms_;
  // User Code End begin
}

uint dbModuleModNetITermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbModuleModNetITermItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbITerm* _iterm = iterm_tbl_->getPtr(id);
  return _iterm->next_modnet_iterm_;
  // User Code End next
}

dbObject* dbModuleModNetITermItr::getObject(uint id, ...)
{
  return iterm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
