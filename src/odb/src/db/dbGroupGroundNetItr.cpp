// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "dbGroupGroundNetItr.h"

#include <algorithm>
#include <cstdarg>

#include "dbBlock.h"
#include "dbGroup.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/dbObject.h"
#include "odb/odb.h"

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
  std::reverse(group->ground_nets_.begin(), group->ground_nets_.end());
}

uint dbGroupGroundNetItr::sequential() const
{
  return 0;
}

uint dbGroupGroundNetItr::size(dbObject* parent) const
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->ground_nets_.size();
}

uint dbGroupGroundNetItr::begin(dbObject*) const
{
  return 0;
}

uint dbGroupGroundNetItr::end(dbObject* parent) const
{
  _dbGroup* group = (_dbGroup*) parent;
  return group->ground_nets_.size();
}

uint dbGroupGroundNetItr::next(uint id, ...) const
{
  return ++id;
}

dbObject* dbGroupGroundNetItr::getObject(uint id, ...)
{
  va_list ap;
  va_start(ap, id);
  _dbGroup* parent = (_dbGroup*) va_arg(ap, dbObject*);
  va_end(ap);
  uint nid = parent->ground_nets_[id];
  return net_tbl_->getPtr(nid);
}

}  // namespace odb
