// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "PhasePolicies.hh"

#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "SetupLegacyPolicy.hh"
#include "dispatch.hh"

namespace rsz {

PhasePolicyBase::PhasePolicyBase(Resizer& resizer,
                                 MoveCommitter& committer,
                                 SetupLegacyPolicy* const parent)
    : OptPolicy(resizer, committer), parent_(parent)
{
}

// LEGACY phase  -  sets up MainRepairState from PhaseRunContext, runs the main
// endpoint repair loop, then writes the updated accumulators back to ctx.
void MainRepairPhasePolicy::iterate()
{
  PhaseRunContext& ctx = *run_ctx_;
  parent_->committer_.capturePrePhaseSlack();

  SetupLegacyPolicy::MainRepairState main_state;
  main_state.opto_iteration = ctx.opto_iteration;
  main_state.initial_tns = ctx.initial_tns;
  main_state.prev_tns = ctx.initial_tns;
  main_state.phase_marker = ctx.step.marker;
  SetupLegacyPolicy::ViolatingEnds violating_ends;
  if (parent_->initializeMainRepair(parent_->config_.setup_slack_margin,
                                    parent_->config_.repair_tns_end_percent,
                                    main_state,
                                    violating_ends)) {
    parent_->runMainRepairLoop(violating_ends,
                               parent_->config_.setup_slack_margin,
                               parent_->config_.max_passes,
                               parent_->config_.max_iterations,
                               parent_->config_.verbose,
                               main_state);
  }
  parent_->committer_.printTrackerPhaseSummary(
      "LEGACY Phase Summary", "LEGACY Phase Endpoint Profiler", true);

  // Propagate accumulator deltas to the sequencer so subsequent phases see
  // the updated TNS / iteration / violation counters.
  ctx.opto_iteration = main_state.opto_iteration;
  ctx.num_viols = main_state.num_viols;
  ctx.initial_tns = main_state.initial_tns;
  ctx.prev_tns = main_state.prev_tns;

  markRunComplete(true);
}

WnsPhasePolicy::WnsPhasePolicy(Resizer& resizer,
                               MoveCommitter& committer,
                               SetupLegacyPolicy* const parent,
                               const bool use_cone)
    : PhasePolicyBase(resizer, committer, parent), use_cone_(use_cone)
{
}

void WnsPhasePolicy::iterate()
{
  parent_->repairSetupWns(parent_->config_.setup_slack_margin,
                          parent_->config_.max_passes,
                          parent_->config_.max_repairs_per_pass,
                          parent_->config_.verbose,
                          /*use_cone_collection=*/use_cone_,
                          rsz::ViolatorSortType::SORT_BY_LOAD_DELAY,
                          *run_ctx_);
  if (use_cone_) {
    parent_->committer_.printTrackerPhaseSummary(
        "WNS_CONE Phase Summary", "WNS_CONE Phase Endpoint Profiler", true);
  } else {
    parent_->committer_.printTrackerPhaseSummary(
        "WNS_PATH Phase Summary", "WNS_PATH Phase Endpoint Profiler", true);
  }
  markRunComplete(true);
}

void TnsPhasePolicy::iterate()
{
  parent_->repairSetupTns(parent_->config_.setup_slack_margin,
                          parent_->config_.max_passes,
                          parent_->config_.max_repairs_per_pass,
                          parent_->config_.verbose,
                          rsz::ViolatorSortType::SORT_BY_LOAD_DELAY,
                          *run_ctx_);
  parent_->committer_.printTrackerPhaseSummary(
      "TNS Phase Summary", "TNS Phase Endpoint Profiler", true);
  markRunComplete(true);
}

DirectionalPhasePolicy::DirectionalPhasePolicy(Resizer& resizer,
                                               MoveCommitter& committer,
                                               SetupLegacyPolicy* const parent,
                                               const bool use_starts)
    : PhasePolicyBase(resizer, committer, parent), use_starts_(use_starts)
{
}

void DirectionalPhasePolicy::iterate()
{
  parent_->repairSetupDirectional(use_starts_,
                                  parent_->config_.setup_slack_margin,
                                  parent_->config_.max_passes,
                                  parent_->config_.verbose,
                                  *run_ctx_);
  if (use_starts_) {
    parent_->committer_.printTrackerPhaseSummary(
        "STARTPOINT_FANOUT Phase Summary",
        "STARTPOINT_FANOUT Phase Startpoint Profiler",
        true);
  } else {
    parent_->committer_.printTrackerPhaseSummary(
        "ENDPOINT_FANIN Phase Summary",
        "ENDPOINT_FANIN Phase Endpoint Profiler",
        true);
  }
  markRunComplete(true);
}

void LastGaspPhasePolicy::iterate()
{
  if (parent_->config_.skip_last_gasp) {
    markRunComplete(false);
    return;
  }
  parent_->committer_.capturePrePhaseSlack();
  RepairSetupParams params
      = parent_->makeRepairSetupParams(parent_->config_.setup_slack_margin);
  parent_->repairSetupLastGasp(
      params, parent_->config_.max_iterations, *run_ctx_);
  parent_->committer_.printTrackerPhaseSummary(
      "LAST_GASP Phase Summary", "LAST_GASP Phase Endpoint Profiler", true);
  markRunComplete(true);
}

}  // namespace rsz
