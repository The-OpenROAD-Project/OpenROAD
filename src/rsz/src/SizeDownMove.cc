// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "SizeDownMove.hh"

#include <algorithm>
#include <cmath>
#include <string>
#include <tuple>

#include "BaseMove.hh"

namespace rsz {

using std::pair;
using std::string;
using std::vector;

using utl::RSZ;

using sta::ArcDelay;
using sta::Edge;
using sta::Instance;
using sta::InstancePinIterator;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::LoadPinIndexMap;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::Slack;
using sta::Slew;
using sta::Vertex;
using sta::VertexOutEdgeIterator;

bool SizeDownMove::doMove(const Path* drvr_path,
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

  // Divide and conquer.
  debugPrint(logger_,
             RSZ,
             "size_down",
             2,
             "sizing down crit fanout {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));

  const DcalcAnalysisPt* dcalc_ap = drvr_path->dcalcAnalysisPt(sta_);
  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);

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
          = sta_->vertexSlack(fanout_vertex, resizer_->max_);
      Pin* load_pin = load_vertex->pin();
      Instance* load_inst = network_->instance(load_pin);
      debugPrint(logger_,
                 RSZ,
                 "size_down",
                 3,
                 " unsorted {} slack: {} drvr slack: {}",
                 network_->pathName(fanout_vertex->pin()),
                 delayAsString(fanout_slack, sta_, 3),
                 delayAsString(drvr_slack, sta_, 3))
          // If we already have a move on the load, don't try to size down
          if (!hasMoves(load_inst))
      {
        fanout_slacks.emplace_back(fanout_vertex, fanout_slack);
      }
    }
  }

  // Sort fanouts by slack margin, so we can try the most margin first.
  sort(fanout_slacks.begin(),
       fanout_slacks.end(),
       [=](const pair<Vertex*, Slack>& pair1,
           const pair<Vertex*, Slack>& pair2) {
         return (pair1.second > pair2.second
                 || (pair1.second == pair2.second
                     && network_->pathNameLess(pair1.first->pin(),
                                               pair2.first->pin())));
       });

  for (auto& fanout_slack : fanout_slacks) {
    debugPrint(logger_,
               RSZ,
               "size_down",
               2,
               " fanout {} slack: {} drvr slack: {}",
               network_->pathName(fanout_slack.first->pin()),
               delayAsString(fanout_slack.second, sta_, 3),
               delayAsString(drvr_slack, sta_, 3))
  }

  // Try each one but only accept the first that we are able to size down.
  // Many will not have smaller size options, most likely.
  for (auto& fanout_slack : fanout_slacks) {
    Vertex* load_vertex = fanout_slack.first;

    Pin* load_pin = load_vertex->pin();
    LibertyPort* load_port = network_->libertyPort(load_pin);
    // Skip primary outputs
    if (!load_port) {
      continue;
    }
    LibertyCell* load_cell = load_port->libertyCell();
    Instance* load_inst = network_->instance(load_pin);

    if (resizer_->dontTouch(load_inst)
        || !resizer_->isLogicStdCell(load_inst)) {
      continue;
    }

    LibertyCell* new_cell = downsizeFanout(drvr_port,
                                           drvr_pin,
                                           load_port,
                                           load_pin,
                                           dcalc_ap,
                                           fanout_slack.second);
    if (new_cell && replaceCell(load_inst, new_cell)) {
      debugPrint(logger_,
                 RSZ,
                 "opt_moves",
                 1,
                 "ACCEPT size_down {} -> {} ({} -> {}) slack={}",
                 network_->pathName(drvr_pin),
                 network_->pathName(load_pin),
                 load_cell->name(),
                 new_cell->name(),
                 delayAsString(fanout_slack.second, sta_, 3));
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 3,
                 "size_down {} -> {} ({} -> {}) slack={}",
                 network_->pathName(drvr_pin),
                 network_->pathName(load_pin),
                 load_cell->name(),
                 new_cell->name(),
                 delayAsString(fanout_slack.second, sta_, 3));

      addMove(load_inst);
      return true;
    }
    debugPrint(logger_,
               RSZ,
               "opt_moves",
               3,
               "REJECT size_down {} -> {} ({} -> {}) slack={}",
               network_->pathName(drvr_pin),
               network_->pathName(load_pin),
               load_cell->name(),
               new_cell ? new_cell->name() : "none",
               delayAsString(fanout_slack.second, sta_, 3));
  }

  return false;
}

LibertyCell* SizeDownMove::downsizeFanout(const LibertyPort* drvr_port,
                                          const Pin* drvr_pin,
                                          const LibertyPort* fanout_port,
                                          const Pin* fanout_pin,
                                          const DcalcAnalysisPt* dcalc_ap,
                                          float fanout_slack)
{
  const int lib_ap = dcalc_ap->libertyIndex();
  LibertyCell* fanout_cell = fanout_port->libertyCell();
  const Instance* fanout_inst = network_->instance(fanout_pin);

  LibertyCellSeq swappable_cells = resizer_->getSwappableCells(fanout_cell);
  LibertyCell* best_cell = nullptr;

  if (swappable_cells.size() > 1) {
    const char* fanout_port_name = fanout_port->name();

    // Sort from the smallest input capacitance to the smallest
    // breaking tie by the intrinsic delay
    sort(&swappable_cells,
         [=](const LibertyCell* cell1, const LibertyCell* cell2) {
           LibertyPort* port1
               = cell1->findLibertyPort(fanout_port_name)->cornerPort(lib_ap);
           const LibertyPort* port2
               = cell2->findLibertyPort(fanout_port_name)->cornerPort(lib_ap);

           const float cap1 = port1->capacitance();
           const float cap2 = port2->capacitance();
           const ArcDelay intrinsic1 = port1->intrinsicDelay(this);
           const ArcDelay intrinsic2 = port2->intrinsicDelay(this);
           return (std::tie(cap1, intrinsic2) < std::tie(cap2, intrinsic1));
         });

    const float drvr_load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap);
    const float fanout_cap = fanout_port->cornerPort(lib_ap)->capacitance();

    // Get fanouts based on Liberty since STA arcs are not present in DFFs
    vector<const Pin*> fanouts = getFanouts(fanout_inst);

    int max_fanout_id = 0;
    float fanout_load_cap = -1.0;
    // Find the output pin with max cap load and use that if more than one.
    // We might want to use the most critical one instead?
    for (int i = 0; i < fanouts.size(); i++) {
      const float cur_fanout_load_cap
          = graph_delay_calc_->loadCap(fanouts[i], dcalc_ap);
      if (cur_fanout_load_cap > fanout_load_cap) {
        fanout_load_cap = cur_fanout_load_cap;
        max_fanout_id = i;
      }
    }
    const Pin* fanout_output_pin = fanouts[max_fanout_id];
    LibertyPort* fanout_output_port = network_->libertyPort(fanout_output_pin);
    const char* fanout_output_port_name = fanout_output_port->name();

    const float drvr_delay
        = resizer_->gateDelay(drvr_port, drvr_load_cap, dcalc_ap);
    const float fanout_delay
        = resizer_->gateDelay(fanout_output_port, fanout_load_cap, dcalc_ap);
    debugPrint(
        logger_,
        RSZ,
        "size_down",
        3,
        "base delay {} FO={} drvr_delay={} fanout_delay={} fanout_slack={}",
        network_->pathName(fanout_pin),
        fanout_cell->name(),
        delayAsString(drvr_delay, sta_, 3),
        delayAsString(fanout_delay, sta_, 3),
        delayAsString(fanout_slack, sta_, 3));

    best_cell = fanout_cell;
    float best_cap = fanout_cap;
    float best_area = fanout_cell->area();
    for (LibertyCell* swappable : swappable_cells) {
      if (swappable == fanout_cell) {
        continue;
      }
      LibertyPort* new_fanout_port
          = swappable->findLibertyPort(fanout_port_name);
      LibertyPort* new_fanout_output_port
          = swappable->findLibertyPort(fanout_output_port_name);
      float new_fanout_cap = new_fanout_port->cornerPort(lib_ap)->capacitance();
      float new_drvr_load_cap = drvr_load_cap - fanout_cap + new_fanout_cap;
      float new_area = swappable->area();

      // Only evalute delay if needed
      if (new_fanout_cap > best_cap || new_area > best_area) {
        debugPrint(logger_,
                   RSZ,
                   "size_down",
                   3,
                   " skip based on cap/area {} FO={} cap={}>{} area={}>{}",
                   network_->pathName(fanout_pin),
                   swappable->name(),
                   new_fanout_cap,
                   best_cap,
                   new_area,
                   best_area);
        continue;
      }

      float cap, max_cap, cap_slack;
      const Corner* corner;
      const RiseFall* tr;
      sta_->checkCapacitance(drvr_pin,
                             nullptr /* corner */,
                             resizer_->max_,
                             // return values
                             corner,
                             tr,
                             cap,
                             max_cap,
                             cap_slack);
      if (max_cap > 0.0 && corner) {
        // Check if the new driver load cap is within the max cap limit
        if (new_drvr_load_cap > max_cap) {
          debugPrint(logger_,
                     RSZ,
                     "size_down",
                     2,
                     " skip based on max cap {} FO={} cap={} max_cap={}",
                     network_->pathName(fanout_pin),
                     swappable->name(),
                     new_drvr_load_cap,
                     max_cap);
          continue;
        }
      }

      float new_drvr_delay
          = resizer_->gateDelay(drvr_port, new_drvr_load_cap, dcalc_ap);
      float new_fanout_delay = resizer_->gateDelay(
          new_fanout_output_port, fanout_load_cap, dcalc_ap);
      debugPrint(
          logger_,
          RSZ,
          "size_down",
          3,
          " new delay {} FO={} drvr_delay {}<{}, fanout_delay: {}<{} ({}+{}) ",
          network_->pathName(fanout_pin),
          swappable->name(),
          delayAsString(new_drvr_delay, sta_, 3),
          delayAsString(drvr_delay, sta_, 3),
          delayAsString(new_fanout_delay, sta_, 3),
          delayAsString(fanout_delay - fanout_slack, sta_, 3),
          delayAsString(fanout_delay, sta_, 3),
          delayAsString(fanout_slack, sta_, 3));
      // Check if the combined new delay is better than the old one
      // If fanout slack is positive, this allows us to slow down the fanout
      // gate If fnaout slack is negative, this allows us to accept it as long
      // as the total delay improves
      if (new_drvr_delay + new_fanout_delay
          < drvr_delay + fanout_delay + fanout_slack) {
        best_cell = swappable;
        best_cap = new_fanout_cap;
        best_area = new_area;
      }
    }
  }

  if (best_cell != fanout_cell) {
    return best_cell;
  }
  return nullptr;
}

}  // namespace rsz
