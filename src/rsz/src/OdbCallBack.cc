// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "rsz/OdbCallBack.hh"

#include "BottleneckAnalysis.hh"
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
using sta::Pin;

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

void OdbCallBack::inDbITermDestroy(odb::dbITerm* iterm)
{
  Pin* pin = db_network_->dbToSta(iterm);
  resizer_->bottleneck_analysis_->pinRemoved(pin);
}

void OdbCallBack::inDbBTermDestroy(odb::dbBTerm* bterm)
{
  Pin* pin = db_network_->dbToSta(bterm);
  resizer_->bottleneck_analysis_->pinRemoved(pin);
}

}  // namespace rsz
