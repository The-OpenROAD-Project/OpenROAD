// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dr/FlexDRFlowStateMachine.h"

#include "dr/FlexDR.h"

namespace drt {

FlexDRFlowStateMachine::FlexDRFlowStateMachine()
    : tried_guide_flow_(false), waiting_for_change_(false)
{
}

FlexDRFlowStateMachine::FlowState FlexDRFlowStateMachine::determineNextFlow(
    const FlowContext& context)
{
  if (shouldSkipIteration(context)) {
    return FlowState::SKIP;
  }

  if (shouldUseGuidesFlow(context)) {
    return FlowState::GUIDES;
  }

  if (shouldUseStubbornFlow(context)) {
    return FlowState::STUBBORN;
  }

  return FlowState::OPTIMIZATION;
}

void FlexDRFlowStateMachine::reset()
{
  tried_guide_flow_ = false;
  waiting_for_change_ = false;
}

const char* FlexDRFlowStateMachine::getFlowName(FlowState state)
{
  switch (state) {
    case FlowState::OPTIMIZATION:
      return "optimization";
    case FlowState::STUBBORN:
      return "stubborn tiles";
    case FlowState::GUIDES:
      return "guides tiles";
    case FlowState::SKIP:
      return "skip";
    default:
      return "unknown";
  }
}

bool FlexDRFlowStateMachine::shouldUseStubbornFlow(
    const FlowContext& context) const
{
  const bool violation_count_ok
      = context.num_violations <= STUBBORN_FLOW_VIOLATION_THRESHOLD;

  const bool ripup_mode_ok = context.ripup_mode != RipUpMode::ALL
                             && context.ripup_mode != RipUpMode::INCR;

  const bool not_fixing_spacing = !context.fixing_max_spacing;

  const bool can_proceed = !waiting_for_change_ || context.args_changed;

  return violation_count_ok && ripup_mode_ok && not_fixing_spacing
         && can_proceed;
}

bool FlexDRFlowStateMachine::shouldUseGuidesFlow(
    const FlowContext& context) const
{
  const bool stubborn_conditions_met
      = context.num_violations <= STUBBORN_FLOW_VIOLATION_THRESHOLD
        && context.ripup_mode != RipUpMode::ALL
        && context.ripup_mode != RipUpMode::INCR && !context.fixing_max_spacing;

  const bool stuck = waiting_for_change_ && !context.args_changed;

  const bool not_tried_yet = !tried_guide_flow_;

  return stubborn_conditions_met && stuck && not_tried_yet;
}

bool FlexDRFlowStateMachine::shouldSkipIteration(
    const FlowContext& context) const
{
  const bool stuck = waiting_for_change_ && !context.args_changed;

  return stuck && tried_guide_flow_;
}

}  // namespace drt
