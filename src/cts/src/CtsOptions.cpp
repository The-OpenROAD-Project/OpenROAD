// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "CtsOptions.h"

namespace cts {

CtsOptions::MasterType CtsOptions::getType(odb::dbInst* inst) const
{
  if (inst->getName().substr(0, dummyload_prefix_.size())
      == dummyload_prefix_) {
    return MasterType::DUMMY;
  }

  return MasterType::TREE;
}

void CtsOptions::inDbInstCreate(odb::dbInst* inst)
{
  recordBuffer(inst->getMaster(), getType(inst));
}

void CtsOptions::inDbInstCreate(odb::dbInst* inst, odb::dbRegion* region)
{
  recordBuffer(inst->getMaster(), getType(inst));
}

void CtsOptions::recordBuffer(odb::dbMaster* master, MasterType type)
{
  switch (type) {
    case MasterType::DUMMY:
      dummy_count_[master]++;
      break;
    case MasterType::TREE:
      buffer_count_[master]++;
      break;
  }
}

}  // namespace cts
