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
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
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
bool SplitLoadMove::doMove(const sta::Path* drvr_path, float setup_slack_margin)
{
  const sta::Pin* drvr_pin = drvr_path->pin(sta_);
  sta::Vertex* drvr_vertex = graph_->pinDrvrVertex(drvr_pin);

  const int fanout = this->fanout(drvr_vertex);
  // Don't split loads on low fanout nets.
  if (fanout <= split_load_min_fanout_) {
    debugPrint(logger_,
               RSZ,
               "split_load_move",
               2,
               "REJECT SplitLoadMove {}: Fanout {} <= {} min fanout",
               network_->pathName(drvr_pin),
               fanout,
               split_load_min_fanout_);
    return false;
  }
  if (!resizer_->okToBufferNet(drvr_pin)) {
    debugPrint(logger_,
               RSZ,
               "split_load_move",
               2,
               "REJECT SplitLoadMove {}: Not OK to buffer net",
               network_->pathName(drvr_pin));
    return false;
  }

  const sta::RiseFall* rf = drvr_path->transition(sta_);
  // Sort fanouts of the drvr on the critical path by slack margin
  // wrt the critical path slack.
  const sta::Slack drvr_slack = sta_->slack(drvr_vertex, resizer_->max_);
  vector<pair<sta::Vertex*, sta::Slack>> fanout_slacks;
  sta::VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    // Watch out for problematic asap7 output->output timing arcs.
    if (edge->isWire()) {
      sta::Vertex* fanout_vertex = edge->to(graph_);
      const sta::Slack fanout_slack
          = sta_->slack(fanout_vertex, rf, resizer_->max_);
      const sta::Slack slack_margin = fanout_slack - drvr_slack;
      debugPrint(logger_,
                 RSZ,
                 "split_load_move",
                 3,
                 "SplitLoadMove {}: fanin {} slack_margin = {}",
                 network_->pathName(drvr_pin),
                 network_->pathName(fanout_vertex->pin()),
                 delayAsString(slack_margin, 3, sta_));
      fanout_slacks.emplace_back(fanout_vertex, slack_margin);
    }
  }

  std::ranges::sort(fanout_slacks,
                    [=, this](const pair<sta::Vertex*, sta::Slack>& pair1,
                              const pair<sta::Vertex*, sta::Slack>& pair2) {
                      return (pair1.second > pair2.second
                              || (pair1.second == pair2.second
                                  && network_->pathNameLess(
                                      pair1.first->pin(), pair2.first->pin())));
                    });

  // Get both the mod net and db net (if present).
  dbNet* db_drvr_net;
  dbModNet* db_mod_drvr_net;
  db_network_->net(drvr_pin, db_drvr_net, db_mod_drvr_net);

  sta::LibertyCell* buffer_cell = resizer_->buffer_lowest_drive_;
  const Point drvr_loc = db_network_->location(drvr_pin);

  // Identify loads to split (top 50% with most slack)
  sta::PinSet load_pins(network_);
  const int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    sta::Vertex* load_vertex = fanout_slacks[i].first;
    sta::Pin* load_pin = load_vertex->pin();

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
  sta::Net* drvr_net = db_network_->dbToSta(db_drvr_net);
  sta::Instance* buffer
      = resizer_->insertBufferBeforeLoads(drvr_net,
                                          &load_pins,
                                          buffer_cell,
                                          &drvr_loc,
                                          "split",
                                          nullptr,
                                          odb::dbNameUniquifyType::IF_NEEDED);

  if (!buffer) {
    debugPrint(logger_,
               RSZ,
               "split_load_move",
               2,
               "REJECT SplitLoadMove {}: Couldn't insert buffer",
               network_->pathName(drvr_pin));
    return false;
  }

  sta::LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  sta::Pin* buffer_out_pin = network_->findPin(buffer, output);
  resizer_->resizeToTargetSlew(buffer_out_pin);

  // Invalidate parasitics for both original and new nets
  estimate_parasitics_->parasiticsInvalid(db_network_->dbToSta(db_drvr_net));
  sta::Net* out_net = network_->net(buffer_out_pin);
  estimate_parasitics_->parasiticsInvalid(out_net);

  // Invalidate vertex level ordering
  resizer_->invalidateVertexOrdering();

  debugPrint(logger_,
             RSZ,
             "split_load_move",
             1,
             "ACCEPT SplitLoadMove {}: Inserted buffer {}",
             network_->pathName(drvr_pin),
             network_->pathName(buffer));
  countMove(buffer);
  return true;
}

}  // namespace rsz
