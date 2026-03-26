// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "UnbufferMove.hh"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

#include "BaseMove.hh"
#include "BufferMove.hh"
#include "CloneMove.hh"
#include "SizeUpMove.hh"
#include "SplitLoadMove.hh"
#include "SwapPinsMove.hh"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using std::string;

using odb::dbInst;

using utl::RSZ;

// Remove driver if
// 1) it is a buffer without attributes like dont-touch
// 2) it doesn't create new max fanout violations
// 3) it doesn't create new max cap violations
// 4) it doesn't worsen slack
bool UnbufferMove::doMove(const sta::Path* drvr_path, float setup_slack_margin)
{
  const sta::Pin* drvr_pin = drvr_path->pin(sta_);
  sta::Instance* drvr = network_->instance(drvr_pin);
  sta::LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  sta::LibertyCell* drvr_cell = drvr_port ? drvr_port->libertyCell() : nullptr;

  // TODO:
  // 1. add max slew check

  if (!drvr_cell) {
    debugPrint(logger_,
               RSZ,
               "unbuffer_move",
               2,
               "REJECT UnbufferMove {}: No liberty cell found for {}",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }

  if (!drvr_cell->isBuffer()) {
    debugPrint(logger_,
               RSZ,
               "unbuffer_move",
               2,
               "REJECT UnbufferMove {}: Driver cell ({}) isn't a buffer",
               network_->pathName(drvr_pin),
               drvr_cell->name());
    return false;
  }

  // Don't remove buffers from previous sizing, pin swapping, rebuffering, or
  // cloning because such removal may lead to an inifinte loop or long runtime
  std::string reason;
  if (resizer_->swap_pins_move_->hasMoves(drvr)) {
    reason = "its pins have been swapped";
  } else if (resizer_->clone_move_->hasMoves(drvr)) {
    reason = "it has been cloned";
  } else if (resizer_->split_load_move_->hasMoves(drvr)) {
    reason = "it was from split load buffering";
  } else if (resizer_->buffer_move_->hasMoves(drvr)) {
    reason = "it was from rebuffering";
  } else if (resizer_->size_up_move_->hasMoves(drvr)) {
    reason = "it has been resized";
  }
  if (!reason.empty()) {
    debugPrint(logger_,
               RSZ,
               "unbuffer_move",
               2,
               "REJECT UnbufferMove {}: Buffer is not removed because {}",
               network_->pathName(drvr_pin),
               reason);
    return false;
  }

  // Don't remove buffer if new max fanout violations are created
  sta::Vertex* drvr_vertex = graph_->pinDrvrVertex(drvr_pin);
  sta::Pin* drvr_input_pin = drvr_path->prevPath()->pin(sta_);
  sta::Path* prev_drvr_path = drvr_path->prevPath()->prevPath();
  sta::Pin* prev_drvr_pin
      = prev_drvr_path ? prev_drvr_path->pin(sta_) : nullptr;

  sta::Scene* scene = drvr_path->scene(sta_);
  float curr_fanout, max_fanout, load_slack;
  sta_->checkFanout(prev_drvr_pin,
                    scene->mode(),
                    resizer_->max_,
                    curr_fanout,
                    max_fanout,
                    load_slack);
  float new_fanout = curr_fanout + fanout(drvr_vertex) - 1;
  if (max_fanout > 0.0) {
    // Honor max fanout when the constraint exists
    if (new_fanout > max_fanout) {
      debugPrint(logger_,
                 RSZ,
                 "unbuffer_move",
                 2,
                 "REJECT UnbufferMove {}: "
                 "New fanout {} >= {} max fanout limit at {}",
                 network_->pathName(drvr_pin),
                 new_fanout,
                 max_fanout,
                 network_->pathName(prev_drvr_pin));
      return false;
    }
  } else {
    // No max fanout exists, but don't exceed default fanout limit
    if (new_fanout > buffer_removal_max_fanout_) {
      debugPrint(logger_,
                 RSZ,
                 "unbuffer_move",
                 2,
                 "REJECT UnbufferMove {}: "
                 "New fanout {} >= {} default fanout limit at {}",
                 network_->pathName(drvr_pin),
                 new_fanout,
                 buffer_removal_max_fanout_,
                 network_->pathName(prev_drvr_pin));
      return false;
    }
  }

  // Watch out for new max cap violations
  float cap, max_cap, cap_slack;
  const sta::Scene* prev_drvr_scene;
  const sta::RiseFall* prev_drvr_rf;
  sta_->checkCapacitance(prev_drvr_pin,
                         sta_->scenes(),
                         resizer_->max_,
                         // return values
                         cap,
                         max_cap,
                         cap_slack,
                         prev_drvr_rf,
                         prev_drvr_scene);
  if (max_cap > 0.0 && prev_drvr_scene) {
    sta::GraphDelayCalc* dcalc = sta_->graphDelayCalc();
    float drvr_cap = dcalc->loadCap(drvr_pin, prev_drvr_scene, resizer_->max_);
    sta::LibertyPort *buffer_input_port, *buffer_output_port;
    drvr_cell->bufferPorts(buffer_input_port, buffer_output_port);
    float new_cap
        = cap + drvr_cap
          - resizer_->portCapacitance(buffer_input_port, prev_drvr_scene);
    if (new_cap > max_cap) {
      debugPrint(logger_,
                 RSZ,
                 "unbuffer_move",
                 2,
                 "REJECT UnbufferMove {}: "
                 "Max cap limit of {} at {} ({} -> {})",
                 network_->pathName(drvr_pin),
                 max_cap,
                 network_->pathName(prev_drvr_pin),
                 cap,
                 new_cap);
      return false;
    }
  }

  SlackEstimatorParams params(setup_slack_margin, prev_drvr_scene);
  params.driver_pin = drvr_vertex->pin();
  params.prev_driver_pin = prev_drvr_pin;
  params.driver_input_pin = drvr_input_pin;
  params.driver = drvr;
  params.driver_cell = drvr_cell;
  params.driver_path = drvr_path;
  params.prev_driver_path = drvr_path->prevPath()->prevPath();
  if (!estimatedSlackOK(params)) {
    debugPrint(logger_,
               RSZ,
               "unbuffer_move",
               2,
               "REJECT UnbufferMove {}: Estimated slack not OK",
               network_->pathName(drvr_pin));
    return false;
  }

  if (!canRemoveBuffer(drvr, /* honorDontTouch */ true)) {
    debugPrint(logger_,
               RSZ,
               "unbuffer_move",
               2,
               "REJECT UnbufferMove {}: Can't remove buffer {}",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }

  debugPrint(logger_,
             RSZ,
             "unbuffer_move",
             1,
             "ACCEPT UnbufferMove {}: Removed buffer {}",
             network_->pathName(drvr_pin),
             network_->pathName(drvr));
  bool removed = removeBuffer(drvr);
  if (removed) {
    // Invalidate vertex level ordering
    resizer_->invalidateVertexOrdering();
  }
  return removed;
}

bool UnbufferMove::removeBufferIfPossible(sta::Instance* buffer,
                                          bool honorDontTouchFixed)
{
  if (canRemoveBuffer(buffer, honorDontTouchFixed)) {
    return removeBuffer(buffer);
  }
  return false;
}

bool UnbufferMove::bufferBetweenPorts(sta::Instance* buffer)
{
  sta::LibertyCell* lib_cell = network_->libertyCell(buffer);
  sta::LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);
  sta::Pin* in_pin = db_network_->findPin(buffer, in_port);
  sta::Pin* out_pin = db_network_->findPin(buffer, out_port);
  sta::Net* in_net = db_network_->net(in_pin);
  sta::Net* out_net = db_network_->net(out_pin);
  return db_network_->hasPort(in_net) && db_network_->hasPort(out_net);
}

// There are two buffer removal modes: auto and manual:
// 1) auto mode: this happens during setup fixing, power recovery, buffer
// removal
//      Dont-touch, fixed cell and boundary buffer constraints are honored
// 2) manual mode: this happens during manual buffer removal during ECO
//      This ignores dont-touch and fixed cell (boundary buffer constraints are
//      still honored)
bool UnbufferMove::canRemoveBuffer(sta::Instance* buffer,
                                   bool honorDontTouchFixed)
{
  sta::LibertyCell* lib_cell = network_->libertyCell(buffer);
  if (!lib_cell || !resizer_->isLogicStdCell(buffer) || !lib_cell->isBuffer()) {
    return false;
  }

  sta::Pin* buffer_ip_pin;
  sta::Pin* buffer_op_pin;
  getBufferPins(buffer, buffer_ip_pin, buffer_op_pin);

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
  sta::LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);
  sta::Pin* in_pin = db_network_->findPin(buffer, in_port);
  sta::Pin* out_pin = db_network_->findPin(buffer, out_port);
  sta::Net* in_net = db_network_->net(in_pin);
  sta::Net* out_net = db_network_->net(out_pin);
  odb::dbNet* in_db_net = db_network_->findFlatDbNet(in_net);
  odb::dbNet* out_db_net = db_network_->findFlatDbNet(out_net);
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
  bool out_net_ports = db_network_->hasPort(out_net);
  sta::Net* removed = nullptr;
  odb::dbNet* db_net_survivor = nullptr;
  odb::dbNet* db_net_removed = nullptr;
  if (out_net_ports) {
    removed = in_net;
    db_net_survivor = out_db_net;
    db_net_removed = in_db_net;
  } else {
    // default or out_net_ports
    // Default to in_net surviving so drivers (cached in dbNetwork)
    // do not change.
    removed = out_net;
    db_net_survivor = in_db_net;
    db_net_removed = out_db_net;
  }

  sta::Sdc* sdc = sta_->cmdMode()->sdc();

  if (!sdc->isConstrained(in_pin) && !sdc->isConstrained(out_pin)
      && (!removed || !sdc->isConstrained(removed))
      && !sdc->isConstrained(buffer)) {
    return db_net_removed == nullptr
           || (db_net_survivor && db_net_survivor->canMergeNet(db_net_removed));
  }

  return false;
}

bool UnbufferMove::removeBuffer(sta::Instance* buffer)
{
  sta::LibertyCell* lib_cell = network_->libertyCell(buffer);
  debugPrint(logger_,
             RSZ,
             "unbuffer_move",
             3,
             "remove_buffer {} ({})",
             network_->pathName(buffer),
             lib_cell->name());

  sta::LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);

  sta::Pin* in_pin = db_network_->findPin(buffer, in_port);
  sta::Pin* out_pin = db_network_->findPin(buffer, out_port);

  odb::dbNet* in_db_net = db_network_->flatNet(in_pin);
  odb::dbNet* out_db_net = db_network_->flatNet(out_pin);

  // Handle undriven buffer
  if (in_db_net == nullptr) {
    logger_->warn(
        RSZ,
        168,
        "The input pin of buffer '{}' is undriven. Do not remove the buffer.",
        network_->pathName(buffer));
    return false;
  }

  countMove(buffer);

  // Remove the unused buffer
  if (out_db_net == nullptr) {
    dbInst* dbinst_buffer = db_network_->staToDb(buffer);
    dbInst::destroy(dbinst_buffer);
    return true;
  }

  // in_net and out_net are flat nets.
  sta::Net* in_net = db_network_->dbToSta(in_db_net);
  sta::Net* out_net = db_network_->dbToSta(out_db_net);

  // Decide survivor net when two nets are merged
  sta::Net* survivor = in_net;  // buffer input net is the default survivor
  sta::Net* removed = out_net;
  odb::dbModNet* op_modnet = db_network_->hierNet(out_pin);
  odb::dbModNet* ip_modnet = db_network_->hierNet(in_pin);
  odb::dbModNet* survivor_modnet = ip_modnet;
  odb::dbModNet* removed_modnet = op_modnet;

  // When the buffer removal creates a feedthrough (input port directly
  // wired to output port), the input ModNet must survive so that
  // VerilogWriter sees port_name != net_name and emits the assign.
  if (!bufferRemovalCreatesFeedthrough(ip_modnet, op_modnet)) {
    bool in_net_has_port = db_network_->hasPort(in_net);
    bool out_net_has_port = db_network_->hasPort(out_net);
    if (!in_net_has_port && out_net_has_port) {
      survivor = out_net;
      removed = in_net;
      survivor_modnet = op_modnet;
      removed_modnet = ip_modnet;
    }
  }

  // Disconnect the buffer and handle the nets
  debugPrint(logger_,
             RSZ,
             "unbuffer_move",
             3,
             "remove_buffer {} (input net) - {} ({}) - {} (output net)",
             db_network_->name(in_net),
             network_->pathName(buffer),
             lib_cell->name(),
             db_network_->name(out_net));

  odb::dbNet* db_survivor = db_network_->staToDb(survivor);
  odb::dbNet* db_removed = db_network_->staToDb(removed);

  // If removed net name is higher in hierarchy, rename survivor with it.
  // This must be done before mergeNet because db_removed is destroyed inside
  // mergeNet.
  std::optional<std::string> new_net_name;
  std::optional<std::string> new_modnet_name;
  if (db_survivor->isDeeperThan(db_removed)) {
    new_net_name = db_removed->getName();
    if (removed_modnet != nullptr) {
      new_modnet_name = removed_modnet->getName();
    }
  }

  // Disconnect buffer input/output pins
  sta_->disconnectPin(in_pin);
  sta_->disconnectPin(out_pin);

  // Merge hier net
  // - mergeModNet() should be done before mergeNet() because
  //   mergeNet() can involve the hierarchical net traversal during the merge
  //   operation.
  if (survivor_modnet != nullptr && removed_modnet != nullptr) {
    // Merge two hier nets
    survivor_modnet->mergeModNet(removed_modnet);
  } else if (survivor_modnet != nullptr) {
    // If there is a single modnet, copy terminals of the flat net to be
    // removed
    survivor_modnet->connectTermsOf(db_removed);
  } else if (removed_modnet != nullptr) {
    // If there is a single modnet, it should survive.
    survivor_modnet = removed_modnet;
    removed_modnet = nullptr;

    // Copy terminals of the survivor flat net
    survivor_modnet->connectTermsOf(db_survivor);

    // survivor_modnet should be renamed later.
    new_modnet_name
        = db_survivor->getBlock()->getBaseName(db_survivor->getName().c_str());
  }

  // Merge flat net
  db_survivor->mergeNet(db_removed);

  // Remove buffer
  sta_->deleteInstance(buffer);

  // Rename if needed
  if (new_net_name) {
    db_survivor->rename(new_net_name->c_str());
  }
  if (survivor_modnet != nullptr && new_modnet_name) {
    survivor_modnet->rename(new_modnet_name->c_str());
  }

  return true;
}

// Check if removing a buffer creates a feedthrough: input port directly
// wired to output port within the same module.
bool UnbufferMove::bufferRemovalCreatesFeedthrough(
    odb::dbModNet* ip_modnet,
    odb::dbModNet* op_modnet) const
{
  if (!ip_modnet || !op_modnet
      || ip_modnet->getParent() != op_modnet->getParent()) {
    return false;
  }
  odb::dbSet<odb::dbModBTerm> ip_bterms = ip_modnet->getModBTerms();
  const bool ip_has_input_port
      = std::ranges::any_of(ip_bterms, [](odb::dbModBTerm* bt) {
          return bt->getIoType() == odb::dbIoType::INPUT;
        });

  odb::dbSet<odb::dbModBTerm> op_bterms = op_modnet->getModBTerms();
  const bool op_has_output_port
      = std::ranges::any_of(op_bterms, [](odb::dbModBTerm* bt) {
          return bt->getIoType() == odb::dbIoType::OUTPUT;
        });

  return ip_has_input_port && op_has_output_port;
}

}  // namespace rsz
