// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbCapNodeItr.h"

#include "dbBlock.h"
#include "dbCapNode.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbCapNodeItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbCapNodeItr::reversible()
{
  return true;
}

bool dbCapNodeItr::orderReversed()
{
  return true;
}

void dbCapNodeItr::reverse(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  uint id = net->_cap_nodes;
  uint list = 0;

  while (id != 0) {
    _dbCapNode* seg = _seg_tbl->getPtr(id);
    uint n = seg->_next;
    seg->_next = list;
    list = id;
    id = n;
  }

  net->_cap_nodes = list;
}

uint dbCapNodeItr::sequential()
{
  return 0;
}

uint dbCapNodeItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbCapNodeItr::begin(parent); id != dbCapNodeItr::end(parent);
       id = dbCapNodeItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbCapNodeItr::begin(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  return net->_cap_nodes;
}

uint dbCapNodeItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbCapNodeItr::next(uint id, ...)
{
  _dbCapNode* seg = _seg_tbl->getPtr(id);
  return seg->_next;
}

dbObject* dbCapNodeItr::getObject(uint id, ...)
{
  return _seg_tbl->getPtr(id);
}

}  // namespace odb
