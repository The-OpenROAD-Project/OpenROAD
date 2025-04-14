// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBlockItr.h"

#include <algorithm>

#include "dbBlock.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

bool dbBlockItr::reversible()
{
  return true;
}

bool dbBlockItr::orderReversed()
{
  return false;
}

void dbBlockItr::reverse(dbObject* parent)
{
  _dbBlock* block = (_dbBlock*) parent;
  std::reverse(block->_children.begin(), block->_children.end());
}

uint dbBlockItr::sequential()
{
  return 0;
}

uint dbBlockItr::size(dbObject* parent)
{
  _dbBlock* block = (_dbBlock*) parent;
  return block->_children.size();
}

uint dbBlockItr::begin(dbObject*)
{
  return 0;
}

uint dbBlockItr::end(dbObject* parent)
{
  _dbBlock* block = (_dbBlock*) parent;
  return block->_children.size();
}

uint dbBlockItr::next(uint id, ...)
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
  uint cid = parent->_children[id];
  return _block_tbl->getPtr(cid);
}

}  // namespace odb
