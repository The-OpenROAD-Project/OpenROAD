// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SetupLegacyPolicy.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "RepairTargetCollector.hh"
#include "VtSwapGenerator.hh"
#include "est/EstimateParasitics.h"
#include "rsz/Resizer.hh"
#include "sta/Fuzzy.hh"
#include "sta/GraphClass.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace rsz {

using std::pair;
using utl::RSZ;

namespace {
static constexpr int kDelayDigits = 3;

}  // namespace

void SetupLegacyPolicy::iterate()
{
  committer_.capturePrePhaseSlack();
  buildMainMoveSequence(/*log_sequence=*/false);

  MainRepairState main_state;
  main_state.opto_iteration = setup_context_.iteration;
  main_state.initial_tns = setup_context_.initial_tns;
  main_state.prev_tns = setup_context_.initial_tns;
  main_state.phase_marker = phaseMarkerForIndex(setup_context_.phase_index);

  ViolatingEnds violating_ends;
  if (initializeMainRepair(main_state, violating_ends)) {
    runMainRepairLoop(violating_ends, main_state);
  }
  committer_.printTrackerPhaseSummary(
      phaseSummaryTitle(), phaseEndpointProfilerTitle(), true);

  // Propagate accumulator deltas back to the sequencer.
  setup_context_.iteration = main_state.opto_iteration;
  setup_context_.violation_count = main_state.num_viols;
  setup_context_.initial_tns = main_state.initial_tns;
  setup_context_.previous_tns = main_state.prev_tns;
  markRunComplete(true);
}

bool SetupLegacyPolicy::initializeMainRepair(MainRepairState& main_state,
                                             ViolatingEnds& violating_ends)
{
  const float setup_slack_margin = config_.setup_slack_margin;
  violating_ends = collectViolatingEndpoints(setup_slack_margin);

  // prepareForPhasePipeline already rejected the zero-violation case, but the
  // collector can re-filter here if slack margin semantics differ.
  if (violating_ends.empty()) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "{}{} Phase: No violating endpoints, exiting",
               phaseName(),
               main_state.phase_marker);
    return false;
  }

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "{}{} Phase: {} violating endpoints found",
             phaseName(),
             main_state.phase_marker,
             violating_ends.size());

  main_state.max_end_count = std::max(
      static_cast<int>(violating_ends.size() * config_.repair_tns_end_percent),
      1);
  main_state.initial_tns = sta_->totalNegativeSlack(max_);
  main_state.prev_tns = main_state.initial_tns;
  main_state.num_viols = violating_ends.size();
  main_state.fix_rate_threshold = inc_fix_rate_threshold_;

  printProgress(main_state.opto_iteration, false, main_state.phase_marker);

  setup_context_.min_viol = -violating_ends.back().second;
  setup_context_.max_viol = -violating_ends.front().second;
  return true;
}

bool SetupLegacyPolicy::beginEndpointRepair(
    const pair<sta::Vertex*, sta::Slack>& end_original_slack,
    MainRepairState& main_state,
    EndpointRepairState& endpoint_state)
{
  if (!beginJournaledEndpointSearch(end_original_slack,
                                    main_state.max_end_count,
                                    main_state.end_index,
                                    endpoint_state)) {
    return false;
  }

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "{}{} Phase: Doing endpoint {} ({}/{}) WNS = {}, "
             "endpoint slack = {}, TNS = {}",
             phaseName(),
             main_state.phase_marker,
             endpoint_state.end->name(network_),
             main_state.end_index,
             main_state.max_end_count,
             delayAsString(endpoint_state.worst_slack, kDelayDigits, sta_),
             delayAsString(endpoint_state.end_slack, kDelayDigits, sta_),
             delayAsString(main_state.prev_tns, 1, sta_));

  endpoint_state.prev_end_slack = endpoint_state.end_slack;
  endpoint_state.prev_worst_slack = endpoint_state.worst_slack;
  return true;
}

void SetupLegacyPolicy::repairEndpoint(EndpointRepairState& endpoint_state,
                                       MainRepairState& main_state)
{
  const float setup_slack_margin = config_.setup_slack_margin;
  while (endpoint_state.pass <= config_.max_passes) {
    ++main_state.opto_iteration;
    if (config_.verbose || main_state.opto_iteration == 1) {
      printProgress(main_state.opto_iteration, false, main_state.phase_marker);
    }

    if (terminateProgress(main_state.opto_iteration,
                          main_state.initial_tns,
                          main_state.prev_tns,
                          main_state.fix_rate_threshold,
                          main_state.end_index,
                          main_state.max_end_count,
                          phaseName(),
                          main_state.phase_marker)) {
      recordTermination(main_state.prev_termination,
                        main_state.two_cons_terminations);
      debugPrint(
          logger_,
          RSZ,
          "repair_setup",
          2,
          "{}{} Phase: Restoring best slack; endpoint slack = {}, "
          "WNS = {}",
          phaseName(),
          main_state.phase_marker,
          delayAsString(endpoint_state.prev_end_slack, kDelayDigits, sta_),
          delayAsString(endpoint_state.prev_worst_slack, kDelayDigits, sta_));
      restoreEndpointState(endpoint_state);
      break;
    }
    if (main_state.opto_iteration % opto_small_interval_ == 0) {
      main_state.prev_termination = false;
    }

    // Exit early once the endpoint is clean or the move sequence stalls.
    if (sta::fuzzyGreaterEqual(endpoint_state.end_slack, setup_slack_margin)) {
      --main_state.num_viols;
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: Endpoint slack {} meets slack margin {}, "
                 "done",
                 phaseName(),
                 main_state.phase_marker,
                 delayAsString(endpoint_state.worst_slack, kDelayDigits, sta_),
                 delayAsString(setup_slack_margin, kDelayDigits, sta_));
      finishEndpointSearch(endpoint_state);
      break;
    }

    const bool log_change_summary = logger_->debugCheck(RSZ, "repair_setup", 3);
    float prev_tns_local = 0.0;
    if (log_change_summary) {
      prev_tns_local = sta_->totalNegativeSlack(max_);
    }

    sta::Path* end_path = sta_->vertexWorstSlackPath(endpoint_state.end, max_);
    const bool changed = repairPath(
        end_path, endpoint_state.end_slack, endpoint_state.force_single_repair);
    if (!changed) {
      if (endpoint_state.pass != 1) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "{}{} Phase: No change after {} decreasing slack "
                   "passes.",
                   phaseName(),
                   main_state.phase_marker,
                   endpoint_state.decreasing_slack_passes);
        debugPrint(
            logger_,
            RSZ,
            "repair_setup",
            2,
            "{}{} Phase: Restoring best slack; endpoint slack = "
            "{}, WNS = {}",
            phaseName(),
            main_state.phase_marker,
            delayAsString(endpoint_state.prev_end_slack, kDelayDigits, sta_),
            delayAsString(endpoint_state.prev_worst_slack, kDelayDigits, sta_));
      }
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: No change possible for endpoint {} ",
                 phaseName(),
                 main_state.phase_marker,
                 endpoint_state.end->name(network_));
      finishEndpointSearch(endpoint_state);
      break;
    }

    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();
    refreshEndpointSlacks(endpoint_state);

    // Promote only the passes that improve the tracked objective.
    const bool better = pathImproved(main_state.end_index,
                                     endpoint_state.end_slack,
                                     endpoint_state.worst_slack,
                                     endpoint_state.prev_end_slack,
                                     endpoint_state.prev_worst_slack);
    if (log_change_summary) {
      const float new_tns = sta_->totalNegativeSlack(max_);
      debugPrint(
          logger_,
          RSZ,
          "repair_setup",
          3,
          "{}{} Phase: {} after changes: WNS ({} -> {}) TNS "
          "({} -> {}) Endpoint slack ({} -> {})",
          phaseName(),
          main_state.phase_marker,
          better ? "Improved" : "Worsened",
          delayAsString(endpoint_state.prev_worst_slack, kDelayDigits, sta_),
          delayAsString(endpoint_state.worst_slack, kDelayDigits, sta_),
          delayAsString(prev_tns_local, 1, sta_),
          delayAsString(new_tns, 1, sta_),
          delayAsString(endpoint_state.prev_end_slack, kDelayDigits, sta_),
          delayAsString(endpoint_state.end_slack, kDelayDigits, sta_));
    }
    if (better) {
      if (sta::fuzzyGreaterEqual(endpoint_state.end_slack,
                                 setup_slack_margin)) {
        --main_state.num_viols;
      }
      endpoint_state.prev_end_slack = endpoint_state.end_slack;
      endpoint_state.prev_worst_slack = endpoint_state.worst_slack;
      endpoint_state.decreasing_slack_passes = 0;
      saveImprovedCheckpoint(endpoint_state);
    } else {
      endpoint_state.force_single_repair = true;
      ++endpoint_state.decreasing_slack_passes;
      if (endpoint_state.decreasing_slack_passes
          > decreasing_slack_max_passes_) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "{}{} Phase: Endpoint {} stuck after {} "
                   "non-improving passes (limit {})",
                   phaseName(),
                   main_state.phase_marker,
                   network_->pathName(endpoint_state.end->pin()),
                   endpoint_state.decreasing_slack_passes,
                   decreasing_slack_max_passes_);
        debugPrint(
            logger_,
            RSZ,
            "repair_setup",
            2,
            "{}{} Phase: Restoring best slack; endpoint slack = "
            "{}, WNS = {}",
            phaseName(),
            main_state.phase_marker,
            delayAsString(endpoint_state.prev_end_slack, kDelayDigits, sta_),
            delayAsString(endpoint_state.prev_worst_slack, kDelayDigits, sta_));
        restoreEndpointState(endpoint_state);
        break;
      }
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 3,
                 "{}{} Phase: Allowing decreasing slack for {}/{} passes",
                 phaseName(),
                 main_state.phase_marker,
                 endpoint_state.decreasing_slack_passes,
                 decreasing_slack_max_passes_);
    }

    // Global guards keep the current endpoint state, then stop searching.
    if (resizer_.overMaxArea()) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: Over max area, exiting",
                 phaseName(),
                 main_state.phase_marker);
      acceptEndpointState(endpoint_state);
      break;
    }
    if (main_state.end_index == 1) {
      endpoint_state.end = endpoint_state.worst_vertex;
      if (endpoint_state.end != nullptr) {
        target_collector_->useWorstEndpoint(endpoint_state.end);
        committer_.setCurrentEndpoint(endpoint_state.end->pin());
      }
    }

    ++endpoint_state.pass;
    if (config_.max_iterations > 0
        && main_state.opto_iteration >= config_.max_iterations) {
      acceptEndpointState(endpoint_state);
      break;
    }
  }

  acceptEndpointState(endpoint_state);
}

void SetupLegacyPolicy::runMainRepairLoop(const ViolatingEnds& violating_ends,
                                          MainRepairState& main_state)
{
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      10,
      fmt::format(
          "{}{} Phase Time: {{}}", phaseName(), main_state.phase_marker));
  for (const pair<sta::Vertex*, sta::Slack>& end_original_slack :
       violating_ends) {
    EndpointRepairState endpoint_state;
    if (!beginEndpointRepair(end_original_slack, main_state, endpoint_state)) {
      break;
    }

    repairEndpoint(endpoint_state, main_state);

    if (config_.verbose || main_state.opto_iteration == 1) {
      printProgress(main_state.opto_iteration, true, main_state.phase_marker);
    }
    if (main_state.two_cons_terminations) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: Exiting due to no TNS progress for two "
                 "opto cycles",
                 phaseName(),
                 main_state.phase_marker);
      break;
    }
  }

  printProgress(main_state.opto_iteration, true, main_state.phase_marker);
  if (logger_->debugCheck(RSZ, "repair_setup", 1)) {
    sta::Slack final_wns;
    sta::Vertex* final_worst;
    sta_->worstSlack(max_, final_wns, final_worst);
    const float final_tns = sta_->totalNegativeSlack(max_);
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "{}{} Phase complete. WNS: {}, TNS: {}",
               phaseName(),
               main_state.phase_marker,
               delayAsString(final_wns, kDelayDigits, sta_),
               delayAsString(final_tns, 1, sta_));
  }
}

bool SetupLegacyPolicy::pathImproved(const int end_index,
                                     const sta::Slack end_slack,
                                     const sta::Slack worst_slack,
                                     const sta::Slack prev_end_slack,
                                     const sta::Slack prev_worst_slack) const
{
  return sta::fuzzyGreater(worst_slack, prev_worst_slack)
         || (end_index != 1 && sta::fuzzyEqual(worst_slack, prev_worst_slack)
             && sta::fuzzyGreater(end_slack, prev_end_slack));
}

}  // namespace rsz
