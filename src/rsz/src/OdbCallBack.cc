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

#include <memory>

#include "rsz/Resizer.hh"

namespace rsz {

using sta::dbNetwork;
using sta::Instance;
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

void OdbCallBack::inDbInstCreate(dbInst* inst)
{
  debugPrint(resizer_->logger(),
             utl::RSZ,
             "odb",
             1,
             "inDbInstCreate {}",
             inst->getName());
  Instance* sta_inst = db_network_->dbToSta(inst);
  std::unique_ptr<InstancePinIterator> pin_iter{
      network_->pinIterator(sta_inst)};
  while (pin_iter->hasNext()) {
    Pin* pin = pin_iter->next();
    Net* net = network_->net(pin);
    if (net) {
      resizer_->parasiticsInvalid(net);
    }
  }
}

void OdbCallBack::inDbNetCreate(dbNet* net)
{
  debugPrint(resizer_->logger(),
             utl::RSZ,
             "odb",
             1,
             "inDbNetCreate {}",
             net->getName());
  resizer_->parasiticsInvalid(net);
}

void OdbCallBack::inDbNetDestroy(dbNet* net)
{
  debugPrint(resizer_->logger(),
             utl::RSZ,
             "odb",
             1,
             "inDbNetDestroy {}",
             net->getName());
  Net* sta_net = db_network_->dbToSta(net);
  if (sta_net) {
    resizer_->eraseParasitics(sta_net);
  }
}

void OdbCallBack::inDbITermPostConnect(dbITerm* iterm)
{
  debugPrint(resizer_->logger(),
             utl::RSZ,
             "odb",
             1,
             "inDbITermPostConnect iterm {}",
             iterm->getName());
  dbNet* db_net = iterm->getNet();
  if (db_net) {
    resizer_->parasiticsInvalid(db_net);
  }
}

void OdbCallBack::inDbITermPostDisconnect(dbITerm* iterm, dbNet* net)
{
  debugPrint(resizer_->logger(),
             utl::RSZ,
             "odb",
             1,
             "inDbITermPostDisconnect iterm={} net={}",
             iterm->getName(),
             net->getName());
  resizer_->parasiticsInvalid(net);
}

void OdbCallBack::inDbInstSwapMasterAfter(dbInst* inst)
{
  debugPrint(resizer_->logger(),
             utl::RSZ,
             "odb",
             1,
             "inDbInstSwapMasterAfter {}",
             inst->getName());
  Instance* sta_inst = db_network_->dbToSta(inst);
  std::unique_ptr<InstancePinIterator> pin_iter{
      network_->pinIterator(sta_inst)};
  while (pin_iter->hasNext()) {
    Pin* pin = pin_iter->next();
    Net* net = network_->net(pin);
    // we can only update parasitics for flat net
    odb::dbNet* db_net = db_network_->flatNet(net);
    resizer_->parasiticsInvalid(db_network_->dbToSta(db_net));
  }
}

}  // namespace rsz
