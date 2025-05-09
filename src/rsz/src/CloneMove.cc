// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "CloneMove.hh"

#include <algorithm>
#include <cmath>
#include <string>

#include "BufferMove.hh"
#include "SplitLoadMove.hh"

namespace rsz {

using std::pair;
using std::string;
using std::vector;

using odb::Point;

using utl::RSZ;

using sta::dbITerm;
using sta::Edge;
using sta::Instance;
using sta::InstancePinIterator;
using sta::LibertyCell;
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
using sta::VertexOutEdgeIterator;

Point CloneMove::computeCloneGateLocation(
    const Pin* drvr_pin,
    const vector<pair<Vertex*, Slack>>& fanout_slacks)
{
  int count(1);  // driver_pin counts as one

  int centroid_x = db_network_->location(drvr_pin).getX();
  int centroid_y = db_network_->location(drvr_pin).getY();

  const int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    const pair<Vertex*, Slack>& fanout_slack = fanout_slacks[i];
    const Vertex* load_vertex = fanout_slack.first;
    const Pin* load_pin = load_vertex->pin();
    centroid_x += db_network_->location(load_pin).getX();
    centroid_y += db_network_->location(load_pin).getY();
    ++count;
  }
  return {centroid_x / count, centroid_y / count};
}

bool CloneMove::doMove(const Path* drvr_path,
                       int drvr_index,
                       Slack drvr_slack,
                       PathExpanded* expanded,
                       float setup_slack_margin)
{
  Pin* drvr_pin = drvr_path->pin(this);
  Vertex* drvr_vertex = drvr_path->vertex(sta_);
  const Path* load_path = expanded->path(drvr_index + 1);
  Vertex* load_vertex = load_path->vertex(sta_);
  Pin* load_pin = load_vertex->pin();

  const int fanout = this->fanout(drvr_vertex);
  if (fanout <= split_load_min_fanout_) {
    return false;
  }
  const bool tristate_drvr = resizer_->isTristateDriver(drvr_pin);
  if (tristate_drvr) {
    return false;
  }
  const Net* net = db_network_->dbToSta(db_network_->flatNet(drvr_pin));
  if (resizer_->dontTouch(net)) {
    return false;
  }
  // We can probably relax this with the new ECO code
  if (resizer_->buffer_move->hasPendingMoves(db_network_->instance(drvr_pin))
      > 0) {
    return false;
  }
  // We can probably relax this with the new ECO code
  if (resizer_->split_load_move->hasPendingMoves(
          db_network_->instance(drvr_pin))
      > 0) {
    return false;
  }

  // Divide and conquer.
  debugPrint(logger_,
             RSZ,
             "moves",
             3,
             "clone driver {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "clone driver {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));

  const RiseFall* rf = drvr_path->transition(sta_);
  // Sort fanouts of the drvr on the critical path by slack margin
  // wrt the critical path slack.
  vector<pair<Vertex*, Slack>> fanout_slacks;
  VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge* edge = edge_iter.next();
    Vertex* fanout_vertex = edge->to(graph_);
    const Slack fanout_slack
        = sta_->vertexSlack(fanout_vertex, rf, resizer_->max_);
    const Slack slack_margin = fanout_slack - drvr_slack;
    debugPrint(logger_,
               RSZ,
               "moves",
               4,
               " fanin {} slack_margin = {}",
               network_->pathName(fanout_vertex->pin()),
               delayAsString(slack_margin, sta_, 3));
    fanout_slacks.emplace_back(fanout_vertex, slack_margin);
  }

  sort(fanout_slacks.begin(),
       fanout_slacks.end(),
       [=](const pair<Vertex*, Slack>& pair1,
           const pair<Vertex*, Slack>& pair2) {
         return (pair1.second > pair2.second
                 || (pair1.second == pair2.second
                     && network_->pathNameLess(pair1.first->pin(),
                                               pair2.first->pin())));
       });

  Instance* drvr_inst = db_network_->instance(drvr_pin);

  if (!resizer_->isSingleOutputCombinational(drvr_inst)) {
    return false;
  }

  const string buffer_name = resizer_->makeUniqueInstName("clone");

  // Hierarchy fix
  Instance* parent = db_network_->getOwningInstanceParent(drvr_pin);

  // This is the meat of the gate cloning code.
  // We need to downsize the current driver AND we need to insert another
  // drive that splits the load For now we will defer the downsize to a later
  // juncture.

  LibertyCell* original_cell = network_->libertyCell(drvr_inst);
  LibertyCell* clone_cell = resizer_->halfDrivingPowerCell(original_cell);

  if (clone_cell == nullptr) {
    clone_cell = original_cell;  // no clone available use original
  }

  Point drvr_loc = computeCloneGateLocation(drvr_pin, fanout_slacks);
  Instance* clone_inst = resizer_->makeInstance(
      clone_cell, buffer_name.c_str(), parent, drvr_loc);

  debugPrint(logger_,
             RSZ,
             "moves",
             1,
             "clone_move {} ({}) -> {} ({})",
             network_->pathName(drvr_pin),
             original_cell->name(),
             network_->pathName(clone_inst),
             clone_cell->name());
  addMove(clone_inst);
  // We add the driver instance to the pending move set, but don't count it as a
  // move.
  addMove(drvr_inst, 0);

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "clone {} ({}) -> {} ({})",
             network_->pathName(drvr_pin),
             original_cell->name(),
             network_->pathName(clone_inst),
             clone_cell->name());

  // Hierarchy fix, make out_net in parent.

  //  Net* out_net = resizer_->makeUniqueNet();
  std::string out_net_name = resizer_->makeUniqueNetName();
  Net* out_net = db_network_->makeNet(out_net_name.c_str(), parent);

  std::unique_ptr<InstancePinIterator> inst_pin_iter{
      network_->pinIterator(drvr_inst)};

  while (inst_pin_iter->hasNext()) {
    Pin* pin = inst_pin_iter->next();
    if (network_->direction(pin)->isInput()) {
      // Connect to all the inputs of the original cell.
      auto libPort = network_->libertyPort(
          pin);  // get the liberty port of the original inst/pin
      // Hierarchy fix: make sure modnet on input supported
      dbNet* dbnet = db_network_->flatNet(pin);
      odb::dbModNet* modnet = db_network_->hierNet(pin);
      // get the iterm
      Pin* clone_pin = db_network_->findPin(clone_inst, libPort->name());
      dbITerm* iterm = db_network_->flatPin(clone_pin);

      sta_->connectPin(
          clone_inst,
          libPort,
          db_network_->dbToSta(
              dbnet));  // connect the same liberty port of the new instance

      // Hierarchy fix
      if (modnet) {
        iterm->connect(modnet);
      }
      resizer_->parasiticsInvalid(db_network_->dbToSta(dbnet));
    }
  }

  // Get the output pin
  Pin* clone_output_pin = nullptr;
  std::unique_ptr<InstancePinIterator> clone_pin_iter{
      network_->pinIterator(clone_inst)};
  while (clone_pin_iter->hasNext()) {
    Pin* pin = clone_pin_iter->next();
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

  // Divide the list of pins in half and connect them to the new net we
  // created as part of gate cloning. Skip ports connected to the original net
  int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    pair<Vertex*, Slack> fanout_slack = fanout_slacks[i];
    Vertex* load_vertex = fanout_slack.first;
    Pin* load_pin = load_vertex->pin();
    dbITerm* load_iterm = db_network_->flatPin(load_pin);

    // Leave top level ports connected to original net so verilog port names are
    // preserved.
    if (!network_->isTopLevelPort(load_pin)) {
      auto* load_port = network_->port(load_pin);
      Instance* load = network_->instance(load_pin);
      Instance* load_parent_inst
          = db_network_->getOwningInstanceParent(load_pin);

      // disconnects everything
      sta_->disconnectPin(load_pin);
      // hierarchy fix: if load and clone in different modules
      // do the cross module wiring.
      if (load_parent_inst != parent) {
        std::string unique_connection_name = resizer_->makeUniqueNetName();
        db_network_->hierarchicalConnect(
            clone_output_iterm, load_iterm, unique_connection_name.c_str());
      } else {
        sta_->connectPin(load, load_port, out_net);
      }
    }
  }
  resizer_->parasiticsInvalid(out_net);
  resizer_->parasiticsInvalid(network_->net(drvr_pin));
  return true;
}

}  // namespace rsz
