// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "OptimizerTypes.hh"

#include <string>

#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"

namespace rsz {

const char* failReasonName(const FailReason reason)
{
  switch (reason) {
    // General status.
    case FailReason::kNone:
      return "";
    case FailReason::kEstimateLegal:
      return "estimate_legal";
    case FailReason::kEstimateNonImproving:
      return "estimate_non_improving";

    // Target identity and move eligibility.
    case FailReason::kMissingTargetInstance:
      return "missing_target_instance";

    // Path and current timing-arc context.
    case FailReason::kPathIndexOutOfRange:
      return "path_index_out_of_range";
    case FailReason::kMissingPrevTimingArc:
      return "missing_prev_timing_arc";
    case FailReason::kMissingArcTransition:
      return "missing_arc_transition";
    case FailReason::kMissingSceneOrMinMax:
      return "missing_scene_or_minmax";
    case FailReason::kMissingOutputPort:
      return "missing_output_port";
    case FailReason::kMissingCurrentPortMap:
      return "missing_current_port_map";
    case FailReason::kInvalidSelectedArc:
      return "invalid_selected_arc";
    case FailReason::kMissingInputSlewArc:
      return "missing_input_slew_arc";
    case FailReason::kMissingCurrentTimingArc:
      return "missing_current_timing_arc";

    // Prepared arc delay state.
    case FailReason::kInvalidContext:
      return "invalid_context";

    // Candidate timing context.
    case FailReason::kMissingCandidatePort:
      return "missing_candidate_port";
    case FailReason::kMissingCandidateTimingArc:
      return "missing_candidate_timing_arc";
  }
  return "unknown_fail_reason";
}

bool ArcDelayState::isValid() const
{
  return arc.scene != nullptr && arc.min_max != nullptr
         && arc.input_port != nullptr && arc.output_port != nullptr;
}

bool Target::canBePathDriver() const
{
  return (views & kPathDriverView) != 0 && driver_pin != nullptr
         && endpoint_path != nullptr && driver_path != nullptr
         && path_index >= 0;
}

bool Target::canBeInstance() const
{
  return (views & kInstanceView) != 0 && driver_pin != nullptr;
}

Target makePathDriverTarget(const sta::Path* endpoint_path,
                            const sta::PathExpanded& expanded,
                            const int path_index,
                            const sta::Slack slack,
                            const Resizer& resizer)
{
  Target target;
  target.views = kPathDriverView;
  target.endpoint_path = endpoint_path;
  target.driver_path = expanded.path(path_index);
  target.scene = endpoint_path != nullptr
                     ? endpoint_path->scene(resizer.staState())
                     : nullptr;
  target.driver_pin = target.driver_path != nullptr
                          ? target.driver_path->pin(resizer.staState())
                          : nullptr;
  target.path_index = path_index;
  target.slack = slack;
  if (resizer.network()->instance(target.driver_pin) != nullptr) {
    target.views |= kInstanceView;
  }
  return target;
}

bool Target::isPrepared(const PrepareCacheMask mask) const
{
  if ((mask & ~kArcDelayStateCache) != 0) {
    return false;
  }
  if ((mask & kArcDelayStateCache) == 0) {
    return true;
  }
  return arc_delay.has_value() && arc_delay->isValid();
}

const sta::Pin* Target::endpointPin(const Resizer& resizer) const
{
  return endpoint_path != nullptr ? endpoint_path->pin(resizer.staState())
                                  : nullptr;
}

sta::Pin* Target::resolvedPin(const Resizer& resizer) const
{
  if (driver_pin != nullptr) {
    return driver_pin;
  }
  return endpoint_path != nullptr ? endpoint_path->pin(resizer.staState())
                                  : nullptr;
}

sta::Instance* Target::inst(const Resizer& resizer) const
{
  sta::Pin* target_pin = resolvedPin(resizer);
  return target_pin != nullptr ? resizer.network()->instance(target_pin)
                               : nullptr;
}

sta::Vertex* Target::vertex(const Resizer& resizer) const
{
  sta::Pin* target_pin = resolvedPin(resizer);
  return target_pin != nullptr ? resizer.graph()->pinDrvrVertex(target_pin)
                               : nullptr;
}

const sta::Scene* Target::activeScene(const Resizer& resizer) const
{
  if (scene != nullptr) {
    return scene;
  }
  return endpoint_path != nullptr ? endpoint_path->scene(resizer.staState())
                                  : nullptr;
}

const sta::MinMax* Target::minMax(const Resizer& resizer) const
{
  if (endpoint_path == nullptr) {
    return resizer.maxAnalysisMode();
  }

  const sta::MinMax* min_max = endpoint_path->minMax(resizer.staState());
  return min_max != nullptr ? min_max : resizer.maxAnalysisMode();
}

const sta::Path* Target::driverPath(const Resizer&) const
{
  return driver_path;
}

const sta::Path* Target::inputPath(const Resizer&) const
{
  return driver_path != nullptr ? driver_path->prevPath() : nullptr;
}

const sta::Path* Target::prevDriverPath(const Resizer& resizer) const
{
  const sta::Path* input_path = inputPath(resizer);
  return input_path != nullptr ? input_path->prevPath() : nullptr;
}

std::string Target::name(const Resizer& resizer) const
{
  sta::Pin* target_pin = resolvedPin(resizer);
  if (target_pin != nullptr) {
    return resizer.network()->pathName(target_pin);
  }

  sta::Instance* target_inst = inst(resizer);
  if (target_inst != nullptr) {
    return resizer.network()->pathName(target_inst);
  }

  return "<none>";
}

}  // namespace rsz
