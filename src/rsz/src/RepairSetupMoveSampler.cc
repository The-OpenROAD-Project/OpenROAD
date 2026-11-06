// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RepairSetupMoveSampler.hh"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

#include "OptimizerTypes.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"

namespace rsz {

namespace {

bool finiteDelay(const sta::Delay& delay, const sta::StaState* sta)
{
  return !sta::delayInf(delay, sta) && std::isfinite(sta::delayAsFloat(delay));
}

}  // namespace

RepairSetupMoveSampler::RepairSetupMoveSampler(
    Resizer& resizer,
    const Target& target,
    const sta::Pin* original_endpoint,
    const sta::Pin* policy_endpoint)
    : resizer_(resizer)
{
  sta::Network* network = resizer_.network();
  if (target.driver_pin != nullptr) {
    target_pin_name_ = network->pathName(target.driver_pin);
  }

  const sta::Pin* endpoint = original_endpoint != nullptr
                                 ? original_endpoint
                                 : target.endpointPin(resizer_);
  if (endpoint != nullptr) {
    endpoint_pin_name_ = network->pathName(endpoint);
  }

  // A path target that does not carry its violated endpoint, as the LEGACY
  // policies build, takes the endpoint the policy repairs.
  const sta::Pin* violated_endpoint = target.violatedEndpointPin(resizer_);
  if (target.violated_endpoint_pin == nullptr && violated_endpoint != nullptr
      && policy_endpoint != nullptr) {
    violated_endpoint = policy_endpoint;
  }
  if (violated_endpoint != nullptr) {
    violated_endpoint_pin_name_ = network->pathName(violated_endpoint);
  }

  // Capture every path identity before any reporting query can refresh STA.
  if (target.endpoint_path != nullptr) {
    selected_path_end_
        = network->pathName(target.endpoint_path->pin(resizer_.staState()));
    captureWindow(target);
  }

  target_net_pin_names_ = targetNetPins();
  captureSelectedWindow(target);
}

void RepairSetupMoveSampler::captureWindow(const Target& target)
{
  if (!target.estimator_context.has_value()
      || target.estimator_context->path_stages.empty()) {
    return;
  }

  const sta::StaState* sta = resizer_.staState();
  sta::PathExpanded expanded(target.endpoint_path, sta);
  if (latchDataPath(expanded) != nullptr) {
    return;
  }

  // Preserve the full expanded chain, including the launch clock context.
  for (std::size_t index = 0; index < expanded.size(); ++index) {
    const sta::Path* path = expanded.path(index);
    const sta::Edge* edge = path->prevEdge(sta);
    path_points_.push_back({resizer_.network()->pathName(path->pin(sta)),
                            path->scene(sta)->name(),
                            path->minMax(sta),
                            path->transition(sta),
                            path->clkEdge(sta),
                            path->prevArc(sta),
                            edge != nullptr && edge->isWire(),
                            path->vertex(sta)->isBidirectDriver()});
  }

  for (const DelayStageState& stage : target.estimator_context->path_stages) {
    if (stage.path_index < 0
        || static_cast<std::size_t>(stage.path_index) >= expanded.size()) {
      stage_indices_.clear();
      return;
    }

    const sta::Path* path = expanded.path(stage.path_index);
    const sta::Path* input = path->prevPath();
    if (input == nullptr || stage.arc.min_max != resizer_.maxAnalysisMode()
        || path->minMax(sta) != stage.arc.min_max
        || path->scene(sta) != stage.arc.scene
        || path->transition(sta) != stage.arc.outputRiseFall()
        || input->transition(sta) != stage.arc.inputRiseFall()
        || matchTimingArc(
               stage.arc.ref_arc, path->prevArc(sta), ArcMatchMode::kExact)
               != ArcMatchType::kExact
        || path->prevEdge(sta)->isWire()) {
      stage_indices_.clear();
      return;
    }

    stage_indices_.push_back(stage.path_index);
  }
}

std::set<std::string> RepairSetupMoveSampler::targetNetPins() const
{
  std::set<std::string> names;
  sta::Network* network = resizer_.network();
  const sta::Pin* driver = network->findPin(target_pin_name_);
  if (driver != nullptr) {
    std::unique_ptr<sta::PinConnectedPinIterator> pins(
        network->connectedPinIterator(driver));
    while (pins->hasNext()) {
      names.insert(network->pathName(pins->next()));
    }
  }

  return names;
}

std::optional<double> RepairSetupMoveSampler::pinSlack(
    const std::string& pin_name) const
{
  const sta::Pin* pin = resizer_.network()->findPin(pin_name);
  if (pin == nullptr) {
    return std::nullopt;
  }

  sta::Vertex* vertex;
  sta::Vertex* bidirect_driver;
  resizer_.graph()->pinVertices(pin, vertex, bidirect_driver);
  const sta::StaState* sta = resizer_.staState();
  double minimum = std::numeric_limits<double>::infinity();

  // Read existing propagated required/arrival times, without a path search.
  for (sta::Vertex* observed : {vertex, bidirect_driver}) {
    if (observed == nullptr) {
      continue;
    }

    sta::VertexPathIterator paths(observed, sta);
    while (paths.hasNext()) {
      const sta::Path* path = paths.next();
      const sta::Slack slack = path->slack(sta);
      if (path->minMax(sta) == resizer_.maxAnalysisMode()
          && finiteDelay(path->required(), sta)
          && finiteDelay(path->arrival(), sta) && finiteDelay(slack, sta)) {
        minimum
            = std::min(minimum, static_cast<double>(sta::delayAsFloat(slack)));
      }
    }
  }

  return std::isfinite(minimum) ? std::optional<double>(minimum) : std::nullopt;
}

bool RepairSetupMoveSampler::matchesPath(
    const sta::PathExpanded& expanded) const
{
  if (expanded.size() != path_points_.size()
      || latchDataPath(expanded) != nullptr) {
    return false;
  }

  const sta::StaState* sta = resizer_.staState();
  for (std::size_t index = 0; index < path_points_.size(); ++index) {
    const PathPoint& fixed = path_points_[index];
    const sta::Path* path = expanded.path(index);
    const sta::Edge* edge = path->prevEdge(sta);
    const sta::TimingArc* arc = path->prevArc(sta);
    if (resizer_.network()->findPin(fixed.pin_name) != path->pin(sta)
        || path->scene(sta)->name() != fixed.scene_name
        || path->minMax(sta) != fixed.min_max
        || path->transition(sta) != fixed.rf
        || path->clkEdge(sta) != fixed.clock_edge
        || path->vertex(sta)->isBidirectDriver() != fixed.bidirect_driver
        || (edge != nullptr && edge->isWire()) != fixed.wire
        || (arc == nullptr) != (fixed.arc == nullptr)) {
      return false;
    }

    if (!fixed.wire && fixed.arc != nullptr
        && matchTimingArc(fixed.arc, arc, ArcMatchMode::kExact)
               != ArcMatchType::kExact) {
      return false;
    }
  }

  return true;
}

std::optional<double> RepairSetupMoveSampler::windowDelay() const
{
  if (stage_indices_.empty() || targetNetPins() != target_net_pin_names_) {
    return std::nullopt;
  }

  // The window is measured on the captured path, which ends at the selected
  // path end: the endpoint, the target driver of a prefix, or a latch D pin.
  const sta::Pin* path_end
      = resizer_.network()->findPin(path_points_.back().pin_name);
  if (path_end == nullptr) {
    return std::nullopt;
  }

  sta::Vertex* vertex = path_points_.back().bidirect_driver
                            ? resizer_.graph()->pinDrvrVertex(path_end)
                            : resizer_.graph()->pinLoadVertex(path_end);
  if (vertex == nullptr) {
    return std::nullopt;
  }

  const sta::StaState* sta = resizer_.staState();
  sta::VertexPathIterator paths(vertex, sta);
  while (paths.hasNext()) {
    sta::PathExpanded expanded(paths.next(), sta);
    if (!matchesPath(expanded)) {
      continue;
    }

    double delay = 0.0;
    for (const std::size_t index : stage_indices_) {
      const sta::Path* path = expanded.path(index);
      const sta::ArcDelay arc_delay
          = resizer_.graph()->arcDelay(path->prevEdge(sta),
                                       path->prevArc(sta),
                                       path->dcalcAnalysisPtIndex(sta));
      if (!finiteDelay(arc_delay, sta)) {
        return std::nullopt;
      }

      delay += sta::delayAsFloat(arc_delay);
    }
    return delay;
  }

  return std::nullopt;
}

std::map<std::string, double> RepairSetupMoveSampler::sample() const
{
  std::map<std::string, double> values;
  sta::dbSta* sta = resizer_.sta();
  const sta::MinMax* max = resizer_.maxAnalysisMode();
  const sta::Slack wns = sta->worstSlack(max);
  const sta::Slack tns = sta->totalNegativeSlack(max);
  if (finiteDelay(wns, sta)) {
    values["wns_s"] = sta::delayAsFloat(wns);
  }

  if (finiteDelay(tns, sta)) {
    values["tns_s"] = sta::delayAsFloat(tns);
  }

  if (const std::optional<double> slack = pinSlack(target_pin_name_)) {
    values["target_slack_s"] = *slack;
  }

  if (const std::optional<double> delay = windowDelay()) {
    values["window_delay_s"] = *delay;
  }

  return values;
}

}  // namespace rsz
