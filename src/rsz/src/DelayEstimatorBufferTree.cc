// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// The buffer-tree part of DelayEstimator: the timing of a RebufferMt tree
// plan along the target path and its side paths.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

#include "DelayEstimator.hh"
#include "DelayEstimatorInternal.hh"
#include "OptimizerTypes.hh"
#include "Rebuffer.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/SearchClass.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"

namespace rsz {

using estimator_detail::estimateOutputSlew;
using estimator_detail::estimateReceiverInputSlew;
using estimator_detail::kElmoreSlewFactor;
using estimator_detail::kMinSlewRatioDenominator;
using estimator_detail::lookupArcDelayAndSlew;
using estimator_detail::lookupBufferDelayAndSlew;
using estimator_detail::makeEstimate;
using estimator_detail::wireElmoreDelay;

namespace {

// Calibration of the buffer-tree model.  The factors were fitted against STA
// on the rebuffer accuracy report (report_delay_estimator_accuracy with
// -rebuffer_tree), which prints the estimate of every stage of the planned
// tree next to the delay STA measures after the tree is built.
//
// The effective load of a buffer stage as a fraction of its lumped downstream
// capacitance: on the target path, on a side path, and for a direct load.
constexpr float kBufferTreeTargetEffectiveLoadCapFactor = 0.65f;
constexpr float kBufferTreeSideEffectiveLoadCapFactor = 0.875f;
constexpr float kBufferTreeDirectLoadCapFactor = 0.60f;
// A final buffer whose downstream cap per input slew reaches the ratio takes
// its output slew at the lower effective-load fraction.
constexpr float kBufferTreeHighCapSlewRatio = 2.5e-3f;
constexpr float kBufferTreeHighCapSlewFactor = 0.60f;
// A stage that keeps part of its load keeps its slew at least to this power
// of the load ratio.
constexpr float kLoadRatioSlewFloorExponent = 0.85f;
// Elmore wire delays past the threshold shrink by the scale, and short wires
// add slew at a higher factor than the Elmore slew rule.
constexpr float kBufferTreeWireDelayScaleThreshold = 20.0e-12f;
constexpr float kBufferTreeWireDelayScale = 0.78f;
constexpr float kBufferTreeShortWireSlewFactor = 1.35f;

float loadRatioSlewFloor(const DelayStageState& stage, const float load_cap)
{
  if (stage.load_cap <= 0.0f || stage.current_slew <= 0.0f
      || load_cap <= 0.0f) {
    return 0.0f;
  }

  const float load_ratio = std::clamp(load_cap / stage.load_cap, 0.0f, 1.0f);
  return stage.current_slew * std::pow(load_ratio, kLoadRatioSlewFloorExponent);
}

float wireOutputSlewFromDelay(const float wire_delay, const float input_slew)
{
  const float slew_factor = wire_delay < kBufferTreeWireDelayScaleThreshold
                                ? kBufferTreeShortWireSlewFactor
                                : kElmoreSlewFactor;
  const float wire_slew = slew_factor * std::max(wire_delay, 0.0f);
  return std::hypot(std::max(input_slew, 0.0f), wire_slew);
}

float bufferTreeEffectiveLoadCap(const float total_load_cap, const float factor)
{
  // Tree buffers drive distributed RC, not a lumped cap.  The gate lookup uses
  // a lighter effective cap, while reports and legality keep the full cap.
  return std::max(total_load_cap * factor, 0.0f);
}

float boundBufferTreeDownstreamStageDelay(const DelayStageState& stage,
                                          const float input_slew,
                                          const float stage_delay)
{
  const float current_input_slew
      = std::max(stage.input_slew, kMinSlewRatioDenominator);
  const float input_slew_ratio
      = std::clamp(input_slew / current_input_slew, 0.0f, 1.0f);
  // 0.45 + 0.35 * sqrt(ratio) tops out at 0.80 for a ratio of one.
  const float min_delay_ratio
      = std::max(0.5f, 0.45f + 0.35f * std::sqrt(input_slew_ratio));

  return std::max(stage_delay, stage.current_delay * min_delay_ratio);
}

float bufferTreeWireDelay(const WireRC& wire, const float load_cap)
{
  const float raw_delay = wireElmoreDelay(wire, load_cap);
  if (raw_delay < kBufferTreeWireDelayScaleThreshold) {
    return raw_delay;
  }

  return raw_delay * kBufferTreeWireDelayScale;
}

struct BranchTiming
{
  bool legal{false};
  float delay{0.0f};
  float load_slew{0.0f};
  FailReason reason{FailReason::kNone};
};

// Dummy-root trie cache for replayed buffer-tree branch prefixes.  The key
// has no effective-load-cap factor: a prefix the target path shares with a
// side path keeps the timing of whichever was evaluated first.
struct BranchTimingCacheNode
{
  bool valid{false};
  BufferTreePathStage stage;
  BranchTiming prefix;
  StageEvaluation trace_entry;
  std::vector<BranchTimingCacheNode> children;
};

bool sameWireRC(const WireRC& lhs, const WireRC& rhs)
{
  return lhs.length == rhs.length && lhs.res == rhs.res && lhs.cap == rhs.cap;
}

bool sameBufferTreePathStage(const BufferTreePathStage& lhs,
                             const BufferTreePathStage& rhs)
{
  if (lhs.type != rhs.type || lhs.downstream_cap != rhs.downstream_cap
      || lhs.drives_distributed_load != rhs.drives_distributed_load) {
    return false;
  }

  if (lhs.type == BufferTreeStageType::kBuffer) {
    return lhs.buffer_cell == rhs.buffer_cell;
  }

  return lhs.buffer_cell == rhs.buffer_cell
         && sameWireRC(lhs.wire_rc, rhs.wire_rc);
}

BranchTimingCacheNode* findBranchTimingCacheChild(
    BranchTimingCacheNode& node,
    const BufferTreePathStage& stage)
{
  for (BranchTimingCacheNode& child : node.children) {
    if (child.valid && sameBufferTreePathStage(child.stage, stage)) {
      return &child;
    }
  }
  return nullptr;
}

// The target driver into the tree input, taken from the cache root when it
// already holds it.  False when the driver arc lookup fails.
bool branchDriverTiming(const DelayStageState& target_stage,
                        const RebufferTreePlan& plan,
                        std::vector<StageEvaluation>* trace,
                        BranchTimingCacheNode& cache_root,
                        BranchTiming& branch)
{
  if (cache_root.valid) {
    branch = cache_root.prefix;
    if (trace != nullptr) {
      trace->push_back(cache_root.trace_entry);
    }
    return true;
  }

  // The driver delay is the plain table model at the tree input cap.  The
  // propagated slew comes from loadRatioSlewFloor(), not from the table.
  float driver_delay = 0.0f;
  float unused_slew = 0.0f;
  if (!lookupArcDelayAndSlew(target_stage.arc,
                             target_stage.input_slew,
                             plan.tree_input_cap,
                             target_stage.arc.currentCell(),
                             driver_delay,
                             unused_slew)) {
    return false;
  }

  const float propagated_slew
      = loadRatioSlewFloor(target_stage, plan.tree_input_cap);
  const StageEvaluation driver_trace{.path_index = target_stage.path_index,
                                     .cell = target_stage.arc.currentCell(),
                                     .input_slew = target_stage.input_slew,
                                     .load_cap = plan.tree_input_cap,
                                     .stage_delay = driver_delay,
                                     .output_slew = propagated_slew,
                                     .wire_delay = 0.0f};
  branch = {.legal = true,
            .delay = driver_delay,
            .load_slew = propagated_slew,
            .reason = FailReason::kEstimateLegal};

  cache_root.valid = true;
  cache_root.prefix = branch;
  cache_root.trace_entry = driver_trace;

  if (trace != nullptr) {
    trace->push_back(driver_trace);
  }

  return true;
}

// The timing of tree stage stage_index after branch: wires and vias add
// their Elmore delay, buffers add a Liberty lookup on the effective load.
// False when the buffer arc lookup fails.
bool treeStageTiming(const DelayStageState& target_stage,
                     const BufferTreeLoadPath& load_path,
                     const size_t stage_index,
                     const float path_effective_load_cap_factor,
                     const BranchTiming& branch,
                     StageEvaluation& stage_trace,
                     BranchTiming& next_branch)
{
  const BufferTreePathStage& tree_stage = load_path.stages[stage_index];
  switch (tree_stage.type) {
    case BufferTreeStageType::kWire:
    case BufferTreeStageType::kVia: {
      const float input_slew = branch.load_slew;
      const float wire_delay
          = bufferTreeWireDelay(tree_stage.wire_rc, tree_stage.downstream_cap);
      const float output_slew = wireOutputSlewFromDelay(wire_delay, input_slew);
      stage_trace = {.path_index = target_stage.path_index,
                     .cell = nullptr,
                     .input_slew = input_slew,
                     .load_cap = tree_stage.downstream_cap,
                     .stage_delay = wire_delay,
                     .output_slew = output_slew,
                     .wire_delay = wire_delay};
      next_branch = {.legal = true,
                     .delay = branch.delay + wire_delay,
                     .load_slew = output_slew,
                     .reason = FailReason::kEstimateLegal};
      break;
    }
    case BufferTreeStageType::kBuffer: {
      const float input_slew = branch.load_slew;

      // Distributed and direct load trees use separate effective-cap models,
      // because STA's distributed RC is not a lumped-cap Liberty lookup.
      const float gate_load_cap = bufferTreeEffectiveLoadCap(
          tree_stage.downstream_cap,
          tree_stage.drives_distributed_load ? path_effective_load_cap_factor
                                             : kBufferTreeDirectLoadCapFactor);

      float buffer_delay = 0.0f;
      float buffer_output_slew = 0.0f;
      if (!lookupBufferDelayAndSlew(target_stage,
                                    tree_stage.buffer_cell,
                                    input_slew,
                                    gate_load_cap,
                                    buffer_delay,
                                    buffer_output_slew)) {
        return false;
      }

      const bool high_cap_final_buffer
          = tree_stage.drives_distributed_load
            && !load_path.hasBufferAfter(stage_index) && input_slew > 0.0f
            && tree_stage.downstream_cap / input_slew
                   >= kBufferTreeHighCapSlewRatio;
      if (high_cap_final_buffer) {
        float unused_buffer_delay = 0.0f;
        float high_cap_output_slew = 0.0f;
        const float slew_load_cap = bufferTreeEffectiveLoadCap(
            tree_stage.downstream_cap, kBufferTreeHighCapSlewFactor);
        if (lookupBufferDelayAndSlew(target_stage,
                                     tree_stage.buffer_cell,
                                     input_slew,
                                     slew_load_cap,
                                     unused_buffer_delay,
                                     high_cap_output_slew)) {
          buffer_output_slew = high_cap_output_slew;
        }
      }

      stage_trace = {.path_index = target_stage.path_index,
                     .cell = tree_stage.buffer_cell,
                     .input_slew = input_slew,
                     .load_cap = tree_stage.downstream_cap,
                     .stage_delay = buffer_delay,
                     .output_slew = buffer_output_slew,
                     .wire_delay = 0.0f};
      next_branch = {.legal = true,
                     .delay = branch.delay + buffer_delay,
                     .load_slew = buffer_output_slew,
                     .reason = FailReason::kEstimateLegal};
      break;
    }
  }

  return true;
}

BranchTiming estimateBranchTiming(const DelayStageState& target_stage,
                                  const RebufferTreePlan& plan,
                                  const BufferTreeLoadPath& load_path,
                                  const float path_effective_load_cap_factor,
                                  std::vector<StageEvaluation>* trace,
                                  BranchTimingCacheNode& cache_root)
{
  BranchTiming branch;
  if (!branchDriverTiming(target_stage, plan, trace, cache_root, branch)) {
    return {.legal = false, .reason = FailReason::kMissingCurrentTimingArc};
  }

  BranchTimingCacheNode* parent = &cache_root;
  for (size_t stage_index = 0; stage_index < load_path.stages.size();
       ++stage_index) {
    const BufferTreePathStage& tree_stage = load_path.stages[stage_index];
    BranchTimingCacheNode* cached_child
        = findBranchTimingCacheChild(*parent, tree_stage);
    if (cached_child != nullptr) {
      branch = cached_child->prefix;
      parent = cached_child;
      if (trace != nullptr) {
        trace->push_back(cached_child->trace_entry);
      }
      continue;
    }

    StageEvaluation stage_trace;
    BranchTiming next_branch;
    if (!treeStageTiming(target_stage,
                         load_path,
                         stage_index,
                         path_effective_load_cap_factor,
                         branch,
                         stage_trace,
                         next_branch)) {
      return {.legal = false, .reason = FailReason::kMissingBufferTimingArc};
    }

    parent->children.push_back({.valid = true,
                                .stage = tree_stage,
                                .prefix = next_branch,
                                .trace_entry = stage_trace,
                                .children = {}});
    parent = &parent->children.back();

    branch = next_branch;
    if (trace != nullptr) {
      trace->push_back(stage_trace);
    }
  }

  return branch;
}

DelayEstimate estimateBufferTreeWindow(const EstimatorContext& context,
                                       const RebufferTreePlan& plan,
                                       std::vector<StageEvaluation>* trace,
                                       BranchTimingCacheNode& branch_cache)
{
  // RebufferMt plans always have buffers and a target load path.
  if (!std::isfinite(plan.tree_input_cap)) {
    return {.legal = false, .reason = FailReason::kInvalidBufferInsertion};
  }

  const BufferTreeLoadPath& target_load_path = *plan.targetPath();
  const std::vector<DelayStageState>& stages = context.path_stages;
  const int target_index = context.target_stage_index;
  const DelayStageState& target_stage = context.target();
  const float current_total_delay
      = context.current_total_delay + target_stage.current_wire_delay;

  float candidate_total_delay = 0.0f;

  // Pre-target stages replay unchanged.
  for (int stage_index = 0; stage_index < target_index; ++stage_index) {
    const DelayStageState& stage = stages[stage_index];
    candidate_total_delay += stage.current_delay;
    if (trace != nullptr) {
      trace->push_back({.path_index = stage.path_index,
                        .cell = stage.arc.currentCell(),
                        .input_slew = stage.input_slew,
                        .load_cap = stage.load_cap,
                        .stage_delay = stage.current_delay,
                        .output_slew = stage.current_slew,
                        .wire_delay = stage.current_wire_delay});
    }
  }

  // Target driver + buffer-tree branch (shared with side-paths via cache).
  const BranchTiming branch
      = estimateBranchTiming(target_stage,
                             plan,
                             target_load_path,
                             kBufferTreeTargetEffectiveLoadCapFactor,
                             trace,
                             branch_cache);
  if (!branch.legal) {
    return {.legal = false, .reason = branch.reason};
  }

  candidate_total_delay += branch.delay;
  float propagated_slew = branch.load_slew;

  for (int stage_index = target_index + 1;
       stage_index < static_cast<int>(stages.size());
       ++stage_index) {
    const DelayStageState& stage = stages[stage_index];
    const float input_slew
        = stage_index == target_index + 1
              ? propagated_slew
              : estimateReceiverInputSlew(
                    stages[stage_index - 1], stage, propagated_slew);

    float model_stage_delay = 0.0f;
    float stage_slew = 0.0f;
    if (!lookupArcDelayAndSlew(stage.arc,
                               input_slew,
                               stage.load_cap,
                               stage.arc.currentCell(),
                               model_stage_delay,
                               stage_slew)) {
      return {.legal = false, .reason = FailReason::kMissingCurrentTimingArc};
    }

    const float stage_delay = boundBufferTreeDownstreamStageDelay(
        stage,
        input_slew,
        stage.current_delay + (model_stage_delay - stage.current_model_delay));
    candidate_total_delay += stage_delay;
    propagated_slew = estimateOutputSlew(
        stage, stage.arc.currentCell(), input_slew, stage.load_cap, stage_slew);

    if (trace != nullptr) {
      trace->push_back({.path_index = stage.path_index,
                        .cell = stage.arc.currentCell(),
                        .input_slew = input_slew,
                        .load_cap = stage.load_cap,
                        .stage_delay = stage_delay,
                        .output_slew = propagated_slew,
                        .wire_delay = stage.current_wire_delay});
    }
  }

  return makeEstimate(current_total_delay, candidate_total_delay);
}

DelayEstimate estimateSidePathWindow(const EstimatorContext& target_context,
                                     const RebufferTreePlan& plan,
                                     const BufferTreeSidePathContext& side,
                                     std::vector<StageEvaluation>* trace,
                                     BranchTimingCacheNode& branch_cache)
{
  // Side contexts come from the plan's own load paths.
  const BufferTreeLoadPath& load_path = *plan.loadPath(side.load_pin);
  const DelayStageState& target_stage = target_context.target();
  const BranchTiming branch
      = estimateBranchTiming(target_stage,
                             plan,
                             load_path,
                             kBufferTreeSideEffectiveLoadCapFactor,
                             trace,
                             branch_cache);
  if (!branch.legal) {
    return {.legal = false, .reason = branch.reason};
  }

  const float current_total_delay
      = target_stage.current_delay + side.current_driver_to_load_delay;
  return makeEstimate(current_total_delay, branch.delay);
}

}  // namespace

sta::PathEndSeq DelayEstimator::findPathEndsThrough(
    sta::dbSta* sta,
    const std::vector<sta::Pin*>& through_pins,
    const sta::MinMaxAll* min_max_all,
    const int count)
{
  // The search owns the exception objects.
  sta::PinSet* pins = new sta::PinSet(sta->network());
  for (sta::Pin* pin : through_pins) {
    pins->insert(pin);
  }

  sta::ExceptionThru* through = sta->makeExceptionThru(
      pins, nullptr, nullptr, sta::RiseFallBoth::riseFall(), sta->cmdSdc());

  sta::ExceptionThruSeq* thrus = new sta::ExceptionThruSeq;
  thrus->push_back(through);
  sta::StringSeq group_names;

  return sta->findPathEnds(nullptr,
                           thrus,
                           nullptr,
                           false,
                           sta->scenes(),
                           min_max_all,
                           count,
                           count,
                           false,
                           false,
                           -sta::INF,
                           sta::INF,
                           true,
                           group_names,
                           true,
                           false,
                           false,
                           false,
                           false,
                           false);
}

DelayEstimate DelayEstimator::estimateBufferTreeWithSidePaths(
    const EstimatorContext& context,
    const RebufferTreePlan& plan,
    const std::vector<BufferTreeSidePathContext>& side_contexts,
    std::vector<StageEvaluation>* trace,
    std::vector<BufferTreeSidePathEvaluation>* side_evaluations)
{
  if (side_evaluations != nullptr) {
    side_evaluations->clear();
    side_evaluations->reserve(side_contexts.size());
  }

  if (trace != nullptr) {
    trace->clear();
  }

  BranchTimingCacheNode branch_cache;
  DelayEstimate estimate
      = estimateBufferTreeWindow(context, plan, trace, branch_cache);
  const bool target_estimate_available
      = estimate.reason == FailReason::kEstimateLegal
        || estimate.reason == FailReason::kEstimateNonImproving;
  if (!target_estimate_available) {
    return estimate;
  }

  // Side loads pay for their buffered detour below zero slack.  Only a drop
  // below the side slack floor rejects the plan.
  for (const BufferTreeSidePathContext& side_context : side_contexts) {
    std::vector<StageEvaluation> side_trace;
    const DelayEstimate side_estimate = estimateSidePathWindow(
        context,
        plan,
        side_context,
        side_evaluations != nullptr ? &side_trace : nullptr,
        branch_cache);
    const bool side_estimate_available
        = side_estimate.reason == FailReason::kEstimateLegal
          || side_estimate.reason == FailReason::kEstimateNonImproving;
    const float side_impr = side_estimate_available
                                ? side_estimate.arrival_impr
                                : -std::numeric_limits<float>::infinity();
    const float slack_after = side_context.current_slack + side_impr;
    estimate.side_paths_evaluated++;

    bool side_legal = side_estimate_available;
    if (!side_estimate_available) {
      estimate.legal = false;
      estimate.reason = side_estimate.reason;
    } else {
      estimate.side_loss += side_context.weight
                            * (std::min(side_context.current_slack, 0.0f)
                               - std::min(slack_after, 0.0f));
      if (slack_after < context.sideSlackFloor(estimate.arrival_impr)) {
        side_legal = false;
        estimate.legal = false;
        estimate.reason = FailReason::kSideSlackFloorViolation;
      }
    }

    if (side_evaluations != nullptr) {
      side_evaluations->push_back({.load_pin = side_context.load_pin,
                                   .legal = side_legal,
                                   .arrival_impr = side_impr,
                                   .slack_after = slack_after,
                                   .trace = side_trace});
    }
  }

  return estimate;
}

}  // namespace rsz
