// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetITermItr.h"

#include <cstdint>

#include "dbITerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetITermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetITermItr::reversible() const
{
  return true;
}

bool dbModuleModNetITermItr::orderReversed() const
{
  return true;
}

void dbModuleModNetITermItr::reverse(dbObject* parent)
{
}

uint32_t dbModuleModNetITermItr::sequential() const
{
  return 0;
}

uint32_t dbModuleModNetITermItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbModuleModNetITermItr::begin(parent);
       id != dbModuleModNetITermItr::end(parent);
       id = dbModuleModNetITermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbModuleModNetITermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->iterms_;
  // User Code End begin
}

uint32_t dbModuleModNetITermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbModuleModNetITermItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbITerm* _iterm = iterm_tbl_->getPtr(id);
  return _iterm->next_modnet_iterm_;
  // User Code End next
}

dbObject* dbModuleModNetITermItr::getObject(uint32_t id, ...)
{
  return iterm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
