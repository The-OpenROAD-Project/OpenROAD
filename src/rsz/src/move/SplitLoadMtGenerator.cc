// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SplitLoadMtGenerator.hh"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SplitLoadMtCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Transition.hh"

namespace rsz {

namespace {

constexpr int kSplitLoadMinFanout = 8;
using FanoutSlack = std::pair<sta::Vertex*, sta::Slack>;

// Half-perimeter of a pin bounding box, grown point by point.
class BoundingBox
{
 public:
  void add(const odb::Point& point)
  {
    if (empty_) {
      box_ = odb::Rect(point, point);
      empty_ = false;
      return;
    }
    box_.merge(point);
  }

  int halfPerimeter() const { return empty_ ? 0 : box_.dx() + box_.dy(); }

 private:
  odb::Rect box_;
  bool empty_{true};
};

// Fills the split model of input from its net, driver location and moved
// loads: the moved loads with their slack budgets, their pin capacitance,
// and the wire capacitance each net gains or loses.
void buildLoadSplitModel(
    Resizer& resizer,
    const SelectedArc& arc,
    const std::unordered_set<const sta::Pin*>* critical_load_pins,
    const float off_path_weight,
    SplitLoadMtInput& input)
{
  sta::dbNetwork* network = resizer.dbNetwork();
  double wire_res_per_meter = 0.0;
  double wire_cap_per_meter = 0.0;
  resizer.estimateParasitics()->wireSignalRC(
      arc.scene, wire_res_per_meter, wire_cap_per_meter);

  const int lib_ap = arc.scene->libertyIndex(arc.min_max);
  const std::unordered_set<const sta::Pin*> moved(input.load_pins.begin(),
                                                  input.load_pins.end());
  BoundingBox all_box;
  BoundingBox kept_box;
  BoundingBox moved_box;
  all_box.add(input.drvr_loc);
  kept_box.add(input.drvr_loc);
  moved_box.add(input.drvr_loc);

  sta::dbSta* sta = resizer.sta();
  std::unique_ptr<sta::NetConnectedPinIterator> pin_iter(
      network->connectedPinIterator(input.drvr_net));
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    if (!network->isLoad(pin) || network->isHierarchical(pin)) {
      continue;
    }

    const odb::Point location = network->location(pin);
    all_box.add(location);
    if (!moved.contains(pin)) {
      kept_box.add(location);
      continue;
    }

    moved_box.add(location);
    const sta::LibertyPort* port = network->libertyPort(pin);
    const sta::LibertyPort* scene_port
        = port != nullptr ? port->scenePort(lib_ap) : nullptr;
    if (scene_port != nullptr) {
      input.moved_pin_cap += scene_port->capacitance();
    }

    // The moved load keeps its current slack as the budget for the new
    // driver's extra delay.
    sta::Vertex* vertex = sta->graph()->pinLoadVertex(pin);
    const sta::Slack slack = vertex != nullptr ? sta->slack(vertex, arc.min_max)
                                               : sta::Slack(sta::INF);
    if (sta::delayInf(slack, sta)) {
      continue;
    }

    input.moved_loads.push_back(
        {.pin = pin,
         .slack = sta::delayAsFloat(slack),
         .weight = sideLoadWeight(pin, critical_load_pins, off_path_weight)});
  }

  const double meters_per_dbu = resizer.dbuToMeters(1);
  input.target_wire_cap_saved = static_cast<float>(
      wire_cap_per_meter * meters_per_dbu
      * std::max(all_box.halfPerimeter() - kept_box.halfPerimeter(), 0));
  input.moved_wire_cap = static_cast<float>(wire_cap_per_meter * meters_per_dbu
                                            * moved_box.halfPerimeter());
}

std::vector<FanoutSlack> collectRankedFanoutSlacks(Resizer& resizer,
                                                   const Target& target,
                                                   sta::Vertex* drvr_vertex)
{
  std::vector<FanoutSlack> fanout_slacks;
  const sta::Path* driver_path = target.driverPath(resizer);
  const sta::RiseFall* rf = driver_path != nullptr
                                ? driver_path->transition(resizer.staState())
                                : nullptr;
  if (rf == nullptr) {
    return fanout_slacks;
  }

  const sta::Slack drvr_slack
      = resizer.sta()->slack(drvr_vertex, resizer.maxAnalysisMode());
  sta::VertexOutEdgeIterator edge_iter(drvr_vertex, resizer.graph());
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (!edge->isWire()) {
      continue;
    }

    sta::Vertex* fanout_vertex = edge->to(resizer.graph());
    const sta::Slack fanout_slack
        = resizer.sta()->slack(fanout_vertex, rf, resizer.maxAnalysisMode());
    fanout_slacks.emplace_back(fanout_vertex, fanout_slack - drvr_slack);
  }

  std::ranges::sort(fanout_slacks,
                    [&resizer](const FanoutSlack& lhs, const FanoutSlack& rhs) {
                      return lhs.second > rhs.second
                             || (lhs.second == rhs.second
                                 && resizer.network()->pathNameLess(
                                     lhs.first->pin(), rhs.first->pin()));
                    });
  return fanout_slacks;
}

// The less critical half of the loads, without top-level ports.
sta::PinSeq chooseSplitLoads(Resizer& resizer,
                             const std::vector<FanoutSlack>& fanout_slacks)
{
  sta::PinSeq load_pins;
  const size_t split_index = fanout_slacks.size() / 2;
  for (size_t i = 0; i < split_index; ++i) {
    const sta::Pin* split_pin = fanout_slacks[i].first->pin();
    if (!resizer.network()->isTopLevelPort(split_pin)) {
      load_pins.push_back(split_pin);
    }
  }
  return load_pins;
}

}  // namespace

SplitLoadMtGenerator::SplitLoadMtGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool SplitLoadMtGenerator::isApplicable(const Target& target) const
{
  // A split pays off only on a large fanout, and the net must accept a
  // buffer.
  return MoveGenerator::isApplicable(target)
         && target.estimator_context.has_value()
         && target.fanout > kSplitLoadMinFanout
         && resizer_.okToBufferNet(target.driver_pin);
}

void SplitLoadMtGenerator::prepareInputs(Target& target) const
{
  Resizer& resizer = resizer_;
  sta::Pin* drvr_pin = target.driver_pin;
  sta::Vertex* drvr_vertex = target.vertex(resizer);
  if (drvr_vertex == nullptr) {
    return;
  }

  const std::vector<FanoutSlack> fanout_slacks
      = collectRankedFanoutSlacks(resizer, target, drvr_vertex);
  if (fanout_slacks.empty()) {
    return;
  }

  SplitLoadMtInput input;
  input.load_pins = chooseSplitLoads(resizer, fanout_slacks);
  if (input.load_pins.empty()) {
    return;
  }

  input.buffer_cell = resizer.lowestDriveBufferCell();
  odb::dbNet* db_drvr_net = nullptr;
  odb::dbModNet* db_mod_drvr_net = nullptr;
  resizer.dbNetwork()->net(drvr_pin, db_drvr_net, db_mod_drvr_net);
  input.drvr_net = resizer.dbNetwork()->dbToSta(db_drvr_net);
  if (input.buffer_cell == nullptr || input.drvr_net == nullptr) {
    return;
  }

  // The split model charges the moved loads their new arrival.
  input.drvr_loc = resizer.dbNetwork()->location(drvr_pin);
  const SelectedArc& arc = target.estimator_context->target().arc;
  buildLoadSplitModel(resizer,
                      arc,
                      critical_load_pins_,
                      policy_config_.fanin_side_offpath_weight,
                      input);

  // apply() resizes the new buffer to the target slew of its load, so the
  // estimate scores the cell it ends with.  This keeps the decision in
  // prepareInputs(): findTargetCell() sizes with the Resizer's delay
  // calculator, which a worker thread cannot use, and the serial move sizes
  // the same way.
  input.resized_cell = resizer.findTargetCell(
      input.buffer_cell, input.moved_pin_cap + input.moved_wire_cap, false);
  sta::LibertyCell* scene_buffer
      = input.resized_cell->sceneCell(arc.scene, arc.min_max);
  sta::LibertyPort* buffer_input = nullptr;
  sta::LibertyPort* buffer_output = nullptr;
  if (scene_buffer != nullptr) {
    scene_buffer->bufferPorts(buffer_input, buffer_output);
  }
  if (buffer_input == nullptr) {
    return;
  }

  input.target_load_delta = buffer_input->capacitance() - input.moved_pin_cap
                            - input.target_wire_cap_saved;
  target.split_load_input
      = std::make_shared<const SplitLoadMtInput>(std::move(input));
}

std::vector<std::unique_ptr<MoveCandidate>> SplitLoadMtGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  if (target.split_load_input == nullptr) {
    return candidates;
  }

  candidates.push_back(std::make_unique<SplitLoadMtCandidate>(
      resizer_,
      target,
      target.split_load_input,
      policy_config_.split_load_mt_score_scale));
  return candidates;
}

}  // namespace rsz
