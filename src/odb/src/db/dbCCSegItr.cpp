// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbCCSegItr.h"

#include <cstdarg>

#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCapNode.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbCCSegItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbCCSegItr::reversible()
{
  return true;
}

bool dbCCSegItr::orderReversed()
{
  return true;
}

void dbCCSegItr::reverse(dbObject* parent)
{
  _dbCapNode* node = (_dbCapNode*) parent;
  uint id = node->_cc_segs;
  uint pid = parent->getId();
  uint list = 0;

  while (id != 0) {
    _dbCCSeg* seg = _seg_tbl->getPtr(id);
    uint n = seg->next(pid);
    seg->next(pid) = list;
    list = id;
    id = n;
  }

  node->_cc_segs = list;
}

uint dbCCSegItr::sequential()
{
  return 0;
}

uint dbCCSegItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbCCSegItr::begin(parent); id != dbCCSegItr::end(parent);
       id = dbCCSegItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbCCSegItr::begin(dbObject* parent)
{
  _dbCapNode* node = (_dbCapNode*) parent;
  return node->_cc_segs;
}

uint dbCCSegItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbCCSegItr::next(uint id, ...)
{
  va_list ap;
  va_start(ap, id);
  uint pid = va_arg(ap, uint);
  va_end(ap);
  _dbCCSeg* seg = _seg_tbl->getPtr(id);
  return seg->next(pid);
}

dbObject* dbCCSegItr::getObject(uint id, ...)
{
  return _seg_tbl->getPtr(id);
}

}  // namespace odb
