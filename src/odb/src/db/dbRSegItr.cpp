// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbRSegItr.h"

#include "dbBlock.h"
#include "dbNet.h"
#include "dbRSeg.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbRSegItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbRSegItr::reversible()
{
  return true;
}

bool dbRSegItr::orderReversed()
{
  return true;
}

void dbRSegItr::reverse(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  uint id = net->_r_segs;
  uint list = 0;

  while (id != 0) {
    _dbRSeg* seg = _seg_tbl->getPtr(id);
    uint n = seg->_next;
    seg->_next = list;
    list = id;
    id = n;
  }

  net->_r_segs = list;
}

uint dbRSegItr::sequential()
{
  return 0;
}

uint dbRSegItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbRSegItr::begin(parent); id != dbRSegItr::end(parent);
       id = dbRSegItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbRSegItr::begin(dbObject* parent)
{
  _dbNet* net = (_dbNet*) parent;
  if (net->_r_segs == 0) {
    return 0;
  }
  _dbRSeg* seg = _seg_tbl->getPtr(net->_r_segs);
  return seg->_next;
}

uint dbRSegItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbRSegItr::next(uint id, ...)
{
  _dbRSeg* seg = _seg_tbl->getPtr(id);
  return seg->_next;
}

dbObject* dbRSegItr::getObject(uint id, ...)
{
  return _seg_tbl->getPtr(id);
}

}  // namespace odb
