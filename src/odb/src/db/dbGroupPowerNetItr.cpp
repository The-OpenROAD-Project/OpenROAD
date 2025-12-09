// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "dbGroupPowerNetItr.h"

#include <algorithm>
#include <cstdarg>

#include "dbBlock.h"
#include "dbGroup.h"
#include "dbNet.h"
#include "dbTable.h"
#include "odb/dbObject.h"
#include "odb/odb.h"

namespace odb {

bool dbGroupPowerNetItr::reversible() const
{
  return true;
}

bool dbGroupPowerNetItr::orderReversed() const
{
  return false;
}

void dbGroupPowerNetItr::reverse(dbObject* parent)
{
  _dbGroup* group = (_dbGroup*) parent;
  std::reverse(group->power_nets_.begin(), group->power_nets_.end());
}

uint dbGroupPowerNetItr::sequential() const
{
  return 0;
}

uint dbGroupPowerNetItr::size(dbObject* parent) const
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->power_nets_.size();
}

uint dbGroupPowerNetItr::begin(dbObject*) const
{
  return 0;
}

uint dbGroupPowerNetItr::end(dbObject* parent) const
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->power_nets_.size();
}

uint dbGroupPowerNetItr::next(uint id, ...) const
{
  return ++id;
}

dbObject* dbGroupPowerNetItr::getObject(uint id, ...)
{
  va_list ap;
  va_start(ap, id);
  _dbGroup* parent = (_dbGroup*) va_arg(ap, dbObject*);
  va_end(ap);
  uint nid = parent->power_nets_[id];
  return net_tbl_->getPtr(nid);
}

}  // namespace odb
