// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "SetupLegacyPolicy.hh"

namespace rsz {

class MainRepairPhasePolicy : public SetupLegacyPolicy
{
 public:
  using SetupLegacyPolicy::SetupLegacyPolicy;

  const char* name() const override { return "MainRepairPhasePolicy"; }
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

  void runMainRepairPhase();
  bool initializeMainRepair(float setup_slack_margin,
                            double repair_tns_end_percent,
                            MainRepairState& main_state,
                            ViolatingEnds& violating_ends);
  void runMainRepairLoop(const ViolatingEnds& violating_ends,
                         float setup_slack_margin,
                         int max_passes,
                         int max_iterations,
                         bool verbose,
                         MainRepairState& main_state);
  virtual bool beginEndpointRepair(
      const std::pair<sta::Vertex*, sta::Slack>& end_original_slack,
      MainRepairState& main_state,
      EndpointRepairState& endpoint_state);
  virtual void repairEndpoint(EndpointRepairState& endpoint_state,
                              MainRepairState& main_state,
                              float setup_slack_margin,
                              int max_passes,
                              int max_iterations,
                              bool verbose);
  virtual bool shouldStopEndpointRepair(EndpointRepairState& endpoint_state,
                                        float setup_slack_margin);
  bool shouldStopMainRepair(const MainRepairState& main_state) const;
  bool pathImproved(int end_index,
                    sta::Slack end_slack,
                    sta::Slack worst_slack,
                    sta::Slack prev_end_slack,
                    sta::Slack prev_worst_slack) const;
};

class WnsPhasePolicy : public SetupLegacyPolicy
{
 public:
  WnsPhasePolicy(Resizer& resizer,
                 MoveCommitter& committer,
                 RepairSetupContext& setup_context,
                 bool use_cone)
      : SetupLegacyPolicy(resizer, committer, setup_context),
        use_cone_(use_cone)
  {
  }

  const char* name() const override { return "WnsPhasePolicy"; }
  void iterate() override;

 private:
  void repairSetupWns(float setup_slack_margin,
                      int max_passes_per_endpoint,
                      int max_repairs_per_pass,
                      bool verbose,
                      bool use_cone_collection,
                      rsz::ViolatorSortType sort_type,
                      PhaseRunContext& ctx);

  bool use_cone_{false};
};

class TnsPhasePolicy : public SetupLegacyPolicy
{
 public:
  using SetupLegacyPolicy::SetupLegacyPolicy;

  const char* name() const override { return "TnsPhasePolicy"; }
  void iterate() override;

 private:
  void repairSetupTns(float setup_slack_margin,
                      int max_passes_per_endpoint,
                      int max_repairs_per_pass,
                      bool verbose,
                      rsz::ViolatorSortType sort_type,
                      PhaseRunContext& ctx);
};

class DirectionalPhasePolicy : public SetupLegacyPolicy
{
 public:
  DirectionalPhasePolicy(Resizer& resizer,
                         MoveCommitter& committer,
                         RepairSetupContext& setup_context,
                         bool use_starts)
      : SetupLegacyPolicy(resizer, committer, setup_context),
        use_starts_(use_starts)
  {
  }

  const char* name() const override { return "DirectionalPhasePolicy"; }
  void iterate() override;

 private:
  void repairSetupDirectional(bool use_startpoints,
                              float setup_slack_margin,
                              int max_passes_per_point,
                              bool verbose,
                              PhaseRunContext& ctx);

  bool use_starts_{false};
};

class LastGaspPhasePolicy : public SetupLegacyPolicy
{
 public:
  using SetupLegacyPolicy::SetupLegacyPolicy;

  const char* name() const override { return "LastGaspPhasePolicy"; }
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

  void repairSetupLastGasp(const RepairSetupParams& params,
                           int max_iterations,
                           PhaseRunContext& ctx);
  void buildLastGaspMoveSequence(const RepairSetupParams& params);
  bool initializeLastGaspRepair(const RepairSetupParams& params,
                                int opto_iteration,
                                float initial_tns,
                                LastGaspState& last_gasp_state,
                                ViolatingEnds& violating_ends);
  void runLastGaspLoop(const ViolatingEnds& violating_ends,
                       const RepairSetupParams& params,
                       int max_iterations,
                       LastGaspState& last_gasp_state);
  bool beginLastGaspEndpoint(
      const std::pair<sta::Vertex*, sta::Slack>& end_original_slack,
      LastGaspState& last_gasp_state,
      EndpointRepairState& endpoint_state);
  void repairLastGaspEndpoint(EndpointRepairState& endpoint_state,
                              LastGaspState& last_gasp_state,
                              const RepairSetupParams& params,
                              int max_iterations);
  bool advanceLastGaspProgress(EndpointRepairState& endpoint_state,
                               LastGaspState& last_gasp_state,
                               const RepairSetupParams& params,
                               float curr_tns);
  bool lastGaspImproved(sta::Slack worst_slack,
                        float curr_tns,
                        sta::Slack prev_worst_slack,
                        float prev_tns) const;
  bool shouldStopLastGasp(const LastGaspState& last_gasp_state,
                          int max_iterations) const;

  static constexpr int max_last_gasp_passes_ = 10;
};

class CritVtSwapPhasePolicy : public SetupLegacyPolicy
{
 public:
  using SetupLegacyPolicy::SetupLegacyPolicy;

  const char* name() const override { return "CritVtSwapPhasePolicy"; }
  void iterate() override;

 private:
  void runCriticalVtSwapPhase(int& num_viols);
  bool swapVTCritCells(const RepairSetupParams& params, int& num_viols);
  sta::Pin* worstOutputPin(sta::Instance* inst);
  void traverseFaninCone(sta::Vertex* endpoint,
                         std::unordered_map<sta::Instance*, float>& crit_insts,
                         std::unordered_set<sta::Vertex*>& visited,
                         std::unordered_set<sta::Instance*>& notSwappable,
                         const RepairSetupParams& params);
  sta::Slack getInstanceSlack(sta::Instance* inst);
};

}  // namespace rsz
