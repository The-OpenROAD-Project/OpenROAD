// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SplitLoadMtCandidate.hh"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "odb/dbTypes.h"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

SplitLoadMtCandidate::SplitLoadMtCandidate(
    Resizer& resizer,
    const Target& target,
    std::shared_ptr<const SplitLoadMtInput> input,
    const float score_scale)
    : MoveCandidate(resizer, target),
      input_(std::move(input)),
      score_scale_(score_scale)
{
}

Estimate SplitLoadMtCandidate::estimate()
{
  const EstimatorContext& context = target().estimator_context.value();
  const DelayStageState& stage = context.target();
  const SplitLoadMtInput& input = *input_;

  // The path gain: the target driver sheds the moved loads.
  DelayEstimate delay_est
      = DelayEstimator::estimateLoadChange(context, input.target_load_delta);
  if (!delay_est.legal) {
    return {.legal = false,
            .score = delay_est.arrival_impr * score_scale_,
            .raw_gain = delay_est.reason == FailReason::kEstimateNonImproving
                            ? std::optional<float>(delay_est.arrival_impr)
                            : std::nullopt,
            .reason = failReasonName(delay_est.reason)};
  }

  // The moved loads arrive through the faster driver and the new buffer.
  float driver_delay = 0.0f;
  float buffer_delay = 0.0f;
  if (!DelayEstimator::findArcDelay(
          stage.arc,
          stage.input_slew,
          std::max(stage.load_cap + input.target_load_delta, 0.0f),
          stage.arc.currentCell(),
          driver_delay)
      || !DelayEstimator::findBufferDelay(
          context,
          input.resized_cell,
          stage.current_slew,
          input.moved_pin_cap + input.moved_wire_cap,
          buffer_delay)) {
    return {.legal = false,
            .reason = failReasonName(FailReason::kMissingBufferTimingArc)};
  }

  const float moved_delta
      = driver_delay - stage.current_model_delay + buffer_delay;
  DelayEstimator::chargeMovedLoads(
      context, input.moved_loads, moved_delta, delay_est);

  const float gain = delay_est.arrival_impr - delay_est.side_loss;
  Estimate estimate{.legal = delay_est.legal && gain > 0.0f,
                    .raw_gain = delay_est.arrival_impr,
                    .reason = failReasonName(delay_est.reason),
                    .side_loss = delay_est.side_loss};
  estimate.score = gain * score_scale_;
  if (delay_est.legal && gain <= 0.0f) {
    estimate.reason = "side_loss_non_improving";
    estimate.tail_admissible = delay_est.arrival_impr > 0.0f;
  }

  return estimate;
}

sta::Pin* SplitLoadMtCandidate::bufferOutputPin(sta::Instance* buffer) const
{
  sta::LibertyPort* input = nullptr;
  sta::LibertyPort* output = nullptr;
  input_->buffer_cell->bufferPorts(input, output);
  return resizer_.network()->findPin(buffer, output);
}

MoveResult SplitLoadMtCandidate::apply()
{
  // The insertion takes a mutable load list, so it works on a copy.
  sta::PinSeq load_pins = input_->load_pins;
  sta::Instance* buffer
      = resizer_.insertBufferBeforeLoads(input_->drvr_net,
                                         &load_pins,
                                         input_->buffer_cell,
                                         &input_->drvr_loc,
                                         "split",
                                         nullptr,
                                         odb::dbNameUniquifyType::IF_NEEDED);
  if (buffer == nullptr) {
    debugPrint(resizer_.logger(),
               RSZ,
               "split_load_move",
               2,
               "REJECT SplitLoadMove {}: Couldn't insert buffer",
               resizer_.network()->pathName(target().driver_pin));
    return rejectedMove();
  }

  debugPrint(resizer_.logger(),
             RSZ,
             "split_load_move",
             1,
             "ACCEPT SplitLoadMove {}: Inserted buffer {}",
             resizer_.network()->pathName(target().driver_pin),
             resizer_.network()->pathName(buffer));

  // The new buffer takes the target slew of its load, and both nets drop
  // their stale parasitics.
  static_cast<void>(resizer_.resizeToTargetSlew(bufferOutputPin(buffer)));
  resizer_.invalidateParasitics(input_->drvr_net);
  resizer_.invalidateParasitics(
      resizer_.network()->net(bufferOutputPin(buffer)));

  return {
      .accepted = true,
      .type = MoveType::kSplitLoad,
      .move_count = 1,
      .touched_instances = {buffer},
  };
}

MoveImpact SplitLoadMtCandidate::impact() const
{
  MoveImpact impact = MoveCandidate::impact();
  impact.complete = true;

  // The driver net loses the moved loads; the driver keeps its cell but no
  // other move may count on its current load.
  sta::Network* network = resizer_.network();
  sta::Instance* driver = network->instance(target().driver_pin);
  appendUnique(impact.write_instances, driver);
  appendUnique(impact.timing_effect_instances, driver);
  appendUnique(impact.write_nets, input_->drvr_net);
  appendUnique(impact.timing_effect_nets, input_->drvr_net);
  for (const sta::Pin* pin : input_->load_pins) {
    impact.reconnect_pins.push_back(pin);
    appendUnique(impact.reconnect_instances, network->instance(pin));
    appendUnique(impact.timing_effect_instances, network->instance(pin));
  }

  return impact;
}

std::vector<const sta::Pin*> SplitLoadMtCandidate::guardedSidePins() const
{
  std::vector<const sta::Pin*> pins;
  pins.reserve(input_->moved_loads.size());
  for (const SideLoad& load : input_->moved_loads) {
    pins.push_back(load.pin);
  }

  return pins;
}

}  // namespace rsz
