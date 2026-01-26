// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbModuleModNetModBTermItr.h"

#include <cstdint>

#include "dbModBTerm.h"
#include "dbModNet.h"
#include "dbModule.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbModuleModNetModBTermItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbModuleModNetModBTermItr::reversible() const
{
  return true;
}

bool dbModuleModNetModBTermItr::orderReversed() const
{
  return true;
}

void dbModuleModNetModBTermItr::reverse(dbObject* parent)
{
}

uint32_t dbModuleModNetModBTermItr::sequential() const
{
  return 0;
}

uint32_t dbModuleModNetModBTermItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbModuleModNetModBTermItr::begin(parent);
       id != dbModuleModNetModBTermItr::end(parent);
       id = dbModuleModNetModBTermItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbModuleModNetModBTermItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbModNet* mod_net = (_dbModNet*) parent;
  return mod_net->modbterms_;
  // User Code End begin
}

uint32_t dbModuleModNetModBTermItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbModuleModNetModBTermItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbModBTerm* _modbterm = modbterm_tbl_->getPtr(id);
  return _modbterm->next_net_modbterm_;
  // User Code End next
}

dbObject* dbModuleModNetModBTermItr::getObject(uint32_t id, ...)
{
  return modbterm_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
