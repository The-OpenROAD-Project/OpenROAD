// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "MoveCandidate.hh"

#include "OptimizerTypes.hh"
#include "SequentialTimingEstimator.hh"
#include "rsz/Resizer.hh"

namespace rsz {

MoveCandidate::MoveCandidate(Resizer& resizer, const Target& target)
    : resizer_(resizer), target_(target)
{
}

MoveResult MoveCandidate::rejectedMove() const
{
  return {
      .accepted = false,
      .type = type(),
      .touched_instances = {},
  };
}

void MoveCandidate::chargeCellSwapSideCosts(const DelayEstimate& delay_est,
                                            const sta::LibertyCell* replacement,
                                            const float score_scale,
                                            Estimate& estimate) const
{
  float adjusted_gain = delay_est.arrival_impr;
  if (target().sequential_timing_estimator != nullptr) {
    const SequentialTimingEstimator::Result side
        = target().sequential_timing_estimator->estimate(replacement);
    estimate.sequential_penalty = side.weighted_side_slack_loss;
    estimate.sequential_side_loss = side.max_side_slack_loss;
    adjusted_gain -= *estimate.sequential_penalty;
    estimate.score = adjusted_gain * score_scale;
    if (!side.legal || adjusted_gain <= 0.0f) {
      estimate.legal = false;
      estimate.reason = side.legal ? "sequential_non_improving" : side.reason;
      estimate.tail_admissible = side.legal && delay_est.arrival_impr > 0.0f;
      return;
    }
  }

  // Sibling paths on the fanin net pay for the larger input capacitance.
  if (delay_est.side_paths_evaluated > 0) {
    estimate.side_loss = delay_est.side_loss;
    adjusted_gain -= delay_est.side_loss;
    estimate.score = adjusted_gain * score_scale;
    if (adjusted_gain <= 0.0f) {
      estimate.legal = false;
      estimate.reason = "side_loss_non_improving";
      estimate.tail_admissible = delay_est.arrival_impr > 0.0f;
    }
  }
}

}  // namespace rsz
