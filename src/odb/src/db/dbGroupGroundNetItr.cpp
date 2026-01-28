// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "dbGroupGroundNetItr.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>

#include "dbBlock.h"
#include "dbGroup.h"
#include "dbNet.h"
#include "dbTable.h"
#include "odb/dbObject.h"

namespace odb {

bool dbGroupGroundNetItr::reversible() const
{
  return true;
}

bool dbGroupGroundNetItr::orderReversed() const
{
  return false;
}

void dbGroupGroundNetItr::reverse(dbObject* parent)
{
  _dbGroup* group = (_dbGroup*) parent;
  std::ranges::reverse(group->ground_nets_);
}

uint32_t dbGroupGroundNetItr::sequential() const
{
  return 0;
}

uint32_t dbGroupGroundNetItr::size(dbObject* parent) const
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->ground_nets_.size();
}

uint32_t dbGroupGroundNetItr::begin(dbObject*) const
{
  return 0;
}

uint32_t dbGroupGroundNetItr::end(dbObject* parent) const
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->ground_nets_.size();
}

uint32_t dbGroupGroundNetItr::next(uint32_t id, ...) const
{
  return ++id;
}

dbObject* dbGroupGroundNetItr::getObject(uint32_t id, ...)
{
  va_list ap;
  va_start(ap, id);
  _dbGroup* parent = (_dbGroup*) va_arg(ap, dbObject*);
  va_end(ap);
  uint32_t nid = parent->ground_nets_[id];
  return net_tbl_->getPtr(nid);
}

}  // namespace odb
