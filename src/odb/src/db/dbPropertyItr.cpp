// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbPropertyItr.h"

#include "dbProperty.h"
#include "dbTable.h"
#include "dbTable.hpp"
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
  uint id = table->getPropList(parent_impl->getOID());

  if (id) {
    uint list = 0;

    while (id != 0) {
      _dbProperty* p = _prop_tbl->getPtr(id);
      uint n = p->next_;
      p->next_ = list;
      list = id;
      id = n;
    }

    table->setPropList(parent_impl->getOID(), list);
  }
}

uint dbPropertyItr::sequential() const
{
  return 0;
}

uint dbPropertyItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbPropertyItr::begin(parent); id != dbPropertyItr::end(parent);
       id = dbPropertyItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbPropertyItr::begin(dbObject* parent) const
{
  dbObjectTable* table = parent->getImpl()->getTable();
  uint id = table->getPropList(parent->getImpl()->getOID());
  return id;
}

uint dbPropertyItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbPropertyItr::next(uint id, ...) const
{
  _dbProperty* prop = _prop_tbl->getPtr(id);
  return prop->next_;
}

dbObject* dbPropertyItr::getObject(uint id, ...)
{
  return _prop_tbl->getPtr(id);
}

}  // namespace odb
