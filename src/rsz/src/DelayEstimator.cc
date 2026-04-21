// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "DelayEstimator.hh"

#include <cassert>
#include <optional>
#include <string>

#include "OptimizerTypes.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/StaState.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"

namespace rsz {

namespace {

void setFailReason(FailReason* fail_reason, const FailReason reason)
{
  if (fail_reason != nullptr) {
    *fail_reason = reason;
  }
}

bool gateDelayFromTableModel(const sta::Pvt* pvt,
                             const sta::TimingArc* arc,
                             const float in_slew,
                             const float load_cap,
                             float& delay)
{
  const sta::GateTableModel* model
      = dynamic_cast<const sta::GateTableModel*>(arc->model());
  if (model == nullptr) {
    return false;
  }

  float arc_slew = 0.0f;
  model->gateDelay(pvt, in_slew, load_cap, delay, arc_slew);
  return true;
}

struct SelectedPathArc
{
  std::string from_port_name;
  std::string to_port_name;
  const sta::RiseFall* in_rf{nullptr};
  const sta::RiseFall* out_rf{nullptr};
};

const sta::Pvt* findPvt(const sta::Scene* scene,
                        sta::Instance* inst,
                        const sta::MinMax* min_max)
{
  if (scene == nullptr || min_max == nullptr) {
    return nullptr;
  }

  const sta::Pvt* pvt = scene->sdc()->pvt(inst, min_max);
  if (pvt == nullptr) {
    pvt = scene->sdc()->operatingConditions(min_max);
  }
  return pvt;
}

std::optional<SelectedPathArc> selectedPathArc(
    const sta::PathExpanded& expanded,
    const int path_index,
    const sta::StaState* sta,
    FailReason* fail_reason)
{
  if (path_index < expanded.startIndex() || path_index >= expanded.size()) {
    setFailReason(fail_reason, FailReason::kPathIndexOutOfRange);
    return std::nullopt;
  }

  const sta::Path* driver_path = expanded.path(path_index);
  const sta::TimingArc* arc
      = driver_path != nullptr ? driver_path->prevArc(sta) : nullptr;
  if (driver_path == nullptr || arc == nullptr || arc->from() == nullptr
      || arc->to() == nullptr || arc->fromEdge() == nullptr
      || arc->toEdge() == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingPrevTimingArc);
    return std::nullopt;
  }

  const sta::RiseFall* in_rf = arc->fromEdge()->asRiseFall();
  const sta::RiseFall* out_rf = arc->toEdge()->asRiseFall();
  if (in_rf == nullptr || out_rf == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingArcTransition);
    return std::nullopt;
  }

  return SelectedPathArc{.from_port_name = arc->from()->name(),
                         .to_port_name = arc->to()->name(),
                         .in_rf = in_rf,
                         .out_rf = out_rf};
}

std::optional<SelectedArc> buildSelectedArc(const Resizer& resizer,
                                            sta::Instance* inst,
                                            sta::Pin* driver_pin,
                                            const sta::PathExpanded& expanded,
                                            int path_index,
                                            const sta::Scene* scene,
                                            const sta::MinMax* min_max,
                                            FailReason* fail_reason);

std::optional<SelectedArc> buildSelectedArc(const Resizer& resizer,
                                            sta::Instance* inst,
                                            sta::Pin* driver_pin,
                                            const sta::PathExpanded& expanded,
                                            const int path_index,
                                            const sta::Scene* scene,
                                            const sta::MinMax* min_max,
                                            FailReason* fail_reason)
{
  assert(driver_pin != nullptr);
  assert(inst != nullptr);

  const sta::MinMax* actual_min_max
      = min_max != nullptr ? min_max : resizer.maxAnalysisMode();
  if (scene == nullptr || actual_min_max == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingSceneOrMinMax);
    return std::nullopt;
  }

  const sta::LibertyPort* output_port
      = resizer.network()->libertyPort(driver_pin);
  if (output_port == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingOutputPort);
    return std::nullopt;
  }

  sta::LibertyCell* current_cell
      = output_port->libertyCell()->sceneCell(scene, actual_min_max);
  if (current_cell == nullptr) {
    current_cell = output_port->libertyCell();
  }

  const std::optional<SelectedPathArc> path_arc
      = selectedPathArc(expanded, path_index, resizer.staState(), fail_reason);
  if (!path_arc.has_value()) {
    return std::nullopt;
  }

  const sta::LibertyPort* input_port
      = current_cell->findLibertyPort(path_arc->from_port_name);
  const sta::LibertyPort* current_output_port
      = current_cell->findLibertyPort(output_port->name());
  if (input_port == nullptr || current_output_port == nullptr
      || path_arc->to_port_name != current_output_port->name()) {
    setFailReason(fail_reason, FailReason::kMissingCurrentPortMap);
    return std::nullopt;
  }

  return SelectedArc{.scene = scene,
                     .min_max = actual_min_max,
                     .pvt = findPvt(scene, inst, actual_min_max),
                     .input_port = input_port,
                     .output_port = current_output_port,
                     .in_rf = path_arc->in_rf,
                     .out_rf = path_arc->out_rf};
}

bool findInputSlew(const Resizer& resizer,
                   sta::Instance* inst,
                   const SelectedArc& arc,
                   float& input_slew,
                   FailReason* fail_reason)
{
  input_slew = 0.0f;
  if (inst == nullptr || arc.scene == nullptr || arc.min_max == nullptr
      || arc.input_port == nullptr || arc.output_port == nullptr
      || arc.in_rf == nullptr || arc.out_rf == nullptr) {
    setFailReason(fail_reason, FailReason::kInvalidSelectedArc);
    return false;
  }

  const sta::Pin* input_pin
      = resizer.network()->findPin(inst, arc.input_port->name());
  if (input_pin == nullptr) {
    return true;
  }

  const sta::Vertex* input_vertex = resizer.graph()->pinDrvrVertex(input_pin);
  if (input_vertex == nullptr) {
    return true;
  }

  sta::LibertyCell* current_cell
      = const_cast<sta::LibertyPort*>(arc.output_port)->libertyCell();
  for (sta::TimingArcSet* arc_set : current_cell->timingArcSets(
           nullptr, const_cast<sta::LibertyPort*>(arc.output_port))) {
    if (arc_set->to() != arc.output_port || arc_set->role()->isTimingCheck()) {
      continue;
    }
    for (sta::TimingArc* candidate_arc : arc_set->arcs()) {
      if (candidate_arc->from() != arc.input_port
          || candidate_arc->fromEdge()->asRiseFall() != arc.in_rf
          || candidate_arc->toEdge()->asRiseFall() != arc.out_rf) {
        continue;
      }
      input_slew = resizer.staState()->graphDelayCalc()->edgeFromSlew(
          input_vertex, arc.in_rf, arc_set->role(), arc.scene, arc.min_max);
      return true;
    }
  }

  setFailReason(fail_reason, FailReason::kMissingInputSlewArc);
  return false;
}

}  // namespace

bool DelayEstimator::findArcDelay(const SelectedArc& arc,
                                  const float input_slew,
                                  const float load_cap,
                                  const sta::LibertyCell* cell,
                                  float& delay)
{
  delay = -sta::INF;
  if (cell == nullptr || arc.scene == nullptr || arc.min_max == nullptr
      || arc.input_port == nullptr || arc.output_port == nullptr
      || arc.in_rf == nullptr || arc.out_rf == nullptr) {
    return false;
  }

  sta::LibertyCell* candidate_corner_cell
      = const_cast<sta::LibertyCell*>(cell)->sceneCell(arc.scene, arc.min_max);
  if (candidate_corner_cell == nullptr) {
    candidate_corner_cell = const_cast<sta::LibertyCell*>(cell);
  }

  sta::LibertyPort* candidate_input
      = candidate_corner_cell->findLibertyPort(arc.input_port->name());
  sta::LibertyPort* candidate_output
      = candidate_corner_cell->findLibertyPort(arc.output_port->name());
  if (candidate_input == nullptr || candidate_output == nullptr) {
    return false;
  }

  const sta::TimingArcSetSeq arc_sets
      = candidate_corner_cell->timingArcSets(candidate_input, candidate_output);
  for (sta::TimingArcSet* arc_set : arc_sets) {
    if (arc_set->role()->isTimingCheck()) {
      continue;
    }
    for (sta::TimingArc* candidate_arc : arc_set->arcs()) {
      if (candidate_arc->fromEdge()->asRiseFall() != arc.in_rf
          || candidate_arc->toEdge()->asRiseFall() != arc.out_rf) {
        continue;
      }
      return gateDelayFromTableModel(
          arc.pvt, candidate_arc, input_slew, load_cap, delay);
    }
  }

  return false;
}

std::optional<ArcDelayState> DelayEstimator::buildContext(
    const Resizer& resizer,
    const Target& target,
    FailReason* fail_reason)
{
  // Capture one active path arc and its electrical state so candidates can be
  // scored without touching shared timing state.
  assert(target.canBePathDriver());

  sta::Instance* inst = target.inst(resizer);
  if (inst == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingTargetInstance);
    return std::nullopt;
  }

  sta::PathExpanded expanded(target.endpoint_path, resizer.staState());
  const sta::Scene* scene = target.endpoint_path->scene(resizer.staState());
  const sta::MinMax* min_max = target.endpoint_path->minMax(resizer.staState());
  if (min_max == nullptr) {
    min_max = resizer.maxAnalysisMode();
  }

  return buildContext(resizer,
                      inst,
                      target.driver_pin,
                      expanded,
                      target.path_index,
                      scene,
                      min_max,
                      fail_reason);
}

std::optional<ArcDelayState> DelayEstimator::buildContext(
    const Resizer& resizer,
    sta::Instance* inst,
    sta::Pin* driver_pin,
    const sta::PathExpanded& expanded,
    const int path_index,
    const sta::Scene* scene,
    const sta::MinMax* min_max,
    FailReason* fail_reason)
{
  // Capture one active path arc and its electrical state so candidates can be
  // scored without touching shared timing state.
  const std::optional<SelectedArc> selected_arc = buildSelectedArc(resizer,
                                                                   inst,
                                                                   driver_pin,
                                                                   expanded,
                                                                   path_index,
                                                                   scene,
                                                                   min_max,
                                                                   fail_reason);
  if (!selected_arc.has_value()) {
    return std::nullopt;
  }

  float input_slew = 0.0f;
  if (!findInputSlew(resizer, inst, *selected_arc, input_slew, fail_reason)) {
    return std::nullopt;
  }

  const float load_cap = resizer.staState()->graphDelayCalc()->loadCap(
      driver_pin, selected_arc->scene, selected_arc->min_max);
  sta::LibertyCell* current_cell
      = const_cast<sta::LibertyPort*>(selected_arc->output_port)->libertyCell();
  float current_delay = 0.0f;
  if (!findArcDelay(
          *selected_arc, input_slew, load_cap, current_cell, current_delay)) {
    setFailReason(fail_reason, FailReason::kMissingCurrentTimingArc);
    return std::nullopt;
  }

  ArcDelayState context;
  context.arc = *selected_arc;
  context.input_slew = input_slew;
  context.load_cap = load_cap;
  context.current_delay = current_delay;
  return context;
}

DelayEstimate DelayEstimator::estimate(const ArcDelayState& context,
                                       const sta::LibertyCell* candidate_cell)
{
  return estimate(context, candidate_cell, context.load_cap);
}

DelayEstimate DelayEstimator::estimate(const ArcDelayState& context,
                                       const sta::LibertyCell* candidate_cell,
                                       const float load_cap)
{
  // Replay the same path arc on the candidate cell without touching shared
  // timing state.
  const SelectedArc& arc = context.arc;
  if (candidate_cell == nullptr || arc.scene == nullptr
      || arc.min_max == nullptr || arc.input_port == nullptr
      || arc.output_port == nullptr) {
    return {.legal = false, .reason = FailReason::kInvalidContext};
  }

  sta::LibertyCell* candidate_corner_cell
      = const_cast<sta::LibertyCell*>(candidate_cell)
            ->sceneCell(arc.scene, arc.min_max);
  if (candidate_corner_cell == nullptr) {
    candidate_corner_cell = const_cast<sta::LibertyCell*>(candidate_cell);
  }

  sta::LibertyPort* candidate_input
      = candidate_corner_cell->findLibertyPort(arc.input_port->name());
  sta::LibertyPort* candidate_output
      = candidate_corner_cell->findLibertyPort(arc.output_port->name());
  if (candidate_input == nullptr || candidate_output == nullptr) {
    return {.legal = false, .reason = FailReason::kMissingCandidatePort};
  }

  const auto arc_sets
      = candidate_corner_cell->timingArcSets(candidate_input, candidate_output);
  for (sta::TimingArcSet* arc_set : arc_sets) {
    if (arc_set->role()->isTimingCheck()) {
      continue;
    }
    for (sta::TimingArc* arc : arc_set->arcs()) {
      if (arc->fromEdge()->asRiseFall() != context.arc.in_rf
          || arc->toEdge()->asRiseFall() != context.arc.out_rf) {
        continue;
      }

      float candidate_delay = -sta::INF;
      if (!gateDelayFromTableModel(context.arc.pvt,
                                   arc,
                                   context.input_slew,
                                   load_cap,
                                   candidate_delay)) {
        continue;
      }

      const float arrival_impr = context.current_delay - candidate_delay;
      return {.legal = arrival_impr > 0.0f,
              .candidate_delay = candidate_delay,
              .arrival_impr = arrival_impr,
              .reason = arrival_impr > 0.0f
                            ? FailReason::kEstimateLegal
                            : FailReason::kEstimateNonImproving};
    }
  }

  return {.legal = false, .reason = FailReason::kMissingCandidateTimingArc};
}

DelayEstimate DelayEstimator::estimate(const SelectedArc& arc,
                                       const float input_slew,
                                       const float load_cap,
                                       const float current_delay,
                                       const sta::LibertyCell* candidate_cell)
{
  float candidate_delay = 0.0f;
  if (!findArcDelay(
          arc, input_slew, load_cap, candidate_cell, candidate_delay)) {
    return {.legal = false, .reason = FailReason::kMissingCandidateTimingArc};
  }

  const float arrival_impr = current_delay - candidate_delay;
  return {.legal = arrival_impr > 0.0f,
          .candidate_delay = candidate_delay,
          .arrival_impr = arrival_impr,
          .reason = arrival_impr > 0.0f ? FailReason::kEstimateLegal
                                        : FailReason::kEstimateNonImproving};
}

}  // namespace rsz
