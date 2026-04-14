// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include <mutex>
#include <unordered_set>
#include <vector>

#include "odb/dbChipletCallBackObj.h"

namespace odb {
class UnfoldedModel;

// Callback consumer that forwards chiplet mutations to
// UnfoldedModel surgical update methods under a unique_lock.
class UnfoldedModelUpdater : public dbChipletCallBackObj
{
 public:
  explicit UnfoldedModelUpdater(UnfoldedModel* model);

  void inDbChipInstCreate(dbChipInst*) override;
  void inDbChipInstDestroy(dbChipInst*) override;
  void inDbChipInstPreModify(dbChipInst*) override;
  void inDbChipInstPostModify(dbChipInst*) override;
  void inDbChipPostModify(dbChip*) override;
  void inDbChipConnCreate(dbChipConn*) override;
  void inDbChipConnDestroy(dbChipConn*) override;
  void inDbChipNetCreate(dbChipNet*) override;
  void inDbChipNetDestroy(dbChipNet*) override;
  void inDbChipNetAddBumpInst(dbChipNet*,
                              dbChipBumpInst*,
                              const std::vector<dbChipInst*>& path) override;
  void inDbChipRegionPostModify(dbChipRegion*) override;
  void inDbChipBumpCreate(dbChipBump*) override;
  void inDbChipBumpDestroy(dbChipBump*) override;

 private:
  UnfoldedModel* model_;
  // Tracks instances with a pending PreModify so PostModify is a no-op
  // if no PreModify was received. Only the presence of the key matters;
  // no value is stored because recomputeSubtreeTransforms recomputes
  // transforms from scratch without needing the old value.
  std::mutex pending_modifies_mutex_;
  std::unordered_set<dbChipInst*> pending_modifies_;
};

}  // namespace odb
