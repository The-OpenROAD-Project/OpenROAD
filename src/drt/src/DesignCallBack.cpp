// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "DesignCallBack.h"

#include "frDesign.h"
#include "triton_route/TritonRoute.h"

namespace drt {

static inline int defdist(odb::dbBlock* block, int x)
{
  return x * (double) block->getDefUnits()
         / (double) block->getDbUnitsPerMicron();
}

void DesignCallBack::inDbPreMoveInst(odb::dbInst* db_inst)
{
  auto design = router_->getDesign();
  if (design != nullptr && design->getTopBlock() != nullptr) {
    auto inst = design->getTopBlock()->getInst(db_inst->getName());
    if (inst == nullptr) {
      return;
    }
    if (design->getRegionQuery() != nullptr) {
      design->getRegionQuery()->removeBlockObj(inst);
    }
  }
}

void DesignCallBack::inDbPostMoveInst(odb::dbInst* db_inst)
{
  auto design = router_->getDesign();
  if (design != nullptr && design->getTopBlock() != nullptr) {
    auto inst = design->getTopBlock()->getInst(db_inst->getName());
    if (inst == nullptr) {
      return;
    }
    int x, y;
    db_inst->getLocation(x, y);
    auto block = db_inst->getBlock();
    x = defdist(block, x);
    y = defdist(block, y);
    inst->setOrigin({x, y});
    inst->setOrient(db_inst->getOrient());
    if (design->getRegionQuery() != nullptr) {
      design->getRegionQuery()->addBlockObj(inst);
    }
  }
}

void DesignCallBack::inDbInstDestroy(odb::dbInst* db_inst)
{
  auto design = router_->getDesign();
  if (design != nullptr && design->getTopBlock() != nullptr) {
    auto inst = design->getTopBlock()->getInst(db_inst->getName());
    if (inst == nullptr) {
      return;
    }
    if (design->getRegionQuery() != nullptr) {
      design->getRegionQuery()->removeBlockObj(inst);
    }
    design->getTopBlock()->removeInst(inst);
  }
}

}  // namespace drt
