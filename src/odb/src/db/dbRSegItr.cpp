// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbRSegItr.h"

#include <cstdint>

#include "dbBlock.h"
#include "dbNet.h"
#include "dbRSeg.h"
#include "dbTable.h"
#include "odb/dbObject.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbRSegItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbRSegItr::reversible() const
{
  return true;
}

bool dbRSegItr::orderReversed() const
{
  return true;
}

void dbRSegItr::reverse(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  uint32_t id = net->r_segs_;
  uint32_t list = 0;

  while (id != 0) {
    _dbRSeg* seg = _seg_tbl->getPtr(id);
    uint32_t n = seg->next_;
    seg->next_ = list;
    list = id;
    id = n;
  }

  net->r_segs_ = list;
}

uint32_t dbRSegItr::sequential() const
{
  return 0;
}

uint32_t dbRSegItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbRSegItr::begin(parent); id != dbRSegItr::end(parent);
       id = dbRSegItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbRSegItr::begin(dbObject* parent) const
{
  _dbNet* net = (_dbNet*) parent;
  if (net->r_segs_ == 0) {
    return 0;
  }
  _dbRSeg* seg = _seg_tbl->getPtr(net->r_segs_);
  return seg->next_;
}

uint32_t dbRSegItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbRSegItr::next(uint32_t id, ...) const
{
  _dbRSeg* seg = _seg_tbl->getPtr(id);
  return seg->next_;
}

dbObject* dbRSegItr::getObject(uint32_t id, ...)
{
  return _seg_tbl->getPtr(id);
}

}  // namespace odb
