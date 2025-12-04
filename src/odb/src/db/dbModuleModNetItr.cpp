// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetItr.h"

#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetItr::reversible() const
{
  return true;
}

bool dbModuleModNetItr::orderReversed() const
{
  return true;
}

void dbModuleModNetItr::reverse(dbObject* parent)
{
}

uint dbModuleModNetItr::sequential() const
{
  return 0;
}

uint dbModuleModNetItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleModNetItr::begin(parent);
       id != dbModuleModNetItr::end(parent);
       id = dbModuleModNetItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleModNetItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModule* module = (_dbModule*) parent;
  return module->modnets_;
  // User Code End begin
}

uint dbModuleModNetItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbModuleModNetItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbModNet* modnet = modnet_tbl_->getPtr(id);
  return modnet->next_entry_;
  // User Code End next
}

dbObject* dbModuleModNetItr::getObject(uint id, ...)
{
  return modnet_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
