// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

namespace rsz {

using odb::dbBlockCallBackObj;
using odb::dbInst;
using odb::dbITerm;
using odb::dbNet;
using sta::dbNetwork;
using sta::Network;

class Resizer;

class OdbCallBack : public dbBlockCallBackObj
{
 public:
  OdbCallBack(Resizer* resizer, Network* network, dbNetwork* db_network);

  void inDbInstCreate(dbInst* inst) override;
  void inDbNetCreate(dbNet* net) override;
  void inDbNetDestroy(dbNet* net) override;
  void inDbITermPostConnect(dbITerm* iterm) override;
  void inDbITermPostDisconnect(dbITerm* iterm, dbNet* net) override;
  void inDbInstSwapMasterAfter(dbInst* inst) override;

 private:
  Resizer* resizer_;
  Network* network_;
  dbNetwork* db_network_;
};

}  // namespace rsz
