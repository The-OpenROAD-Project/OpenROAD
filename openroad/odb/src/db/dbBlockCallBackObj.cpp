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

  if (owner_) {
    removeOwner();
  }

  _dbBlock* block = (_dbBlock*) new_owner;
  block->callbacks_.insert(block->callbacks_.end(), this);
  owner_ = new_owner;
}

void dbBlockCallBackObj::removeOwner()
{
  if (owner_) {
    _dbBlock* block = (_dbBlock*) owner_;
    block->callbacks_.remove(this);
    owner_ = nullptr;
  }
}

}  // namespace odb
