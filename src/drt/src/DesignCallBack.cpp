// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "DesignCallBack.h"

#include "drt/TritonRoute.h"
#include "frDesign.h"
#include "io/io.h"
#include "odb/db.h"

namespace drt {

void DesignCallBack::inDbInstCreate(odb::dbInst* db_inst)
{
  auto design = router_->getDesign();
  if (design == nullptr || design->getTopBlock() == nullptr) {
    return;
  }
  io::Parser parser(router_->getDb(),
                    design,
                    router_->getLogger(),
                    router_->getRouterConfiguration());

  parser.setInst(db_inst);
}

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
  if (design->getRegionQuery() != nullptr) {
    design->getRegionQuery()->removeBlockObj(inst);
  }
  router_->deleteInstancePAData(inst, false);
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
  int x, y;
  db_inst->getLocation(x, y);
  inst->setOrigin({x, y});
  inst->setOrient(db_inst->getOrient());
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
  if (design->getRegionQuery() != nullptr) {
    design->getRegionQuery()->removeBlockObj(inst);
  }
  design->getTopBlock()->removeInst(inst);
  router_->deleteInstancePAData(inst, true);
}

void DesignCallBack::inDbInstSwapMasterBefore(odb::dbInst* db_inst,
                                              odb::dbMaster* db_master)
{
  inDbInstDestroy(db_inst);
}

void DesignCallBack::inDbInstSwapMasterAfter(odb::dbInst* db_inst)
{
  inDbInstCreate(db_inst);
  inDbPostMoveInst(db_inst);
  for (auto inst_term : db_inst->getITerms()) {
    if (inst_term->getNet()) {
      inDbITermPostConnect(inst_term);
    }
  }
}
void DesignCallBack::inDbNetCreate(odb::dbNet* db_net)
{
  auto design = router_->getDesign();
  if (design == nullptr || design->getTopBlock() == nullptr) {
    return;
  }
  io::Parser parser(router_->getDb(),
                    design,
                    router_->getLogger(),
                    router_->getRouterConfiguration());
  parser.addNet(db_net);
}

void DesignCallBack::inDbNetDestroy(odb::dbNet* db_net)
{
  auto design = router_->getDesign();
  if (design == nullptr || design->getTopBlock() == nullptr) {
    return;
  }
  auto net = design->getTopBlock()->findNet(db_net->getName());
  if (net == nullptr) {
    return;
  }
  design->getTopBlock()->removeNet(net);
}

void DesignCallBack::inDbITermPostDisconnect(odb::dbITerm* db_iterm,
                                             odb::dbNet* db_net)
{
  auto design = router_->getDesign();
  if (design == nullptr || design->getTopBlock() == nullptr) {
    return;
  }
  auto inst = design->getTopBlock()->findInst(db_iterm->getInst());
  auto net = design->getTopBlock()->findNet(db_net->getName());
  if (inst == nullptr || net == nullptr) {
    return;
  }
  auto inst_term = inst->getInstTerm(db_iterm->getMTerm()->getIndex());
  if (inst_term == nullptr) {
    return;
  }
  net->removeInstTerm(inst_term);
  inst_term->addToNet(nullptr);
}

void DesignCallBack::inDbITermPostConnect(odb::dbITerm* db_iterm)
{
  auto design = router_->getDesign();
  if (design == nullptr || design->getTopBlock() == nullptr) {
    return;
  }
  auto inst = design->getTopBlock()->findInst(db_iterm->getInst());
  auto net = design->getTopBlock()->findNet(db_iterm->getNet()->getName());
  if (inst == nullptr || net == nullptr) {
    return;
  }
  auto inst_term = inst->getInstTerm(db_iterm->getMTerm()->getIndex());
  if (inst_term == nullptr) {
    return;
  }
  inst_term->addToNet(net);
  net->addInstTerm(inst_term);
  router_->addInstancePAData(inst);
}

}  // namespace drt
