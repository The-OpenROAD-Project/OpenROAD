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

void DesignCallBack::inDbInstCreate(odb::dbInst* db_inst)
{
  auto design = router_->getDesign();
  frMaster* master = design->getMaster(db_inst->getMaster()->getName());
  auto inst = std::make_unique<frInst>(master, db_inst);
  int numInstTerms = 0;
  inst->setPinAccessIdx(db_inst->getPinAccessIdx());
  for (auto& term : inst->getMaster()->getTerms()) {
    std::unique_ptr<frInstTerm> instTerm
        = std::make_unique<frInstTerm>(inst.get(), term.get());
    instTerm->setIndexInOwner(numInstTerms++);
    int pinCnt = term->getPins().size();
    instTerm->setAPSize(pinCnt);
    inst->addInstTerm(std::move(instTerm));
  }
  for (auto& blk : inst->getMaster()->getBlockages()) {
    std::unique_ptr<frInstBlockage> instBlk
        = std::make_unique<frInstBlockage>(inst.get(), blk.get());
    inst->addInstBlockage(std::move(instBlk));
  }
  design->getTopBlock()->addInst(std::move(inst));
}

void DesignCallBack::inDbITermPreConnect(odb::dbITerm* db_iterm,
                                         odb::dbNet* db_net)
{
}

}  // namespace drt
