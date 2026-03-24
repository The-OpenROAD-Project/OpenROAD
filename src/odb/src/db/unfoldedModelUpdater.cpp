// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "odb/unfoldedModelUpdater.h"

#include <mutex>
#include <unordered_set>
#include <vector>

#include "odb/db.h"
#include "odb/unfoldedModel.h"

namespace odb {

UnfoldedModelUpdater::UnfoldedModelUpdater(UnfoldedModel* model) : model_(model)
{
}

void UnfoldedModelUpdater::inDbChipInstCreate(dbChipInst* inst)
{
  UnfoldedModel::WriteGuard guard(model_);
  model_->addChipInst(inst);
}

void UnfoldedModelUpdater::inDbChipInstDestroy(dbChipInst* inst)
{
  UnfoldedModel::WriteGuard guard(model_);
  model_->removeChipInst(inst);
}

void UnfoldedModelUpdater::inDbChipInstPreModify(dbChipInst* inst)
{
  // Record that a modification is in flight for this instance
  std::lock_guard map_guard(pending_modifies_mutex_);
  pending_modifies_.insert(inst);
}

void UnfoldedModelUpdater::inDbChipInstPostModify(dbChipInst* inst)
{
  {
    // Bail out if no matching PreModify was received
    std::lock_guard map_guard(pending_modifies_mutex_);
    auto it = pending_modifies_.find(inst);
    if (it == pending_modifies_.end()) {
      return;
    }
    pending_modifies_.erase(it);
  }
  UnfoldedModel::WriteGuard guard(model_);
  model_->recomputeSubtreeTransforms(inst);
}

void UnfoldedModelUpdater::inDbChipPostModify(dbChip* chip)
{
  UnfoldedModel::WriteGuard guard(model_);
  model_->updateChipGeometry(chip);
}

void UnfoldedModelUpdater::inDbChipConnCreate(dbChipConn* conn)
{
  UnfoldedModel::WriteGuard guard(model_);
  model_->addConnection(conn);
}

void UnfoldedModelUpdater::inDbChipConnDestroy(dbChipConn* conn)
{
  UnfoldedModel::WriteGuard guard(model_);
  model_->removeConnection(conn);
}

void UnfoldedModelUpdater::inDbChipNetCreate(dbChipNet* net)
{
  UnfoldedModel::WriteGuard guard(model_);
  model_->addNet(net);
}

void UnfoldedModelUpdater::inDbChipNetDestroy(dbChipNet* net)
{
  UnfoldedModel::WriteGuard guard(model_);
  model_->removeNet(net);
}

void UnfoldedModelUpdater::inDbChipNetAddBumpInst(
    dbChipNet* net,
    dbChipBumpInst* bump_inst,
    const std::vector<dbChipInst*>& path)
{
  UnfoldedModel::WriteGuard guard(model_);
  model_->addBumpInstToNet(net, bump_inst, path);
}

void UnfoldedModelUpdater::inDbChipRegionPostModify(dbChipRegion* region)
{
  UnfoldedModel::WriteGuard guard(model_);
  model_->updateRegionGeometry(region);
}

void UnfoldedModelUpdater::inDbChipBumpCreate(dbChipBump* bump)
{
  UnfoldedModel::WriteGuard guard(model_);
  model_->addBump(bump);
}

void UnfoldedModelUpdater::inDbChipBumpDestroy(dbChipBump* bump)
{
  UnfoldedModel::WriteGuard guard(model_);
  model_->removeBump(bump);
}

}  // namespace odb
