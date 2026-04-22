// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "SizeDownMove.hh"

#include <algorithm>
#include <cmath>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "BaseMove.hh"
#include "db_sta/dbSta.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "utl/Logger.h"

namespace rsz {

using std::pair;
using std::string;
using std::vector;

using utl::RSZ;

bool SizeDownMove::doMove(const sta::Path* drvr_path, float setup_slack_margin)
{
  const sta::Pin* drvr_pin = drvr_path->pin(sta_);
  sta::Vertex* drvr_vertex = graph_->pinDrvrVertex(drvr_pin);
  const int fanout = this->fanout(drvr_vertex);
  // Skip nets with large fanout because we will need to buffer them.
  if (fanout >= size_down_max_fanout_) {
    debugPrint(logger_,
               RSZ,
               "size_down_move",
               2,
               "REJECT SizeDownMove {}: Fanout {} >= {} max fanout",
               network_->pathName(drvr_pin),
               fanout,
               size_down_max_fanout_);
    return false;
  }

  sta::LibertyPort* drvr_port = network_->libertyPort(drvr_pin);

  // Sort fanouts of the drvr by slack
  const sta::Slack drvr_slack = sta_->slack(drvr_vertex, resizer_->max_);
  vector<pair<sta::Vertex*, sta::Slack>> fanout_slacks;
  sta::VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    // Watch out for problematic asap7 output->output timing arcs.
    if (edge->isWire()) {
      sta::Vertex* fanout_vertex = edge->to(graph_);
      const sta::Slack fanout_slack
          = sta_->slack(fanout_vertex, resizer_->max_);
      sta::Pin* fanout_pin = fanout_vertex->pin();
      sta::Instance* fanout_inst = network_->instance(fanout_pin);
      // If we already have a move on the fanout gate, don't try to size down
      // again
      if (hasMoves(fanout_inst)) {
        debugPrint(logger_,
                   RSZ,
                   "size_down_move",
                   5,
                   "SizeDownMove {}: {} is not downsized because "
                   "it has other moves",
                   network_->pathName(drvr_pin),
                   network_->pathName(fanout_inst));
        continue;
      }
      debugPrint(logger_,
                 RSZ,
                 "size_down_move",
                 5,
                 "SizeDownMove {}: Unsorted {}, slack = {}, driver slack = {}",
                 network_->pathName(drvr_pin),
                 network_->pathName(fanout_vertex->pin()),
                 delayAsString(fanout_slack, 3, sta_),
                 delayAsString(drvr_slack, 3, sta_));
      fanout_slacks.emplace_back(fanout_vertex, fanout_slack);
    }
  }

  // Sort fanouts by slack margin, so we can try the most margin first.
  std::ranges::sort(fanout_slacks,
                    [this](const pair<sta::Vertex*, sta::Slack>& pair1,
                           const pair<sta::Vertex*, sta::Slack>& pair2) {
                      return (pair1.second > pair2.second
                              || (pair1.second == pair2.second
                                  && network_->pathNameLess(
                                      pair1.first->pin(), pair2.first->pin())));
                    });

  for (auto& fanout_slack : fanout_slacks) {
    debugPrint(logger_,
               RSZ,
               "size_down_move",
               4,
               "SizeDownMove {}: Fanout {}, slack = {}, driver slack = {}",
               network_->pathName(drvr_pin),
               network_->pathName(fanout_slack.first->pin()),
               delayAsString(fanout_slack.second, 3, sta_),
               delayAsString(drvr_slack, 3, sta_))
  }

  sta::Scene* scene = drvr_path->scene(sta_);
  const sta::MinMax* min_max = drvr_path->minMax(sta_);

  int num_down_sizes = 0;

  for (auto& fanout_slack : fanout_slacks) {
    sta::Vertex* load_vertex = fanout_slack.first;

    sta::Pin* load_pin = load_vertex->pin();
    sta::LibertyPort* load_port = network_->libertyPort(load_pin);
    // Skip primary outputs
    if (!load_port) {
      continue;
    }
    sta::LibertyCell* load_cell = load_port->libertyCell();
    sta::Instance* load_inst = network_->instance(load_pin);

    if (resizer_->dontTouch(load_inst)
        || !resizer_->isLogicStdCell(load_inst)) {
      continue;
    }

    sta::LibertyCell* new_cell = downSizeGate(
        drvr_port, load_port, load_pin, scene, min_max, fanout_slack.second);
    if (new_cell && replaceCell(load_inst, new_cell)) {
      debugPrint(logger_,
                 RSZ,
                 "size_down_move",
                 3,
                 "ACCEPT SizeDownMove {} -> {}: ({} -> {}) slack={}",
                 network_->pathName(drvr_pin),
                 network_->pathName(load_pin),
                 load_cell->name(),
                 new_cell->name(),
                 delayAsString(fanout_slack.second, 3, sta_));
      countMove(load_inst);
      num_down_sizes++;
    } else {
      debugPrint(logger_,
                 RSZ,
                 "size_down_move",
                 3,
                 "REJECT SizeDownMove {} -> {}: ({} -> {}) slack={}",
                 network_->pathName(drvr_pin),
                 network_->pathName(load_pin),
                 load_cell->name(),
                 new_cell ? new_cell->name() : "none",
                 delayAsString(fanout_slack.second, 3, sta_));
    }
  }

  if (num_down_sizes == 0) {
    debugPrint(logger_,
               RSZ,
               "size_down_move",
               2,
               "REJECT SizeDownMove {}: Couldn't size down any gates",
               network_->pathName(drvr_pin));
    return false;
  }

  debugPrint(logger_,
             RSZ,
             "size_down_move",
             1,
             "ACCEPT SizeDownMove {}: Downsized {} gates",
             network_->pathName(drvr_pin),
             num_down_sizes);
  return true;
}

// This will downsize the gate to the smallest input capacitance that satisfies
// the given slack margin.
sta::LibertyCell* SizeDownMove::downSizeGate(const sta::LibertyPort* drvr_port,
                                             const sta::LibertyPort* load_port,
                                             const sta::Pin* load_pin,
                                             const sta::Scene* scene,
                                             const sta::MinMax* min_max,
                                             float slack_margin)
{
  const int lib_ap = scene->libertyIndex(min_max);
  sta::LibertyCell* load_cell = load_port->libertyCell();
  const char* load_port_name = load_port->name().c_str();

  sta::LibertyCellSeq swappable_cells = BaseMove::getSwappableCells(load_cell);
  sta::LibertyCell* best_cell = nullptr;

  if (swappable_cells.size() > 1) {
    // Sort from the smallest input capacitance to the smallest
    // breaking tie by the intrinsic delay
    std::ranges::sort(
        swappable_cells,
        [=, this](const sta::LibertyCell* cell1,
                  const sta::LibertyCell* cell2) {
          // Cast to const to use the public version of scenePort
          const sta::LibertyPort* port1
              = static_cast<const sta::LibertyPort*>(
                    cell1->findLibertyPort(load_port_name))
                    ->scenePort(lib_ap);
          const sta::LibertyPort* port2
              = static_cast<const sta::LibertyPort*>(
                    cell2->findLibertyPort(load_port_name))
                    ->scenePort(lib_ap);

          const float cap1 = port1->capacitance();
          const float cap2 = port2->capacitance();

          const sta::ArcDelay intrinsic1 = getWorstIntrinsicDelay(port1);
          const sta::ArcDelay intrinsic2 = getWorstIntrinsicDelay(port2);
          return (std::tie(cap1, intrinsic2) < std::tie(cap2, intrinsic1));
        });
  }

  if (logger_->debugCheck(RSZ, "size_down_move", 6)) {
    string swappable_names;
    for (sta::LibertyCell* swappable : swappable_cells) {
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
               "size_down_move",
               6,
               " fanout {}, swappables = {}",
               network_->pathName(load_pin),
               swappable_names.c_str());
  }

  const float load_input_cap = load_port->scenePort(lib_ap)->capacitance();

  // Get fanouts based on Liberty since STA arcs are not present in DFFs
  // Could have more than one fanout (e.g. Q and QN of a flop)
  const sta::Instance* fanout_inst = network_->instance(load_pin);
  auto output_pins = getOutputPins(fanout_inst);

  vector<float> output_caps;
  vector<float> output_slew_factors;
  vector<float> output_delays;
  vector<const char*> output_port_names;

  for (const auto& output_pin : output_pins) {
    sta::LibertyPort* output_port = network_->libertyPort(output_pin);
    const char* output_port_name = output_port->name().c_str();
    output_port_names.push_back(output_port_name);
    sta::Vertex* output_vertex = graph_->pinLoadVertex(output_pin);

    // Find output capacitance
    const float output_load_cap
        = graph_delay_calc_->loadCap(output_pin, scene, min_max);
    output_caps.push_back(output_load_cap);

    // Find output slew and slew factor
    const sta::Slew output_slew = sta_->slew(output_vertex,
                                             sta::RiseFallBoth::riseFall(),
                                             sta_->scenes(),
                                             resizer_->max_);
    float output_res = output_port->driveResistance();
    float elmore_slew_factor = 0.0;
    // Can have gates without fanout (e.g. QN of flop) which have no load
    if (output_res > 0.0 and output_load_cap > 0.0) {
      elmore_slew_factor = output_slew / (output_res * output_load_cap);
    }
    output_slew_factors.push_back(elmore_slew_factor);

    // Compute the baseline delay of the existing fanout cell outputs
    const float load_delay
        = resizer_->gateDelay(output_port, output_load_cap, scene, min_max);
    output_delays.push_back(load_delay);

    debugPrint(logger_,
               RSZ,
               "size_down_move",
               6,
               " current {}->{} gate={} delay={} cap={} slew={} slack={}",
               network_->pathName(load_pin),
               network_->pathName(output_pin),
               load_cell->name(),
               delayAsString(load_delay, 3, sta_),
               output_load_cap,
               delayAsString(output_slew, 3, sta_),
               delayAsString(slack_margin, 3, sta_));
  }

  best_cell = load_cell;
  float best_cap = load_input_cap;
  float best_area = load_cell->area();
  for (sta::LibertyCell* swappable : swappable_cells) {
    if (swappable == load_cell) {
      continue;
    }
    debugPrint(logger_,
               RSZ,
               "size_down_move",
               6,
               " considering swap {} {} -> {}",
               network_->pathName(load_pin),
               load_cell->name(),
               swappable->name());
    sta::LibertyPort* new_load_port
        = swappable->findLibertyPort(load_port_name);
    float new_input_cap = static_cast<const sta::LibertyPort*>(new_load_port)
                              ->scenePort(lib_ap)
                              ->capacitance();
    float new_area = swappable->area();

    // Input cap and area improvement checking
    // Note: we check area because we don't want a bigger cell with smaller
    // cap.
    if (new_input_cap > best_cap || new_area > best_area) {
      debugPrint(logger_,
                 RSZ,
                 "size_down_move",
                 6,
                 "  skip based on cap/area {} gate={} cap={}>{} area={}>{}",
                 network_->pathName(load_pin),
                 swappable->name(),
                 new_input_cap,
                 best_cap,
                 new_area,
                 best_area);
      continue;
    }

    // Max capacitance and slew checking
    bool skip_cell = false;
    for (int i = 0; i < output_pins.size(); i++) {
      sta::LibertyPort* output_port
          = swappable->findLibertyPort(output_port_names[i]);

      // FIXME: Only applies to current corner
      if (checkMaxCapViolation(output_pins[i], output_port, output_caps[i])) {
        skip_cell = true;
        break;
      }

      if (checkMaxSlewViolation(output_pins[i],
                                output_port,
                                output_slew_factors[i],
                                output_caps[i],
                                scene)) {
        skip_cell = true;
        break;
      }
    }

    // This is here because delay computation gets more expensive to compute.
    if (skip_cell) {
      continue;
    }

    // Compute actual slack margin once before the loop
    float actual_slack_margin = slack_margin;
    sta::Instance* load_inst = network_->instance(load_pin);
    if (load_cell->hasSequentials()) {
      // Sequential elements: use worst output slack
      actual_slack_margin = getWorstOutputSlack(load_inst);
      debugPrint(logger_,
                 RSZ,
                 "size_down_move",
                 6,
                 " Sequential element: using worst output slack: {} "
                 "(pin slack: {})",
                 delayAsString(actual_slack_margin, 3, sta_),
                 delayAsString(slack_margin, 3, sta_));
    } else {
      // For combinational gates, consider worst slack of all input pins
      float worst_input_slack = getWorstInputSlack(load_inst);
      actual_slack_margin = std::min(slack_margin, worst_input_slack);
      debugPrint(logger_,
                 RSZ,
                 "size_down_move",
                 6,
                 " Combinational gate: using worst input slack: {} (pin slack: "
                 "{}, worst input: {})",
                 delayAsString(actual_slack_margin, 3, sta_),
                 delayAsString(slack_margin, 3, sta_),
                 delayAsString(worst_input_slack, 3, sta_));
    }

    float drvr_res = drvr_port->driveResistance();
    float drvr_delta_delay = -drvr_res * (load_input_cap - new_input_cap);
    float worst_delay_change = -sta::INF;

    // Check delay change for each output pin
    for (int output_index = 0; output_index < output_pins.size();
         output_index++) {
      sta::LibertyPort* output_port
          = swappable->findLibertyPort(output_port_names[output_index]);
      float new_load_delay = resizer_->gateDelay(
          output_port, output_caps[output_index], scene, min_max);
      float delay_change = 0.0;

      if (load_cell->hasSequentials()) {
        // Sequential elements: output timing determined by clock, not inputs
        // so we don't consider the driver delay change
        delay_change = new_load_delay - output_delays[output_index];
      } else {
        // Combinational gates: include driver delay change
        delay_change
            = new_load_delay + drvr_delta_delay - output_delays[output_index];
      }

      // Track worst delay change across all outputs
      worst_delay_change = std::max(worst_delay_change, delay_change);
    }

    // Use first output for debug display (representative)
    sta::LibertyPort* first_output_port
        = swappable->findLibertyPort(output_port_names[0]);
    float first_new_load_delay = resizer_->gateDelay(
        first_output_port, output_caps[0], scene, min_max);

    debugPrint(logger_,
               RSZ,
               "size_down_move",
               6,
               " new delay {}->{} gate={} drvr_delta {} + new_delay {} - "
               "old_delay {} < "
               "slack {} ({} < {})",
               network_->pathName(load_pin),
               network_->pathName(output_pins[0]),
               swappable->name(),
               delayAsString(drvr_delta_delay, 3, sta_),
               delayAsString(first_new_load_delay, 3, sta_),
               delayAsString(output_delays[0], 3, sta_),
               delayAsString(actual_slack_margin, 3, sta_),
               delayAsString(worst_delay_change, 3, sta_),
               delayAsString(actual_slack_margin, 3, sta_));

    // First case is positive slack and delay change doesn't get worse than
    // that slack. Second case is negative slack and delay is improved, but
    // doesn't necessarily fix the slack violation entirely.
    if ((actual_slack_margin > 0 && worst_delay_change > actual_slack_margin)
        || (actual_slack_margin < 0 && worst_delay_change > 0)) {
      debugPrint(logger_,
                 RSZ,
                 "size_down_move",
                 6,
                 " skip based on delay {}->{} gate={} drvr_delta {} + "
                 "new_delay {} - "
                 "old_delay {} < "
                 "slack {} ({} < {})",
                 network_->pathName(load_pin),
                 network_->pathName(output_pins[0]),
                 swappable->name(),
                 delayAsString(drvr_delta_delay, 3, sta_),
                 delayAsString(first_new_load_delay, 3, sta_),
                 delayAsString(output_delays[0], 3, sta_),
                 delayAsString(actual_slack_margin, 3, sta_),
                 delayAsString(worst_delay_change, 3, sta_),
                 delayAsString(actual_slack_margin, 3, sta_));
      continue;
    }

    // This cell passed all checks - update best candidate
    best_cell = swappable;
    best_cap = new_input_cap;
    best_area = new_area;
    debugPrint(logger_,
               RSZ,
               "size_down_move",
               6,
               " new best size down {} -> {} ({} -> {})",
               network_->pathName(load_pin),
               network_->pathName(output_pins[0]),
               load_cell->name(),
               swappable->name());
  }

  if (best_cell != load_cell) {
    return best_cell;
  }
  return nullptr;
}

}  // namespace rsz
