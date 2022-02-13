#include "DesignCallBack.h"

#include "frDesign.h"
#include "triton_route/TritonRoute.h"
using namespace fr;

static inline int defdist(odb::dbBlock* block, int x)
{
  return x * (double) block->getDefUnits()
         / (double) block->getDbUnitsPerMicron();
}

void DesignCallBack::inDbPostMoveInst(odb::dbInst* db_inst)
{
  auto design = router_->getDesign();
  if (design != nullptr && design->getTopBlock() != nullptr) {
    auto inst = design->getTopBlock()->getInst(db_inst->getName());
    if (inst == nullptr)
      return;
    if (design->getRegionQuery() != nullptr)
      design->getRegionQuery()->removeBlockObj(inst);
    int x, y;
    auto block = db_inst->getBlock();
    db_inst->getLocation(x, y);
    x = defdist(block, x);
    y = defdist(block, y);
    inst->setOrigin({x, y});
    inst->setOrient(db_inst->getOrient());
    if (design->getRegionQuery() != nullptr)
      design->getRegionQuery()->addBlockObj(inst);
  }
}
