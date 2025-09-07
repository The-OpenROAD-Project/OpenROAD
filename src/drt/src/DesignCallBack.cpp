// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "DesignCallBack.h"

#include "frDesign.h"
#include "odb/db.h"
#include "triton_route/TritonRoute.h"

namespace drt {

void DesignCallBack::inDbPreMoveInst(odb::dbInst* db_inst)
{
  auto design = router_->getDesign();
  if (design == nullptr || design->getTopBlock() == nullptr) {
    return;
  }
  auto inst = design->getTopBlock()->getInst(db_inst->getName());
  if (inst == nullptr) {
    return;
  }
  router_->deleteInstancePAData(inst);
  if (design->getRegionQuery() != nullptr) {
    design->getRegionQuery()->removeBlockObj(inst);
  }
}

void DesignCallBack::inDbPostMoveInst(odb::dbInst* db_inst)
{
  auto design = router_->getDesign();
  if (design == nullptr || design->getTopBlock() == nullptr) {
    return;
  }
  auto inst = design->getTopBlock()->getInst(db_inst->getName());
  if (inst == nullptr) {
    return;
  }
  inst->updateFromDB();
  router_->addInstancePAData(inst);
  if (design->getRegionQuery() != nullptr) {
    design->getRegionQuery()->addBlockObj(inst);
  }
}

void DesignCallBack::inDbInstDestroy(odb::dbInst* db_inst)
{
  auto design = router_->getDesign();
  if (design == nullptr || design->getTopBlock() == nullptr) {
    return;
  }
  auto inst = design->getTopBlock()->getInst(db_inst->getName());
  if (inst == nullptr) {
    return;
  }
  router_->deleteInstancePAData(inst);
  if (design->getRegionQuery() != nullptr) {
    design->getRegionQuery()->removeBlockObj(inst);
  }
  design->getTopBlock()->removeInst(inst);
}

}  // namespace drt
