// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "dbGroupGroundNetItr.h"

#include <algorithm>

#include "dbBlock.h"
#include "dbGroup.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

bool dbGroupGroundNetItr::reversible()
{
  return true;
}

bool dbGroupGroundNetItr::orderReversed()
{
  return false;
}

void dbGroupGroundNetItr::reverse(dbObject* parent)
{
  _dbGroup* group = (_dbGroup*) parent;
  std::reverse(group->_ground_nets.begin(), group->_ground_nets.end());
}

uint dbGroupGroundNetItr::sequential()
{
  return 0;
}

uint dbGroupGroundNetItr::size(dbObject* parent)
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->_ground_nets.size();
}

uint dbGroupGroundNetItr::begin(dbObject*)
{
  return 0;
}

uint dbGroupGroundNetItr::end(dbObject* parent)
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->_ground_nets.size();
}

uint dbGroupGroundNetItr::next(uint id, ...)
{
  return ++id;
}

dbObject* dbGroupGroundNetItr::getObject(uint id, ...)
{
  va_list ap;
  va_start(ap, id);
  _dbGroup* parent = (_dbGroup*) va_arg(ap, dbObject*);
  va_end(ap);
  uint nid = parent->_ground_nets[id];
  return _net_tbl->getPtr(nid);
}

}  // namespace odb
