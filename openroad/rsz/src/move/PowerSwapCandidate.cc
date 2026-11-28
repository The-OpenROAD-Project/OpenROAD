// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "PowerSwapCandidate.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "DelayEstimator.hh"
#include "OptimizerTypes.hh"
#include "PowerVtSwapGenerator.hh"
#include "SequentialTimingEstimator.hh"
#include "VtSwapMtCandidate.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"

namespace rsz {

namespace {

// Floor on the slack cost, so that free swaps rank first without dividing by
// zero.
constexpr float kMinCost = 1e-15f;

Estimate rejected(const std::string_view reason)
{
  return {.legal = false, .reason = reason};
}

void outputCapLimit(const sta::LibertyPort* port,
                    const sta::MinMax* max,
                    float& limit,
                    bool& exists)
{
  port->capacitanceLimit(max, limit, exists);
  if (!exists && port->direction()->isAnyOutput()) {
    port->libertyLibrary()->defaultMaxCapacitance(limit, exists);
  }
}

}  // namespace

PowerSwapCandidate::PowerSwapCandidate(Resizer& resizer,
                                       const Target& target,
                                       sta::Instance* inst,
                                       sta::LibertyCell* candidate_cell,
                                       const float power_gain,
                                       const float slack,
                                       SlewHeadroom slew_headroom,
                                       const SlackFloors& floors,
                                       const float hold_margin,
                                       const float slack_ratio,
                                       const bool cheapest_first)
    : VtSwapMtCandidate(resizer, target, inst, candidate_cell, 1.0f),
      power_gain_(power_gain),
      slack_(slack),
      slew_headroom_(std::move(slew_headroom)),
      floors_(floors),
      hold_margin_(hold_margin),
      slack_ratio_(slack_ratio),
      cheapest_first_(cheapest_first)
{
}

Estimate PowerSwapCandidate::estimate()
{
  // A slower cell is non-improving to the estimator; only its other rejects
  // stop the swap.
  if (!target().estimator_context.has_value()) {
    return rejected("missing_estimator_context");
  }
  const EstimatorContext& context = *target().estimator_context;
  std::vector<StageEvaluation> trace;
  const DelayEstimate delay_est
      = DelayEstimator::estimate(context, candidate_cell_, &trace);
  if (delay_est.reason != FailReason::kEstimateLegal
      && delay_est.reason != FailReason::kEstimateNonImproving) {
    return rejected(failReasonName(delay_est.reason));
  }
  // The estimate follows the path's output transition.  The swap slows the
  // other one too, which spends the target's slack past the gap between
  // their slacks and may grow opposite-RF output slew.
  float cost = std::max(-delay_est.arrival_impr, 0.0f);
  const DelayStageState& stage = context.target();
  const sta::LibertyCell* scene_cell
      = candidate_cell_->sceneCell(stage.arc.scene, stage.arc.min_max);
  float opp_slew_delta = 0.0f;
  float opp_output_slew = 0.0f;
  const std::optional<float> opposite_delta
      = scene_cell != nullptr
            ? oppositeRfDelayDelta(
                  context, scene_cell, &opp_slew_delta, &opp_output_slew)
            : std::nullopt;
  if (breaksDrvLimits(context, trace, opp_slew_delta, opp_output_slew)) {
    return rejected("power_drv_limit");
  }
  if (opposite_delta.has_value() && std::isfinite(context.opposite_rf_slack)) {
    const float gap = std::max(
        context.opposite_rf_slack - sta::delayAsFloat(target().slack), 0.0f);
    cost = std::max(cost, *opposite_delta - gap);
  }
  Estimate estimate{.legal = true, .raw_gain = delay_est.arrival_impr};
  if (target().sequential_timing_estimator != nullptr) {
    const SequentialTimingEstimator::Result side
        = target().sequential_timing_estimator->estimate(
            candidate_cell_,
            floors_.lowest(),
            hold_margin_,
            [this](const sta::Pin* pin) { return floors_.endpoint(pin); });
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

  if (cost > slack_ratio_ * slack_) {
    return rejected(kBudgetReason);
  }
  cost_ = cost;
  estimate.score
      = (cheapest_first_ ? 1.0f : power_gain_) / std::max(cost, kMinCost);
  estimate.reason = failReasonName(FailReason::kEstimateLegal);
  return estimate;
}

bool PowerSwapCandidate::breaksDrvLimits(
    const EstimatorContext& context,
    const std::vector<StageEvaluation>& trace,
    const float opp_slew_delta,
    const float opp_output_slew) const
{
  const DelayStageState& stage = context.target();
  const sta::LibertyPort* target_port = stage.arc.outputPort();
  const sta::LibertyCell* cur_cell = stage.arc.currentCell();
  const sta::LibertyCell* new_cell
      = candidate_cell_->sceneCell(stage.arc.scene, stage.arc.min_max);
  if (cur_cell == nullptr || new_cell == nullptr) {
    return false;
  }
  const sta::MinMax* max = sta::MinMax::max();
  const size_t target_index = context.target_stage_index;
  if (target_index >= trace.size()) {
    return false;
  }

  // Check max capacitance (with library defaultMaxCapacitance fallback) and
  // max slew on every connected output pin of the instance (including sibling
  // outputs on multi-output cells and both output transitions on the target).
  for (const SlewHeadroom::Output& out : slew_headroom_.outputs) {
    const sta::LibertyPort* cur_port = cur_cell->findLibertyPort(out.port);
    const sta::LibertyPort* new_port = new_cell->findLibertyPort(out.port);
    if (cur_port == nullptr || new_port == nullptr) {
      continue;
    }
    const bool is_target
        = target_port != nullptr && out.port == target_port->name();
    const float load_cap = is_target ? stage.load_cap : out.load_cap;

    float cap_limit = 0.0f;
    float new_cap_limit = 0.0f;
    bool cap_exists = false;
    bool new_cap_exists = false;
    outputCapLimit(cur_port, max, cap_limit, cap_exists);
    outputCapLimit(new_port, max, new_cap_limit, new_cap_exists);
    if (new_cap_exists && load_cap > new_cap_limit
        && (!cap_exists || new_cap_limit < cap_limit)) {
      return true;
    }

    float slew_limit = 0.0f;
    bool slew_exists = false;
    resizer_.sta()->findSlewLimit(
        new_port, stage.arc.scene, max, slew_limit, slew_exists);
    if (is_target) {
      const float new_slew = trace[target_index].output_slew;
      const float slew_growth = new_slew - stage.current_slew;
      if (slew_growth > 0.0f && slew_growth > out.slew_margin) {
        return true;
      }
      if (slew_exists && new_slew > slew_limit
          && new_slew > stage.current_slew) {
        return true;
      }
      if (opp_slew_delta > 0.0f) {
        if (opp_slew_delta > out.slew_margin) {
          return true;
        }
        if (slew_exists && opp_output_slew > slew_limit) {
          return true;
        }
      }
    } else if (out.current_slew > 0.0f) {
      const float cur_res = cur_port->driveResistance();
      const float new_res = new_port->driveResistance();
      if (cur_res > 0.0f && new_res > cur_res) {
        const float est_slew = out.current_slew * (new_res / cur_res);
        const float slew_growth = est_slew - out.current_slew;
        if (slew_growth > out.slew_margin
            || (slew_exists && est_slew > slew_limit)) {
          return true;
        }
      }
    }
  }

  // The fanin driver's slew grows about with its load.
  for (const SlewHeadroom::Fanin& fanin : slew_headroom_.fanins) {
    const sta::LibertyPort* input = cur_cell->findLibertyPort(fanin.port);
    const sta::LibertyPort* new_input = new_cell->findLibertyPort(fanin.port);
    if (input == nullptr || new_input == nullptr || fanin.load_cap <= 0.0f) {
      continue;
    }
    const float added = new_input->capacitance() - input->capacitance();
    if (added > 0.0f
        && (fanin.load_cap + added) / fanin.load_cap > fanin.headroom) {
      return true;
    }
  }
  return false;
}

}  // namespace rsz
