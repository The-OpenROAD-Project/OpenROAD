// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "dbGroupPowerNetItr.h"

#include <algorithm>

#include "dbBlock.h"
#include "dbGroup.h"
#include "dbNet.h"
#include "dbTable.h"

namespace odb {

bool dbGroupPowerNetItr::reversible()
{
  return true;
}

bool dbGroupPowerNetItr::orderReversed()
{
  return false;
}

void dbGroupPowerNetItr::reverse(dbObject* parent)
{
  _dbGroup* group = (_dbGroup*) parent;
  std::reverse(group->_power_nets.begin(), group->_power_nets.end());
}

uint dbGroupPowerNetItr::sequential()
{
  return 0;
}

uint dbGroupPowerNetItr::size(dbObject* parent)
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->_power_nets.size();
}

uint dbGroupPowerNetItr::begin(dbObject*)
{
  return 0;
}

uint dbGroupPowerNetItr::end(dbObject* parent)
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->_power_nets.size();
}

uint dbGroupPowerNetItr::next(uint id, ...)
{
  return ++id;
}

dbObject* dbGroupPowerNetItr::getObject(uint id, ...)
{
  va_list ap;
  va_start(ap, id);
  _dbGroup* parent = (_dbGroup*) va_arg(ap, dbObject*);
  va_end(ap);
  uint nid = parent->_power_nets[id];
  return _net_tbl->getPtr(nid);
}

}  // namespace odb
