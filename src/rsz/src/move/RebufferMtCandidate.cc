// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RebufferMtCandidate.hh"

#include <optional>
#include <string>
#include <vector>

#include "BufferedNet.hh"
#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "Rebuffer.hh"
#include "rsz/Resizer.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

RebufferMtCandidate::RebufferMtCandidate(
    Resizer& resizer,
    const Target& target,
    const EstimatorContext& estimator_context,
    const RebufferTreePlan& plan,
    const float score_scale,
    const float min_score,
    std::vector<BufferTreeSidePathContext> side_contexts)
    : MoveCandidate(resizer, target),
      estimator_context_(estimator_context),
      plan_(plan),
      side_contexts_(std::move(side_contexts)),
      score_scale_(score_scale),
      min_score_(min_score)
{
}

Estimate RebufferMtCandidate::estimate()
{
  // Score the same topology that apply() commits.  Side loads moved behind
  // new buffers pay their negative-region slack loss out of the target gain.
  const DelayEstimate delay_est
      = DelayEstimator::estimateBufferTreeWithSidePaths(
          estimator_context_, plan_, side_contexts_, nullptr, nullptr);
  raw_score_ = delay_est.arrival_impr - delay_est.side_loss;
  Estimate estimate{.reason = failReasonName(delay_est.reason)};
  estimate.score = scaledScore();
  if (delay_est.side_paths_evaluated > 0) {
    estimate.side_loss = delay_est.side_loss;
  }

  // An estimator reject keeps its own gain only when it computed one.
  if (!delay_est.legal) {
    if (delay_est.reason == FailReason::kEstimateNonImproving
        || delay_est.reason == FailReason::kSideSlackFloorViolation) {
      estimate.raw_gain = delay_est.arrival_impr;
    }
    return estimate;
  }

  // The side loss or the score floor can still reject a legal estimate; a
  // positive own gain stays admissible in the WNS tail.
  estimate.raw_gain = delay_est.arrival_impr;
  if (raw_score_ <= 0.0f || raw_score_ < min_score_) {
    estimate.reason
        = raw_score_ <= 0.0f ? "side_loss_non_improving" : "min_score";
    estimate.tail_admissible = delay_est.arrival_impr > 0.0f;
    return estimate;
  }

  estimate.legal = true;
  return estimate;
}

MoveResult RebufferMtCandidate::apply()
{
  const int inserted_count
      = resizer_.rebuffer().applyRebufferTreePlan(plan_, "rebuffer_mt");
  if (inserted_count <= 0) {
    debugPrint(resizer_.logger(),
               RSZ,
               "rebuffer_mt",
               1,
               "REJECT rebuffer_mt {}: exportBufferTree failed",
               logName());

    return rejectedMove();
  }

  sta::Instance* driver_inst
      = resizer_.network()->instance(target().driver_pin);

  debugPrint(resizer_.logger(),
             RSZ,
             "rebuffer_mt",
             1,
             "ACCEPT rebuffer_mt {}: inserted {} buffers, tree loads {}, "
             "raw_score {:.3e}, scaled_score {:.3e}",
             logName(),
             inserted_count,
             plan_.load_count,
             raw_score_,
             scaledScore());

  return {
      .accepted = true,
      .type = MoveType::kBuffer,
      .move_count = inserted_count,
      .touched_instances = {driver_inst},
  };
}

std::string RebufferMtCandidate::logName() const
{
  return resizer_.network()->pathName(target().driver_pin);
}

}  // namespace rsz
