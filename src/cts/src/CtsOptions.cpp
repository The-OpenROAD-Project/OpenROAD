// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "CtsOptions.h"

#include <algorithm>

#include "odb/db.h"

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

void CtsOptions::limitSinkClusteringSizes(unsigned limit)
{
  if (sinkClustersSizeSet_) {
    setSinkClusteringSize(std::min(limit, sinkClustersSize_));
    return;
  }
  auto lowerBound = std::ranges::lower_bound(sinkClusteringSizes_, limit);
  sinkClusteringSizes_.erase(lowerBound, sinkClusteringSizes_.end());
  sinkClusteringSizes_.push_back(limit);
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
