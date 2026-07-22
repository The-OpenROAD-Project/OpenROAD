// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbCCSegItr.h"

#include <cstdarg>
#include <cstdint>

#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCapNode.h"
#include "dbTable.h"
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
  uint32_t id = node->cc_segs_;
  uint32_t pid = parent->getId();
  uint32_t list = 0;

  while (id != 0) {
    _dbCCSeg* seg = seg_tbl_->getPtr(id);
    uint32_t n = seg->next(pid);
    seg->next(pid) = list;
    list = id;
    id = n;
  }

  node->cc_segs_ = list;
}

uint32_t dbCCSegItr::sequential() const
{
  return 0;
}

uint32_t dbCCSegItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbCCSegItr::begin(parent); id != dbCCSegItr::end(parent);
       id = dbCCSegItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbCCSegItr::begin(dbObject* parent) const
{
  _dbCapNode* node = (_dbCapNode*) parent;
  return node->cc_segs_;
}

uint32_t dbCCSegItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbCCSegItr::next(uint32_t id, ...) const
{
  va_list ap;
  va_start(ap, id);
  uint32_t pid = va_arg(ap, uint32_t);
  va_end(ap);
  _dbCCSeg* seg = seg_tbl_->getPtr(id);
  return seg->next(pid);
}

dbObject* dbCCSegItr::getObject(uint32_t id, ...)
{
  return seg_tbl_->getPtr(id);
}

}  // namespace odb
