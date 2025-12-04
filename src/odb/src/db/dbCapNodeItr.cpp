// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbCapNodeItr.h"

#include "dbBlock.h"
#include "dbCapNode.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/dbObject.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbCapNodeItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbCapNodeItr::reversible() const
{
  return true;
}

bool dbCapNodeItr::orderReversed() const
{
  return true;
}

void dbCapNodeItr::reverse(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  uint id = net->cap_nodes_;
  uint list = 0;

  while (id != 0) {
    _dbCapNode* seg = seg_tbl_->getPtr(id);
    uint n = seg->next_;
    seg->next_ = list;
    list = id;
    id = n;
  }

  net->cap_nodes_ = list;
}

uint dbCapNodeItr::sequential() const
{
  return 0;
}

uint dbCapNodeItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbCapNodeItr::begin(parent); id != dbCapNodeItr::end(parent);
       id = dbCapNodeItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbCapNodeItr::begin(dbObject* parent) const
{
  _dbNet* net = (_dbNet*) parent;
  return net->cap_nodes_;
}

uint dbCapNodeItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbCapNodeItr::next(uint id, ...) const
{
  _dbCapNode* seg = seg_tbl_->getPtr(id);
  return seg->next_;
}

dbObject* dbCapNodeItr::getObject(uint id, ...)
{
  return seg_tbl_->getPtr(id);
}

}  // namespace odb
