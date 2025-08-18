// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "CtsOptions.h"

namespace cts {

void CtsOptions::setApplyNDR(const std::string& strategy)
{
  std::string lower_strategy = strategy;
  std::transform(lower_strategy.begin(),
                 lower_strategy.end(),
                 lower_strategy.begin(),
                 ::tolower);

  if (lower_strategy == "none") {
    ndrStrategy_ = NdrStrategy::NONE;
  } else if (lower_strategy == "root_only") {
    ndrStrategy_ = NdrStrategy::ROOT_ONLY;
  } else if (lower_strategy == "half") {
    ndrStrategy_ = NdrStrategy::HALF;
  } else if (lower_strategy == "full") {
    ndrStrategy_ = NdrStrategy::FULL;
  } else {
    logger_->warn(utl::CTS,
                  33,
                  "Invalid NDR strategy: {}. Defaulting to root_only.",
                  strategy);
    ndrStrategy_ = NdrStrategy::ROOT_ONLY;
  }
}

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
