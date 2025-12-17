// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBlockItr.h"

#include <algorithm>
#include <cstdarg>

#include "dbBlock.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/dbObject.h"
#include "odb/odb.h"

namespace odb {

bool dbBlockItr::reversible() const
{
  return true;
}

bool dbBlockItr::orderReversed() const
{
  return false;
}

void dbBlockItr::reverse(dbObject* parent)
{
  _dbBlock* block = (_dbBlock*) parent;
  std::reverse(block->children_.begin(), block->children_.end());
}

uint dbBlockItr::sequential() const
{
  return 0;
}

uint dbBlockItr::size(dbObject* parent) const
{
  _dbBlock* block = (_dbBlock*) parent;
  return block->children_.size();
}

uint dbBlockItr::begin(dbObject*) const
{
  return 0;
}

uint dbBlockItr::end(dbObject* parent) const
{
  _dbBlock* block = (_dbBlock*) parent;
  return block->children_.size();
}

uint dbBlockItr::next(uint id, ...) const
{
  return ++id;
}

// dbBlockItr.cpp:54:5: warning: undefined behavior when second parameter of
// ‘va_start’ is declared with ‘register’ storage [-Wvarargs]
//     va_start(ap,id);

// dbObject * dbBlockItr::getObject( uint id, ... )
dbObject* dbBlockItr::getObject(uint id, ...)
{
  va_list ap;
  va_start(ap, id);
  _dbBlock* parent = (_dbBlock*) va_arg(ap, dbObject*);
  va_end(ap);
  uint cid = parent->children_[id];
  return block_tbl_->getPtr(cid);
}

}  // namespace odb
