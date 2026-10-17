// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SplitLoadMtGenerator.hh"

#include <algorithm>
#include <memory>
#include <optional>
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
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

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

// Prepare the target-net and new-net cap estimates for a split candidate.
std::optional<LoadSplitModel> buildLoadSplitModel(
    Resizer& resizer,
    const Target& target,
    const std::vector<const sta::Pin*>& moved_pins,
    const odb::Point& new_driver_location,
    const std::unordered_set<const sta::Pin*>* critical_load_pins,
    const float off_path_weight)
{
  if (!target.estimator_context.has_value() || moved_pins.empty()) {
    return std::nullopt;
  }

  sta::dbNetwork* network = resizer.dbNetwork();
  sta::Net* net = network->dbToSta(network->flatNet(target.driver_pin));
  if (net == nullptr) {
    return std::nullopt;
  }

  const SelectedArc& arc = target.estimator_context->target().arc;
  double wire_res_per_meter = 0.0;
  double wire_cap_per_meter = 0.0;
  resizer.estimateParasitics()->wireSignalRC(
      arc.scene, wire_res_per_meter, wire_cap_per_meter);

  const int lib_ap = arc.scene->libertyIndex(arc.min_max);
  LoadSplitModel model;
  const std::unordered_set<const sta::Pin*> moved(moved_pins.begin(),
                                                  moved_pins.end());

  const odb::Point driver_location = network->location(target.driver_pin);
  BoundingBox all_box;
  BoundingBox kept_box;
  BoundingBox moved_box;
  all_box.add(driver_location);
  kept_box.add(driver_location);
  moved_box.add(new_driver_location);

  sta::dbSta* sta = resizer.sta();
  std::unique_ptr<sta::NetConnectedPinIterator> pin_iter(
      network->connectedPinIterator(net));
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
      model.moved_pin_cap += scene_port->capacitance();
    }

    // The moved load keeps its current slack as the budget for the new
    // driver's extra delay.
    sta::Vertex* vertex = sta->graph()->pinLoadVertex(pin);
    const sta::Slack slack = vertex != nullptr ? sta->slack(vertex, arc.min_max)
                                               : sta::Slack(sta::INF);
    if (sta::delayInf(slack, sta)) {
      continue;
    }

    model.moved_loads.push_back(
        {.pin = pin,
         .slack = sta::delayAsFloat(slack),
         .weight = sideLoadWeight(pin, critical_load_pins, off_path_weight)});
  }

  const double meters_per_dbu = resizer.dbuToMeters(1);
  model.target_wire_cap_saved = static_cast<float>(
      wire_cap_per_meter * meters_per_dbu
      * std::max(all_box.halfPerimeter() - kept_box.halfPerimeter(), 0));
  model.moved_wire_cap = static_cast<float>(wire_cap_per_meter * meters_per_dbu
                                            * moved_box.halfPerimeter());
  return model;
}

bool resolveDriverPin(Resizer& resizer,
                      const Target& target,
                      sta::Pin*& drvr_pin,
                      sta::Vertex*& drvr_vertex)
{
  if (!target.canBePathDriver()) {
    return false;
  }

  drvr_pin = target.resolvedPin(resizer);
  if (drvr_pin == nullptr) {
    return false;
  }
  if (target.fanout <= kSplitLoadMinFanout) {
    debugPrint(resizer.logger(),
               RSZ,
               "split_load_move",
               2,
               "REJECT SplitLoadMove {}: Fanout {} <= {} min fanout",
               resizer.network()->pathName(drvr_pin),
               target.fanout,
               kSplitLoadMinFanout);
    return false;
  }
  if (!resizer.okToBufferNet(drvr_pin)) {
    debugPrint(resizer.logger(),
               RSZ,
               "split_load_move",
               2,
               "REJECT SplitLoadMove {}: Not OK to buffer net",
               resizer.network()->pathName(drvr_pin));
    return false;
  }

  drvr_vertex = target.vertex(resizer);
  return drvr_vertex != nullptr;
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
sta::PinSet chooseSplitLoads(Resizer& resizer,
                             const std::vector<FanoutSlack>& fanout_slacks)
{
  sta::PinSet load_pins(resizer.network());
  const int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; ++i) {
    sta::Pin* split_pin = fanout_slacks[i].first->pin();
    if (!resizer.network()->isTopLevelPort(split_pin)) {
      load_pins.insert(split_pin);
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
  return MoveGenerator::isApplicable(target)
         && target.isPrepared(kEstimatorContextCache);
}

void SplitLoadMtGenerator::prepareInputs(const GeneratorContext& context,
                                         Target& target)
{
  Resizer& resizer = context.resizer;
  sta::Pin* drvr_pin = nullptr;
  sta::Vertex* drvr_vertex = nullptr;
  if (!resolveDriverPin(resizer, target, drvr_pin, drvr_vertex)) {
    return;
  }

  const std::vector<FanoutSlack> fanout_slacks
      = collectRankedFanoutSlacks(resizer, target, drvr_vertex);
  if (fanout_slacks.empty()) {
    return;
  }

  sta::PinSet load_pins = chooseSplitLoads(resizer, fanout_slacks);
  if (load_pins.empty()) {
    return;
  }

  sta::LibertyCell* buffer_cell = resizer.lowestDriveBufferCell();
  odb::dbNet* db_drvr_net = nullptr;
  odb::dbModNet* db_mod_drvr_net = nullptr;
  resizer.dbNetwork()->net(drvr_pin, db_drvr_net, db_mod_drvr_net);
  sta::Net* drvr_net = resizer.dbNetwork()->dbToSta(db_drvr_net);
  if (buffer_cell == nullptr || drvr_net == nullptr) {
    return;
  }

  // The split model charges the moved loads their new arrival.
  const odb::Point drvr_loc = resizer.dbNetwork()->location(drvr_pin);
  const std::vector<const sta::Pin*> moved_pins(load_pins.begin(),
                                                load_pins.end());
  std::optional<LoadSplitModel> model
      = buildLoadSplitModel(resizer,
                            target,
                            moved_pins,
                            drvr_loc,
                            context.critical_load_pins,
                            context.policy_config.fanin_side_offpath_weight);
  if (!model.has_value()) {
    return;
  }

  // apply() resizes the new buffer to the target slew of its load, so the
  // estimate scores the cell it ends with.
  sta::LibertyCell* resized_cell = resizer.findTargetCell(
      buffer_cell, model->moved_pin_cap + model->moved_wire_cap, false);
  const SelectedArc& arc = target.estimator_context->target().arc;
  sta::LibertyCell* scene_buffer
      = resized_cell->sceneCell(arc.scene, arc.min_max);
  sta::LibertyPort* buffer_input = nullptr;
  sta::LibertyPort* buffer_output = nullptr;
  if (scene_buffer != nullptr) {
    scene_buffer->bufferPorts(buffer_input, buffer_output);
  }
  if (buffer_input == nullptr) {
    return;
  }

  const float target_load_delta = buffer_input->capacitance()
                                  - model->moved_pin_cap
                                  - model->target_wire_cap_saved;
  target.split_load_input = std::make_shared<const SplitLoadMtInput>(
      SplitLoadMtInput{.drvr_net = drvr_net,
                       .drvr_loc = drvr_loc,
                       .load_pins = std::move(load_pins),
                       .buffer_cell = buffer_cell,
                       .resized_cell = resized_cell,
                       .model = std::move(*model),
                       .target_load_delta = target_load_delta});
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
