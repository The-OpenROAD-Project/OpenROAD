// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <utility>

#include "SetupLegacyBase.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"

namespace rsz {

class SetupLastGaspPolicy : public SetupLegacyBase
{
 public:
  using SetupLegacyBase::SetupLegacyBase;

  const char* name() const override { return "SetupLastGaspPolicy"; }
  void iterate() override;

 private:
  // Mutable progress for the last-gasp phase executed after the main repair
  // loop has converged or hit its iteration limit.
  struct LastGaspState
  {
    int end_index{0};
    int max_end_count{0};
    int num_viols{0};
    int opto_iteration{0};
    float initial_tns{0.0f};
    float prev_tns{0.0f};
    float fix_rate_threshold{0.0f};
    sta::Slack prev_worst_slack{0.0};
    bool prev_termination{false};
    bool two_cons_terminations{false};
    char phase_marker{'+'};
  };

  bool initializeLastGaspRepair(LastGaspState& last_gasp_state,
                                ViolatingEnds& violating_ends);
  void runLastGaspLoop(const ViolatingEnds& violating_ends,
                       LastGaspState& last_gasp_state);
  bool beginLastGaspEndpoint(
      const std::pair<sta::Vertex*, sta::Slack>& end_original_slack,
      LastGaspState& last_gasp_state,
      EndpointRepairState& endpoint_state);
  void repairLastGaspEndpoint(EndpointRepairState& endpoint_state,
                              LastGaspState& last_gasp_state);
  bool advanceLastGaspProgress(EndpointRepairState& endpoint_state,
                               LastGaspState& last_gasp_state,
                               float curr_tns);
  bool lastGaspImproved(sta::Slack worst_slack,
                        float curr_tns,
                        sta::Slack prev_worst_slack,
                        float prev_tns) const;
  bool shouldStopLastGasp(const LastGaspState& last_gasp_state) const;

  static constexpr int max_last_gasp_passes_ = 10;
};

}  // namespace rsz
