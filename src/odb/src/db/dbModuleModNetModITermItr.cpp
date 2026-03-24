// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetModITermItr.h"

#include <cstdint>

#include "dbModITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetModITermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetModITermItr::reversible() const
{
  return true;
}

bool dbModuleModNetModITermItr::orderReversed() const
{
  return true;
}

void dbModuleModNetModITermItr::reverse(dbObject* parent)
{
}

uint32_t dbModuleModNetModITermItr::sequential() const
{
  return 0;
}

uint32_t dbModuleModNetModITermItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbModuleModNetModITermItr::begin(parent);
       id != dbModuleModNetModITermItr::end(parent);
       id = dbModuleModNetModITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbModuleModNetModITermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->moditerms_;
  // User Code End begin
}

uint32_t dbModuleModNetModITermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbModuleModNetModITermItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbModITerm* _moditerm = moditerm_tbl_->getPtr(id);
  return _moditerm->next_net_moditerm_;
  // User Code End next
}

dbObject* dbModuleModNetModITermItr::getObject(uint32_t id, ...)
{
  return moditerm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
