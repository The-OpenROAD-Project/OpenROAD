// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModInstItr.h"

#include "dbModInst.h"
#include "dbModule.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModInstItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModInstItr::reversible() const
{
  return true;
}

bool dbModuleModInstItr::orderReversed() const
{
  return true;
}

void dbModuleModInstItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbModule* module = (_dbModule*) parent;
  uint id = module->modinsts_;
  uint list = 0;

  while (id != 0) {
    _dbModInst* modinst = modinst_tbl_->getPtr(id);
    uint n = modinst->module_next_;
    modinst->module_next_ = list;
    list = id;
    id = n;
  }
  module->modinsts_ = list;
  // User Code End reverse
}

uint dbModuleModInstItr::sequential() const
{
  return 0;
}

uint dbModuleModInstItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbModuleModInstItr::begin(parent);
       id != dbModuleModInstItr::end(parent);
       id = dbModuleModInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbModuleModInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModule* module = (_dbModule*) parent;
  return module->modinsts_;
  // User Code End begin
}

uint dbModuleModInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbModuleModInstItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbModInst* modinst = modinst_tbl_->getPtr(id);
  return modinst->module_next_;
  // User Code End next
}

dbObject* dbModuleModInstItr::getObject(uint id, ...)
{
  return modinst_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
