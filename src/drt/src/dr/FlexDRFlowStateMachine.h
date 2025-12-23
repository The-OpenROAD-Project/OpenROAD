// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "frBaseTypes.h"

namespace drt {

struct SearchRepairArgs;

class FlexDRFlowStateMachine
{
 public:
  enum class FlowState
  {
    OPTIMIZATION,
    STUBBORN,
    GUIDES,
    SKIP
  };

  struct FlowContext
  {
    int num_violations{0};
    RipUpMode ripup_mode{RipUpMode::DRC};
    bool fixing_max_spacing{false};
    bool args_changed{true};
  };

  FlexDRFlowStateMachine();

  FlowState determineNextFlow(const FlowContext& context);

  void reset();

  void setWaitingForChange(bool waiting_for_change)
  {
    waiting_for_change_ = waiting_for_change;
  }

  void setTriedGuideFlow(bool tried_guide_flow)
  {
    tried_guide_flow_ = tried_guide_flow;
  }
  static const char* getFlowName(FlowState state);

  bool hasTriedGuideFlow() const { return tried_guide_flow_; }
  bool isWaitingForChange() const { return waiting_for_change_; }

 private:
  bool shouldUseStubbornFlow(const FlowContext& context) const;

  bool shouldUseGuidesFlow(const FlowContext& context) const;

  bool shouldSkipIteration(const FlowContext& context) const;

  bool tried_guide_flow_{false};
  bool waiting_for_change_{false};

  static constexpr int STUBBORN_FLOW_VIOLATION_THRESHOLD = 11;
};

}  // namespace drt
