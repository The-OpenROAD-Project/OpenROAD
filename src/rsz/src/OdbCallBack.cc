// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

//
//  These are callback functions to keep track of nets that need parasitics
//  update after 5 optimization transforms:
//  1. buffer insertion
//  2. buffer/cell removal
//  3. cell sizing
//  4. pin swapping
//  5. gate cloning
//
//

#include "rsz/OdbCallBack.hh"

#include "db_sta/dbNetwork.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

namespace rsz {

using sta::dbNetwork;
using sta::InstancePinIterator;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::Network;

OdbCallBack::OdbCallBack(Resizer* resizer,
                         Network* network,
                         dbNetwork* db_network)
    : resizer_(resizer), network_(network), db_network_(db_network)
{
}

void OdbCallBack::inDbNetDestroy(odb::dbNet* net)
{
  debugPrint(resizer_->logger(),
             utl::RSZ,
             "odb",
             1,
             "inDbNetDestroy {}",
             net->getName());
  Net* sta_net = db_network_->dbToSta(net);
  if (resizer_->net_slack_map_.count(sta_net)) {
    resizer_->net_slack_map_.erase(sta_net);
  }
}

}  // namespace rsz
