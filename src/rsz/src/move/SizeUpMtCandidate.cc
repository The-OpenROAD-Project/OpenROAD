// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeUpMtCandidate.hh"

#include <optional>
#include <string>

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

SizeUpMtCandidate::SizeUpMtCandidate(Resizer& resizer,
                                     const Target& target,
                                     sta::Instance* inst,
                                     sta::LibertyCell* replacement,
                                     float score_scale)
    : MoveCandidate(resizer, target),
      inst_(inst),
      current_cell_(resizer.network()->libertyCell(inst)),
      replacement_(replacement),
      score_scale_(score_scale)
{
}

Estimate SizeUpMtCandidate::estimate()
{
  // Score one size-up replacement from immutable inputs prepared before the
  // worker pool runs candidate estimation.
  const DelayEstimate delay_est = DelayEstimator::estimate(
      target().estimator_context.value(), replacement_);
  Estimate estimate{.legal = delay_est.legal,
                    .raw_gain = delay_est.arrival_impr,
                    .reason = failReasonName(delay_est.reason)};
  estimate.score = delay_est.arrival_impr * score_scale_;
  if (!delay_est.legal) {
    // The opposite-transition reject carries the target gain in its score.
    return {
        .legal = false,
        .score = estimate.score,
        .raw_gain = delay_est.reason == FailReason::kEstimateNonImproving
                        ? std::optional<float>(delay_est.arrival_impr)
                        : std::nullopt,
        .reason = failReasonName(delay_est.reason),
        .tail_admissible = delay_est.reason == FailReason::kDualRfNonImproving
                           && estimate.score > 0.0f};
  }

  chargeCellSwapSideCosts(delay_est, replacement_, score_scale_, estimate);
  return estimate;
}

MoveResult SizeUpMtCandidate::apply()
{
  // Re-validate max-cap against state mutated earlier in this commit batch.
  // Candidates were screened on a pre-batch snapshot, so two accepted
  // size-ups on different sinks of the same fanin net could each pass
  // independently yet combine past the driver's max-cap limit.
  if (!resizer_.replacementPreservesMaxCap(
          inst_, replacement_, target().mode(resizer_))) {
    debugPrint(resizer_.logger(),
               RSZ,
               "opt_moves",
               1,
               "REJECT size_up_mt {}: {} -> {} max-cap re-check failed",
               logName(),
               current_cell_->name(),
               replacement_->name());
    return {
        .accepted = false,
        .type = MoveType::kSizeUp,
        .touched_instances = {},
    };
  }

  const bool accepted = resizer_.replaceCell(inst_, replacement_);
  if (accepted) {
    const std::string& current_cell_name = current_cell_->name();
    const std::string& replacement_name = replacement_->name();
    debugPrint(resizer_.logger(),
               RSZ,
               "opt_moves",
               1,
               "ACCEPT size_up_mt {}: {} -> {}",
               logName(),
               current_cell_name,
               replacement_name);
    return {
        .accepted = true,
        .type = MoveType::kSizeUp,
        .move_count = 1,
        .touched_instances = {inst_},
    };
  }

  debugPrint(resizer_.logger(),
             RSZ,
             "opt_moves",
             1,
             "REJECT size_up_mt {}: {} -> {} replace failed",
             logName(),
             current_cell_->name(),
             replacement_->name());
  return {
      .accepted = false,
      .type = MoveType::kSizeUp,
      .touched_instances = {},
  };
}

MoveImpact SizeUpMtCandidate::impact() const
{
  return cellSwapImpact(inst_);
}

std::string SizeUpMtCandidate::logName() const
{
  return resizer_.network()->pathName(target().driver_pin);
}

}  // namespace rsz
