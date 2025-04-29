// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "SplitLoadMove.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/PathExpanded.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"
#include "sta/VerilogWriter.hh"
#include "utl/Logger.h"

namespace rsz {

using std::max;
using std::pair;
using std::string;
using std::vector;
using utl::RSZ;

using sta::Edge;
using sta::fuzzyEqual;
using sta::fuzzyGreater;
using sta::fuzzyGreaterEqual;
using sta::fuzzyLess;
using sta::GraphDelayCalc;
using sta::InstancePinIterator;
using sta::NetConnectedPinIterator;
using sta::PathExpanded;
using sta::Slew;
using sta::VertexOutEdgeIterator;
using sta::INF;



bool SplitLoadMove::doMove(const Path* drvr_path,
                           const int drvr_index,
                           const Slack drvr_slack,
                           PathExpanded* expanded)
{
  Pin* drvr_pin = drvr_path->pin(this);

  const Path* load_path = expanded->path(drvr_index + 1);
  Vertex* load_vertex = load_path->vertex(sta_);
  Pin* load_pin = load_vertex->pin();
  // Divide and conquer.
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "split loads {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));

  Vertex* drvr_vertex = drvr_path->vertex(sta_);
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
      const Slack fanout_slack = sta_->vertexSlack(fanout_vertex, rf, resizer_->max_);
      const Slack slack_margin = fanout_slack - drvr_slack;
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 4,
                 " fanin {} slack_margin = {}",
                 network_->pathName(fanout_vertex->pin()),
                 delayAsString(slack_margin, sta_, 3));
      fanout_slacks.emplace_back(fanout_vertex, slack_margin);
    }
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

  // H-fix get both the mod net and db net (if present).
  dbNet* db_drvr_net;
  odb::dbModNet* db_mod_drvr_net;
  db_network_->net(drvr_pin, db_drvr_net, db_mod_drvr_net);

  const string buffer_name = resizer_->makeUniqueInstName("split");

  // H-Fix Use driver parent for hierarchy, not the top instance
  Instance* parent = db_network_->getOwningInstanceParent(drvr_pin);

  LibertyCell* buffer_cell = resizer_->buffer_lowest_drive_;
  const Point drvr_loc = db_network_->location(drvr_pin);

  // H-Fix make the buffer in the parent of the driver pin
  Instance* buffer = resizer_->makeBuffer(
      buffer_cell, buffer_name.c_str(), parent, drvr_loc);
  all_inst_set_.insert(buffer);

  // H-fix make the out net in the driver parent
  std::string out_net_name = resizer_->makeUniqueNetName();
  Net* out_net = db_network_->makeNet(out_net_name.c_str(), parent);

  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);

  Pin* buffer_ip_pin;
  Pin* buffer_op_pin;
  resizer_->getBufferPins(buffer, buffer_ip_pin, buffer_op_pin);
  (void) buffer_ip_pin;

  // Split the loads with extra slack to an inserted buffer.
  // before
  // drvr_pin -> net -> load_pins
  // after
  // drvr_pin -> net -> load_pins with low slack
  //                 -> buffer_in -> net -> rest of loads

  // Hierarchical case:
  // If the driver was hooked to a modnet.
  //
  // If the loads are partitioned then we introduce new modnets
  // punch through.
  //
  // Create the buffer in the driver module.
  //
  // For non-buffered loads, use original modnet (if any).
  //
  // For buffered loads use dbNetwork::hierarchicalConnect
  // which may introduce new modnets.
  //
  // Before:
  // drvr_pin -> modnet -> load pins {Partition1, Partition2}
  //
  // after
  // drvr_pin -> mod_net -> load pins with low slack {Partition1}
  //                    -> buffer_in -> mod_net* -> rest of loads {Partition2}
  //

  // connect input of buffer to the original driver db net
  sta_->connectPin(buffer, input, db_network_->dbToSta(db_drvr_net));

  // invalidate the dbNet
  resizer_->parasiticsInvalid(db_network_->dbToSta(db_drvr_net));

  // out_net is the db net
  sta_->connectPin(buffer, output, out_net);

  const int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    pair<Vertex*, Slack> fanout_slack = fanout_slacks[i];
    Vertex* load_vertex = fanout_slack.first;
    Pin* load_pin = load_vertex->pin();

    odb::dbITerm* load_iterm;
    odb::dbBTerm* load_bterm;
    odb::dbModITerm* load_moditerm;
    odb::dbModBTerm* load_modbterm;

    db_network_->staToDb(
        load_pin, load_iterm, load_bterm, load_moditerm, load_modbterm);

    // Leave ports connected to original net so verilog port names are
    // preserved.
    if (!network_->isTopLevelPort(load_pin)) {
      LibertyPort* load_port = network_->libertyPort(load_pin);
      Instance* load = network_->instance(load_pin);
      (void) (load_port);
      (void) (load);

      // stash the modnet,if any,  for the load
      odb::dbModNet* db_mod_load_net = db_network_->hierNet(load_pin);

      // This will kill both the flat (dbNet) and hier (modnet) connection
      load_iterm->disconnect();

      // Flat connection to dbNet
      load_iterm->connect(db_network_->staToDb(out_net));

      //
      // H-Fix. Support connecting across hierachy.
      //
      Instance* load_parent = db_network_->getOwningInstanceParent(load_pin);

      if (load_parent != parent) {
        std::string unique_connection_name = resizer_->makeUniqueNetName();
        odb::dbITerm* buffer_op_pin_iterm = db_network_->flatPin(buffer_op_pin);
        odb::dbITerm* load_pin_iterm = db_network_->flatPin(load_pin);
        if (load_pin_iterm && buffer_op_pin_iterm) {
          db_network_->hierarchicalConnect(buffer_op_pin_iterm,
                                           load_pin_iterm,
                                           unique_connection_name.c_str());
        }
      } else {
        odb::dbITerm* iterm;
        iterm = db_network_->flatPin(load_pin);
        if (iterm && db_mod_load_net) {
          iterm->connect(db_mod_load_net);
        }
      }
    }
  }

  Pin* buffer_out_pin = network_->findPin(buffer, output);
  resizer_->resizeToTargetSlew(buffer_out_pin);
  // H-Fix, only invalidate db nets.
  // resizer_->parasiticsInvalid(net);
  resizer_->parasiticsInvalid(db_network_->dbToSta(db_drvr_net));
  resizer_->parasiticsInvalid(out_net);

  return(true);
}

}
