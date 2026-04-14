// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2026, The OpenROAD Authors

#include "odb/dbChipletCallBackObj.h"

#include "dbDatabase.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipletCallBackObj - Methods
//
////////////////////////////////////////////////////////////////////

dbChipletCallBackObj::~dbChipletCallBackObj()
{
  removeOwner();
}

void dbChipletCallBackObj::addOwner(dbDatabase* db)
{
  if (db == nullptr) {
    return;
  }

  if (owner_ != nullptr) {
    removeOwner();
  }

  _dbDatabase* impl = (_dbDatabase*) db;
  impl->chiplet_callbacks_.push_back(this);
  owner_ = db;
}

void dbChipletCallBackObj::removeOwner()
{
  if (owner_ != nullptr) {
    _dbDatabase* impl = (_dbDatabase*) owner_;
    impl->chiplet_callbacks_.remove(this);
    owner_ = nullptr;
  }
}

}  // namespace odb
