// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "DelayEstimator.hh"

#include <algorithm>
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

bool gateDelayAndSlewFromTableModel(const sta::Pvt* pvt,
                                    const sta::TimingArc* arc,
                                    const float in_slew,
                                    const float load_cap,
                                    float& delay,
                                    float& output_slew)
{
  const sta::GateTableModel* model
      = dynamic_cast<const sta::GateTableModel*>(arc->model());
  if (model == nullptr) {
    return false;
  }

  output_slew = 0.0f;
  model->gateDelay(pvt, in_slew, load_cap, delay, output_slew);
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
    setFailReason(fail_reason, FailReason::kMissingCurrentPortMap);
    return std::nullopt;
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

sta::LibertyCell* currentCell(const SelectedArc& arc)
{
  return const_cast<sta::LibertyPort*>(arc.output_port)->libertyCell();
}

bool findInputSlew(const Resizer& resizer,
                   sta::Instance* inst,
                   const SelectedArc& arc,
                   float& input_slew,
                   FailReason* fail_reason)
{
  input_slew = 0.0f;
  const sta::Pin* input_pin
      = resizer.network()->findPin(inst, arc.input_port->name());
  if (input_pin == nullptr) {
    return true;
  }

  const sta::Vertex* input_vertex = resizer.graph()->pinDrvrVertex(input_pin);
  if (input_vertex == nullptr) {
    return true;
  }

  sta::LibertyCell* current_cell = currentCell(arc);
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

bool findCandidatePorts(const SelectedArc& arc,
                        const sta::LibertyCell* cell,
                        sta::LibertyCell*& scene_cell,
                        sta::LibertyPort*& candidate_input,
                        sta::LibertyPort*& candidate_output)
{
  scene_cell = nullptr;
  candidate_input = nullptr;
  candidate_output = nullptr;
  if (cell == nullptr) {
    return false;
  }

  scene_cell
      = const_cast<sta::LibertyCell*>(cell)->sceneCell(arc.scene, arc.min_max);
  if (scene_cell == nullptr) {
    return false;
  }
  candidate_input = scene_cell->findLibertyPort(arc.input_port->name());
  candidate_output = scene_cell->findLibertyPort(arc.output_port->name());
  return candidate_input != nullptr && candidate_output != nullptr;
}

bool findArcDelayAndSlew(const SelectedArc& arc,
                         const float input_slew,
                         const float load_cap,
                         const sta::LibertyCell* cell,
                         float& delay,
                         float& output_slew)
{
  delay = -sta::INF;
  output_slew = 0.0f;

  sta::LibertyCell* scene_cell = nullptr;
  sta::LibertyPort* candidate_input = nullptr;
  sta::LibertyPort* candidate_output = nullptr;
  if (!findCandidatePorts(
          arc, cell, scene_cell, candidate_input, candidate_output)) {
    return false;
  }

  const sta::TimingArcSetSeq arc_sets
      = scene_cell->timingArcSets(candidate_input, candidate_output);
  for (sta::TimingArcSet* arc_set : arc_sets) {
    if (arc_set->role()->isTimingCheck()) {
      continue;
    }
    for (sta::TimingArc* candidate_arc : arc_set->arcs()) {
      if (candidate_arc->fromEdge()->asRiseFall() != arc.in_rf
          || candidate_arc->toEdge()->asRiseFall() != arc.out_rf) {
        continue;
      }
      return gateDelayAndSlewFromTableModel(
          arc.pvt, candidate_arc, input_slew, load_cap, delay, output_slew);
    }
  }

  return false;
}

std::optional<DelayStageState> buildDelayStageState(
    const Resizer& resizer,
    sta::Instance* inst,
    sta::Pin* driver_pin,
    const sta::PathExpanded& expanded,
    const int path_index,
    const sta::Scene* scene,
    const sta::MinMax* min_max,
    FailReason* fail_reason)
{
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
  const sta::LibertyCell* current_cell = currentCell(*selected_arc);
  float current_delay = 0.0f;
  float current_slew = 0.0f;
  if (!findArcDelayAndSlew(*selected_arc,
                           input_slew,
                           load_cap,
                           current_cell,
                           current_delay,
                           current_slew)) {
    setFailReason(fail_reason, FailReason::kMissingCurrentTimingArc);
    return std::nullopt;
  }

  DelayStageState stage;
  stage.arc = *selected_arc;
  stage.input_slew = input_slew;
  stage.load_cap = load_cap;
  stage.current_delay = current_delay;
  stage.current_slew = current_slew;
  stage.path_index = path_index;
  return stage;
}

std::optional<DelayStageState> buildDelayStageStateFromPath(
    const Resizer& resizer,
    const sta::PathExpanded& expanded,
    const int path_index,
    const sta::Scene* scene,
    const sta::MinMax* min_max,
    FailReason* fail_reason)
{
  if (path_index < expanded.startIndex() || path_index >= expanded.size()) {
    setFailReason(fail_reason, FailReason::kPathIndexOutOfRange);
    return std::nullopt;
  }

  const sta::Path* path = expanded.path(path_index);
  sta::Pin* driver_pin
      = path != nullptr ? path->pin(resizer.staState()) : nullptr;
  sta::Instance* inst = driver_pin != nullptr
                            ? resizer.network()->instance(driver_pin)
                            : nullptr;
  if (inst == nullptr) {
    setFailReason(fail_reason, FailReason::kMissingTargetInstance);
    return std::nullopt;
  }

  return buildDelayStageState(resizer,
                              inst,
                              driver_pin,
                              expanded,
                              path_index,
                              scene,
                              min_max,
                              fail_reason);
}

ArcDelayState makeArcDelayState(const DelayStageState& target_stage,
                                const int delay_levels)
{
  ArcDelayState context;
  context.arc = target_stage.arc;
  context.input_slew = target_stage.input_slew;
  context.load_cap = target_stage.load_cap;
  context.current_delay = target_stage.current_delay;
  context.current_slew = target_stage.current_slew;
  context.path_index = target_stage.path_index;
  context.delay_estimation_levels = delay_levels;
  return context;
}

void collectDelayWindow(const Resizer& resizer,
                        const sta::PathExpanded& expanded,
                        const int target_path_index,
                        const sta::Scene* scene,
                        const sta::MinMax* min_max,
                        const int delay_levels,
                        ArcDelayState& context,
                        const DelayStageState& target_stage)
{
  std::vector<DelayStageState> fanin_stages;
  fanin_stages.reserve(delay_levels);
  int found_fanin_stages = 0;
  for (int path_index = target_path_index - 1;
       path_index >= expanded.startIndex() && found_fanin_stages < delay_levels;
       --path_index) {
    FailReason ignored_reason = FailReason::kNone;
    const std::optional<DelayStageState> stage = buildDelayStageStateFromPath(
        resizer, expanded, path_index, scene, min_max, &ignored_reason);
    if (!stage.has_value()) {
      continue;
    }
    fanin_stages.push_back(*stage);
    ++found_fanin_stages;
  }
  std::reverse(fanin_stages.begin(), fanin_stages.end());

  context.path_stages = std::move(fanin_stages);
  context.target_stage_index = static_cast<int>(context.path_stages.size());
  context.path_stages.reserve(context.path_stages.size() + 1 + delay_levels);
  context.path_stages.push_back(target_stage);

  int found_fanout_stages = 0;
  for (int path_index = target_path_index + 1;
       path_index < expanded.size() && found_fanout_stages < delay_levels;
       ++path_index) {
    FailReason ignored_reason = FailReason::kNone;
    const std::optional<DelayStageState> stage = buildDelayStageStateFromPath(
        resizer, expanded, path_index, scene, min_max, &ignored_reason);
    if (!stage.has_value()) {
      continue;
    }
    context.path_stages.push_back(*stage);
    ++found_fanout_stages;
  }
}

DelayEstimate makeEstimate(const float current_delay,
                           const float candidate_delay)
{
  const float arrival_impr = current_delay - candidate_delay;
  return {.legal = arrival_impr > 0.0f,
          .candidate_delay = candidate_delay,
          .arrival_impr = arrival_impr,
          .reason = arrival_impr > 0.0f ? FailReason::kEstimateLegal
                                        : FailReason::kEstimateNonImproving};
}

float totalCurrentDelay(const std::vector<DelayStageState>& stages)
{
  float total_delay = 0.0f;
  for (const DelayStageState& stage : stages) {
    total_delay += stage.current_delay;
  }
  return total_delay;
}

// Score a candidate over a path-local fanin/fanout window using cached delay
// state.
//
// Approximation note: from the fanin neighbor onward, each stage feeds the
// previous stage's table-model gate output slew (propagated_slew) into the
// next stage as input slew.  This bypasses the interconnect (RC) slew
// degradation that STA accounts for between a driver's output pin and the
// next cell's input pin.  As a result, post-swap input slews along the
// chain are sharper than what STA would report, which biases gate delays to
// look slightly faster than reality (optimistic).  This is an intentional
// speed/accuracy trade-off for candidate ranking.
DelayEstimate estimateWindow(const ArcDelayState& context,
                             const sta::LibertyCell* candidate_cell)
{
  const std::vector<DelayStageState>& stages = context.path_stages;
  const int target_index = context.target_stage_index;
  const DelayStageState& target_stage = stages[target_index];

  sta::LibertyCell* target_scene_cell = nullptr;
  sta::LibertyPort* candidate_input = nullptr;
  sta::LibertyPort* candidate_output = nullptr;
  if (!findCandidatePorts(target_stage.arc,
                          candidate_cell,
                          target_scene_cell,
                          candidate_input,
                          candidate_output)) {
    return {.legal = false, .reason = FailReason::kMissingCandidatePort};
  }

  const float current_target_input_cap
      = target_stage.arc.input_port->capacitance();
  const float candidate_target_input_cap = candidate_input->capacitance();
  const float target_input_cap_delta
      = candidate_target_input_cap - current_target_input_cap;

  const float current_total_delay = totalCurrentDelay(stages);
  float candidate_total_delay = 0.0f;
  float propagated_slew = 0.0f;
  bool has_propagated_slew = false;

  for (int stage_index = 0; stage_index < static_cast<int>(stages.size());
       ++stage_index) {
    const DelayStageState& stage = stages[stage_index];

    // Stages before the fanin neighbor are unaffected by the swap since
    // load cap cannot be changed.
    if (stage_index < target_index - 1) {
      candidate_total_delay += stage.current_delay;
      continue;
    }

    // Per-stage role: the target stage uses the candidate cell; the fanin
    // neighbor sees the candidate's adjusted input cap; downstream stages
    // reuse the original cell.  The propagated slew formula is uniform: the
    // fanin neighbor reaches here first, so has_propagated_slew is still
    // false there and the ternary collapses to stage.input_slew.
    const bool is_target = (stage_index == target_index);
    const bool is_fanin_neighbor = (stage_index == target_index - 1);
    const sta::LibertyCell* cell
        = is_target ? candidate_cell : currentCell(stage.arc);
    const float load_cap = is_fanin_neighbor
                               ? stage.load_cap + target_input_cap_delta
                               : stage.load_cap;
    const float input_slew
        = has_propagated_slew ? propagated_slew : stage.input_slew;
    const FailReason fail_reason = is_target
                                       ? FailReason::kMissingCandidateTimingArc
                                       : FailReason::kMissingCurrentTimingArc;

    float stage_delay = 0.0f;
    float stage_slew = 0.0f;
    if (!findArcDelayAndSlew(
            stage.arc, input_slew, load_cap, cell, stage_delay, stage_slew)) {
      return {.legal = false, .reason = fail_reason};
    }
    candidate_total_delay += stage_delay;
    propagated_slew = stage_slew;
    has_propagated_slew = true;
  }

  return makeEstimate(current_total_delay, candidate_total_delay);
}

}  // namespace

bool DelayEstimator::findArcDelay(const SelectedArc& arc,
                                  const float input_slew,
                                  const float load_cap,
                                  const sta::LibertyCell* cell,
                                  float& delay)
{
  float output_slew = 0.0f;
  return findArcDelayAndSlew(
      arc, input_slew, load_cap, cell, delay, output_slew);
}

std::optional<ArcDelayState> DelayEstimator::buildContext(
    const Resizer& resizer,
    const Target& target,
    const int delay_levels,
    FailReason* fail_reason)
{
  // Capture one active path arc and its electrical state so candidates can be
  // scored without touching shared timing state.  Higher delay levels expand
  // this into a path-local fanin/fanout timing window.
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
                      delay_levels,
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
    const int delay_levels,
    FailReason* fail_reason)
{
  // The target stage must be valid; surrounding stages are best-effort because
  // path endpoints and non-cell arcs are expected at window boundaries.
  const std::optional<DelayStageState> target_stage
      = buildDelayStageState(resizer,
                             inst,
                             driver_pin,
                             expanded,
                             path_index,
                             scene,
                             min_max,
                             fail_reason);
  if (!target_stage.has_value()) {
    return std::nullopt;
  }

  const int normalized_delay_levels = delay_levels > 0 ? delay_levels : 0;
  ArcDelayState context
      = makeArcDelayState(*target_stage, normalized_delay_levels);
  if (normalized_delay_levels == 0) {
    return context;
  }

  collectDelayWindow(resizer,
                     expanded,
                     path_index,
                     scene,
                     min_max,
                     normalized_delay_levels,
                     context,
                     *target_stage);
  return context;
}

DelayEstimate DelayEstimator::estimate(const ArcDelayState& context,
                                       const sta::LibertyCell* candidate_cell)
{
  if (!context.path_stages.empty()) {
    return estimateWindow(context, candidate_cell);
  }
  return estimate(context, candidate_cell, context.load_cap);
}

DelayEstimate DelayEstimator::estimate(const ArcDelayState& context,
                                       const sta::LibertyCell* candidate_cell,
                                       const float load_cap)
{
  // Replay the same path arc on the candidate cell without touching shared
  // timing state.  Resolve candidate ports first so a port-mismatch failure
  // is reported distinctly from a missing timing arc.
  sta::LibertyCell* scene_cell = nullptr;
  sta::LibertyPort* candidate_input = nullptr;
  sta::LibertyPort* candidate_output = nullptr;
  if (!findCandidatePorts(context.arc,
                          candidate_cell,
                          scene_cell,
                          candidate_input,
                          candidate_output)) {
    return {.legal = false, .reason = FailReason::kMissingCandidatePort};
  }
  return estimate(context.arc,
                  context.input_slew,
                  load_cap,
                  context.current_delay,
                  candidate_cell);
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
  return makeEstimate(current_delay, candidate_delay);
}

}  // namespace rsz
