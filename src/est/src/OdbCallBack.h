// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "sta/Network.hh"

namespace est {

class EstimateParasitics;

using sta::dbNetwork;
using sta::Network;

class OdbCallBack : public odb::dbBlockCallBackObj
{
 public:
  OdbCallBack(EstimateParasitics* estimate_parasitics,
              Network* network,
              dbNetwork* db_network);

  void inDbInstCreate(odb::dbInst* inst) override;
  void inDbNetCreate(odb::dbNet* net) override;
  void inDbNetDestroy(odb::dbNet* net) override;
  void inDbITermPostConnect(odb::dbITerm* iterm) override;
  void inDbITermPostDisconnect(odb::dbITerm* iterm, odb::dbNet* net) override;
  void inDbInstSwapMasterAfter(odb::dbInst* inst) override;

 private:
  EstimateParasitics* estimate_parasitics_;
  Network* network_;
  dbNetwork* db_network_;
};

}  // namespace est
