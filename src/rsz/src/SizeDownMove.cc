// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "SizeDownMove.hh"

#include <algorithm>
#include <cmath>
#include <string>

#include "BaseMove.hh"
#include "ord/Timing.h"
#include "sta/DelayFloat.hh"

namespace rsz {

using std::pair;
using std::string;
using std::vector;

using utl::RSZ;

using sta::ArcDelay;
using sta::DcalcAnalysisPt;
using sta::Edge;
using sta::GraphDelayCalc;
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
  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  Vertex* drvr_vertex = drvr_path->vertex(sta_);
  const Path* load_path = expanded->path(drvr_index + 1);
  Vertex* load_vertex = load_path->vertex(sta_);
  Pin* load_pin = load_vertex->pin();

  // Skip nets with large fanout because we will need to buffer them.
  const int fanout = this->fanout(drvr_vertex);
  if (fanout >= size_down_max_fanout_) {
    return false;
  }

  // Divide and conquer.
  debugPrint(logger_,
             RSZ,
             "size_down",
             2,
             "sizing down for crit fanout {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));

  const DcalcAnalysisPt* dcalc_ap = drvr_path->dcalcAnalysisPt(sta_);

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
      // If we already have a move on the load, don't try to size down
      if (!hasMoves(load_inst)) {
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

    if (resizer_->dontTouch(load_inst)) {
      continue;
    }

    LibertyCell* new_cell = downsizeGate(
        drvr_port, load_port, load_pin, dcalc_ap, fanout_slack.second);
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

// This will downsize the gate to the smallest input capacitance that that
// satisfies the given slack margin.
LibertyCell* SizeDownMove::downsizeGate(const LibertyPort* drvr_port,
                                        const LibertyPort* load_port,
                                        const Pin* load_pin,
                                        const DcalcAnalysisPt* dcalc_ap,
                                        float slack_margin)
{
  const int lib_ap = dcalc_ap->libertyIndex();
  LibertyCell* load_cell = load_port->libertyCell();
  const char* load_port_name = load_port->name();

  LibertyCellSeq swappable_cells = getSwappableCells(load_cell);
  LibertyCell* best_cell = nullptr;

  if (swappable_cells.size() > 1) {
    // Sort from the smallest input capacitance to the smallest
    // breaking tie by the intrinsic delay
    sort(&swappable_cells,
         [=](const LibertyCell* cell1, const LibertyCell* cell2) {
           LibertyPort* port1
               = cell1->findLibertyPort(load_port_name)->cornerPort(lib_ap);
           const LibertyPort* port2
               = cell2->findLibertyPort(load_port_name)->cornerPort(lib_ap);

           const float cap1 = port1->capacitance();
           const float cap2 = port2->capacitance();
           const ArcDelay intrinsic1 = port1->intrinsicDelay(this);
           const ArcDelay intrinsic2 = port2->intrinsicDelay(this);
           return (std::tie(cap1, intrinsic2) < std::tie(cap2, intrinsic1));
         });
  }

  string swappable_names;
  for (LibertyCell* swappable : swappable_cells) {
    if (swappable == load_cell) {
      swappable_names += "*";
    }
    swappable_names += swappable->name();
    if (swappable == load_cell) {
      swappable_names += "*";
    }
    swappable_names += " ";
  }
  debugPrint(logger_,
             RSZ,
             "size_down",
             3,
             "size_down fanout {} swaps={}",
             network_->pathName(load_pin),
             swappable_names.c_str());

  const float load_input_cap = load_port->cornerPort(lib_ap)->capacitance();

  // Get fanouts based on Liberty since STA arcs are not present in DFFs
  // Could have more than one fanout (e.g. Q and QN of a flop)
  const Instance* fanout_inst = network_->instance(load_pin);
  auto output_pins = getFanouts(fanout_inst);

  vector<float> output_caps;
  vector<float> output_slew_factors;
  vector<float> output_delays;
  vector<const char*> output_port_names;

  for (const auto& output_pin : output_pins) {
    LibertyPort* output_port = network_->libertyPort(output_pin);
    const char* output_port_name = output_port->name();
    output_port_names.push_back(output_port_name);
    Vertex* output_vertex = graph_->pinLoadVertex(output_pin);

    // Find output capacitance
    const float output_load_cap
        = graph_delay_calc_->loadCap(output_pin, dcalc_ap);
    output_caps.push_back(output_load_cap);

    // Find output slew and slew factor
    const Slew output_slew = sta_->vertexSlew(output_vertex, resizer_->max_);
    float output_res = output_port->driveResistance();
    float elmore_slew_factor = 0.0;
    // Can have gates without fanout (e.g. QN of flop) which have no load
    if (output_res > 0.0 and output_load_cap > 0.0) {
      elmore_slew_factor = output_slew / (output_res * output_load_cap);
    }
    output_slew_factors.push_back(elmore_slew_factor);

    // Compute the baseline delay of the existing fanout cell outputs
    const float load_delay
        = resizer_->gateDelay(output_port, output_load_cap, dcalc_ap);
    output_delays.push_back(load_delay);

    debugPrint(logger_,
               RSZ,
               "size_down",
               3,
               " current {}->{} gate={} delay={} cap={} slew={} slack={}",
               network_->pathName(load_pin),
               network_->pathName(output_pin),
               load_cell->name(),
               delayAsString(load_delay, sta_, 3),
               output_load_cap,
               delayAsString(output_slew, sta_, 3),
               delayAsString(slack_margin, sta_, 3));
  }

  best_cell = load_cell;
  float best_cap = load_input_cap;
  float best_area = load_cell->area();
  for (LibertyCell* swappable : swappable_cells) {
    if (swappable == load_cell) {
      continue;
    }
    debugPrint(logger_,
               RSZ,
               "size_down",
               3,
               " considering swap {} {} -> {}",
               network_->pathName(load_pin),
               load_cell->name(),
               swappable->name());
    LibertyPort* new_load_port = swappable->findLibertyPort(load_port_name);
    float new_input_cap = new_load_port->cornerPort(lib_ap)->capacitance();
    float new_area = swappable->area();

    // Input cap and area improvement checking
    // Note: we check area because we don't want a bigger cell with smaller
    // cap.
    if (new_input_cap > best_cap || new_area > best_area) {
      debugPrint(logger_,
                 RSZ,
                 "size_down",
                 3,
                 "  skip based on cap/area {} gate={} cap={}>{} area={}>{}",
                 network_->pathName(load_pin),
                 swappable->name(),
                 new_input_cap,
                 best_cap,
                 new_area,
                 best_area);
      continue;
    }

    bool reject = false;

    // Max capacitance checking
    int i = 0;
    for (; i < output_pins.size(); i++) {
      LibertyPort* output_port
          = swappable->findLibertyPort(output_port_names[i]);
      float max_cap;
      bool cap_limit_exists;
      output_port->capacitanceLimit(resizer_->max_, max_cap, cap_limit_exists);
      debugPrint(logger_,
                 RSZ,
                 "size_down",
                 3,
                 " fanout pin {} cap {} new_cap {} ",
                 output_port_names[i],
                 max_cap,
                 new_input_cap);

      if (cap_limit_exists && max_cap > 0.0 && output_caps[i] > max_cap) {
        debugPrint(logger_,
                   RSZ,
                   "size_down",
                   2,
                   "  skip based on max cap {} gate={} cap={} max_cap={}",
                   network_->pathName(load_pin),
                   swappable->name(),
                   output_caps[i],
                   max_cap);
        reject = true;
      }
      // Max slew checking
      float output_res = output_port->driveResistance();
      float output_slew = output_slew_factors[i] * output_res * output_caps[i];
      float max_slew;
      bool slew_limit_exists;
      sta_->findSlewLimit(output_port,
                          dcalc_ap->corner(),
                          resizer_->max_,
                          max_slew,
                          slew_limit_exists);

      if (output_slew > max_slew) {
        debugPrint(logger_,
                   RSZ,
                   "size_down",
                   2,
                   "  skip based on max slew {} gate={} slew={} max_slew={}",
                   network_->pathName(load_pin),
                   swappable->name(),
                   output_slew,
                   max_slew);
        reject = true;
      }

      // Compute the change in delay due to decreased load cap using simple RC
      // model
      float drvr_res = drvr_port->driveResistance();
      float drvr_delta_delay = drvr_res * (load_input_cap - new_input_cap);

      // Find the gate delay with the new gate and old output cap
      float new_load_delay
          = resizer_->gateDelay(output_port, output_caps[i], dcalc_ap);

      // This includes the improvement in delay of the driver gate
      float new_total_delay = new_load_delay - drvr_delta_delay;

      debugPrint(logger_,
                 RSZ,
                 "size_down",
                 4,
                 " new delay {}->{} gate={} drvr_delta {}, old_delay {}, "
                 "new_delay: {} ",
                 network_->pathName(load_pin),
                 network_->pathName(output_pins[i]),
                 swappable->name(),
                 delayAsString(output_delays[i], sta_, 3),
                 delayAsString(new_load_delay, sta_, 3));

      // This just applies the slack margin to the fanout gate.
      // It also assumes all outputs get the same slack margin.
      if (new_total_delay > output_delays[i] + slack_margin) {
        debugPrint(logger_,
                   RSZ,
                   "size_down",
                   4,
                   " new delay {}->{} gate={} new_delay {} < old_delay {} + "
                   "slack {} ({})",
                   network_->pathName(load_pin),
                   network_->pathName(output_pins[i]),
                   swappable->name(),
                   delayAsString(new_delay, sta_, 3),
                   delayAsString(output_delays[i], sta_, 3),
                   delayAsString(slack_margin, sta_, 3),
                   delayAsString(output_delays[i] + slack_margin, sta_, 3));
        reject = true;
      }
    }

    if (!reject) {
      best_cell = swappable;
      best_cap = new_input_cap;
      best_area = new_area;
      debugPrint(logger_,
                 RSZ,
                 "size_down",
                 3,
                 " new best size down {} -> {} ({} -> {})",
                 network_->pathName(load_pin),
                 network_->pathName(output_pins[0]),
                 load_cell->name(),
                 swappable->name());
    }
  }

  if (best_cell != load_cell) {
    return best_cell;
  }
  return nullptr;
}

LibertyCellSeq SizeDownMove::getSwappableCells(LibertyCell* base)
{
  if (base->isBuffer()) {
    // Do this once to populate the set
    if (buffer_sizes_.empty()) {
      for (LibertyCell* buffer : resizer_->buffer_fast_sizes_) {
        buffer_sizes_.push_back(buffer);
      }
    }
    if (resizer_->buffer_fast_sizes_.count(base) == 0) {
      return LibertyCellSeq();
    }
    return buffer_sizes_;
  }
  return resizer_->getSwappableCells(base);
}

}  // namespace rsz
