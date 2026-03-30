// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "CloneMove.hh"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "BufferMove.hh"
#include "SplitLoadMove.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using odb::Point;
using std::pair;
using std::string;
using std::vector;

using utl::RSZ;

Point CloneMove::computeCloneGateLocation(
    const sta::Pin* drvr_pin,
    const vector<pair<sta::Vertex*, sta::Slack>>& fanout_slacks)
{
  int count(1);  // driver_pin counts as one

  int centroid_x = db_network_->location(drvr_pin).getX();
  int centroid_y = db_network_->location(drvr_pin).getY();

  const int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    const pair<sta::Vertex*, sta::Slack>& fanout_slack = fanout_slacks[i];
    const sta::Vertex* load_vertex = fanout_slack.first;
    const sta::Pin* load_pin = load_vertex->pin();
    centroid_x += db_network_->location(load_pin).getX();
    centroid_y += db_network_->location(load_pin).getY();
    ++count;
  }
  return {centroid_x / count, centroid_y / count};
}

// CloneMove: Optimize timing by cloning a combinational gate to split loads
//
// Purpose: Reduce capacitive load on critical path by dividing loads into
//          two groups based on timing slack.
//
// Algorithm:
//   1. Check if lower than #fanout threshold, tri-state driver, dont_touch,
//   ECO pending cell, or single output  -> return false.
//   2. Sort fanout loads by decreasing order of slack margin (fanout_slack -
//   driver_slack). High slack margin first.
//   - It is safer to drive high slack margin cells with the clone. The
//   original cell drives the critical path.
//   3. Decide the location for the clone cell
//   4. Move half of loads w/ high slack margin to the output of the clone
//   cell.
//
// Result: Critical loads see reduced capacitance, improving setup timing.
//
// Precondition:
// - Fanout count must exceed split_load_min_fanout_
// - No tri-state driver
// - No dont_touch
// - No pending-move cell
// - No multiple output cell
//
bool CloneMove::doMove(const sta::Path* drvr_path, float setup_slack_margin)
{
  const sta::Pin* drvr_pin = drvr_path->pin(sta_);
  sta::Vertex* drvr_vertex = graph_->pinDrvrVertex(drvr_pin);

  const int fanout = this->fanout(drvr_vertex);
  if (fanout <= split_load_min_fanout_) {
    debugPrint(logger_,
               RSZ,
               "clone_move",
               2,
               "REJECT CloneMove {}: Fanout {} <= {} min fanout",
               network_->pathName(drvr_pin),
               fanout,
               split_load_min_fanout_);
    return false;
  }

  if (!resizer_->okToBufferNet(drvr_pin)) {
    debugPrint(logger_,
               RSZ,
               "clone_move",
               2,
               "REJECT CloneMove {}: Not OK to buffer net",
               network_->pathName(drvr_pin));
    return false;
  }

  // We can probably relax this with the new ECO code
  if (resizer_->buffer_move_->hasPendingMoves(db_network_->instance(drvr_pin))
      > 0) {
    debugPrint(logger_,
               RSZ,
               "clone_move",
               2,
               "REJECT CloneMove {}: Has pending BufferMove",
               network_->pathName(drvr_pin));
    return false;
  }

  // We can probably relax this with the new ECO code
  if (resizer_->split_load_move_->hasPendingMoves(
          db_network_->instance(drvr_pin))
      > 0) {
    debugPrint(logger_,
               RSZ,
               "clone_move",
               2,
               "REJECT CloneMove {}: Has pending SplitLoadMove",
               network_->pathName(drvr_pin));
    return false;
  }

  sta::Instance* drvr_inst = db_network_->instance(drvr_pin);
  if (!resizer_->isSingleOutputCombinational(drvr_inst)) {
    debugPrint(logger_,
               RSZ,
               "clone_move",
               2,
               "REJECT CloneMove {}: Not single output combinational",
               network_->pathName(drvr_pin));
    return false;
  }

  const sta::RiseFall* rf = drvr_path->transition(sta_);
  // Sort fanouts of the drvr on the critical path by slack margin
  // wrt the critical path slack.
  const sta::Slack drvr_slack = sta_->slack(drvr_vertex, rf, resizer_->max_);
  vector<pair<sta::Vertex*, sta::Slack>> fanout_slacks;
  sta::VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    sta::Vertex* fanout_vertex = edge->to(graph_);
    const sta::Slack fanout_slack
        = sta_->slack(fanout_vertex, rf, resizer_->max_);
    const sta::Slack slack_margin = fanout_slack - drvr_slack;
    debugPrint(logger_,
               RSZ,
               "clone_move",
               3,
               "CloneMove {}: fanin {} slack_margin = {}",
               network_->pathName(drvr_pin),
               network_->pathName(fanout_vertex->pin()),
               delayAsString(slack_margin, 3, sta_));
    fanout_slacks.emplace_back(fanout_vertex, slack_margin);
  }

  std::ranges::sort(fanout_slacks,
                    [this](const pair<sta::Vertex*, sta::Slack>& pair1,
                           const pair<sta::Vertex*, sta::Slack>& pair2) {
                      return (pair1.second > pair2.second
                              || (pair1.second == pair2.second
                                  && network_->pathNameLess(
                                      pair1.first->pin(), pair2.first->pin())));
                    });

  // Hierarchy fix
  sta::Instance* parent
      = db_network_->getOwningInstanceParent(drvr_vertex->pin());

  // This is the meat of the gate cloning code.
  // We need to downsize the current driver AND we need to insert another
  // drive that splits the load For now we will defer the downsize to a later
  // juncture.

  sta::LibertyCell* original_cell = network_->libertyCell(drvr_inst);
  sta::LibertyCell* clone_cell = resizer_->halfDrivingPowerCell(original_cell);

  if (clone_cell == nullptr) {
    clone_cell = original_cell;  // no clone available use original
  }

  Point drvr_loc = computeCloneGateLocation(drvr_pin, fanout_slacks);
  sta::Instance* clone_inst
      = resizer_->makeInstance(clone_cell, "clone", parent, drvr_loc);

  debugPrint(logger_,
             RSZ,
             "clone_move",
             1,
             "ACCEPT CloneMove {}: ({}) -> {} ({})",
             network_->pathName(drvr_pin),
             original_cell->name(),
             network_->pathName(clone_inst),
             clone_cell->name());
  // We add the driver instance to the pending move set, but don't count it as a
  // move.
  countMove(clone_inst);
  countMove(drvr_inst, 0);

  // Hierarchy fix, make out_net in parent.
  sta::Net* out_net = db_network_->makeNet(parent);

  std::unique_ptr<sta::InstancePinIterator> inst_pin_iter{
      network_->pinIterator(drvr_inst)};

  while (inst_pin_iter->hasNext()) {
    sta::Pin* pin = inst_pin_iter->next();
    if (network_->direction(pin)->isInput()) {
      // Connect to all the inputs of the original cell.
      auto libPort = network_->libertyPort(
          pin);  // get the liberty port of the original inst/pin
      // Hierarchy fix: make sure modnet on input supported
      odb::dbNet* dbnet = db_network_->flatNet(pin);
      odb::dbModNet* modnet = db_network_->hierNet(pin);
      // get the iterm
      sta::Pin* clone_pin = db_network_->findPin(clone_inst, libPort->name());
      odb::dbITerm* iterm = db_network_->flatPin(clone_pin);

      sta_->connectPin(
          clone_inst,
          libPort,
          db_network_->dbToSta(
              dbnet));  // connect the same liberty port of the new instance

      // Hierarchy fix
      if (modnet) {
        iterm->connect(modnet);
      }
    }
  }

  // Get the output pin
  sta::Pin* clone_output_pin = nullptr;
  std::unique_ptr<sta::InstancePinIterator> clone_pin_iter{
      network_->pinIterator(clone_inst)};
  while (clone_pin_iter->hasNext()) {
    sta::Pin* pin = clone_pin_iter->next();
    // If output pin then cache for later use.
    if (network_->direction(pin)->isOutput()) {
      clone_output_pin = pin;
      break;
    }
  }

  // Connect to the new output net we just created
  auto* clone_output_port = network_->port(clone_output_pin);
  sta_->connectPin(clone_inst, clone_output_port, out_net);
  // Hierarchy: stash the iterm just in case we need to do some
  // hierarchical wiring

  odb::dbITerm* clone_output_iterm = db_network_->flatPin(clone_output_pin);
  if (clone_output_iterm == nullptr) {
    logger_->error(
        RSZ,
        100,
        "Cannot find output pin of the clone instance. Driver pin: {}, "
        "Clone output pin: {}",
        (drvr_pin) ? network_->pathName(drvr_pin) : "Null",
        (clone_output_pin) ? network_->pathName(clone_output_pin) : "Null");
  }

  // Divide the list of pins in half and connect them to the new net we
  // created as part of gate cloning. Skip ports connected to the original net
  int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    pair<sta::Vertex*, sta::Slack> fanout_slack = fanout_slacks[i];
    sta::Vertex* load_vertex = fanout_slack.first;
    sta::Pin* load_pin = load_vertex->pin();
    odb::dbITerm* load_iterm = db_network_->flatPin(load_pin);

    // Leave top level ports connected to original net so verilog port names are
    // preserved.
    if (!network_->isTopLevelPort(load_pin)) {
      auto* load_port = network_->port(load_pin);
      sta::Instance* load = network_->instance(load_pin);
      sta::Instance* load_parent_inst
          = db_network_->getOwningInstanceParent(load_pin);

      // disconnects everything
      sta_->disconnectPin(load_pin);
      // hierarchy fix: if load and clone in different modules
      // do the cross module wiring.
      if (load_parent_inst != parent) {
        db_network_->hierarchicalConnect(clone_output_iterm, load_iterm);
      } else {
        sta_->connectPin(load, load_port, out_net);
      }
    }
  }

  // Invalidate vertex level ordering
  resizer_->invalidateVertexOrdering();

  return true;
}

}  // namespace rsz
