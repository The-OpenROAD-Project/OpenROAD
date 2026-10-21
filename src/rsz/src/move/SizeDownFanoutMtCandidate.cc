// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeDownFanoutMtCandidate.hh"

#include <memory>
#include <optional>
#include <utility>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

SizeDownFanoutMtCandidate::SizeDownFanoutMtCandidate(
    Resizer& resizer,
    const Target& target,
    std::shared_ptr<const SizeDownFanoutMtLoad> load,
    const float score_scale)
    : MoveCandidate(resizer, target),
      load_(std::move(load)),
      score_scale_(score_scale)
{
}

Estimate SizeDownFanoutMtCandidate::estimate()
{
  const DelayEstimate delay_est = DelayEstimator::estimateLoadChange(
      target().estimator_context.value(), *load_->step_down.target_load_delta);
  Estimate estimate{
      .legal = delay_est.legal,
      .raw_gain
      = delay_est.legal || delay_est.reason == FailReason::kEstimateNonImproving
            ? std::optional<float>(delay_est.arrival_impr)
            : std::nullopt,
      .reason = failReasonName(delay_est.reason)};
  estimate.score = delay_est.arrival_impr * score_scale_;
  return estimate;
}

MoveResult SizeDownFanoutMtCandidate::apply()
{
  const bool accepted
      = resizer_.replaceCell(load_->load_inst, load_->step_down.cell);
  debugPrint(resizer_.logger(),
             RSZ,
             "size_down_fanout_move",
             3,
             "{} SizeDownFanoutMove {} -> {}: ({} -> {}) slack={}",
             accepted ? "ACCEPT" : "REJECT",
             resizer_.network()->pathName(target().driver_pin),
             resizer_.network()->pathName(load_->load_pin),
             load_->current_cell->name(),
             load_->step_down.cell->name(),
             delayAsString(load_->load_slack, 3, resizer_.staState()));
  if (!accepted) {
    return rejectedMove();
  }

  return {
      .accepted = true,
      .type = MoveType::kSizeDownFanout,
      .move_count = 1,
      .touched_instances = {load_->load_inst},
  };
}

MoveImpact SizeDownFanoutMtCandidate::impact() const
{
  // The load instance changes its cell, and the target driver sees the smaller
  // pin.
  MoveImpact impact = cellSwapImpact(load_->load_inst);
  appendUnique(impact.timing_effect_instances,
               resizer_.network()->instance(target().driver_pin));
  return impact;
}

}  // namespace rsz
