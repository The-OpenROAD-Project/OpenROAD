/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "RemoveBuffer.hh"

#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace rsz {

using std::string;
using utl::RSZ;

using sta::NetPinIterator;
using sta::Pin;
using sta::Port;

RemoveBuffer::RemoveBuffer(Resizer* resizer) : resizer_(resizer)
{
}

void RemoveBuffer::init()
{
  logger_ = resizer_->logger_;
  sta_ = resizer_->sta_;
  db_network_ = resizer_->db_network_;
  copyState(sta_);
}

// There are two buffer removal modes: auto and manual:
// 1) auto mode: this happens during setup fixing, power recovery, buffer
// removal
//      Dont-touch, fixed cell and boundary buffer constraints are honored
// 2) manual mode: this happens during manual buffer removal during ECO
//      This ignores dont-touch, fixed cell and boundary buffer constraints
bool RemoveBuffer::removeBuffer(Instance* buffer, bool honorDontTouchFixed)
{
  init();
  LibertyCell* lib_cell = network_->libertyCell(buffer);
  if (!lib_cell || !lib_cell->isBuffer()) {
    return false;
  }
  dbInst* db_inst = db_network_->staToDb(buffer);
  if (honorDontTouchFixed
      && (db_inst->isDoNotTouch() || db_inst->isFixed() ||
          // Do not remove buffers connected to input/output ports
          // because verilog netlists use the net name for the port.
          bufferBetweenPorts(buffer))) {
    return false;
  }

  LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);
  Pin* in_pin = db_network_->findPin(buffer, in_port);
  Pin* out_pin = db_network_->findPin(buffer, out_port);
  Net* in_net = db_network_->net(in_pin);
  Net* out_net = db_network_->net(out_pin);
  bool out_net_ports = hasPort(out_net);
  Net *survivor, *removed;
  if (out_net_ports) {
    survivor = out_net;
    removed = in_net;
  } else {
    // default or out_net_ports
    // Default to in_net surviving so drivers (cached in dbNetwork)
    // do not change.
    survivor = in_net;
    removed = out_net;
  }

  bool buffer_removed = false;
  if (!sdc_->isConstrained(in_pin) && !sdc_->isConstrained(out_pin)
      && !sdc_->isConstrained(removed) && !sdc_->isConstrained(buffer)) {
    debugPrint(logger_,
               RSZ,
               "remove_buffer",
               1,
               "remove {}",
               db_network_->name(buffer));
    buffer_removed = true;
    resizer_->incrementalParasiticsBegin();
    sta_->disconnectPin(in_pin);
    sta_->disconnectPin(out_pin);
    sta_->deleteInstance(buffer);

    if (removed) {
      NetPinIterator* pin_iter = db_network_->pinIterator(removed);
      while (pin_iter->hasNext()) {
        const Pin* pin = pin_iter->next();
        Instance* pin_inst = db_network_->instance(pin);
        if (pin_inst != buffer) {
          Port* pin_port = db_network_->port(pin);
          sta_->disconnectPin(const_cast<Pin*>(pin));
          sta_->connectPin(pin_inst, pin_port, survivor);
        }
      }
      delete pin_iter;
      sta_->deleteNet(removed);
      resizer_->parasitics_invalid_.erase(removed);
    }
    resizer_->parasiticsInvalid(survivor);
    resizer_->updateParasitics();
    resizer_->incrementalParasiticsEnd();
  }
  return buffer_removed;
}

bool RemoveBuffer::hasPort(const Net* net)
{
  if (!net) {
    return false;
  }

  dbNet* db_net = db_network_->staToDb(net);
  return !db_net->getBTerms().empty();
}

bool RemoveBuffer::bufferBetweenPorts(Instance* buffer)
{
  LibertyCell* lib_cell = network_->libertyCell(buffer);
  LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);
  Pin* in_pin = db_network_->findPin(buffer, in_port);
  Pin* out_pin = db_network_->findPin(buffer, out_port);
  Net* in_net = db_network_->net(in_pin);
  Net* out_net = db_network_->net(out_pin);
  return hasPort(in_net) && hasPort(out_net);
}

}  // namespace rsz
