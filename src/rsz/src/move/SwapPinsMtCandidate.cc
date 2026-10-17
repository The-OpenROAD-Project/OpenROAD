// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SwapPinsMtCandidate.hh"

#include <algorithm>
#include <memory>
#include <utility>

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

SwapPinsMtCandidate::SwapPinsMtCandidate(
    Resizer& resizer,
    const Target& target,
    std::shared_ptr<const SwapPinsMtInput> input,
    const SwapPinsMtInput::Equivalent& swap)
    : MoveCandidate(resizer, target), input_(std::move(input)), swap_(swap)
{
}

Estimate SwapPinsMtCandidate::estimate()
{
  const float gain = input_->input_delay - swap_.delay;
  if (swap_.pin == nullptr) {
    const bool legal = gain > 0.0f;
    return {
        .legal = legal,
        .score = gain,
        .raw_gain = gain,
        .reason = failReasonName(legal ? FailReason::kEstimateLegal
                                       : FailReason::kEstimateNonImproving)};
  }

  // The displaced signal loses what the critical signal gains.  Charge only the
  // part below zero slack, and reject a fall below the side slack floor.
  const EstimatorContext& context = target().estimator_context.value();
  const float victim_after = swap_.slack - gain;
  const float side_loss
      = context.fanin_side_loss_weight
        * (std::min(swap_.slack, 0.0f) - std::min(victim_after, 0.0f));
  const float adjusted_gain = gain - side_loss;
  Estimate estimate{.legal = adjusted_gain > 0.0f,
                    .raw_gain = gain,
                    .reason = failReasonName(FailReason::kEstimateLegal),
                    .side_loss = side_loss};
  estimate.score = adjusted_gain;
  if (gain <= 0.0f) {
    estimate.legal = false;
    estimate.reason = failReasonName(FailReason::kEstimateNonImproving);
  } else if (victim_after < context.sideSlackFloor(gain)) {
    estimate.legal = false;
    estimate.reason = failReasonName(FailReason::kSideSlackFloorViolation);
  } else if (!estimate.legal) {
    estimate.reason = "side_loss_non_improving";
    estimate.tail_admissible = true;
  }

  return estimate;
}

MoveImpact SwapPinsMtCandidate::impact() const
{
  const SwapPinsMtInput& input = *input_;
  MoveImpact impact = MoveCandidate::impact();
  impact.complete = true;
  impact.write_instances.push_back(input.drvr);
  impact.timing_effect_instances.push_back(input.drvr);

  // Both input nets exchange load pins and the output arcs change.
  sta::Network* network = resizer_.network();
  for (sta::LibertyPort* port :
       {input.input_port, swap_.port, input.drvr_port}) {
    sta::Pin* pin = network->findPin(input.drvr, port);
    if (pin == nullptr) {
      continue;
    }

    if (port != input.drvr_port) {
      impact.reconnect_pins.push_back(pin);
    }

    appendUnique(impact.timing_effect_nets, network->net(pin));
  }

  return impact;
}

MoveResult SwapPinsMtCandidate::apply()
{
  const SwapPinsMtInput& input = *input_;
  sta::Pin* drvr_pin = resizer_.network()->findPin(input.drvr, input.drvr_port);
  debugPrint(resizer_.logger(),
             RSZ,
             "swap_pins_move",
             1,
             "ACCEPT SwapPinsMove {}: Cell {}, pins {} <-> {}",
             resizer_.network()->name(drvr_pin),
             input.drvr_port->libertyCell()->name(),
             input.input_port->name(),
             swap_.port->name());

  if (!resizer_.swapPins(input.drvr, input.input_port, swap_.port)) {
    return rejectedMove();
  }
  return {
      .accepted = true,
      .type = MoveType::kSwapPins,
      .move_count = 1,
      .touched_instances = {input.drvr},
  };
}

}  // namespace rsz
