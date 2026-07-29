// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SetupLastGaspPolicy.hh"

#include <cmath>
#include <utility>
#include <vector>

#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "RepairTargetCollector.hh"
#include "est/EstimateParasitics.h"
#include "policy/OptimizationPolicy.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/GraphClass.hh"
#include "sta/Path.hh"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace rsz {

using std::pair;
using utl::RSZ;

namespace {
static constexpr int kDelayDigits = 3;

}  // namespace

void SetupLastGaspPolicy::iterate()
{
  if (config_.skip_last_gasp) {
    markRunComplete(false);
    return;
  }

  committer_.capturePrePhaseSlack();
  int& num_viols = setup_context_.violation_count;
  const char phase_marker = phaseMarkerForIndex(setup_context_.phase_index);
  {
    const utl::DebugScopedTimer timer(
        logger_,
        RSZ,
        "repair_setup",
        10,
        fmt::format("LAST_GASP{} Phase Time: {{}}", phase_marker));
    LastGaspState last_gasp_state;
    last_gasp_state.phase_marker = phase_marker;
    ViolatingEnds violating_ends;
    if (!initializeLastGaspRepair(last_gasp_state, violating_ends)) {
      num_viols = last_gasp_state.num_viols;
    } else {
      runLastGaspLoop(violating_ends, last_gasp_state);
      num_viols = last_gasp_state.num_viols;
      if (logger_->debugCheck(RSZ, "repair_setup", 1)) {
        sta::Slack final_wns;
        sta::Vertex* final_worst;
        sta_->worstSlack(max_, final_wns, final_worst);
        const float final_tns = sta_->totalNegativeSlack(max_);
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "LAST_GASP{} Phase complete. WNS: {}, TNS: {}",
                   phase_marker,
                   delayAsString(final_wns, kDelayDigits, sta_),
                   delayAsString(final_tns, 1, sta_));
      }
    }
  }
  committer_.printTrackerPhaseSummary(
      "LAST_GASP Phase Summary", "LAST_GASP Phase Endpoint Profiler", true);
  markRunComplete(true);
}

bool SetupLastGaspPolicy::initializeLastGaspRepair(
    SetupLastGaspPolicy::LastGaspState& last_gasp_state,
    SetupLegacyBase::ViolatingEnds& violating_ends)
{
  // Last-gasp intentionally narrows the move sequence to transforms that still
  // have a chance to improve slack without large topology changes.
  move_sequence_.clear();
  pushMoveIfEnabled(!config_.skip_vt_swap, MoveType::kVtSwap);
  move_sequence_.push_back(MoveType::kSizeUpMatch);
  move_sequence_.push_back(MoveType::kSizeUp);
  pushMoveIfEnabled(!config_.skip_pin_swap, MoveType::kSwapPins);
  activateMoveSequence(false);

  violating_ends = collectViolatingEndpoints(config_.setup_slack_margin);
  last_gasp_state.num_viols = violating_ends.size();
  const float curr_tns = sta_->totalNegativeSlack(max_);
  if (sta::fuzzyGreaterEqual(curr_tns, 0)) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "LAST_GASP{} Phase: TNS is {}, exiting",
               last_gasp_state.phase_marker,
               delayAsString(curr_tns, 1, sta_));
    return false;
  }
  if (violating_ends.empty()) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "LAST_GASP{} Phase: No violating endpoints found",
               last_gasp_state.phase_marker);
    return false;
  }

  last_gasp_state.max_end_count = violating_ends.size();
  last_gasp_state.opto_iteration = setup_context_.iteration;
  last_gasp_state.initial_tns = setup_context_.initial_tns;
  last_gasp_state.prev_tns = curr_tns;
  last_gasp_state.prev_worst_slack = violating_ends.front().second;
  last_gasp_state.fix_rate_threshold = inc_fix_rate_threshold_;

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "LAST_GASP{} Phase: {} violating endpoints remain",
             last_gasp_state.phase_marker,
             last_gasp_state.max_end_count);
  printProgress(
      last_gasp_state.opto_iteration, false, last_gasp_state.phase_marker);
  return true;
}

bool SetupLastGaspPolicy::beginLastGaspEndpoint(
    const pair<sta::Vertex*, sta::Slack>& end_original_slack,
    SetupLastGaspPolicy::LastGaspState& last_gasp_state,
    SetupLegacyBase::EndpointRepairState& endpoint_state)
{
  if (!beginJournaledEndpointSearch(end_original_slack,
                                    last_gasp_state.max_end_count,
                                    last_gasp_state.end_index,
                                    endpoint_state)) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "LAST_GASP{} Phase: Hit maximum endpoint repairs of {}",
               last_gasp_state.phase_marker,
               last_gasp_state.max_end_count);
    return false;
  }

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "LAST_GASP{} Phase: Doing endpoint {} ({}/{}) endpoint slack = "
             "{}, WNS = {}",
             last_gasp_state.phase_marker,
             endpoint_state.end->name(network_),
             last_gasp_state.end_index,
             last_gasp_state.max_end_count,
             delayAsString(endpoint_state.end_slack, kDelayDigits, sta_),
             delayAsString(endpoint_state.worst_slack, kDelayDigits, sta_));
  return true;
}

bool SetupLastGaspPolicy::lastGaspImproved(const sta::Slack worst_slack,
                                           const float curr_tns,
                                           const sta::Slack prev_worst_slack,
                                           const float prev_tns) const
{
  return sta::fuzzyGreaterEqual(worst_slack, prev_worst_slack)
         && sta::fuzzyGreaterEqual(curr_tns, prev_tns);
}

bool SetupLastGaspPolicy::advanceLastGaspProgress(
    SetupLegacyBase::EndpointRepairState& endpoint_state,
    SetupLastGaspPolicy::LastGaspState& last_gasp_state,
    const float curr_tns)
{
  if (!lastGaspImproved(endpoint_state.worst_slack,
                        curr_tns,
                        last_gasp_state.prev_worst_slack,
                        last_gasp_state.prev_tns)) {
    debugPrint(
        logger_,
        RSZ,
        "repair_setup",
        2,
        "LAST_GASP{} Phase: Move rejected for endpoint {} pass {} "
        "because WNS worsened {} -> {} and TNS worsened {} -> {}",
        last_gasp_state.phase_marker,
        last_gasp_state.end_index,
        endpoint_state.pass,
        delayAsString(last_gasp_state.prev_worst_slack, kDelayDigits, sta_),
        delayAsString(endpoint_state.worst_slack, kDelayDigits, sta_),
        delayAsString(last_gasp_state.prev_tns, 1, sta_),
        delayAsString(curr_tns, 1, sta_));
    restoreEndpointState(endpoint_state);
    return false;
  }

  debugPrint(
      logger_,
      RSZ,
      "repair_setup",
      2,
      "LAST_GASP{} Phase: Move accepted for endpoint {} pass {} "
      "because WNS improved {} -> {} and TNS improved {} -> {}",
      last_gasp_state.phase_marker,
      last_gasp_state.end_index,
      endpoint_state.pass,
      delayAsString(last_gasp_state.prev_worst_slack, kDelayDigits, sta_),
      delayAsString(endpoint_state.worst_slack, kDelayDigits, sta_),
      delayAsString(last_gasp_state.prev_tns, 1, sta_),
      delayAsString(curr_tns, 1, sta_));
  last_gasp_state.prev_worst_slack = endpoint_state.worst_slack;
  last_gasp_state.prev_tns = curr_tns;
  if (sta::fuzzyGreaterEqual(endpoint_state.end_slack,
                             config_.setup_slack_margin)) {
    --last_gasp_state.num_viols;
    acceptEndpointState(endpoint_state);
    return false;
  }
  saveImprovedCheckpoint(endpoint_state);
  return true;
}

void SetupLastGaspPolicy::repairLastGaspEndpoint(
    SetupLegacyBase::EndpointRepairState& endpoint_state,
    SetupLastGaspPolicy::LastGaspState& last_gasp_state)
{
  while (endpoint_state.pass <= max_last_gasp_passes_) {
    ++last_gasp_state.opto_iteration;

    if (terminateProgress(last_gasp_state.opto_iteration,
                          last_gasp_state.initial_tns,
                          last_gasp_state.prev_tns,
                          last_gasp_state.fix_rate_threshold,
                          last_gasp_state.end_index,
                          last_gasp_state.max_end_count,
                          "LAST_GASP",
                          last_gasp_state.phase_marker)) {
      recordTermination(last_gasp_state.prev_termination,
                        last_gasp_state.two_cons_terminations);
      acceptEndpointState(endpoint_state);
      break;
    }
    if (last_gasp_state.opto_iteration % opto_small_interval_ == 0) {
      last_gasp_state.prev_termination = false;
    }
    if (config_.verbose || last_gasp_state.opto_iteration == 1) {
      printProgress(
          last_gasp_state.opto_iteration, false, last_gasp_state.phase_marker);
    }
    if (sta::fuzzyGreaterEqual(endpoint_state.end_slack,
                               config_.setup_slack_margin)) {
      --last_gasp_state.num_viols;
      acceptEndpointState(endpoint_state);
      break;
    }

    sta::Path* end_path = sta_->vertexWorstSlackPath(endpoint_state.end, max_);
    const bool changed = repairPath(end_path,
                                    endpoint_state.end_slack,
                                    /*force_single_repair=*/false);
    if (!changed) {
      finishEndpointSearch(endpoint_state);
      break;
    }

    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();
    refreshEndpointSlacks(endpoint_state);
    const float curr_tns = sta_->totalNegativeSlack(max_);
    if (!advanceLastGaspProgress(endpoint_state, last_gasp_state, curr_tns)) {
      break;
    }

    if (resizer_.overMaxArea()) {
      acceptEndpointState(endpoint_state);
      break;
    }
    if (last_gasp_state.end_index == 1
        && endpoint_state.worst_vertex != nullptr) {
      endpoint_state.end = endpoint_state.worst_vertex;
      target_collector_->useWorstEndpoint(endpoint_state.end);
      committer_.setCurrentEndpoint(endpoint_state.end->pin());
    }

    ++endpoint_state.pass;
    if (reachedIterationLimit(last_gasp_state.opto_iteration,
                              config_.max_iterations)) {
      acceptEndpointState(endpoint_state);
      break;
    }
  }

  acceptEndpointState(endpoint_state);
}

bool SetupLastGaspPolicy::shouldStopLastGasp(
    const SetupLastGaspPolicy::LastGaspState& last_gasp_state) const
{
  return last_gasp_state.two_cons_terminations
         || reachedIterationLimit(last_gasp_state.opto_iteration,
                                  config_.max_iterations);
}

void SetupLastGaspPolicy::runLastGaspLoop(
    const SetupLegacyBase::ViolatingEnds& violating_ends,
    SetupLastGaspPolicy::LastGaspState& last_gasp_state)
{
  for (const auto& end_original_slack : violating_ends) {
    if (shouldStopLastGasp(last_gasp_state)) {
      break;
    }

    EndpointRepairState endpoint_state;
    if (!beginLastGaspEndpoint(
            end_original_slack, last_gasp_state, endpoint_state)) {
      break;
    }

    repairLastGaspEndpoint(endpoint_state, last_gasp_state);

    if (config_.verbose || last_gasp_state.opto_iteration == 1) {
      printProgress(
          last_gasp_state.opto_iteration, true, last_gasp_state.phase_marker);
    }
    if (shouldStopLastGasp(last_gasp_state)) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "LAST_GASP{} Phase: No TNS progress for two opto cycles, "
                 "exiting",
                 last_gasp_state.phase_marker);
      break;
    }
  }
}

}  // namespace rsz
