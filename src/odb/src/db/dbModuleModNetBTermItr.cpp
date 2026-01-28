// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetBTermItr.h"

#include <cstdint>

#include "dbBTerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetBTermItr::reversible() const
{
  return true;
}

bool dbModuleModNetBTermItr::orderReversed() const
{
  return true;
}

void dbModuleModNetBTermItr::reverse(dbObject* parent)
{
}

uint32_t dbModuleModNetBTermItr::sequential() const
{
  return 0;
}

uint32_t dbModuleModNetBTermItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbModuleModNetBTermItr::begin(parent);
       id != dbModuleModNetBTermItr::end(parent);
       id = dbModuleModNetBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbModuleModNetBTermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->bterms_;
  // User Code End begin
}

uint32_t dbModuleModNetBTermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbModuleModNetBTermItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbBTerm* _bterm = bterm_tbl_->getPtr(id);
  return _bterm->next_modnet_bterm_;
  // User Code End next
}

dbObject* dbModuleModNetBTermItr::getObject(uint32_t id, ...)
{
  return bterm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
