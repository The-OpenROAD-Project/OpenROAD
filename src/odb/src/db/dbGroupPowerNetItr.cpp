// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "dbGroupPowerNetItr.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>

#include "dbBlock.h"
#include "dbGroup.h"
#include "dbNet.h"
#include "dbTable.h"
#include "odb/dbObject.h"

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
  std::ranges::reverse(group->power_nets_);
}

uint32_t dbGroupPowerNetItr::sequential() const
{
  return 0;
}

uint32_t dbGroupPowerNetItr::size(dbObject* parent) const
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->power_nets_.size();
}

uint32_t dbGroupPowerNetItr::begin(dbObject*) const
{
  return 0;
}

uint32_t dbGroupPowerNetItr::end(dbObject* parent) const
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->power_nets_.size();
}

uint32_t dbGroupPowerNetItr::next(uint32_t id, ...) const
{
  return ++id;
}

dbObject* dbGroupPowerNetItr::getObject(uint32_t id, ...)
{
  va_list ap;
  va_start(ap, id);
  _dbGroup* parent = (_dbGroup*) va_arg(ap, dbObject*);
  va_end(ap);
  uint32_t nid = parent->power_nets_[id];
  return net_tbl_->getPtr(nid);
}

}  // namespace odb
