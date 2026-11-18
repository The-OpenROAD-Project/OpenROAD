// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "VtSwapMtCandidate.hh"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

namespace {

Estimate makeRejectedEstimate(const std::string_view reason,
                              const std::optional<float> raw_gain
                              = std::nullopt)
{
  return {.legal = false,
          .score = raw_gain.value_or(0.0f),
          .raw_gain = raw_gain,
          .reason = reason};
}

const char* applyLogFormat(const bool accepted)
{
  return accepted ? "ACCEPT vt_swap_mt {}: {} -> {}"
                  : "REJECT vt_swap_mt {}: {} -> {} swap failed";
}

}  // namespace

VtSwapMtCandidate::VtSwapMtCandidate(Resizer& resizer,
                                     const Target& target,
                                     sta::Instance* inst,
                                     sta::LibertyCell* candidate_cell,
                                     const float cost_divisor)
    : MoveCandidate(resizer, target),
      inst_(inst),
      current_cell_(resizer.network()->libertyCell(inst)),
      candidate_cell_(candidate_cell),
      cost_divisor_(cost_divisor)
{
}

Estimate VtSwapMtCandidate::estimate()
{
  // Reuse the pre-built local delay context to score the candidate without
  // mutating the design.  The generator already applied the max-cap check.
  const DelayEstimate delay_est = DelayEstimator::estimate(
      target().estimator_context.value(), candidate_cell_);
  if (!delay_est.legal) {
    // Preserve the computed score for non-improving swaps so policy ranking can
    // still compare rejected candidates consistently.
    if (delay_est.reason == FailReason::kEstimateNonImproving) {
      Estimate rejected = makeRejectedEstimate(failReasonName(delay_est.reason),
                                               delay_est.arrival_impr);
      rejected.score = costScaled(rejected.score);
      return rejected;
    }
    return makeRejectedEstimate(failReasonName(delay_est.reason));
  }

  Estimate estimate{.legal = true,
                    .raw_gain = delay_est.arrival_impr,
                    .reason = failReasonName(FailReason::kEstimateLegal)};
  estimate.score = delay_est.arrival_impr;

  // The side costs come off the raw gain before the weight.
  chargeCellSwapSideCosts(delay_est, candidate_cell_, estimate);
  // Rank the gain per unit of cost, as SizeUpMt does, so a jump over more VTs
  // must buy its extra leakage with extra slack.
  estimate.score = costScaled(estimate.score);
  return estimate;
}

float VtSwapMtCandidate::costScaled(const float score) const
{
  // Only a positive score pays the cost: dividing a negative score by a
  // divisor of at least 1 would pull it toward zero and rank a costlier move
  // higher.
  return score > 0.0f ? score / cost_divisor_ : score;
}

MoveResult VtSwapMtCandidate::apply()
{
  // Apply the chosen VT replacement after the policy has selected the best
  // score.
  const bool accepted = resizer_.replaceCell(inst_, candidate_cell_);
  debugPrint(resizer_.logger(),
             RSZ,
             "opt_moves",
             1,
             applyLogFormat(accepted),
             resizer_.network()->pathName(target().driver_pin),
             current_cell_->name(),
             candidate_cell_->name());
  return {
      .accepted = accepted,
      .type = MoveType::kVtSwap,
      .move_count = accepted ? 1 : 0,
      .touched_instances = accepted ? std::vector<sta::Instance*>{inst_}
                                    : std::vector<sta::Instance*>{},
  };
}

MoveImpact VtSwapMtCandidate::impact() const
{
  return cellSwapImpact(inst_);
}

}  // namespace rsz
