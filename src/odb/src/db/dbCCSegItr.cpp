// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbCCSegItr.h"

#include <cstdarg>

#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCapNode.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/dbObject.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbCCSegItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbCCSegItr::reversible() const
{
  return true;
}

bool dbCCSegItr::orderReversed() const
{
  return true;
}

void dbCCSegItr::reverse(dbObject* parent)
{
  _dbCapNode* node = (_dbCapNode*) parent;
  uint id = node->cc_segs_;
  uint pid = parent->getId();
  uint list = 0;

  while (id != 0) {
    _dbCCSeg* seg = seg_tbl_->getPtr(id);
    uint n = seg->next(pid);
    seg->next(pid) = list;
    list = id;
    id = n;
  }

  node->cc_segs_ = list;
}

uint dbCCSegItr::sequential() const
{
  return 0;
}

uint dbCCSegItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbCCSegItr::begin(parent); id != dbCCSegItr::end(parent);
       id = dbCCSegItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbCCSegItr::begin(dbObject* parent) const
{
  _dbCapNode* node = (_dbCapNode*) parent;
  return node->cc_segs_;
}

uint dbCCSegItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbCCSegItr::next(uint id, ...) const
{
  va_list ap;
  va_start(ap, id);
  uint pid = va_arg(ap, uint);
  va_end(ap);
  _dbCCSeg* seg = seg_tbl_->getPtr(id);
  return seg->next(pid);
}

dbObject* dbCCSegItr::getObject(uint id, ...)
{
  return seg_tbl_->getPtr(id);
}

}  // namespace odb
