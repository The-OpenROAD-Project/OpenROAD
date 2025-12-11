// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "SplitLoadMove.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "BaseMove.hh"
#include "odb/db.h"
#include "odb/geom.h"
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

using odb::dbITerm;
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

bool SplitLoadMove::doMove(const Path* drvr_path,
                           int drvr_index,
                           Slack drvr_slack,
                           PathExpanded* expanded,
                           float setup_slack_margin)
{
  // SplitLoadMove: Optimize timing by splitting high-fanout nets
  //
  // Purpose: Reduce capacitive load on critical path by dividing loads into
  //          two groups based on timing slack.
  //
  // Algorithm:
  //   1. Sort all fanout loads by slack margin (timing slack relative to
  //   driver)
  //   2. Upper 50% (loads with MORE timing slack)  → driven by new buffer
  //   3. Lower 50% (loads on CRITICAL path)        → driven by original driver
  //
  // Result: Critical loads see reduced capacitance, improving setup timing.
  //
  // Precondition: Fanout count must exceed split_load_min_fanout_
  //
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
      const Slack fanout_slack
          = sta_->vertexSlack(fanout_vertex, rf, resizer_->max_);
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

  sort(fanout_slacks.begin(),
       fanout_slacks.end(),
       [=, this](const pair<Vertex*, Slack>& pair1,
                 const pair<Vertex*, Slack>& pair2) {
         return (pair1.second > pair2.second
                 || (pair1.second == pair2.second
                     && network_->pathNameLess(pair1.first->pin(),
                                               pair2.first->pin())));
       });

  // Get both the mod net and db net (if present).
  dbNet* db_drvr_net;
  dbModNet* db_mod_drvr_net;
  db_network_->net(drvr_pin, db_drvr_net, db_mod_drvr_net);

  LibertyCell* buffer_cell = resizer_->buffer_lowest_drive_;
  const Point drvr_loc = db_network_->location(drvr_pin);

  // jk: old split load behavior
  if (logger_->debugCheck(utl::RSZ, "split_load_old", 1)) {
    // H-Fix Use driver parent for hierarchy, not the top instance
    Instance* parent = db_network_->getOwningInstanceParent(drvr_pin);

    LibertyCell* buffer_cell = resizer_->buffer_lowest_drive_;
    const Point drvr_loc = db_network_->location(drvr_pin);

    // H-Fix make the buffer in the parent of the driver pin
    Instance* buffer = makeBuffer(buffer_cell, "split", parent, drvr_loc);
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

    // H-fix make the out net in the driver parent
    Net* out_net = db_network_->makeNet(parent);

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
    estimate_parasitics_->parasiticsInvalid(db_network_->dbToSta(db_drvr_net));

    // out_net is the db net
    sta_->connectPin(buffer, output, out_net);

    dbITerm* buffer_op_pin_iterm = db_network_->flatPin(buffer_op_pin);
    dbModNet* buffer_op_modnet = nullptr;  // dbModNet is not connected yet

    const int split_index = fanout_slacks.size() / 2;
    for (int i = 0; i < split_index; i++) {
      pair<Vertex*, Slack> fanout_slack = fanout_slacks[i];
      Vertex* load_vertex = fanout_slack.first;
      Pin* load_pin = load_vertex->pin();

      dbITerm* load_iterm = nullptr;
      load_iterm = db_network_->flatPin(load_pin);

      // Leave ports connected to original net so verilog port names are
      // preserved.
      if (!network_->isTopLevelPort(load_pin)) {
        // This will kill both the flat (dbNet) and hier (modnet) connection
        load_iterm->disconnect();

        // Flat connection to dbNet
        load_iterm->connect(db_network_->staToDb(out_net));

        //
        // H-Fix. Support connecting across hierachy.
        //
        Instance* load_parent = db_network_->getOwningInstanceParent(load_pin);

        if (load_parent != parent) {
          // Connect through different hierarchy
          db_network_->hierarchicalConnect(buffer_op_pin_iterm, load_iterm);

          // New modnet connection is made. Connect to the existing loads.
          buffer_op_modnet = buffer_op_pin_iterm->getModNet();
          assert(buffer_op_modnet != nullptr);
          dbNet* buffer_op_net = db_network_->staToDb(out_net);
          for (dbITerm* iterm : buffer_op_net->getITerms()) {
            // This API disconnects the existing dbModNet first if exists.
            iterm->connect(buffer_op_modnet);
          }
          for (odb::dbBTerm* bterm : buffer_op_net->getBTerms()) {
            bterm->connect(buffer_op_modnet);
          }
        } else if (buffer_op_modnet != nullptr) {
          // Connect at the same hierarchy
          load_iterm->connect(buffer_op_modnet);
        }
      }
    }

    Pin* buffer_out_pin = network_->findPin(buffer, output);
    resizer_->resizeToTargetSlew(buffer_out_pin);
    // H-Fix, only invalidate db nets.
    // resizer_->parasiticsInvalid(net);
    estimate_parasitics_->parasiticsInvalid(db_network_->dbToSta(db_drvr_net));
    estimate_parasitics_->parasiticsInvalid(out_net);
    return true;
  }

  // jk: new behavior
  {
    // Identify loads to split (top 50% with most slack)
    std::set<odb::dbObject*> load_pins;
    const int split_index = fanout_slacks.size() / 2;
    for (int i = 0; i < split_index; i++) {
      Vertex* load_vertex = fanout_slacks[i].first;
      Pin* load_pin = load_vertex->pin();

      // Leave ports connected to original net so verilog port names are
      // preserved.
      if (!network_->isTopLevelPort(load_pin)) {
        dbITerm* iterm = db_network_->flatPin(load_pin);
        if (iterm) {
          load_pins.insert(iterm);
        }
      }
    }

    if (load_pins.empty()) {
      return false;
    }

    // Insert buffer
    dbMaster* buffer_master = db_network_->staToDb(buffer_cell);
    dbInst* buffer_inst = db_drvr_net->insertBufferBeforeLoads(
        load_pins,
        buffer_master,
        &drvr_loc,
        "split",
        odb::dbNameUniquifyType::IF_NEEDED);

    if (buffer_inst) {
      // jk: should avoid this call
      resizer_->insertBufferPostProcess(buffer_inst);

      Instance* buffer = db_network_->dbToSta(buffer_inst);
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
      estimate_parasitics_->parasiticsInvalid(
          db_network_->dbToSta(db_drvr_net));
      Net* out_net = network_->net(buffer_out_pin);
      estimate_parasitics_->parasiticsInvalid(out_net);

      // jk: dbg. Sanity check
      if (logger_->debugCheck(utl::RSZ, "insert_buffer_check_sanity", 1)) {
        dbITerm* in_iterm = buffer_inst->getFirstInput();
        sta_->checkSanityDrvrVertexEdges(in_iterm);
        db_network_->checkSanityNetConnectivity(in_iterm);

        dbITerm* out_iterm = buffer_inst->getFirstOutput();
        sta_->checkSanityDrvrVertexEdges(out_iterm);
        db_network_->checkSanityNetConnectivity(out_iterm);
      }

      return true;
    }
  }

  return false;
}

}  // namespace rsz
