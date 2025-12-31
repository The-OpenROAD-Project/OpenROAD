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

#include "OdbCallBack.h"

#include <memory>

#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

namespace est {

using sta::dbNetwork;
using sta::Instance;
using sta::InstancePinIterator;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::Network;
using sta::Pin;

OdbCallBack::OdbCallBack(est::EstimateParasitics* estimate_parasitics,
                         Network* network,
                         dbNetwork* db_network)
    : estimate_parasitics_(estimate_parasitics),
      network_(network),
      db_network_(db_network)
{
}

void OdbCallBack::inDbInstCreate(odb::dbInst* inst)
{
  debugPrint(estimate_parasitics_->getLogger(),
             utl::EST,
             "odb",
             1,
             "inDbInstCreate {}",
             inst->getName());
}

void OdbCallBack::inDbNetCreate(odb::dbNet* net)
{
  debugPrint(estimate_parasitics_->getLogger(),
             utl::EST,
             "odb",
             1,
             "inDbNetCreate {}",
             net->getName());
  estimate_parasitics_->parasiticsInvalid(net);
}

void OdbCallBack::inDbNetDestroy(odb::dbNet* net)
{
  debugPrint(estimate_parasitics_->getLogger(),
             utl::EST,
             "odb",
             1,
             "inDbNetDestroy {}",
             net->getName());
  Net* sta_net = db_network_->dbToSta(net);
  if (sta_net) {
    estimate_parasitics_->eraseParasitics(sta_net);
  }
}

void OdbCallBack::inDbITermPostConnect(odb::dbITerm* iterm)
{
  debugPrint(estimate_parasitics_->getLogger(),
             utl::EST,
             "odb",
             1,
             "inDbITermPostConnect iterm {}",
             iterm->getName());
  odb::dbNet* db_net = iterm->getNet();
  if (db_net) {
    estimate_parasitics_->parasiticsInvalid(db_net);
  }
}

void OdbCallBack::inDbITermPostDisconnect(odb::dbITerm* iterm, odb::dbNet* net)
{
  debugPrint(estimate_parasitics_->getLogger(),
             utl::EST,
             "odb",
             1,
             "inDbITermPostDisconnect iterm={} net={}",
             iterm->getName(),
             net->getName());
  estimate_parasitics_->parasiticsInvalid(net);
}

void OdbCallBack::inDbInstSwapMasterAfter(odb::dbInst* inst)
{
  debugPrint(estimate_parasitics_->getLogger(),
             utl::EST,
             "odb",
             1,
             "inDbInstSwapMasterAfter {}",
             inst->getName());
  Instance* sta_inst = db_network_->dbToSta(inst);

  // Invalidate estimated parasitics on all instance pins.
  std::unique_ptr<InstancePinIterator> pin_iter{
      network_->pinIterator(sta_inst)};
  while (pin_iter->hasNext()) {
    Pin* pin = pin_iter->next();
    Net* net = network_->net(pin);

    const sta::LibertyPort* port = network_->libertyPort(pin);
    // Tristate nets have multiple drivers and this is drivers^2 if
    // the parasitics are updated for each resize.
    if (!port || !port->direction()->isAnyTristate()) {
      // we can only update parasitics for flat net
      odb::dbNet* db_net = db_network_->findFlatDbNet(net);
      estimate_parasitics_->parasiticsInvalid(db_network_->dbToSta(db_net));
    }
  }
}

}  // namespace est
