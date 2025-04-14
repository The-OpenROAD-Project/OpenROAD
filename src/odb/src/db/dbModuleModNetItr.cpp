// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetItr.h"

#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetItr::reversible()
{
  return true;
}

bool dbModuleModNetItr::orderReversed()
{
  return true;
}

void dbModuleModNetItr::reverse(dbObject* parent)
{
}

uint dbModuleModNetItr::sequential()
{
  return 0;
}

uint dbModuleModNetItr::size(dbObject* parent)
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

uint dbModuleModNetItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModule* module = (_dbModule*) parent;
  return module->_modnets;
  // User Code End begin
}

uint dbModuleModNetItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleModNetItr::next(uint id, ...)
{
  // User Code Begin next
  _dbModNet* modnet = _modnet_tbl->getPtr(id);
  return modnet->_next_entry;
  // User Code End next
}

dbObject* dbModuleModNetItr::getObject(uint id, ...)
{
  return _modnet_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
