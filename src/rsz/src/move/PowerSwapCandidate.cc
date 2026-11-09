// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "PowerSwapCandidate.hh"

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "SequentialTimingEstimator.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"

namespace rsz {

namespace {

// Floor on the slack cost, so that free swaps rank first without dividing by
// zero.
constexpr float kMinCost = 1e-15f;

Estimate rejected(const std::string_view reason)
{
  return {.legal = false, .reason = reason};
}

}  // namespace

PowerSwapCandidate::PowerSwapCandidate(Resizer& resizer,
                                       const Target& target,
                                       sta::Instance* inst,
                                       sta::LibertyCell* candidate_cell,
                                       const float power_gain,
                                       const float slack,
                                       SlewHeadroom slew_headroom)
    : VtSwapMtCandidate(resizer, target, inst, candidate_cell),
      power_gain_(power_gain),
      slack_(slack),
      slew_headroom_(std::move(slew_headroom))
{
}

Estimate PowerSwapCandidate::estimate()
{
  // A slower cell is non-improving to the estimator; only its other rejects
  // stop the swap.
  const EstimatorContext& context = target().estimator_context.value();
  std::vector<StageEvaluation> trace;
  const DelayEstimate delay_est
      = DelayEstimator::estimate(context, candidate_cell_, &trace);
  if (delay_est.reason != FailReason::kEstimateLegal
      && delay_est.reason != FailReason::kEstimateNonImproving) {
    return rejected(failReasonName(delay_est.reason));
  }
  if (breaksDrvLimits(context, trace)) {
    return rejected("power_drv_limit");
  }

  // The estimate follows the path's output transition.  The swap slows the
  // other one too, which spends the target's slack past the gap between
  // their slacks.
  float cost = std::max(-delay_est.arrival_impr, 0.0f);
  const DelayStageState& stage = context.target();
  const sta::LibertyCell* scene_cell
      = candidate_cell_->sceneCell(stage.arc.scene, stage.arc.min_max);
  const std::optional<float> opposite_delta
      = scene_cell != nullptr ? oppositeRfDelayDelta(context, scene_cell)
                              : std::nullopt;
  if (opposite_delta.has_value()) {
    const float gap = std::max(
        context.opposite_rf_slack - sta::delayAsFloat(target().slack), 0.0f);
    cost = std::max(cost, *opposite_delta - gap);
  }
  Estimate estimate{.legal = true, .raw_gain = delay_est.arrival_impr};
  if (target().sequential_timing_estimator != nullptr) {
    const SequentialTimingEstimator::Result side
        = target().sequential_timing_estimator->estimate(candidate_cell_);
    if (!side.legal) {
      return rejected(side.reason);
    }
    estimate.sequential_penalty = side.weighted_side_slack_loss;
    estimate.sequential_side_loss = side.max_side_slack_loss;
    cost += side.weighted_side_slack_loss;
  }
  if (delay_est.side_paths_evaluated > 0) {
    estimate.side_loss = delay_est.side_loss;
    cost += delay_est.side_loss;
  }

  if (cost > kSlackRatio * slack_) {
    return rejected(kBudgetReason);
  }
  cost_ = cost;
  estimate.score = power_gain_ / std::max(cost, kMinCost);
  estimate.reason = failReasonName(FailReason::kEstimateLegal);
  return estimate;
}

bool PowerSwapCandidate::breaksDrvLimits(
    const EstimatorContext& context,
    const std::vector<StageEvaluation>& trace) const
{
  const DelayStageState& stage = context.target();
  // The arc's port is its scene's; so must the candidate's be.
  const sta::LibertyPort* port = stage.arc.outputPort();
  const sta::LibertyCell* new_cell
      = candidate_cell_->sceneCell(stage.arc.scene, stage.arc.min_max);
  const sta::LibertyPort* new_port
      = new_cell != nullptr ? new_cell->findLibertyPort(port->name()) : nullptr;
  if (new_port == nullptr) {
    return false;
  }
  const sta::MinMax* max = sta::MinMax::max();

  // No new or larger max capacitance violation at the output.
  float cap_limit = 0.0f;
  float new_cap_limit = 0.0f;
  bool cap_exists = false;
  bool new_cap_exists = false;
  port->capacitanceLimit(max, cap_limit, cap_exists);
  new_port->capacitanceLimit(max, new_cap_limit, new_cap_exists);
  if (new_cap_exists && stage.load_cap > new_cap_limit
      && (!cap_exists || new_cap_limit < cap_limit)) {
    return true;
  }

  // No new or larger max slew violation: the table slew growth of the new
  // cell added to the STA slew of every pin of the net, and against the new
  // driver port's limit.  A wire-dominated load grows by about the driver's
  // growth, far less than the driver's ratio.
  const size_t target_index = context.target_stage_index;
  if (target_index >= trace.size()) {
    return false;
  }
  const float new_slew = trace[target_index].output_slew;
  const float slew_growth = new_slew - stage.current_slew;
  if (slew_growth > 0.0f && slew_growth > slew_headroom_.output_margin) {
    return true;
  }
  // The fanin driver's slew grows about with its load.
  for (const SlewHeadroom::Fanin& fanin : slew_headroom_.fanins) {
    const sta::LibertyPort* input = current_cell_->findLibertyPort(fanin.port);
    const sta::LibertyPort* new_input
        = candidate_cell_->findLibertyPort(fanin.port);
    if (input == nullptr || new_input == nullptr || fanin.load_cap <= 0.0f) {
      continue;
    }
    const float added = new_input->capacitance() - input->capacitance();
    if (added > 0.0f
        && (fanin.load_cap + added) / fanin.load_cap > fanin.headroom) {
      return true;
    }
  }
  float slew_limit = 0.0f;
  bool slew_exists = false;
  resizer_.sta()->findSlewLimit(
      new_port, stage.arc.scene, max, slew_limit, slew_exists);
  return slew_exists && new_slew > slew_limit && new_slew > stage.current_slew;
}

}  // namespace rsz
