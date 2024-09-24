/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2024, Precision Innonvations, Inc.
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

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
    resizer_->parasiticsInvalid(net);
  }
}

}  // namespace rsz
