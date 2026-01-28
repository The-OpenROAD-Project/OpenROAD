// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbCapNodeItr.h"

#include <cstdint>

#include "dbBlock.h"
#include "dbCapNode.h"
#include "dbNet.h"
#include "dbTable.h"
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
  uint32_t id = net->cap_nodes_;
  uint32_t list = 0;

  while (id != 0) {
    _dbCapNode* seg = seg_tbl_->getPtr(id);
    uint32_t n = seg->next_;
    seg->next_ = list;
    list = id;
    id = n;
  }

  net->cap_nodes_ = list;
}

uint32_t dbCapNodeItr::sequential() const
{
  return 0;
}

uint32_t dbCapNodeItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbCapNodeItr::begin(parent); id != dbCapNodeItr::end(parent);
       id = dbCapNodeItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbCapNodeItr::begin(dbObject* parent) const
{
  _dbNet* net = (_dbNet*) parent;
  return net->cap_nodes_;
}

uint32_t dbCapNodeItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbCapNodeItr::next(uint32_t id, ...) const
{
  _dbCapNode* seg = seg_tbl_->getPtr(id);
  return seg->next_;
}

dbObject* dbCapNodeItr::getObject(uint32_t id, ...)
{
  return seg_tbl_->getPtr(id);
}

}  // namespace odb
