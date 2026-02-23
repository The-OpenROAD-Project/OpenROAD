// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "SplitLoadMove.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "BaseMove.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using std::pair;
using std::string;
using std::vector;

using odb::dbModNet;
using odb::dbNet;
using odb::Point;

using utl::RSZ;

using sta::ArcDelay;
using sta::Edge;
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
using sta::VertexOutEdgeIterator;

/// SplitLoadMove: Optimize timing by splitting high-fanout nets
///
/// Purpose: Reduce capacitive load on critical path by dividing loads into
///          two groups based on timing slack.
///
/// @param drvr_path Path including the driver pin
/// @param drvr_index Index of the driver pin in the path
/// @param drvr_slack Slack at the driver pin
/// @param expanded Expanded path containing detailed timing information
/// @param setup_slack_margin Target slack margin for setup timing optimization.
///                           Paths with slack less than this margin are
///                           considered violating even if it is positive.
///
/// Algorithm:
///   1. Sort all fanout loads by slack margin (timing slack relative to
///      driver)
///   2. Upper 50% (loads with MORE timing slack)  -> driven by new buffer
///   3. Lower 50% (loads on CRITICAL path)        -> driven by original driver
///
/// Result: Critical loads see reduced capacitance, improving setup timing.
///
/// Precondition: Fanout count must exceed split_load_min_fanout_
///
bool SplitLoadMove::doMove(const Path* drvr_path,
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
  // Don't split loads on low fanout nets.
  if (fanout <= split_load_min_fanout_) {
    return false;
  }
  if (!resizer_->okToBufferNet(drvr_pin)) {
    return false;
  }

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "split loads {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));

  debugPrint(logger_,
             RSZ,
             "split_load",
             3,
             "split loads {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));

  const RiseFall* rf = drvr_path->transition(sta_);
  // Sort fanouts of the drvr on the critical path by slack margin
  // wrt the critical path slack.
  vector<pair<Vertex*, Slack>> fanout_slacks;
  VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge* edge = edge_iter.next();
    // Watch out for problematic asap7 output->output timing arcs.
    if (edge->isWire()) {
      Vertex* fanout_vertex = edge->to(graph_);
      const Slack fanout_slack = sta_->slack(fanout_vertex, rf, resizer_->max_);
      const Slack slack_margin = fanout_slack - drvr_slack;
      debugPrint(logger_,
                 RSZ,
                 "split_load",
                 4,
                 " fanin {} slack_margin = {}",
                 network_->pathName(fanout_vertex->pin()),
                 delayAsString(slack_margin, sta_, 3));
      fanout_slacks.emplace_back(fanout_vertex, slack_margin);
    }
  }

  std::ranges::sort(fanout_slacks,
                    [=, this](const pair<Vertex*, Slack>& pair1,
                              const pair<Vertex*, Slack>& pair2) {
                      return (pair1.second > pair2.second
                              || (pair1.second == pair2.second
                                  && network_->pathNameLess(
                                      pair1.first->pin(), pair2.first->pin())));
                    });

  // Get both the mod net and db net (if present).
  dbNet* db_drvr_net;
  dbModNet* db_mod_drvr_net;
  db_network_->net(drvr_pin, db_drvr_net, db_mod_drvr_net);

  LibertyCell* buffer_cell = resizer_->buffer_lowest_drive_;
  const Point drvr_loc = db_network_->location(drvr_pin);

  // Identify loads to split (top 50% with most slack)
  sta::PinSet load_pins(network_);
  const int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    Vertex* load_vertex = fanout_slacks[i].first;
    Pin* load_pin = load_vertex->pin();

    // Leave ports connected to original net so verilog port names are
    // preserved.
    if (!network_->isTopLevelPort(load_pin)) {
      load_pins.insert(load_pin);
    }
  }

  if (load_pins.empty()) {
    return false;
  }

  // Insert buffer
  Net* drvr_net = db_network_->dbToSta(db_drvr_net);
  Instance* buffer
      = resizer_->insertBufferBeforeLoads(drvr_net,
                                          &load_pins,
                                          buffer_cell,
                                          &drvr_loc,
                                          "split",
                                          nullptr,
                                          odb::dbNameUniquifyType::IF_NEEDED);

  if (buffer) {
    debugPrint(logger_,
               RSZ,
               "split_load",
               1,
               "ACCEPT make_buffer {}",
               network_->pathName(buffer));
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               3,
               "split_load make_buffer {}",
               network_->pathName(buffer));
    addMove(buffer);

    LibertyPort *input, *output;
    buffer_cell->bufferPorts(input, output);
    Pin* buffer_out_pin = network_->findPin(buffer, output);
    resizer_->resizeToTargetSlew(buffer_out_pin);

    // Invalidate parasitics for both original and new nets
    estimate_parasitics_->parasiticsInvalid(db_network_->dbToSta(db_drvr_net));
    Net* out_net = network_->net(buffer_out_pin);
    estimate_parasitics_->parasiticsInvalid(out_net);

    return true;
  }

  return false;
}

}  // namespace rsz
