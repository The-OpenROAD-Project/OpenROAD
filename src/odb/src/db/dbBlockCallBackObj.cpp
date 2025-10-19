// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbBlockCallBackObj.h"

#include "dbBlock.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbBlockCallBackObj - Methods
//
////////////////////////////////////////////////////////////////////

void dbBlockCallBackObj::addOwner(dbBlock* new_owner)
{
  if (!new_owner) {
    return;
  }

  if (_owner) {
    removeOwner();
  }

  _dbBlock* block = (_dbBlock*) new_owner;
  block->_callbacks.insert(block->_callbacks.end(), this);
  _owner = new_owner;
}

void dbBlockCallBackObj::removeOwner()
{
  if (_owner) {
    _dbBlock* block = (_dbBlock*) _owner;
    block->_callbacks.remove(this);
    _owner = nullptr;
  }
}

}  // namespace odb
