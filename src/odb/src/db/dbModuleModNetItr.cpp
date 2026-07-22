// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetItr.h"

#include <cstdint>

#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

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

uint32_t dbModuleModNetItr::sequential() const
{
  return 0;
}

uint32_t dbModuleModNetItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbModuleModNetItr::begin(parent);
       id != dbModuleModNetItr::end(parent);
       id = dbModuleModNetItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbModuleModNetItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModule* module = (_dbModule*) parent;
  return module->modnets_;
  // User Code End begin
}

uint32_t dbModuleModNetItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbModuleModNetItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbModNet* modnet = modnet_tbl_->getPtr(id);
  return modnet->next_entry_;
  // User Code End next
}

dbObject* dbModuleModNetItr::getObject(uint32_t id, ...)
{
  return modnet_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
