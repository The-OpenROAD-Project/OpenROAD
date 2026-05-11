// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "SetupLegacyBase.hh"

namespace rsz {

// Default single-threaded setup-repair phase.  This preserves the original
// endpoint/path-driver repair loop when the LEGACY phase is selected.
class SetupLegacyPolicy : public SetupLegacyBase
{
 public:
  using SetupLegacyBase::SetupLegacyBase;

  const char* name() const override { return "SetupLegacyPolicy"; }
  void iterate() override;

 protected:
  // Mutable progress counters for the main endpoint repair loop.
  struct MainRepairState
  {
    int end_index{0};
    int max_end_count{0};
    int num_viols{0};
    int opto_iteration{0};
    float initial_tns{0.0f};
    float prev_tns{0.0f};
    float fix_rate_threshold{0.0f};
    bool prev_termination{false};
    bool two_cons_terminations{false};
    char phase_marker{'*'};
  };

  bool initializeMainRepair(MainRepairState& main_state,
                            ViolatingEnds& violating_ends);
  void runMainRepairLoop(const ViolatingEnds& violating_ends,
                         MainRepairState& main_state);
  virtual bool beginEndpointRepair(
      const std::pair<sta::Vertex*, sta::Slack>& end_original_slack,
      MainRepairState& main_state,
      EndpointRepairState& endpoint_state);
  virtual void repairEndpoint(EndpointRepairState& endpoint_state,
                              MainRepairState& main_state);
  virtual const char* phaseName() const { return "LEGACY"; }
  virtual const char* phaseSummaryTitle() const
  {
    return "LEGACY Phase Summary";
  }
  virtual const char* phaseEndpointProfilerTitle() const
  {
    return "LEGACY Phase Endpoint Profiler";
  }
  bool pathImproved(int end_index,
                    sta::Slack end_slack,
                    sta::Slack worst_slack,
                    sta::Slack prev_end_slack,
                    sta::Slack prev_worst_slack) const;
};

}  // namespace rsz
