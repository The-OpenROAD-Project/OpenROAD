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
  bool pathImproved(int end_index,
                    sta::Slack end_slack,
                    sta::Slack worst_slack,
                    sta::Slack prev_end_slack,
                    sta::Slack prev_worst_slack) const;
};

class SetupWnsPolicy : public SetupLegacyBase
{
 public:
  SetupWnsPolicy(Resizer& resizer,
                 MoveCommitter& committer,
                 RepairSetupContext& setup_context,
                 bool use_cone)
      : SetupLegacyBase(resizer, committer, setup_context), use_cone_(use_cone)
  {
  }

  const char* name() const override { return "SetupWnsPolicy"; }
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

class SetupTnsPolicy : public SetupLegacyBase
{
 public:
  using SetupLegacyBase::SetupLegacyBase;

  const char* name() const override { return "SetupTnsPolicy"; }
  void iterate() override;

 private:
  void repairSetupTns(float setup_slack_margin,
                      int max_passes_per_endpoint,
                      int max_repairs_per_pass,
                      bool verbose,
                      rsz::ViolatorSortType sort_type,
                      PhaseRunContext& ctx);
};

class SetupDirectionalPolicy : public SetupLegacyBase
{
 public:
  SetupDirectionalPolicy(Resizer& resizer,
                         MoveCommitter& committer,
                         RepairSetupContext& setup_context,
                         bool use_starts)
      : SetupLegacyBase(resizer, committer, setup_context),
        use_starts_(use_starts)
  {
  }

  const char* name() const override { return "SetupDirectionalPolicy"; }
  void iterate() override;

 private:
  void repairSetupDirectional(bool use_startpoints,
                              float setup_slack_margin,
                              int max_passes_per_point,
                              bool verbose,
                              PhaseRunContext& ctx);

  bool use_starts_{false};
};

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

class SetupCritVtSwapPolicy : public SetupLegacyBase
{
 public:
  using SetupLegacyBase::SetupLegacyBase;

  const char* name() const override { return "SetupCritVtSwapPolicy"; }
  void iterate() override;

 private:
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
