// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBlockItr.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>

#include "dbBlock.h"
#include "dbTable.h"
#include "odb/dbObject.h"

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
  std::ranges::reverse(block->children_);
}

uint32_t dbBlockItr::sequential() const
{
  return 0;
}

uint32_t dbBlockItr::size(dbObject* parent) const
{
  _dbBlock* block = (_dbBlock*) parent;
  return block->children_.size();
}

uint32_t dbBlockItr::begin(dbObject*) const
{
  return 0;
}

uint32_t dbBlockItr::end(dbObject* parent) const
{
  _dbBlock* block = (_dbBlock*) parent;
  return block->children_.size();
}

uint32_t dbBlockItr::next(uint32_t id, ...) const
{
  return ++id;
}

// dbBlockItr.cpp:54:5: warning: undefined behavior when second parameter of
// ‘va_start’ is declared with ‘register’ storage [-Wvarargs]
//     va_start(ap,id);

// dbObject * dbBlockItr::getObject( uint32_t id, ... )
dbObject* dbBlockItr::getObject(uint32_t id, ...)
{
  va_list ap;
  va_start(ap, id);
  _dbBlock* parent = (_dbBlock*) va_arg(ap, dbObject*);
  va_end(ap);
  uint32_t cid = parent->children_[id];
  return block_tbl_->getPtr(cid);
}

}  // namespace odb
