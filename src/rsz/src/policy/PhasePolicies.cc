// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "PhasePolicies.hh"

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
#include "SetupLegacyBase.hh"
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
static constexpr size_t kMaxCritEndpoints = 100;
static constexpr int kMaxCritInstancesPerEndpoint = 50;

RepairSetupParams makeRepairSetupParams(const OptimizerRunConfig& config,
                                        const float setup_slack_margin)
{
  return RepairSetupParams{.setup_slack_margin = setup_slack_margin,
                           .verbose = config.verbose,
                           .skip_pin_swap = config.skip_pin_swap,
                           .skip_gate_cloning = config.skip_gate_cloning,
                           .skip_size_down = config.skip_size_down,
                           .skip_buffering = config.skip_buffering,
                           .skip_buffer_removal = config.skip_buffer_removal,
                           .skip_vt_swap = config.skip_vt_swap};
}

}  // namespace

void SetupLegacyPolicy::iterate()
{
  PhaseRunContext& ctx = *run_ctx_;
  OptimizerProgress& progress = ctx.progress;
  committer_.capturePrePhaseSlack();
  buildMainMoveSequence(/*log_sequence=*/false);

  MainRepairState main_state;
  main_state.opto_iteration = progress.iteration;
  main_state.initial_tns = progress.initial_tns;
  main_state.prev_tns = progress.initial_tns;
  main_state.phase_marker = phaseMarkerForIndex(ctx.phase_index);

  ViolatingEnds violating_ends;
  if (initializeMainRepair(config_.setup_slack_margin,
                           config_.repair_tns_end_percent,
                           main_state,
                           violating_ends)) {
    runMainRepairLoop(violating_ends,
                      config_.setup_slack_margin,
                      config_.max_passes,
                      config_.max_iterations,
                      config_.verbose,
                      main_state);
  }
  committer_.printTrackerPhaseSummary(
      "LEGACY Phase Summary", "LEGACY Phase Endpoint Profiler", true);

  // Propagate accumulator deltas back to the sequencer.
  progress.iteration = main_state.opto_iteration;
  progress.violation_count = main_state.num_viols;
  progress.initial_tns = main_state.initial_tns;
  progress.previous_tns = main_state.prev_tns;
  markRunComplete(true);
}

void SetupWnsPolicy::iterate()
{
  buildMainMoveSequence(/*log_sequence=*/false);
  repairSetupWns(config_.setup_slack_margin,
                 config_.max_passes,
                 config_.max_repairs_per_pass,
                 config_.verbose,
                 /*use_cone_collection=*/use_cone_,
                 rsz::ViolatorSortType::SORT_BY_LOAD_DELAY,
                 *run_ctx_);
  if (use_cone_) {
    committer_.printTrackerPhaseSummary(
        "WNS_CONE Phase Summary", "WNS_CONE Phase Endpoint Profiler", true);
  } else {
    committer_.printTrackerPhaseSummary(
        "WNS_PATH Phase Summary", "WNS_PATH Phase Endpoint Profiler", true);
  }
  markRunComplete(true);
}

void SetupTnsPolicy::iterate()
{
  buildMainMoveSequence(/*log_sequence=*/false);
  repairSetupTns(config_.setup_slack_margin,
                 config_.max_passes,
                 config_.max_repairs_per_pass,
                 config_.verbose,
                 rsz::ViolatorSortType::SORT_BY_LOAD_DELAY,
                 *run_ctx_);
  committer_.printTrackerPhaseSummary(
      "TNS Phase Summary", "TNS Phase Endpoint Profiler", true);
  markRunComplete(true);
}

void SetupDirectionalPolicy::iterate()
{
  buildMainMoveSequence(/*log_sequence=*/false);
  repairSetupDirectional(use_starts_,
                         config_.setup_slack_margin,
                         config_.max_passes,
                         config_.verbose,
                         *run_ctx_);
  if (use_starts_) {
    committer_.printTrackerPhaseSummary(
        "STARTPOINT_FANOUT Phase Summary",
        "STARTPOINT_FANOUT Phase Startpoint Profiler",
        true);
  } else {
    committer_.printTrackerPhaseSummary(
        "ENDPOINT_FANIN Phase Summary",
        "ENDPOINT_FANIN Phase Endpoint Profiler",
        true);
  }
  markRunComplete(true);
}

void SetupLastGaspPolicy::iterate()
{
  if (config_.skip_last_gasp) {
    markRunComplete(false);
    return;
  }

  committer_.capturePrePhaseSlack();
  RepairSetupParams params
      = makeRepairSetupParams(config_, config_.setup_slack_margin);
  OptimizerProgress& progress = run_ctx_->progress;
  int& num_viols = progress.violation_count;
  const int opto_iteration = progress.iteration;
  const float initial_tns = progress.initial_tns;
  const char phase_marker = phaseMarkerForIndex(run_ctx_->phase_index);
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
    if (!initializeLastGaspRepair(params,
                                  opto_iteration,
                                  initial_tns,
                                  last_gasp_state,
                                  violating_ends)) {
      num_viols = last_gasp_state.num_viols;
    } else {
      runLastGaspLoop(
          violating_ends, params, config_.max_iterations, last_gasp_state);
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

void SetupCritVtSwapPolicy::iterate()
{
  int& num_viols = run_ctx_->progress.violation_count;
  // Critical VT swap runs as a separate phase because it is endpoint-agnostic.
  if (config_.skip_crit_vt_swap || config_.skip_vt_swap || !hasVtSwapCells()) {
    markRunComplete(true);
    return;
  }
  committer_.capturePrePhaseSlack();
  RepairSetupParams params
      = makeRepairSetupParams(config_, config_.setup_slack_margin);
  if (swapVTCritCells(params, num_viols)) {
    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();
  }
  committer_.printTrackerPhaseSummary("VT Swap Phase Summary", nullptr, false);
  markRunComplete(true);
}

bool SetupLegacyPolicy::initializeMainRepair(
    const float setup_slack_margin,
    const double repair_tns_end_percent,
    MainRepairState& main_state,
    ViolatingEnds& violating_ends)
{
  violating_ends = collectViolatingEndpoints(setup_slack_margin);

  // prepareForPhasePipeline already rejected the zero-violation case, but the
  // collector can re-filter here if slack margin semantics differ.
  if (violating_ends.empty()) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "LEGACY{} Phase: No violating endpoints, exiting",
               main_state.phase_marker);
    return false;
  }

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "LEGACY{} Phase: {} violating endpoints found",
             main_state.phase_marker,
             violating_ends.size());

  main_state.max_end_count = std::max(
      static_cast<int>(violating_ends.size() * repair_tns_end_percent), 1);
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
             "LEGACY{} Phase: Doing endpoint {} ({}/{}) WNS = {}, "
             "endpoint slack = {}, TNS = {}",
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
                                       MainRepairState& main_state,
                                       const float setup_slack_margin,
                                       const int max_passes,
                                       const int max_iterations,
                                       const bool verbose)
{
  while (endpoint_state.pass <= max_passes) {
    ++main_state.opto_iteration;
    if (verbose || main_state.opto_iteration == 1) {
      printProgress(main_state.opto_iteration, false, main_state.phase_marker);
    }

    if (terminateProgress(main_state.opto_iteration,
                          main_state.initial_tns,
                          main_state.prev_tns,
                          main_state.fix_rate_threshold,
                          main_state.end_index,
                          main_state.max_end_count,
                          "LEGACY",
                          main_state.phase_marker)) {
      recordTermination(main_state.prev_termination,
                        main_state.two_cons_terminations);
      debugPrint(
          logger_,
          RSZ,
          "repair_setup",
          2,
          "LEGACY{} Phase: Restoring best slack; endpoint slack = {}, "
          "WNS = {}",
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
                 "LEGACY{} Phase: Endpoint slack {} meets slack margin {}, "
                 "done",
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
    const bool changed = repairPath(end_path, endpoint_state.end_slack);
    if (!changed) {
      if (endpoint_state.pass != 1) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "LEGACY{} Phase: No change after {} decreasing slack "
                   "passes.",
                   main_state.phase_marker,
                   endpoint_state.decreasing_slack_passes);
        debugPrint(
            logger_,
            RSZ,
            "repair_setup",
            2,
            "LEGACY{} Phase: Restoring best slack; endpoint slack = "
            "{}, WNS = {}",
            main_state.phase_marker,
            delayAsString(endpoint_state.prev_end_slack, kDelayDigits, sta_),
            delayAsString(endpoint_state.prev_worst_slack, kDelayDigits, sta_));
      }
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "LEGACY{} Phase: No change possible for endpoint {} ",
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
          "LEGACY{} Phase: {} after changes: WNS ({} -> {}) TNS "
          "({} -> {}) Endpoint slack ({} -> {})",
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
      saveImprovedCheckpoint(endpoint_state, max_passes);
    } else {
      setup_context_.fallback = true;
      ++endpoint_state.decreasing_slack_passes;
      if (endpoint_state.decreasing_slack_passes
          > decreasing_slack_max_passes_) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   2,
                   "LEGACY{} Phase: Endpoint {} stuck after {} "
                   "non-improving passes (limit {})",
                   main_state.phase_marker,
                   network_->pathName(endpoint_state.end->pin()),
                   endpoint_state.decreasing_slack_passes,
                   decreasing_slack_max_passes_);
        debugPrint(
            logger_,
            RSZ,
            "repair_setup",
            2,
            "LEGACY{} Phase: Restoring best slack; endpoint slack = "
            "{}, WNS = {}",
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
                 "LEGACY{} Phase: Allowing decreasing slack for {}/{} passes",
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
                 "LEGACY{} Phase: Over max area, exiting",
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
    if (max_iterations > 0 && main_state.opto_iteration >= max_iterations) {
      acceptEndpointState(endpoint_state);
      break;
    }
  }

  acceptEndpointState(endpoint_state);
}

void SetupLegacyPolicy::runMainRepairLoop(const ViolatingEnds& violating_ends,
                                          const float setup_slack_margin,
                                          const int max_passes,
                                          const int max_iterations,
                                          const bool verbose,
                                          MainRepairState& main_state)
{
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      10,
      fmt::format("LEGACY{} Phase Time: {{}}", main_state.phase_marker));
  for (const pair<sta::Vertex*, sta::Slack>& end_original_slack :
       violating_ends) {
    EndpointRepairState endpoint_state;
    if (!beginEndpointRepair(end_original_slack, main_state, endpoint_state)) {
      break;
    }

    repairEndpoint(endpoint_state,
                   main_state,
                   setup_slack_margin,
                   max_passes,
                   max_iterations,
                   verbose);

    if (verbose || main_state.opto_iteration == 1) {
      printProgress(main_state.opto_iteration, true, main_state.phase_marker);
    }
    if (main_state.two_cons_terminations) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "LEGACY{} Phase: Exiting due to no TNS progress for two "
                 "opto cycles",
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
               "LEGACY{} Phase complete. WNS: {}, TNS: {}",
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

void SetupWnsPolicy::repairSetupWns(const float setup_slack_margin,
                                    const int max_passes_per_endpoint,
                                    const int max_repairs_per_pass,
                                    const bool verbose,
                                    const bool use_cone_collection,
                                    const rsz::ViolatorSortType sort_type,
                                    PhaseRunContext& ctx)
{
  OptimizerProgress& progress = ctx.progress;
  int& opto_iteration = progress.iteration;
  const float initial_tns = progress.initial_tns;
  float& prev_tns = progress.previous_tns;
  const char phase_marker = phaseMarkerForIndex(ctx.phase_index);
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      10,
      fmt::format("WNS{} Phase Time: {{}}", phase_marker));
  constexpr int max_no_progress = 4;
  committer_.capturePrePhaseSlack();
  setup_context_.overall_no_progress_count = 0;
  rejected_pin_moves_current_endpoint_.clear();
  target_collector_->resetEndpointPasses();

  std::map<const sta::Pin*, int> endpoint_pass_counts;
  std::set<const sta::Pin*> endpoints_visited_this_round;
  std::set<const sta::Pin*> decreasing_slack_endpoints;
  std::map<const sta::Pin*, int> decreasing_slack_counts;
  std::map<const sta::Pin*, int> endpoint_pass_limits;
  bool any_improvement_this_round = false;
  int total_decreasing_slack_passes = 0;
  float fix_rate_threshold = inc_fix_rate_threshold_;
  sta::Slack target_end_slack = 0.0;
  sta::Slack target_worst_slack = 0.0;
  sta::Slack target_tns_local = 0.0;

  sta::Slack worst_slack = 0.0;
  sta::Vertex* worst_endpoint = nullptr;
  sta_->worstSlack(max_, worst_slack, worst_endpoint);
  if (worst_endpoint == nullptr) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "WNS{} Phase: No worst endpoint found",
               phase_marker);
    return;
  }

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "WNS{} Phase: Focusing on worst slack path{}...",
             phase_marker,
             use_cone_collection ? " with fanin cone collection" : "");
  printProgress(opto_iteration, false, phase_marker);

  sta::Vertex* current_endpoint = worst_endpoint;
  target_collector_->useWorstEndpoint(worst_endpoint);
  target_collector_->markEndpointVisitedInWns(worst_endpoint->pin());
  endpoint_pass_limits[worst_endpoint->pin()]
      = initial_decreasing_slack_max_passes_;
  decreasing_slack_counts[worst_endpoint->pin()] = 0;
  committer_.setCurrentEndpoint(worst_endpoint->pin());

  while (true) {
    sta_->worstSlack(max_, worst_slack, worst_endpoint);
    if (worst_endpoint == nullptr) {
      break;
    }

    const sta::Pin* worst_pin = worst_endpoint->pin();
    if (endpoints_visited_this_round.contains(worst_pin)) {
      if (!any_improvement_this_round) {
        break;
      }
      endpoints_visited_this_round.clear();
      any_improvement_this_round = false;
      continue;
    }

    const sta::Pin* current_pin = current_endpoint->pin();
    bool should_switch = false;
    if (endpoints_visited_this_round.contains(current_pin)) {
      should_switch = true;
    } else if (current_endpoint != worst_endpoint) {
      int current_limit = endpoint_pass_limits[current_pin];
      if (current_limit == 0) {
        current_limit = initial_decreasing_slack_max_passes_;
        endpoint_pass_limits[current_pin] = current_limit;
      }
      if (decreasing_slack_counts[current_pin] >= current_limit) {
        should_switch = true;
      }
    }

    if (should_switch && current_endpoint != worst_endpoint) {
      current_endpoint = worst_endpoint;
      target_collector_->useWorstEndpoint(worst_endpoint);
      target_collector_->markEndpointVisitedInWns(worst_pin);
      rejected_pin_moves_current_endpoint_.clear();
      committer_.setCurrentEndpoint(worst_pin);
      if (!endpoint_pass_limits.contains(worst_pin)) {
        endpoint_pass_limits[worst_pin] = initial_decreasing_slack_max_passes_;
      }
      if (!decreasing_slack_counts.contains(worst_pin)) {
        decreasing_slack_counts[worst_pin] = 0;
      }
    }

    const sta::Pin* endpoint_pin = current_endpoint->pin();
    if (!endpoint_pass_counts.contains(endpoint_pin)) {
      endpoint_pass_counts[endpoint_pin] = 0;
    }
    if (static_cast<int>(endpoint_pass_counts.size())
        > setup_context_.max_end_repairs) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "WNS{} Phase: Hit maximum endpoint repairs of {}",
                 phase_marker,
                 setup_context_.max_end_repairs);
      break;
    }
    if (endpoint_pass_counts[endpoint_pin] >= max_passes_per_endpoint) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "WNS{} Phase: WNS endpoint {} exceeded pass limit {}, "
                 "exiting",
                 phase_marker,
                 network_->pathName(endpoint_pin),
                 max_passes_per_endpoint);
      break;
    }
    if (sta::fuzzyGreaterEqual(worst_slack, setup_slack_margin)) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "WNS{} Phase: WNS {} meets slack margin {}, done",
                 phase_marker,
                 delayAsString(worst_slack, kDelayDigits, sta_),
                 delayAsString(setup_slack_margin, kDelayDigits, sta_));
      printProgress(opto_iteration, true, phase_marker);
      break;
    }
    if (terminateProgress(opto_iteration,
                          initial_tns,
                          prev_tns,
                          fix_rate_threshold,
                          0,
                          1,
                          use_cone_collection ? "WNS_CONE" : "WNS_PATH",
                          phase_marker)) {
      setup_context_.overall_no_progress_count++;
      if (setup_context_.overall_no_progress_count >= max_no_progress) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "WNS{} Phase: No TNS progress for {} cycles, exiting",
                   phase_marker,
                   setup_context_.overall_no_progress_count);
        break;
      }
    }
    if (opto_iteration % opto_small_interval_ == 0) {
      setup_context_.overall_no_progress_count = 0;
    }

    sta::Slack prev_end_slack = 0.0;
    sta::Slack prev_worst_slack = 0.0;
    sta::Slack prev_tns_local = 0.0;
    if (total_decreasing_slack_passes == 0) {
      prev_end_slack = target_collector_->getCurrentEndpointSlack();
      prev_worst_slack = worst_slack;
      prev_tns_local = sta_->totalNegativeSlack(max_);
      setup_context_.fallback = false;
      committer_.beginJournal();
    } else {
      prev_end_slack = target_end_slack;
      prev_worst_slack = target_worst_slack;
      prev_tns_local = target_tns_local;
    }

    opto_iteration++;
    if (verbose || opto_iteration % print_interval_ == 0) {
      printProgress(opto_iteration, false, phase_marker);
    }

    std::vector<const sta::Pin*> viol_pins;
    if (use_cone_collection) {
      viol_pins = target_collector_->collectViolatorsByConeTraversal(
          current_endpoint, sort_type);
    } else {
      viol_pins = target_collector_->collectViolators(1, -1, sort_type);
    }
    sta::Path* focus_path = sta_->vertexWorstSlackPath(current_endpoint, max_);

    const int saved_max_repairs = setup_context_.max_repairs_per_pass;
    if (use_cone_collection) {
      setup_context_.max_repairs_per_pass = viol_pins.size();
    } else {
      setup_context_.max_repairs_per_pass = max_repairs_per_pass;
    }

    std::vector<std::pair<const sta::Pin*, MoveType>> chosen_moves;
    const bool changed = repairPins(viol_pins,
                                    focus_path,
                                    &rejected_pin_moves_current_endpoint_,
                                    &chosen_moves);
    setup_context_.max_repairs_per_pass = saved_max_repairs;

    if (!changed) {
      if (total_decreasing_slack_passes > 0) {
        total_decreasing_slack_passes = 0;
        for (const sta::Pin* decreasing_pin : decreasing_slack_endpoints) {
          decreasing_slack_counts[decreasing_pin] = 0;
        }
        decreasing_slack_endpoints.clear();
        committer_.restoreJournal();
      } else {
        committer_.commitJournal();
      }
      endpoint_pass_counts[endpoint_pin]++;
      endpoints_visited_this_round.insert(endpoint_pin);
      continue;
    }

    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();

    const sta::Slack end_slack = target_collector_->getCurrentEndpointSlack();
    sta::Slack new_wns = 0.0;
    sta::Vertex* new_worst_vertex = nullptr;
    sta_->worstSlack(max_, new_wns, new_worst_vertex);
    const float new_tns = sta_->totalNegativeSlack(max_);
    const bool wns_improved = sta::fuzzyGreater(new_wns, prev_worst_slack);
    const bool wns_same = sta::fuzzyEqual(new_wns, prev_worst_slack);
    const bool endpoint_improved = sta::fuzzyGreater(end_slack, prev_end_slack);
    const bool tns_improved = sta::fuzzyGreater(new_tns, prev_tns_local);
    const bool better
        = wns_improved || (wns_same && (endpoint_improved || tns_improved));

    endpoint_pass_counts[endpoint_pin]++;
    if (better) {
      any_improvement_this_round = true;
      total_decreasing_slack_passes = 0;
      for (const sta::Pin* decreasing_pin : decreasing_slack_endpoints) {
        decreasing_slack_counts[decreasing_pin] = 0;
      }
      decreasing_slack_endpoints.clear();
      int successful_passes = endpoint_pass_counts[endpoint_pin]
                              - decreasing_slack_counts[endpoint_pin];
      int current_limit = endpoint_pass_limits[endpoint_pin];
      if (current_limit == 0) {
        current_limit = initial_decreasing_slack_max_passes_;
      }
      constexpr int max_adaptive_limit = 25;
      if (endpoint_pass_counts[endpoint_pin] >= current_limit
          && successful_passes > current_limit / 2
          && current_limit < max_adaptive_limit) {
        endpoint_pass_limits[endpoint_pin] += pass_limit_increment_;
      }
      committer_.commitJournal();
    } else {
      setup_context_.fallback = true;
      if (total_decreasing_slack_passes == 0) {
        target_end_slack = prev_end_slack;
        target_worst_slack = prev_worst_slack;
        target_tns_local = prev_tns_local;
      }
      total_decreasing_slack_passes++;
      decreasing_slack_endpoints.insert(endpoint_pin);
      decreasing_slack_counts[endpoint_pin]++;

      int current_limit = endpoint_pass_limits[endpoint_pin];
      if (current_limit == 0) {
        current_limit = initial_decreasing_slack_max_passes_;
        endpoint_pass_limits[endpoint_pin] = current_limit;
      }
      if (decreasing_slack_counts[endpoint_pin] > current_limit) {
        total_decreasing_slack_passes = 0;
        for (const sta::Pin* decreasing_pin : decreasing_slack_endpoints) {
          decreasing_slack_counts[decreasing_pin] = 0;
        }
        decreasing_slack_endpoints.clear();
        endpoints_visited_this_round.insert(endpoint_pin);
        committer_.restoreJournal();
        for (const auto& [chosen_pin, chosen_type] : chosen_moves) {
          rejected_pin_moves_current_endpoint_[chosen_pin].insert(chosen_type);
        }
      }
    }

    if (resizer_.overMaxArea()) {
      break;
    }
  }

  printProgress(opto_iteration, true, phase_marker);
  if (logger_->debugCheck(RSZ, "repair_setup", 1)) {
    sta::Slack final_wns;
    sta::Vertex* final_worst;
    sta_->worstSlack(max_, final_wns, final_worst);
    const float final_tns = sta_->totalNegativeSlack(max_);
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "WNS{} Phase complete. WNS: {}, TNS: {}",
               phase_marker,
               delayAsString(final_wns, kDelayDigits, sta_),
               delayAsString(final_tns, 1, sta_));
  }
}

void SetupTnsPolicy::repairSetupTns(const float setup_slack_margin,
                                    const int max_passes_per_endpoint,
                                    const int max_repairs_per_pass,
                                    const bool verbose,
                                    const rsz::ViolatorSortType sort_type,
                                    PhaseRunContext& ctx)
{
  int& opto_iteration = ctx.progress.iteration;
  const char phase_marker = phaseMarkerForIndex(ctx.phase_index);
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      10,
      fmt::format("TNS{} Phase Time: {{}}", phase_marker));
  committer_.capturePrePhaseSlack();
  setup_context_.overall_no_progress_count = 0;
  target_collector_->resetEndpointPasses();
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "TNS{} Phase: Focusing on all violating endpoints...",
             phase_marker);
  printProgress(opto_iteration, false, phase_marker);

  sta::Slack prev_global_wns = 0.0;
  sta::Vertex* initial_tns_phase_worst = nullptr;
  sta_->worstSlack(max_, prev_global_wns, initial_tns_phase_worst);

  target_collector_->collectViolatingEndpoints();
  const int max_endpoint_count = target_collector_->getMaxEndpointCount();
  if (max_endpoint_count == 0) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "TNS{} Phase: No violating endpoints, exiting",
               phase_marker);
    return;
  }

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "TNS{} Phase: Processing {} violating endpoints (skipping worst "
             "endpoint)",
             phase_marker,
             max_endpoint_count);

  std::map<const sta::Pin*, int> endpoint_pass_limits;
  for (int endpoint_index = 1; endpoint_index < max_endpoint_count;
       endpoint_index++) {
    if (endpoint_index >= setup_context_.max_end_repairs) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "TNS{} Phase: Hit maximum endpoint repairs of {}",
                 phase_marker,
                 setup_context_.max_end_repairs);
      break;
    }

    target_collector_->setToEndpoint(endpoint_index);
    sta::Vertex* endpoint = target_collector_->getCurrentEndpoint();
    if (endpoint == nullptr) {
      continue;
    }

    const sta::Pin* endpoint_pin = endpoint->pin();
    sta::Slack endpoint_slack = sta_->slack(endpoint, max_);
    if (sta::fuzzyGreaterEqual(endpoint_slack, setup_slack_margin)) {
      continue;
    }
    if (target_collector_->wasEndpointVisitedInWns(endpoint_pin)) {
      continue;
    }

    committer_.setCurrentEndpoint(endpoint_pin);
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "TNS{} Phase: Working on endpoint {} (index {}), slack = {}",
               phase_marker,
               network_->pathName(endpoint_pin),
               endpoint_index,
               delayAsString(endpoint_slack, kDelayDigits, sta_));
    rejected_pin_moves_current_endpoint_.clear();
    if (!endpoint_pass_limits.contains(endpoint_pin)) {
      endpoint_pass_limits[endpoint_pin] = initial_decreasing_slack_max_passes_;
    }
    int current_limit = endpoint_pass_limits[endpoint_pin];
    int pass = 1;
    int decreasing_slack_passes = 0;
    int successful_passes = 0;
    sta::Slack prev_endpoint_slack = endpoint_slack;

    committer_.beginJournal();
    bool journal_open = true;
    while (pass <= max_passes_per_endpoint) {
      std::vector<const sta::Pin*> viol_pins
          = target_collector_->collectViolators(1, -1, sort_type);
      sta::Path* focus_path = sta_->vertexWorstSlackPath(endpoint, max_);
      const int saved_max_repairs = setup_context_.max_repairs_per_pass;
      setup_context_.max_repairs_per_pass = max_repairs_per_pass;
      std::vector<std::pair<const sta::Pin*, MoveType>> chosen_moves;
      const bool changed = repairPins(viol_pins,
                                      focus_path,
                                      &rejected_pin_moves_current_endpoint_,
                                      &chosen_moves);
      setup_context_.max_repairs_per_pass = saved_max_repairs;

      if (!changed) {
        if (decreasing_slack_passes > current_limit) {
          committer_.restoreJournal();
        } else {
          committer_.commitJournal();
        }
        journal_open = false;
        break;
      }

      estimate_parasitics_->updateParasitics();
      sta_->findRequireds();

      endpoint_slack = sta_->slack(endpoint, max_);
      sta::Slack global_wns = 0.0;
      sta::Vertex* global_wns_vertex = nullptr;
      sta_->worstSlack(max_, global_wns, global_wns_vertex);
      const bool wns_improved = sta::fuzzyGreater(global_wns, prev_global_wns);
      const bool wns_same = sta::fuzzyEqual(global_wns, prev_global_wns);
      const bool endpoint_improved
          = sta::fuzzyGreater(endpoint_slack, prev_endpoint_slack);
      const bool better = wns_improved || (wns_same && endpoint_improved);

      if (better) {
        prev_global_wns = global_wns;
        if (sta::fuzzyGreaterEqual(endpoint_slack, setup_slack_margin)) {
          committer_.commitJournal();
          journal_open = false;
          break;
        }
        prev_endpoint_slack = endpoint_slack;
        decreasing_slack_passes = 0;
        successful_passes++;
        constexpr int max_adaptive_limit = 25;
        if (pass >= current_limit && successful_passes > current_limit / 2
            && current_limit < max_adaptive_limit) {
          endpoint_pass_limits[endpoint_pin] += pass_limit_increment_;
          current_limit = endpoint_pass_limits[endpoint_pin];
        }
        committer_.commitJournal();
        committer_.beginJournal();
      } else {
        setup_context_.fallback = true;
        decreasing_slack_passes++;
        if (decreasing_slack_passes > current_limit) {
          committer_.restoreJournal();
          journal_open = false;
          for (const auto& [chosen_pin, chosen_type] : chosen_moves) {
            rejected_pin_moves_current_endpoint_[chosen_pin].insert(
                chosen_type);
          }
          break;
        }
      }

      if (resizer_.overMaxArea()) {
        printProgress(opto_iteration, true, phase_marker);
        return;
      }

      pass++;
    }
    if (journal_open) {
      committer_.commitJournal();
    }

    opto_iteration++;
    if (verbose || endpoint_index == 1
        || opto_iteration % print_interval_ == 0) {
      printProgress(opto_iteration, false, phase_marker);
    }
  }

  printProgress(opto_iteration, true, phase_marker);
  if (logger_->debugCheck(RSZ, "repair_setup", 1)) {
    sta::Slack final_wns;
    sta::Vertex* final_worst;
    sta_->worstSlack(max_, final_wns, final_worst);
    const float final_tns = sta_->totalNegativeSlack(max_);
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "TNS{} Phase complete. WNS: {}, TNS: {}",
               phase_marker,
               delayAsString(final_wns, kDelayDigits, sta_),
               delayAsString(final_tns, 1, sta_));
  }
}

void SetupDirectionalPolicy::repairSetupDirectional(
    const bool use_startpoints,
    const float setup_slack_margin,
    const int max_passes_per_point,
    const bool verbose,
    PhaseRunContext& ctx)
{
  int& opto_iteration = ctx.progress.iteration;
  const char phase_marker = phaseMarkerForIndex(ctx.phase_index);
  const char* phase_name
      = use_startpoints ? "STARTPOINT_FANOUT" : "ENDPOINT_FANIN";
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      10,
      fmt::format("{}{} Phase Time: {{}}", phase_name, phase_marker));
  const char* point_type = use_startpoints ? "startpoint" : "endpoint";
  const char* point_type_cap = use_startpoints ? "Startpoint" : "Endpoint";
  committer_.capturePrePhaseSlack();
  const float margin_percentages[] = {0.20f, 0.50f, 1.00f};
  const sta::Slack diminishing_returns_threshold(1e-11f);

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "{}{} Phase: Iterative threshold refinement...",
             phase_name,
             phase_marker);
  target_collector_->collectViolatingPoints(use_startpoints);
  const int max_point_count
      = target_collector_->getMaxPointCount(use_startpoints);
  printProgress(opto_iteration, false, phase_marker, use_startpoints);
  if (max_point_count == 0) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "{}{} Phase: No violating {}s, exiting",
               phase_name,
               phase_marker,
               point_type);
    return;
  }

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "{}{} Phase: Processing {} violating {}s",
             phase_name,
             phase_marker,
             max_point_count,
             point_type);
  int points_processed = 0;

  for (int point_index = 0; point_index < max_point_count; point_index++) {
    if (point_index >= setup_context_.max_end_repairs) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: Hit maximum point repairs of {}",
                 phase_name,
                 phase_marker,
                 setup_context_.max_end_repairs);
      break;
    }

    target_collector_->setToPoint(point_index, use_startpoints);
    sta::Vertex* point = target_collector_->getCurrentPoint(use_startpoints);
    if (point == nullptr) {
      continue;
    }

    const sta::Pin* point_pin
        = target_collector_->getCurrentPointPin(use_startpoints);
    sta::Slack point_slack = sta_->slack(point, max_);
    if (sta::fuzzyGreaterEqual(point_slack, setup_slack_margin)) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 2,
                 "{}{} Phase: {} {} (index {}) already meets timing, "
                 "skipping",
                 phase_name,
                 phase_marker,
                 point_type_cap,
                 network_->pathName(point_pin),
                 point_index);
      continue;
    }

    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "{}{} Phase: Processing {} {} (index {}), slack = {}",
               phase_name,
               phase_marker,
               point_type,
               network_->pathName(point_pin),
               point_index,
               delayAsString(point_slack, kDelayDigits, sta_));
    rejected_pin_moves_current_endpoint_.clear();
    committer_.setCurrentEndpoint(point_pin);
    int point_pass_count = 0;
    sta::Slack prev_point_slack = point_slack;

    for (float margin_pct : margin_percentages) {
      if (point_pass_count >= max_passes_per_point) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "{}{} Phase: {} {} reached pass limit {}, moving to next "
                   "{}",
                   phase_name,
                   phase_marker,
                   point_type_cap,
                   network_->pathName(point_pin),
                   max_passes_per_point,
                   point_type);
        break;
      }
      point_slack = sta_->slack(point, max_);
      if (sta::fuzzyGreaterEqual(point_slack, setup_slack_margin)) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "{}{} Phase: {} {} now meets timing, moving to next {}",
                   phase_name,
                   phase_marker,
                   point_type_cap,
                   network_->pathName(point_pin),
                   point_type);
        break;
      }

      const sta::Slack margin = std::abs(point_slack) * margin_pct;
      const sta::Slack threshold = point_slack + margin;
      const sta::Slack prev_wns = target_collector_->getWns();
      const sta::Slack prev_endpoint_tns = target_collector_->getTns(false);
      const sta::Slack prev_startpoint_tns = target_collector_->getTns(true);

      committer_.beginJournal();
      point_pass_count++;
      opto_iteration++;
      if (verbose || opto_iteration % print_interval_ == 0) {
        printProgress(opto_iteration, false, phase_marker, use_startpoints);
      }

      std::vector<const sta::Pin*> viol_pins
          = target_collector_->collectViolatorsByDirectionalTraversal(
              use_startpoints,
              point_index,
              threshold,
              rsz::ViolatorSortType::SORT_BY_LOAD_DELAY);

      const int saved_max_repairs = setup_context_.max_repairs_per_pass;
      setup_context_.max_repairs_per_pass = viol_pins.size();
      std::vector<std::pair<const sta::Pin*, MoveType>> chosen_moves;
      const bool changed = repairPins(viol_pins,
                                      nullptr,
                                      &rejected_pin_moves_current_endpoint_,
                                      &chosen_moves);
      setup_context_.max_repairs_per_pass = saved_max_repairs;
      if (!changed) {
        committer_.commitJournal();
        continue;
      }

      estimate_parasitics_->updateParasitics();
      sta_->findRequireds();

      const sta::Slack new_point_slack = sta_->slack(point, max_);
      const sta::Slack new_wns = target_collector_->getWns();
      const sta::Slack new_endpoint_tns = target_collector_->getTns(false);
      const sta::Slack new_startpoint_tns = target_collector_->getTns(true);
      const bool wns_improved = sta::fuzzyGreater(new_wns, prev_wns);
      const bool wns_same = sta::fuzzyEqual(new_wns, prev_wns);
      const bool point_improved
          = sta::fuzzyGreater(new_point_slack, prev_point_slack);
      const bool endpoint_tns_improved
          = sta::fuzzyGreater(new_endpoint_tns, prev_endpoint_tns);
      const bool startpoint_tns_improved
          = sta::fuzzyGreater(new_startpoint_tns, prev_startpoint_tns);
      const bool endpoint_tns_same
          = sta::fuzzyEqual(new_endpoint_tns, prev_endpoint_tns);
      const bool startpoint_tns_same
          = sta::fuzzyEqual(new_startpoint_tns, prev_startpoint_tns);
      const bool both_tns_ok
          = (endpoint_tns_improved || endpoint_tns_same)
            && (startpoint_tns_improved || startpoint_tns_same);
      const bool focused_tns_improved
          = use_startpoints ? startpoint_tns_improved : endpoint_tns_improved;
      const bool better
          = wns_improved
            || (wns_same && (point_improved || focused_tns_improved)
                && both_tns_ok);

      const sta::Slack improvement = new_point_slack - prev_point_slack;
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: Threshold {}/3: {} slack {} -> {} (imp: {}), "
                 "WNS {} -> {}, EnTNS {} -> {}, StTNS {} -> {}{}",
                 phase_name,
                 phase_marker,
                 point_pass_count,
                 point_type,
                 delayAsString(prev_point_slack, kDelayDigits, sta_),
                 delayAsString(new_point_slack, kDelayDigits, sta_),
                 delayAsString(improvement, kDelayDigits, sta_),
                 delayAsString(prev_wns, kDelayDigits, sta_),
                 delayAsString(new_wns, kDelayDigits, sta_),
                 delayAsString(prev_endpoint_tns, 1, sta_),
                 delayAsString(new_endpoint_tns, 1, sta_),
                 delayAsString(prev_startpoint_tns, 1, sta_),
                 delayAsString(new_startpoint_tns, 1, sta_),
                 better ? " [ACCEPT]" : " [REJECT]");
      if (better) {
        committer_.commitJournal();
        prev_point_slack = new_point_slack;
        if (improvement < diminishing_returns_threshold) {
          debugPrint(
              logger_,
              RSZ,
              "repair_setup",
              1,
              "{}{} Phase: Improvement {} < {} (diminishing returns), "
              "moving to next {}",
              phase_name,
              phase_marker,
              delayAsString(improvement, kDelayDigits, sta_),
              delayAsString(diminishing_returns_threshold, kDelayDigits, sta_),
              point_type);
          break;
        }
      } else {
        committer_.restoreJournal();
        for (const auto& [chosen_pin, chosen_type] : chosen_moves) {
          rejected_pin_moves_current_endpoint_[chosen_pin].insert(chosen_type);
        }
      }
    }
    points_processed++;
  }

  printProgress(opto_iteration, true, phase_marker, use_startpoints);
  if (logger_->debugCheck(RSZ, "repair_setup", 1)) {
    sta::Slack final_wns;
    sta::Vertex* final_worst;
    sta_->worstSlack(max_, final_wns, final_worst);
    const float final_tns = sta_->totalNegativeSlack(max_);
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "{}{} Phase complete. {}s processed: {}, WNS: {}, TNS: {}",
               phase_name,
               phase_marker,
               point_type_cap,
               points_processed,
               delayAsString(final_wns, kDelayDigits, sta_),
               delayAsString(final_tns, 1, sta_));
  }
}

bool SetupLastGaspPolicy::initializeLastGaspRepair(
    const RepairSetupParams& params,
    const int opto_iteration,
    const float initial_tns,
    SetupLastGaspPolicy::LastGaspState& last_gasp_state,
    SetupLegacyBase::ViolatingEnds& violating_ends)
{
  // Last-gasp intentionally narrows the move sequence to transforms that still
  // have a chance to improve slack without large topology changes.
  move_sequence_.clear();
  pushMoveIfEnabled(!params.skip_vt_swap, MoveType::kVtSwap);
  move_sequence_.push_back(MoveType::kSizeUpMatch);
  move_sequence_.push_back(MoveType::kSizeUp);
  pushMoveIfEnabled(!params.skip_pin_swap, MoveType::kSwapPins);
  activateMoveSequence(false);

  violating_ends = collectViolatingEndpoints(params.setup_slack_margin);
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
  last_gasp_state.opto_iteration = opto_iteration;
  last_gasp_state.initial_tns = initial_tns;
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
    const RepairSetupParams& params,
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
    setup_context_.fallback = true;
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
                             params.setup_slack_margin)) {
    --last_gasp_state.num_viols;
  }
  saveImprovedCheckpoint(endpoint_state, max_last_gasp_passes_);
  return true;
}

void SetupLastGaspPolicy::repairLastGaspEndpoint(
    SetupLegacyBase::EndpointRepairState& endpoint_state,
    SetupLastGaspPolicy::LastGaspState& last_gasp_state,
    const RepairSetupParams& params,
    const int max_iterations)
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
    if (params.verbose || last_gasp_state.opto_iteration == 1) {
      printProgress(
          last_gasp_state.opto_iteration, false, last_gasp_state.phase_marker);
    }
    if (sta::fuzzyGreaterEqual(endpoint_state.end_slack,
                               params.setup_slack_margin)) {
      --last_gasp_state.num_viols;
      acceptEndpointState(endpoint_state);
      break;
    }

    sta::Path* end_path = sta_->vertexWorstSlackPath(endpoint_state.end, max_);
    const bool changed = repairPath(end_path, endpoint_state.end_slack);
    if (!changed) {
      finishEndpointSearch(endpoint_state);
      break;
    }

    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();
    refreshEndpointSlacks(endpoint_state);
    const float curr_tns = sta_->totalNegativeSlack(max_);
    if (!advanceLastGaspProgress(
            endpoint_state, last_gasp_state, params, curr_tns)) {
      break;
    }

    if (resizer_.overMaxArea()) {
      acceptEndpointState(endpoint_state);
      break;
    }
    if (last_gasp_state.end_index == 1) {
      endpoint_state.end = endpoint_state.worst_vertex;
      if (endpoint_state.end != nullptr) {
        target_collector_->useWorstEndpoint(endpoint_state.end);
        committer_.setCurrentEndpoint(endpoint_state.end->pin());
      }
    }

    ++endpoint_state.pass;
    if (reachedIterationLimit(last_gasp_state.opto_iteration, max_iterations)) {
      acceptEndpointState(endpoint_state);
      break;
    }
  }

  acceptEndpointState(endpoint_state);
}

bool SetupLastGaspPolicy::shouldStopLastGasp(
    const SetupLastGaspPolicy::LastGaspState& last_gasp_state,
    const int max_iterations) const
{
  return last_gasp_state.two_cons_terminations
         || reachedIterationLimit(last_gasp_state.opto_iteration,
                                  max_iterations);
}

void SetupLastGaspPolicy::runLastGaspLoop(
    const SetupLegacyBase::ViolatingEnds& violating_ends,
    const RepairSetupParams& params,
    const int max_iterations,
    SetupLastGaspPolicy::LastGaspState& last_gasp_state)
{
  for (const auto& end_original_slack : violating_ends) {
    if (shouldStopLastGasp(last_gasp_state, max_iterations)) {
      break;
    }

    EndpointRepairState endpoint_state;
    if (!beginLastGaspEndpoint(
            end_original_slack, last_gasp_state, endpoint_state)) {
      break;
    }

    repairLastGaspEndpoint(
        endpoint_state, last_gasp_state, params, max_iterations);

    if (params.verbose || last_gasp_state.opto_iteration == 1) {
      printProgress(
          last_gasp_state.opto_iteration, true, last_gasp_state.phase_marker);
    }
    if (shouldStopLastGasp(last_gasp_state, max_iterations)) {
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

bool SetupCritVtSwapPolicy::swapVTCritCells(const RepairSetupParams& params,
                                            int& num_viols)
{
  bool changed = false;
  ViolatingEnds violating_ends
      = collectViolatingEndpoints(params.setup_slack_margin);
  if (violating_ends.size() > kMaxCritEndpoints) {
    violating_ends.resize(kMaxCritEndpoints);
  }
  // Collect a bounded fanin cone across the worst endpoints, then VT-swap that
  // deduplicated instance set in one committer batch.
  std::unordered_map<sta::Instance*, float> crit_insts;
  std::unordered_set<sta::Vertex*> visited;
  std::unordered_set<sta::Instance*> notSwappable;
  for (const auto& [endpoint, slack] : violating_ends) {
    traverseFaninCone(endpoint, crit_insts, visited, notSwappable, params);
  }
  debugPrint(logger_,
             RSZ,
             "swap_crit_vt",
             1,
             "identified {} critical instances",
             crit_insts.size());

  VtSwapGenerator generator(makeGeneratorContext(config_), &notSwappable);
  for (auto crit_inst_slack : crit_insts) {
    sta::Pin* output_pin = worstOutputPin(crit_inst_slack.first);
    if (output_pin == nullptr) {
      continue;
    }
    Target target;
    target.views = kInstanceView;
    target.driver_pin = output_pin;
    target.slack = crit_inst_slack.second;
    if (!generator.isApplicable(target)) {
      continue;
    }

    if (committer_.moveTrackerEnabled(2)) {
      committer_.setCurrentEndpoint(output_pin);
      committer_.trackViolatorWithTimingInfo(output_pin,
                                             graph_->pinDrvrVertex(output_pin),
                                             target.slack,
                                             *target_collector_);
    }

    auto candidates = generator.generate(target);
    for (auto& candidate : candidates) {
      const Estimate estimate = candidate->estimate();
      if (!estimate.legal) {
        continue;
      }

      committer_.trackMoveAttempt(output_pin, MoveType::kVtSwap);
      const MoveResult result = committer_.commit(*candidate);
      if (result.accepted) {
        changed = true;
        debugPrint(logger_,
                   RSZ,
                   "swap_crit_vt",
                   1,
                   "inst {} did crit VT swap",
                   network_->pathName(crit_inst_slack.first));
        break;
      }
    }
  }
  if (changed) {
    committer_.acceptPendingMoves();
    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();
    num_viols = collectViolatingEndpoints(params.setup_slack_margin).size();
  } else {
    committer_.rejectPendingMoves();
  }

  return changed;
}

void SetupCritVtSwapPolicy::traverseFaninCone(
    sta::Vertex* endpoint,
    std::unordered_map<sta::Instance*, float>& crit_insts,
    std::unordered_set<sta::Vertex*>& visited,
    std::unordered_set<sta::Instance*>& notSwappable,
    const RepairSetupParams& params)
{
  if (visited.find(endpoint) != visited.end()) {
    return;
  }

  visited.insert(endpoint);
  std::queue<sta::Vertex*> queue;
  queue.push(endpoint);
  int endpoint_insts = 0;
  sta::LibertyCell* best_lib_cell;

  // Walk backward only through violating fanin logic and cap the number of
  // instances contributed by each endpoint.
  while (!queue.empty() && endpoint_insts < kMaxCritInstancesPerEndpoint) {
    sta::Vertex* current = queue.front();
    queue.pop();

    sta::Pin* pin = current->pin();
    sta::Instance* inst = network_->instance(pin);

    if (inst) {
      if (resizer_.checkAndMarkVTSwappable(inst, notSwappable, best_lib_cell)) {
        const sta::Slack inst_slack = getInstanceSlack(inst);
        if (sta::fuzzyLess(inst_slack, params.setup_slack_margin)) {
          auto it = crit_insts.find(inst);
          if (it == crit_insts.end()) {
            crit_insts[inst] = inst_slack;
            endpoint_insts++;
            debugPrint(logger_,
                       RSZ,
                       "swap_crit_vt",
                       1,
                       "swapVTCritCells: found crit inst {}: slack {}",
                       network_->name(inst),
                       float(inst_slack));
          }
        }
      }
    }

    sta::VertexInEdgeIterator edge_iter(current, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* fanin_vertex = edge->from(graph_);
      if (fanin_vertex->isRegClk()) {
        continue;
      }

      if (visited.find(fanin_vertex) == visited.end()) {
        const sta::Slack fanin_slack = sta_->slack(fanin_vertex, max_);
        if (sta::fuzzyLess(fanin_slack, params.setup_slack_margin)) {
          queue.push(fanin_vertex);
          visited.insert(fanin_vertex);
        }
      }
    }
  }

  debugPrint(logger_,
             RSZ,
             "swap_crit_vt",
             1,
             "traverseFaninCone: endpoint {} has {} critical instances:",
             endpoint->name(network_),
             endpoint_insts);
  if (logger_->debugCheck(RSZ, "swap_crit_vt", 1)) {
    for (auto crit_inst_slack : crit_insts) {
      logger_->report(" {}", network_->pathName(crit_inst_slack.first));
    }
  }
}

sta::Slack SetupCritVtSwapPolicy::getInstanceSlack(sta::Instance* inst)
{
  sta::Slack worst_slack = std::numeric_limits<float>::max();
  sta::InstancePinIterator* pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    if (network_->direction(pin)->isAnyOutput()) {
      sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
      if (vertex) {
        const sta::Slack pin_slack = sta_->slack(vertex, max_);
        worst_slack = std::min(worst_slack, pin_slack);
      }
    }
  }
  delete pin_iter;

  return worst_slack;
}

sta::Pin* SetupCritVtSwapPolicy::worstOutputPin(sta::Instance* inst)
{
  sta::Pin* worst_pin = nullptr;
  sta::Slack worst_slack = std::numeric_limits<float>::max();
  sta::InstancePinIterator* pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    if (!network_->direction(pin)->isAnyOutput()) {
      continue;
    }

    sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
    if (vertex == nullptr) {
      continue;
    }

    const sta::Slack pin_slack = sta_->slack(vertex, max_);
    if (worst_pin == nullptr || pin_slack < worst_slack) {
      worst_pin = pin;
      worst_slack = pin_slack;
    }
  }
  delete pin_iter;

  return worst_pin;
}

}  // namespace rsz
