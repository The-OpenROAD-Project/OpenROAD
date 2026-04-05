// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModInstItr.h"

#include <cstdint>

#include "dbModInst.h"
#include "dbModule.h"
#include "dbTable.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

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
  uint32_t id = module->modinsts_;
  uint32_t list = 0;

  while (id != 0) {
    _dbModInst* modinst = modinst_tbl_->getPtr(id);
    uint32_t n = modinst->module_next_;
    modinst->module_next_ = list;
    list = id;
    id = n;
  }
  module->modinsts_ = list;
  // User Code End reverse
}

uint32_t dbModuleModInstItr::sequential() const
{
  return 0;
}

uint32_t dbModuleModInstItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbModuleModInstItr::begin(parent);
       id != dbModuleModInstItr::end(parent);
       id = dbModuleModInstItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbModuleModInstItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModule* module = (_dbModule*) parent;
  return module->modinsts_;
  // User Code End begin
}

uint32_t dbModuleModInstItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbModuleModInstItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbModInst* modinst = modinst_tbl_->getPtr(id);
  return modinst->module_next_;
  // User Code End next
}

dbObject* dbModuleModInstItr::getObject(uint32_t id, ...)
{
  return modinst_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
