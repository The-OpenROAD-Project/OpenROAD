// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetBTermItr.h"

#include "dbBTerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetBTermItr::reversible()
{
  return true;
}

bool dbModuleModNetBTermItr::orderReversed()
{
  return true;
}

void dbModuleModNetBTermItr::reverse(dbObject* parent)
{
}

uint dbModuleModNetBTermItr::sequential()
{
  return 0;
}

uint dbModuleModNetBTermItr::size(dbObject* parent)
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

uint dbModuleModNetBTermItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->_bterms;
  // User Code End begin
}

uint dbModuleModNetBTermItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbModuleModNetBTermItr::next(uint id, ...)
{
  // User Code Begin next
  _dbBTerm* _bterm = _bterm_tbl->getPtr(id);
  return _bterm->_next_modnet_bterm;
  // User Code End next
}

dbObject* dbModuleModNetBTermItr::getObject(uint id, ...)
{
  return _bterm_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
