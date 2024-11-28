/* Authors: Osama */
/*
 * Copyright (c) 2022, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
