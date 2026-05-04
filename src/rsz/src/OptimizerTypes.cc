// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "OptimizerTypes.hh"

#include <string>

#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"

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
    case FailReason::kMissingCurrentTimingArc:
      return "missing_current_timing_arc";

    // Candidate timing context.
    case FailReason::kMissingCandidatePort:
      return "missing_candidate_port";
    case FailReason::kMissingCandidateTimingArc:
      return "missing_candidate_timing_arc";
  }
  return "unknown_fail_reason";
}

// Precondition: ref_arc is non-null (guaranteed by buildSelectedArc).
const sta::LibertyPort* SelectedArc::inputPort() const
{
  return ref_arc->from();
}

const sta::LibertyPort* SelectedArc::outputPort() const
{
  return ref_arc->to();
}

const sta::RiseFall* SelectedArc::inputRiseFall() const
{
  return ref_arc->fromEdge()->asRiseFall();
}

const sta::RiseFall* SelectedArc::outputRiseFall() const
{
  return ref_arc->toEdge()->asRiseFall();
}

sta::LibertyCell* SelectedArc::currentCell() const
{
  return const_cast<sta::LibertyPort*>(outputPort())->libertyCell();
}

namespace {

bool isExactArcMatch(const sta::TimingArc* reference,
                     const sta::TimingArc* candidate_arc)
{
  // Arc-set identity (same condition, mode, functional form) plus per-arc
  // edge/role equivalence.  TimingArcSet::equiv covers from/to/role/cond/sdf;
  // mode and isCondDefault must be checked explicitly.
  const sta::TimingArcSet* ref_set = reference->set();
  const sta::TimingArcSet* candidate_set = candidate_arc->set();
  return sta::TimingArcSet::equiv(ref_set, candidate_set)
         && ref_set->isCondDefault() == candidate_set->isCondDefault()
         && ref_set->modeName() == candidate_set->modeName()
         && ref_set->modeValue() == candidate_set->modeValue()
         && sta::TimingArc::equiv(reference, candidate_arc);
}

bool canTryRelaxedMatch(const sta::TimingArc* reference,
                        const sta::TimingArc* candidate_arc)
{
  // Relax only from a default/unconditional reference arc into a non-check
  // candidate arc.  Conditional reference arcs must keep exact semantics.
  const sta::TimingArcSet* ref_set = reference->set();
  return (ref_set->cond() == nullptr || ref_set->isCondDefault())
         && !candidate_arc->set()->role()->isTimingCheck();
}

bool isRelaxedArcMatch(const sta::TimingArc* reference,
                       const sta::TimingArc* candidate_arc)
{
  // Same port-name + RF transition + mode; condition is allowed to differ.
  if (!canTryRelaxedMatch(reference, candidate_arc)) {
    return false;
  }
  const sta::TimingArcSet* ref_set = reference->set();
  const sta::TimingArcSet* candidate_set = candidate_arc->set();
  return sta::LibertyPort::equiv(reference->from(), candidate_arc->from())
         && sta::LibertyPort::equiv(reference->to(), candidate_arc->to())
         && reference->fromEdge() == candidate_arc->fromEdge()
         && reference->toEdge() == candidate_arc->toEdge()
         && ref_set->modeName() == candidate_set->modeName()
         && ref_set->modeValue() == candidate_set->modeValue();
}

}  // namespace

ArcMatchType matchTimingArc(const sta::TimingArc* reference,
                            const sta::TimingArc* candidate_arc,
                            const ArcMatchMode match_mode)
{
  if (reference == nullptr || candidate_arc == nullptr) {
    return ArcMatchType::kNone;
  }
  if (isExactArcMatch(reference, candidate_arc)) {
    return ArcMatchType::kExact;
  }
  if (match_mode == ArcMatchMode::kRelaxedCandidate
      && isRelaxedArcMatch(reference, candidate_arc)) {
    return ArcMatchType::kRelaxed;
  }
  return ArcMatchType::kNone;
}

const sta::TimingArc* findMatchingTimingArc(const sta::TimingArc* reference,
                                            const sta::TimingArcSet* candidate,
                                            const ArcMatchMode match_mode,
                                            ArcMatchType* match_type)
{
  if (match_type != nullptr) {
    *match_type = ArcMatchType::kNone;
  }
  if (reference == nullptr || candidate == nullptr) {
    return nullptr;
  }

  // Exact match always wins, even when relaxed fallback is enabled.
  for (const sta::TimingArc* arc : candidate->arcs()) {
    if (matchTimingArc(reference, arc, ArcMatchMode::kExact)
        == ArcMatchType::kExact) {
      if (match_type != nullptr) {
        *match_type = ArcMatchType::kExact;
      }
      return arc;
    }
  }

  if (match_mode != ArcMatchMode::kRelaxedCandidate) {
    return nullptr;
  }

  for (const sta::TimingArc* arc : candidate->arcs()) {
    if (matchTimingArc(reference, arc, ArcMatchMode::kRelaxedCandidate)
        == ArcMatchType::kRelaxed) {
      if (match_type != nullptr) {
        *match_type = ArcMatchType::kRelaxed;
      }
      return arc;
    }
  }
  return nullptr;
}

// Find the timing arc in `candidate` that matches the reference arc's
// conditional/mode/sense.  Returns nullptr when no match exists.
const sta::TimingArc* findMatchingTimingArc(const sta::TimingArc* reference,
                                            const sta::TimingArcSet* candidate)
{
  return findMatchingTimingArc(
      reference, candidate, ArcMatchMode::kExact, nullptr);
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
  return arc_delay.has_value();
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
