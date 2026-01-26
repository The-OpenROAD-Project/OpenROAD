// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbChipCallBackObj.h"

#include "dbChip.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipCallBackObj - Methods
//
////////////////////////////////////////////////////////////////////

void dbChipCallBackObj::addOwner(dbChip* new_owner)
{
  if (!new_owner) {
    return;
  }

  if (_owner) {
    removeOwner();
  }

  _dbChip* chip = (_dbChip*) new_owner;
  chip->callbacks_.insert(chip->callbacks_.end(), this);
  _owner = new_owner;
}

void dbChipCallBackObj::removeOwner()
{
  if (_owner) {
    _dbChip* chip = (_dbChip*) _owner;
    chip->callbacks_.remove(this);
    _owner = nullptr;
  }
}

}  // namespace odb
