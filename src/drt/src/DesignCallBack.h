// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
namespace drt {
class TritonRoute;
}
namespace drt {
class DesignCallBack : public odb::dbBlockCallBackObj
{
 public:
  DesignCallBack(TritonRoute* router) : router_(router) {}
  void inDbInstCreate(odb::dbInst*) override;
  void inDbPreMoveInst(odb::dbInst* inst) override;
  void inDbPostMoveInst(odb::dbInst* inst) override;
  void inDbInstDestroy(odb::dbInst* inst) override;
  void inDbInstSwapMasterBefore(odb::dbInst*, odb::dbMaster*) override;
  void inDbInstSwapMasterAfter(odb::dbInst*) override;
  void inDbNetCreate(odb::dbNet* net) override;
  void inDbNetDestroy(odb::dbNet* net) override;
  void inDbITermPostDisconnect(odb::dbITerm* iterm, odb::dbNet* net) override;
  void inDbITermPostConnect(odb::dbITerm* iterm) override;

 private:
  TritonRoute* router_;
};
}  // namespace drt
