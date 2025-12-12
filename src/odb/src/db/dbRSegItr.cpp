// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbRSegItr.h"

#include "dbBlock.h"
#include "dbNet.h"
#include "dbRSeg.h"
#include "dbTable.h"
#include "dbTable.hpp"
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
  uint id = net->r_segs_;
  uint list = 0;

  while (id != 0) {
    _dbRSeg* seg = _seg_tbl->getPtr(id);
    uint n = seg->next_;
    seg->next_ = list;
    list = id;
    id = n;
  }

  net->r_segs_ = list;
}

uint dbRSegItr::sequential() const
{
  return 0;
}

uint dbRSegItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbRSegItr::begin(parent); id != dbRSegItr::end(parent);
       id = dbRSegItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbRSegItr::begin(dbObject* parent) const
{
  _dbNet* net = (_dbNet*) parent;
  if (net->r_segs_ == 0) {
    return 0;
  }
  _dbRSeg* seg = _seg_tbl->getPtr(net->r_segs_);
  return seg->next_;
}

uint dbRSegItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbRSegItr::next(uint id, ...) const
{
  _dbRSeg* seg = _seg_tbl->getPtr(id);
  return seg->next_;
}

dbObject* dbRSegItr::getObject(uint id, ...)
{
  return _seg_tbl->getPtr(id);
}

}  // namespace odb
