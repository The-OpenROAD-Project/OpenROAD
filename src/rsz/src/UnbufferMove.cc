// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "UnbufferMove.hh"

#include <algorithm>
#include <cmath>
#include <string>

#include "BaseMove.hh"
#include "BufferMove.hh"
#include "CloneMove.hh"
#include "SizeMove.hh"
#include "SplitLoadMove.hh"
#include "SwapPinsMove.hh"

namespace rsz {

using std::string;

using odb::dbInst;

using utl::RSZ;

using sta::ArcDelay;
using sta::Corner;
using sta::DcalcAnalysisPt;
using sta::GraphDelayCalc;
using sta::Instance;
using sta::InstancePinIterator;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::LoadPinIndexMap;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::RiseFall;
using sta::Slack;
using sta::Slew;
using sta::Vertex;

// Remove driver if
// 1) it is a buffer without attributes like dont-touch
// 2) it doesn't create new max fanout violations
// 3) it doesn't create new max cap violations
// 4) it doesn't worsen slack
bool UnbufferMove::doMove(const Path* drvr_path,
                          int drvr_index,
                          Slack drvr_slack,
                          PathExpanded* expanded,
                          float setup_slack_margin)
{
  Vertex* drvr_vertex = drvr_path->vertex(sta_);
  const Pin* drvr_pin = drvr_vertex->pin();
  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  LibertyCell* drvr_cell = drvr_port ? drvr_port->libertyCell() : nullptr;

  // TODO:
  // 1. add max slew check
  if (drvr_cell && drvr_cell->isBuffer()) {
    Pin* drvr_pin = drvr_path->pin(this);
    Instance* drvr = network_->instance(drvr_pin);

    // Don't remove buffers from previous sizing, pin swapping, rebuffering, or
    // cloning because such removal may lead to an inifinte loop or long runtime
    std::string reason;
    if (resizer_->swap_pins_move->hasMoves(drvr)) {
      reason = "its pins have been swapped";
    } else if (resizer_->clone_move->hasMoves(drvr)) {
      reason = "it has been cloned";
    } else if (resizer_->split_load_move->hasMoves(drvr)) {
      reason = "it was from split load buffering";
    } else if (resizer_->buffer_move->hasMoves(drvr)) {
      reason = "it was from rebuffering";
    } else if (resizer_->size_move->hasMoves(drvr)) {
      reason = "it has been resized";
    }
    if (!reason.empty()) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 4,
                 "buffer {} is not removed because {}",
                 db_network_->name(drvr),
                 reason);
      return false;
    }

    // Don't remove buffer if new max fanout violations are created
    Vertex* drvr_vertex = drvr_path->vertex(sta_);
    const Path* prev_drvr_path = expanded->path(drvr_index - 2);
    Vertex* prev_drvr_vertex = prev_drvr_path->vertex(sta_);
    Pin* prev_drvr_pin = prev_drvr_vertex->pin();
    float curr_fanout, max_fanout, fanout_slack;
    sta_->checkFanout(
        prev_drvr_pin, resizer_->max_, curr_fanout, max_fanout, fanout_slack);
    float new_fanout = curr_fanout + fanout(drvr_vertex) - 1;
    if (max_fanout > 0.0) {
      // Honor max fanout when the constraint exists
      if (new_fanout > max_fanout) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "buffer {} is not removed because of max fanout limit "
                   "of {} at {}",
                   db_network_->name(drvr),
                   max_fanout,
                   network_->pathName(prev_drvr_pin));
        return false;
      }
    } else {
      // No max fanout exists, but don't exceed default fanout limit
      if (new_fanout > buffer_removal_max_fanout_) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "buffer {} is not removed because of default fanout "
                   "limit of {} at "
                   "{}",
                   db_network_->name(drvr),
                   buffer_removal_max_fanout_,
                   network_->pathName(prev_drvr_pin));
        return false;
      }
    }

    // Watch out for new max cap violations
    float cap, max_cap, cap_slack;
    const Corner* corner;
    const RiseFall* tr;
    sta_->checkCapacitance(prev_drvr_pin,
                           nullptr /* corner */,
                           resizer_->max_,
                           // return values
                           corner,
                           tr,
                           cap,
                           max_cap,
                           cap_slack);
    if (max_cap > 0.0 && corner) {
      const DcalcAnalysisPt* dcalc_ap
          = corner->findDcalcAnalysisPt(resizer_->max_);
      GraphDelayCalc* dcalc = sta_->graphDelayCalc();
      float drvr_cap = dcalc->loadCap(drvr_pin, dcalc_ap);
      LibertyPort *buffer_input_port, *buffer_output_port;
      drvr_cell->bufferPorts(buffer_input_port, buffer_output_port);
      float new_cap = cap + drvr_cap
                      - resizer_->portCapacitance(buffer_input_port, corner);
      if (new_cap > max_cap) {
        debugPrint(
            logger_,
            RSZ,
            "repair_setup",
            2,
            "buffer {} is not removed because of max cap limit of {} at {}",
            db_network_->name(drvr),
            max_cap,
            network_->pathName(prev_drvr_pin));
        return false;
      }
    }

    const Path* drvr_input_path = expanded->path(drvr_index - 1);
    Vertex* drvr_input_vertex = drvr_input_path->vertex(sta_);
    SlackEstimatorParams params(setup_slack_margin, corner);
    params.driver_pin = drvr_pin;
    params.prev_driver_pin = prev_drvr_pin;
    params.driver_input_pin = drvr_input_vertex->pin();
    params.driver = drvr;
    params.driver_path = drvr_path;
    params.prev_driver_path = prev_drvr_path;
    params.driver_cell = drvr_cell;
    if (!estimatedSlackOK(params)) {
      return false;
    }

    if (canRemoveBuffer(drvr, /* honorDontTouch */ true)) {
      removeBuffer(drvr);
      return true;
    }
  }

  return false;
}

bool UnbufferMove::removeBufferIfPossible(Instance* buffer,
                                          bool honorDontTouchFixed)
{
  if (canRemoveBuffer(buffer, honorDontTouchFixed)) {
    removeBuffer(buffer);
    return true;
  }
  return false;
}

bool UnbufferMove::bufferBetweenPorts(Instance* buffer)
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

// There are two buffer removal modes: auto and manual:
// 1) auto mode: this happens during setup fixing, power recovery, buffer
// removal
//      Dont-touch, fixed cell and boundary buffer constraints are honored
// 2) manual mode: this happens during manual buffer removal during ECO
//      This ignores dont-touch and fixed cell (boundary buffer constraints are
//      still honored)
bool UnbufferMove::canRemoveBuffer(Instance* buffer, bool honorDontTouchFixed)
{
  LibertyCell* lib_cell = network_->libertyCell(buffer);
  if (!lib_cell || !resizer_->isLogicStdCell(buffer) || !lib_cell->isBuffer()) {
    return false;
  }
  // Do not remove buffers connected to input/output ports
  // because verilog netlists use the net name for the port.
  if (bufferBetweenPorts(buffer)) {
    return false;
  }
  // Don't remove buffers connected to modnets on both input and output
  // These buffers occupy as special place in hierarchy and cannot
  // be removed without destroying the hierarchy.
  // This is the hierarchical equivalent of "bufferBetweenPorts" above

  Pin* buffer_ip_pin;
  Pin* buffer_op_pin;
  getBufferPins(buffer, buffer_ip_pin, buffer_op_pin);
  if (db_network_->hierNet(buffer_ip_pin)
      && db_network_->hierNet(buffer_op_pin)) {
    return false;
  }

  //
  // Don't remove buffers with (1) an input pin connected to a hierarchical
  // net (1) an output pin not connected to a hierarchical net.
  // These are required to remain visible.
  //
  if (db_network_->hierNet(buffer_ip_pin)
      && !db_network_->hierNet(buffer_op_pin)) {
    return false;
  }

  // We only allow case when we can  get buffer driver
  // and wire in the hierarchical net. This is when there is
  // a dbModNet on the buffer output AND the buffer and the thing
  // driving the instance are in the same module.

  odb::dbModNet* op_hierarchical_net = db_network_->hierNet(buffer_op_pin);
  if (op_hierarchical_net) {
    Pin* ignore_driver_pin = nullptr;
    dbNet* buffer_ip_flat_net = db_network_->flatNet(buffer_ip_pin);
    odb::dbModule* driving_module = db_network_->getNetDriverParentModule(
        db_network_->dbToSta(buffer_ip_flat_net), ignore_driver_pin, true);
    (void) ignore_driver_pin;
    // buffer is a dbInst.
    dbInst* buffer_inst = db_network_->staToDb(buffer);
    odb::dbModule* buffer_owning_module = buffer_inst->getModule();
    if (driving_module != buffer_owning_module) {
      return false;
    }
  }

  dbInst* db_inst = db_network_->staToDb(buffer);
  if (db_inst->isDoNotTouch()) {
    if (honorDontTouchFixed) {
      return false;
    }
    //  remove instance dont touch
    db_inst->setDoNotTouch(false);
  }
  if (db_inst->isFixed()) {
    if (honorDontTouchFixed) {
      return false;
    }
    // change FIXED to PLACED just in case
    db_inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }
  LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);
  Pin* in_pin = db_network_->findPin(buffer, in_port);
  Pin* out_pin = db_network_->findPin(buffer, out_port);
  Net* in_net = db_network_->net(in_pin);
  Net* out_net = db_network_->net(out_pin);
  dbNet* in_db_net = db_network_->staToDb(in_net);
  dbNet* out_db_net = db_network_->staToDb(out_net);
  // honor net dont-touch on input net or output net
  if ((in_db_net && in_db_net->isDoNotTouch())
      || (out_db_net && out_db_net->isDoNotTouch())) {
    if (honorDontTouchFixed) {
      return false;
    }
    // remove net dont touch for manual ECO
    if (in_db_net) {
      in_db_net->setDoNotTouch(false);
    }
    if (out_db_net) {
      out_db_net->setDoNotTouch(false);
    }
  }
  bool out_net_ports = hasPort(out_net);
  Net *survivor, *removed;
  if (out_net_ports) {
    if (hasPort(in_net)) {
      return false;
    }
    survivor = out_net;
    removed = in_net;
  } else {
    // default or out_net_ports
    // Default to in_net surviving so drivers (cached in dbNetwork)
    // do not change.
    survivor = in_net;
    removed = out_net;
  }

  if (!sdc_->isConstrained(in_pin) && !sdc_->isConstrained(out_pin)
      && (!removed || !sdc_->isConstrained(removed))
      && !sdc_->isConstrained(buffer)) {
    odb::dbNet* db_survivor = db_network_->staToDb(survivor);
    odb::dbNet* db_removed = db_network_->staToDb(removed);
    return !db_removed || db_survivor->canMergeNet(db_removed);
  }
  return false;
}

void UnbufferMove::removeBuffer(Instance* buffer)
{
  debugPrint(
      logger_, RSZ, "moves", 1, "unbuffer_move {}", network_->pathName(buffer));
  addMove(buffer);

  LibertyCell* lib_cell = network_->libertyCell(buffer);
  LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);

  Pin* in_pin = db_network_->findPin(buffer, in_port);
  Pin* out_pin = db_network_->findPin(buffer, out_port);

  // Hierarchical net handling
  odb::dbModNet* op_modnet = db_network_->hierNet(out_pin);

  odb::dbNet* in_db_net = db_network_->flatNet(in_pin);
  odb::dbNet* out_db_net = db_network_->flatNet(out_pin);
  if (in_db_net == nullptr || out_db_net == nullptr) {
    return;
  }
  // in_net and out_net are flat nets.
  Net* in_net = db_network_->dbToSta(in_db_net);
  Net* out_net = db_network_->dbToSta(out_db_net);

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
  debugPrint(
      logger_, RSZ, "remove_buffer", 1, "remove {}", db_network_->name(buffer));

  odb::dbNet* db_survivor = db_network_->staToDb(survivor);
  odb::dbNet* db_removed = db_network_->staToDb(removed);
  if (db_removed) {
    db_survivor->mergeNet(db_removed);
  }
  sta_->disconnectPin(in_pin);
  sta_->disconnectPin(out_pin);
  sta_->deleteInstance(buffer);
  if (removed) {
    sta_->deleteNet(removed);
  }

  // Hierarchical case supported:
  // moving an output hierarchical net to the input pin driver.
  // During canBufferRemove check (see above) we require that the
  // input pin driver is in the same module scope as the output hierarchical
  // driver
  //
  if (op_modnet) {
    debugPrint(logger_,
               RSZ,
               "remove_buffer",
               1,
               "Handling hierarchical net {}",
               op_modnet->getName());
    Pin* driver_pin = nullptr;
    db_network_->getNetDriverParentModule(in_net, driver_pin, true);
    db_network_->connectPin(driver_pin, db_network_->dbToSta(op_modnet));
  }

  resizer_->parasitics_invalid_.erase(removed);
  resizer_->parasiticsInvalid(survivor);
  resizer_->updateParasitics();
}

}  // namespace rsz
