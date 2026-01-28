// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModInstModITermItr.h"

#include <cstdint>

#include "dbModITerm.h"
#include "dbModInst.h"
#include "dbModule.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModInstModITermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModInstModITermItr::reversible() const
{
  return true;
}

bool dbModuleModInstModITermItr::orderReversed() const
{
  return true;
}

void dbModuleModInstModITermItr::reverse(dbObject* parent)
{
}

uint32_t dbModuleModInstModITermItr::sequential() const
{
  return 0;
}

uint32_t dbModuleModInstModITermItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbModuleModInstModITermItr::begin(parent);
       id != dbModuleModInstModITermItr::end(parent);
       id = dbModuleModInstModITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbModuleModInstModITermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModInst* mod_inst = (_dbModInst*) parent;
  return mod_inst->moditerms_;
  // User Code End begin
}

uint32_t dbModuleModInstModITermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbModuleModInstModITermItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbModITerm* moditerm = moditerm_tbl_->getPtr(id);
  return moditerm->next_entry_;
  // User Code End next
}

dbObject* dbModuleModInstModITermItr::getObject(uint32_t id, ...)
{
  return moditerm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
