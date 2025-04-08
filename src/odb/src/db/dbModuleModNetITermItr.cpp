// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetITermItr.h"

#include "dbITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetITermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetITermItr::reversible()
{
  return true;
}

bool dbModuleModNetITermItr::orderReversed()
{
  return true;
}

void dbModuleModNetITermItr::reverse(dbObject* parent)
{
}

uint dbModuleModNetITermItr::sequential()
{
  return 0;
}

uint dbModuleModNetITermItr::size(dbObject* parent)
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

uint dbModuleModNetITermItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->_iterms;
  // User Code End begin
}

uint dbModuleModNetITermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleModNetITermItr::next(uint id, ...)
{
  // User Code Begin next
  _dbITerm* _iterm = _iterm_tbl->getPtr(id);
  return _iterm->_next_modnet_iterm;
  // User Code End next
}

dbObject* dbModuleModNetITermItr::getObject(uint id, ...)
{
  return _iterm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
