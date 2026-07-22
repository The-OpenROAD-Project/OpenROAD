// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <string>

#include "dr/FlexDR.h"
#include "frBaseTypes.h"

namespace drt {

FlexDRFlow::State FlexDRFlow::determineNextFlow(const FlowContext& context)
{
  if (context.num_violations > STUBBORN_FLOW_VIOLATION_THRESHOLD
      || context.args.ripupMode == RipUpMode::ALL
      || context.args.ripupMode == RipUpMode::INCR || fixing_max_spacing_) {
    current_state_ = State::OPTIMIZATION;
  } else {
    if (last_iteration_effective_ || isArgsChanged(context)) {
      current_state_ = State::GUIDES;
    } else if (current_state_ == State::GUIDES) {
      current_state_ = State::STUBBORN;
    } else {
      current_state_ = State::SKIP;
    }
  }
  last_args_ = context.args;
  return current_state_;
}

FlexDRFlow::State FlexDRFlow::getCurrentState() const
{
  return current_state_;
}

std::string FlexDRFlow::getFlowName() const
{
  switch (current_state_) {
    case State::OPTIMIZATION:
      return "optimization";
    case State::STUBBORN:
      return "stubborn tiles";
    case State::GUIDES:
      return "guides tiles";
    case State::SKIP:
      return "skip";
  }
  return "";
}

void FlexDRFlow::setLastIterationEffective(bool value)
{
  last_iteration_effective_ = value;
}

void FlexDRFlow::setFixingMaxSpacing(bool value)
{
  fixing_max_spacing_ = value;
}

bool FlexDRFlow::isArgsChanged(const FlowContext& context) const
{
  return !context.args.isEqualIgnoringSizeAndOffset(last_args_);
}

}  // namespace drt
