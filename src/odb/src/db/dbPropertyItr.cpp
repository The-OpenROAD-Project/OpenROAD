// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbPropertyItr.h"

#include "dbProperty.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbPropertyItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbPropertyItr::reversible()
{
  return true;
}

bool dbPropertyItr::orderReversed()
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
      uint n = p->_next;
      p->_next = list;
      list = id;
      id = n;
    }

    table->setPropList(parent_impl->getOID(), list);
  }
}

uint dbPropertyItr::sequential()
{
  return 0;
}

uint dbPropertyItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbPropertyItr::begin(parent); id != dbPropertyItr::end(parent);
       id = dbPropertyItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbPropertyItr::begin(dbObject* parent)
{
  dbObjectTable* table = parent->getImpl()->getTable();
  uint id = table->getPropList(parent->getImpl()->getOID());
  return id;
}

uint dbPropertyItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbPropertyItr::next(uint id, ...)
{
  _dbProperty* prop = _prop_tbl->getPtr(id);
  return prop->_next;
}

dbObject* dbPropertyItr::getObject(uint id, ...)
{
  return _prop_tbl->getPtr(id);
}

}  // namespace odb
