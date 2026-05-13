// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "VtSwapMtCandidate.hh"

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

Estimate makeRejectedEstimate(const float score = 0.0f)
{
  return {.legal = false, .score = score};
}

Estimate makeAcceptedEstimate(const float score)
{
  return {.legal = true, .score = score};
}

const char* applyLogFormat(const bool accepted)
{
  return accepted ? "ACCEPT vt_swap_mt1 {}: {} -> {}"
                  : "REJECT vt_swap_mt1 {}: {} -> {} swap failed";
}

}  // namespace

VtSwapMtCandidate::VtSwapMtCandidate(Resizer& resizer,
                                     const Target& target,
                                     sta::Pin* driver_pin,
                                     sta::Instance* inst,
                                     sta::LibertyCell* current_cell,
                                     sta::LibertyCell* candidate_cell,
                                     const ArcDelayState& arc_delay)
    : MoveCandidate(resizer, target),
      driver_pin_(driver_pin),
      inst_(inst),
      current_cell_(current_cell),
      candidate_cell_(candidate_cell),
      arc_delay_(arc_delay)
{
}

Estimate VtSwapMtCandidate::estimate()
{
  // Reuse the pre-built local delay context to score the candidate without
  // mutating the design.
  if (!resizer_.replacementPreservesMaxCap(inst_, candidate_cell_)) {
    return makeRejectedEstimate();
  }

  const DelayEstimate delay_est
      = DelayEstimator::estimate(arc_delay_, candidate_cell_);
  if (!delay_est.legal) {
    // Preserve the computed score for non-improving swaps so policy ranking can
    // still compare rejected candidates consistently.
    if (delay_est.reason == FailReason::kEstimateNonImproving) {
      return makeRejectedEstimate(delay_est.arrival_impr);
    }
    return makeRejectedEstimate();
  }
  return makeAcceptedEstimate(delay_est.arrival_impr);
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
             getDrvPinName(),
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

std::string VtSwapMtCandidate::getDrvPinName() const
{
  return resizer_.network()->pathName(driver_pin_);
}

}  // namespace rsz
