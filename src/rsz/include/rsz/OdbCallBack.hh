// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "sta/Network.hh"

namespace rsz {

class Resizer;

class OdbCallBack : public odb::dbBlockCallBackObj
{
 public:
  OdbCallBack(Resizer* resizer,
              sta::Network* network,
              sta::dbNetwork* db_network);

  void inDbNetDestroy(odb::dbNet* net) override;

 private:
  Resizer* resizer_;
  sta::Network* network_;
  sta::dbNetwork* db_network_;
};

}  // namespace rsz
