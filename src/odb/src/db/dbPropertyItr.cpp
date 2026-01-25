// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbPropertyItr.h"

#include <cstdint>

#include "dbProperty.h"
#include "dbTable.h"
#include "odb/dbObject.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbPropertyItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbPropertyItr::reversible() const
{
  return true;
}

bool dbPropertyItr::orderReversed() const
{
  return true;
}

void dbPropertyItr::reverse(dbObject* parent)
{
  _dbObject* parent_impl = parent->getImpl();
  dbObjectTable* table = parent_impl->getTable();
  uint32_t id = table->getPropList(parent_impl->getOID());

  if (id) {
    uint32_t list = 0;

    while (id != 0) {
      _dbProperty* p = prop_tbl_->getPtr(id);
      uint32_t n = p->next_;
      p->next_ = list;
      list = id;
      id = n;
    }

    table->setPropList(parent_impl->getOID(), list);
  }
}

uint32_t dbPropertyItr::sequential() const
{
  return 0;
}

uint32_t dbPropertyItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbPropertyItr::begin(parent); id != dbPropertyItr::end(parent);
       id = dbPropertyItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbPropertyItr::begin(dbObject* parent) const
{
  dbObjectTable* table = parent->getImpl()->getTable();
  uint32_t id = table->getPropList(parent->getImpl()->getOID());
  return id;
}

uint32_t dbPropertyItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbPropertyItr::next(uint32_t id, ...) const
{
  _dbProperty* prop = prop_tbl_->getPtr(id);
  return prop->next_;
}

dbObject* dbPropertyItr::getObject(uint32_t id, ...)
{
  return prop_tbl_->getPtr(id);
}

}  // namespace odb
