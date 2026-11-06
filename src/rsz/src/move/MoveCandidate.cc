// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "MoveCandidate.hh"

#include <memory>

#include "OptimizerTypes.hh"
#include "SequentialTimingEstimator.hh"
#include "rsz/Resizer.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"

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

std::vector<const sta::Pin*> MoveCandidate::faninSidePins(
    const EstimatorContext& estimator_context)
{
  std::vector<const sta::Pin*> pins;
  for (const FaninSideNet& side_net : estimator_context.fanin_side_nets) {
    for (const SideLoad& load : side_net.loads) {
      if (load.pin != nullptr) {
        pins.push_back(load.pin);
      }
    }
  }

  return pins;
}

MoveImpact MoveCandidate::impact() const
{
  MoveImpact impact;
  if (!target_.estimator_context.has_value()) {
    return impact;
  }

  sta::Network* network = resizer_.network();
  for (const DelayStageState& stage : target_.estimator_context->path_stages) {
    sta::Pin* driver_pin = stage.driver_pin;
    if (driver_pin == nullptr) {
      continue;
    }

    appendUnique(impact.timing_dependency_instances,
                 network->instance(driver_pin));
    appendUnique(impact.timing_dependency_nets, network->net(driver_pin));
  }

  return impact;
}

MoveImpact MoveCandidate::cellSwapImpact(sta::Instance* inst) const
{
  MoveImpact impact = MoveCandidate::impact();
  impact.complete = true;
  appendUnique(impact.write_instances, inst);
  appendUnique(impact.timing_effect_instances, inst);

  // A new cell changes its input capacitances and output timing, so every
  // net on its pins is in the effect cone.
  sta::Network* network = resizer_.network();
  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network->pinIterator(inst));
  while (pin_iter->hasNext()) {
    appendUnique(impact.timing_effect_nets, network->net(pin_iter->next()));
  }

  return impact;
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
