// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SetupLegacyPolicy.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptPolicy.hh"
#include "OptimizerTypes.hh"
#include "Rebuffer.hh"
#include "RepairTargetCollector.hh"
#include "SizeUpGenerator.hh"
#include "SizeUpMatchGenerator.hh"
#include "SwapPinsGenerator.hh"
#include "VtSwapGenerator.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"
#include "utl/mem_stats.h"
#include "utl/timer.h"

namespace rsz {

using std::max;
using std::pair;
using std::string;
using std::vector;

using utl::RSZ;

namespace {
static constexpr int kDelayDigits = 3;
static constexpr size_t kMaxCritEndpoints = 100;
static constexpr int kMaxCritInstancesPerEndpoint = 50;

}  // namespace

SetupLegacyPolicy::SetupLegacyPolicy(Resizer& resizer, MoveCommitter& committer)
    : OptPolicy(resizer, committer)
{
}

void SetupLegacyPolicy::start(const OptimizerRunConfig& config)
{
  OptPolicy::start(config);
}

void SetupLegacyPolicy::iterate()
{
  if (converged_) {
    return;
  }

  markRunComplete(runSetup());
}

bool SetupLegacyPolicy::repairSetupPin(const sta::Pin* end_pin)
{
  init();
  if (end_pin == nullptr) {
    return false;
  }

  initializeSetupServices();
  max_repairs_per_pass_ = 1;
  resetMovedBufferFlag();

  sta::Vertex* end_vertex = graph_->pinLoadVertex(end_pin);
  if (end_vertex == nullptr) {
    return false;
  }

  const sta::Slack end_slack = sta_->slack(end_vertex, max_);
  sta::Path* end_path = sta_->vertexWorstSlackPath(end_vertex, max_);
  if (end_path == nullptr) {
    return false;
  }
  buildMainMoveSequence();

  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  return repairPath(end_path, end_slack);
}

void SetupLegacyPolicy::init()
{
  target_collector_ = std::make_unique<rsz::RepairTargetCollector>(&resizer_);
  initial_design_area_ = resizer_.computeDesignArea();
}

void SetupLegacyPolicy::initializeSetupServices()
{
  resizer_.rebuffer_->init();
  resizer_.rebuffer_->initOnCorner(sta_->cmdScene());
}

void SetupLegacyPolicy::resetMovedBufferFlag()
{
  resizer_.buffer_moved_into_core_ = false;
}

bool SetupLegacyPolicy::hasVtSwapCells() const
{
  return resizer_.lib_data_->sorted_vt_categories.size() > 1;
}

///////////////////////////////////////////////////////////////////////////////
// Setup sequence helpers
///////////////////////////////////////////////////////////////////////////////

void SetupLegacyPolicy::pushMoveIfEnabled(const bool enabled,
                                          const MoveType type)
{
  if (enabled) {
    move_sequence_.push_back(type);
  }
}

void SetupLegacyPolicy::logMoveSequence() const
{
  string repair_moves = "Repair move sequence: ";
  for (const MoveType type : move_sequence_) {
    if (type == MoveType::kCount) {
      continue;
    }
    repair_moves += moveName(type);
    repair_moves += ' ';
  }
  logger_->info(utl::RSZ, 100, "{}", repair_moves);
}

void SetupLegacyPolicy::activateMoveSequence(const bool log_sequence)
{
  buildMoveGenerators(move_sequence_, makeGeneratorContext(config_));
  if (log_sequence) {
    logMoveSequence();
  }
}

void SetupLegacyPolicy::buildMainMoveSequence()
{
  move_sequence_.clear();

  if (!config_.sequence.empty()) {
    for (const MoveType move : config_.sequence) {
      switch (move) {
        case MoveType::kBuffer:
          pushMoveIfEnabled(!config_.skip_buffering, MoveType::kBuffer);
          break;
        case MoveType::kUnbuffer:
          pushMoveIfEnabled(!config_.skip_buffer_removal, MoveType::kUnbuffer);
          break;
        case MoveType::kSwapPins:
          pushMoveIfEnabled(!config_.skip_pin_swap, MoveType::kSwapPins);
          break;
        case MoveType::kSizeUp:
          move_sequence_.push_back(MoveType::kSizeUp);
          break;
        case MoveType::kSizeDown:
          pushMoveIfEnabled(!config_.skip_size_down, MoveType::kSizeDown);
          break;
        case MoveType::kClone:
          pushMoveIfEnabled(!config_.skip_gate_cloning, MoveType::kClone);
          break;
        case MoveType::kSplitLoad:
          pushMoveIfEnabled(!config_.skip_buffering, MoveType::kSplitLoad);
          break;
        case MoveType::kVtSwap:
          pushMoveIfEnabled(!config_.skip_vt_swap && hasVtSwapCells(),
                            MoveType::kVtSwap);
          break;
        case MoveType::kSizeUpMatch:
          move_sequence_.push_back(MoveType::kSizeUpMatch);
          break;
        case MoveType::kCount:
          break;
      }
    }
  } else {
    pushMoveIfEnabled(!config_.skip_buffer_removal, MoveType::kUnbuffer);
    pushMoveIfEnabled(!config_.skip_vt_swap && hasVtSwapCells(),
                      MoveType::kVtSwap);
    move_sequence_.push_back(MoveType::kSizeUp);
    if (!config_.skip_size_down) {
      // Disabled by default for legacy parity.
    }
    pushMoveIfEnabled(!config_.skip_pin_swap, MoveType::kSwapPins);
    pushMoveIfEnabled(!config_.skip_buffering, MoveType::kBuffer);
    pushMoveIfEnabled(!config_.skip_gate_cloning, MoveType::kClone);
    pushMoveIfEnabled(!config_.skip_buffering, MoveType::kSplitLoad);
  }

  activateMoveSequence(true);
}

void SetupLegacyPolicy::buildLastGaspMoveSequence(
    const RepairSetupParams& params)
{
  move_sequence_.clear();
  pushMoveIfEnabled(!params.skip_vt_swap, MoveType::kVtSwap);
  move_sequence_.push_back(MoveType::kSizeUpMatch);
  move_sequence_.push_back(MoveType::kSizeUp);
  pushMoveIfEnabled(!params.skip_pin_swap, MoveType::kSwapPins);
  activateMoveSequence(false);
}

RepairSetupParams SetupLegacyPolicy::makeRepairSetupParams(
    const float setup_slack_margin) const
{
  return RepairSetupParams{.setup_slack_margin = setup_slack_margin,
                           .verbose = config_.verbose,
                           .skip_pin_swap = config_.skip_pin_swap,
                           .skip_gate_cloning = config_.skip_gate_cloning,
                           .skip_size_down = config_.skip_size_down,
                           .skip_buffering = config_.skip_buffering,
                           .skip_buffer_removal = config_.skip_buffer_removal,
                           .skip_vt_swap = config_.skip_vt_swap};
}

SetupLegacyPolicy::ViolatingEnds SetupLegacyPolicy::collectViolatingEndpoints(
    const float setup_slack_margin) const
{
  target_collector_->init(setup_slack_margin);
  ViolatingEnds violating_ends;
  const auto& endpoint_pins = target_collector_->getViolatingEndpoints();
  violating_ends.reserve(endpoint_pins.size());
  for (const auto& [endpoint_pin, endpoint_slack] : endpoint_pins) {
    sta::Vertex* endpoint_vertex = graph_->pinLoadVertex(endpoint_pin);
    if (endpoint_vertex != nullptr) {
      violating_ends.emplace_back(endpoint_vertex, endpoint_slack);
    }
  }
  return violating_ends;
}

bool SetupLegacyPolicy::initializeMainRepair(
    const float setup_slack_margin,
    const double repair_tns_end_percent,
    SetupLegacyPolicy::MainRepairState& main_state,
    SetupLegacyPolicy::ViolatingEnds& violating_ends)
{
  violating_ends = collectViolatingEndpoints(setup_slack_margin);

  // Defensive: runSetup() already rejected the zero-violation case, but the
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

  main_state.max_end_count = max(
      static_cast<int>(violating_ends.size() * repair_tns_end_percent), 1);
  main_state.initial_tns = sta_->totalNegativeSlack(max_);
  main_state.prev_tns = main_state.initial_tns;
  main_state.num_viols = violating_ends.size();
  main_state.fix_rate_threshold = inc_fix_rate_threshold_;

  printProgress(
      main_state.opto_iteration, false, false, main_state.phase_marker);

  min_viol_ = -violating_ends.back().second;
  max_viol_ = -violating_ends.front().second;
  return true;
}

bool SetupLegacyPolicy::beginJournaledEndpointSearch(
    const pair<sta::Vertex*, sta::Slack>& end_original_slack,
    const int max_end_count,
    int& end_index,
    SetupLegacyPolicy::EndpointRepairState& endpoint_state)
{
  fallback_ = false;
  endpoint_state.end = end_original_slack.first;
  refreshEndpointSlacks(endpoint_state);
  target_collector_->useWorstEndpoint(endpoint_state.end);
  committer_.setCurrentEndpoint(endpoint_state.end->pin());
  ++end_index;
  if (end_index > max_end_count) {
    return false;
  }

  endpoint_state.pass = 1;
  endpoint_state.decreasing_slack_passes = 0;
  resizer_.journalBegin();
  endpoint_state.journal_open = true;
  return true;
}

bool SetupLegacyPolicy::beginEndpointRepair(
    const pair<sta::Vertex*, sta::Slack>& end_original_slack,
    SetupLegacyPolicy::MainRepairState& main_state,
    SetupLegacyPolicy::EndpointRepairState& endpoint_state)
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

void SetupLegacyPolicy::recordTermination(bool& prev_termination,
                                          bool& two_cons_terminations)
{
  if (prev_termination) {
    two_cons_terminations = true;
  } else {
    prev_termination = true;
  }
}

void SetupLegacyPolicy::acceptEndpointState(
    SetupLegacyPolicy::EndpointRepairState& endpoint_state)
{
  if (!endpoint_state.journal_open) {
    return;
  }

  resizer_.journalEnd();
  committer_.acceptPendingMoves();
  endpoint_state.journal_open = false;
}

void SetupLegacyPolicy::restoreEndpointState(
    SetupLegacyPolicy::EndpointRepairState& endpoint_state)
{
  if (!endpoint_state.journal_open) {
    return;
  }

  resizer_.journalRestore();
  committer_.rejectPendingMoves();
  endpoint_state.journal_open = false;
}

void SetupLegacyPolicy::finishEndpointSearch(
    SetupLegacyPolicy::EndpointRepairState& endpoint_state)
{
  if (endpoint_state.pass == 1) {
    acceptEndpointState(endpoint_state);
  } else {
    restoreEndpointState(endpoint_state);
  }
}

void SetupLegacyPolicy::saveImprovedCheckpoint(
    SetupLegacyPolicy::EndpointRepairState& endpoint_state,
    const int max_passes)
{
  acceptEndpointState(endpoint_state);
  if (endpoint_state.pass < max_passes) {
    resizer_.journalBegin();
    endpoint_state.journal_open = true;
  }
}

void SetupLegacyPolicy::refreshEndpointSlacks(
    SetupLegacyPolicy::EndpointRepairState& endpoint_state)
{
  endpoint_state.end_slack = sta_->slack(endpoint_state.end, max_);
  sta_->worstSlack(
      max_, endpoint_state.worst_slack, endpoint_state.worst_vertex);
}

void SetupLegacyPolicy::repairEndpoint(
    SetupLegacyPolicy::EndpointRepairState& endpoint_state,
    SetupLegacyPolicy::MainRepairState& main_state,
    const float setup_slack_margin,
    const int max_passes,
    const int max_iterations,
    const bool verbose)
{
  while (endpoint_state.pass <= max_passes) {
    if (shouldStopEndpointRepair(endpoint_state, setup_slack_margin)) {
      finishEndpointSearch(endpoint_state);
      break;
    }

    ++main_state.opto_iteration;
    if (verbose || main_state.opto_iteration == 1) {
      printProgress(
          main_state.opto_iteration, false, false, main_state.phase_marker);
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
      fallback_ = true;
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

bool SetupLegacyPolicy::shouldStopEndpointRepair(
    SetupLegacyPolicy::EndpointRepairState& endpoint_state,
    const float setup_slack_margin)
{
  static_cast<void>(endpoint_state);
  static_cast<void>(setup_slack_margin);
  return false;
}

bool SetupLegacyPolicy::shouldStopMainRepair(
    const SetupLegacyPolicy::MainRepairState& main_state) const
{
  return main_state.two_cons_terminations;
}

bool SetupLegacyPolicy::reachedIterationLimit(const int iteration,
                                              const int max_iterations) const
{
  return max_iterations > 0 && iteration >= max_iterations;
}

void SetupLegacyPolicy::runMainRepairLoop(
    const SetupLegacyPolicy::ViolatingEnds& violating_ends,
    const float setup_slack_margin,
    const int max_passes,
    const int max_iterations,
    const bool verbose,
    SetupLegacyPolicy::MainRepairState& main_state)
{
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      10,
      fmt::format("LEGACY{} Phase Time: {{}}", main_state.phase_marker));
  for (const auto& end_original_slack : violating_ends) {
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
      printProgress(
          main_state.opto_iteration, true, false, main_state.phase_marker);
    }
    if (shouldStopMainRepair(main_state)) {
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

  printProgress(
      main_state.opto_iteration, true, false, main_state.phase_marker);
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

bool SetupLegacyPolicy::runSetup()
{
  init();
  initializeSetupServices();

  // Pull the run configuration into local aliases so the rest of the control
  // flow reads like the legacy RepairSetup algorithm instead of a config map.
  const float setup_slack_margin = config_.setup_slack_margin;
  const double repair_tns_end_percent = config_.repair_tns_end_percent;
  const int max_passes = config_.max_passes;
  const int max_iterations = config_.max_iterations;
  const int max_repairs_per_pass = config_.max_repairs_per_pass;
  const bool verbose = config_.verbose;
  const bool skip_last_gasp = config_.skip_last_gasp;
  const bool skip_crit_vt_swap = config_.skip_crit_vt_swap;
  const RepairSetupParams repair_setup_params
      = makeRepairSetupParams(setup_slack_margin);

  max_repairs_per_pass_ = max_repairs_per_pass;
  resetMovedBufferFlag();

  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  target_collector_->init(setup_slack_margin);

  // Move sequence is logged before the violation summary so the move-sequence
  // line precedes "Found/Repairing" in the output.
  buildMainMoveSequence();

  const int total_violations = target_collector_->getTotalViolations();
  const sta::VertexSet& endpoints = sta_->endpoints();
  if (total_violations > 0) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "Violating endpoints {}/{} {}%",
               total_violations,
               endpoints.size(),
               int(total_violations / double(endpoints.size()) * 100));
  }
  if (total_violations == 0) {
    logger_->metric("design__instance__count__setup_buffer", 0);
    logger_->info(utl::RSZ, 98, "No setup violations found");
    return false;
  }
  logger_->info(utl::RSZ,
                94,
                "Found {} endpoints with setup violations.",
                total_violations);
  max_end_repairs_
      = std::max(static_cast<int>(target_collector_->getMaxEndpointCount()
                                  * repair_tns_end_percent),
                 1);
  logger_->info(utl::RSZ,
                99,
                "Repairing {} out of {} ({:0.2f}%) violating endpoints...",
                max_end_repairs_,
                total_violations,
                repair_tns_end_percent * 100.0);
  sta_->checkCapacitancesPreamble(sta_->scenes());
  sta_->checkSlewsPreamble();
  sta_->checkFanoutPreamble();

  if (committer_.moveTrackerEnabled()) {
    committer_.captureInitialSlackDistribution();
    committer_.captureOriginalEndpointSlack();
  }
  const std::string phases
      = config_.phases.empty() ? "LEGACY LAST_GASP" : config_.phases;
  const bool custom_phase_sequence = !config_.phases.empty();
  int opto_iteration = 0;
  int num_viols = 0;
  float initial_tns = sta_->totalNegativeSlack(max_);
  float prev_tns = initial_tns;
  if (custom_phase_sequence) {
    reportCustomPhaseSetup();
  }

  auto getPhaseMarker = [](int phase_index) -> char {
    constexpr char special_markers[] = "*+^&@!-=";
    constexpr int num_special = 8;
    if (phase_index < num_special) {
      return special_markers[phase_index];
    }
    phase_index -= num_special;
    if (phase_index < 26) {
      return 'a' + phase_index;
    }
    phase_index -= 26;
    if (phase_index < 26) {
      return 'A' + phase_index;
    }
    return '?';
  };

  std::istringstream phase_stream(phases);
  std::string phase_name;
  int phase_index = 0;
  while (phase_stream >> phase_name) {
    const char phase_marker = getPhaseMarker(phase_index++);
    if (phase_name == "LEGACY") {
      committer_.capturePrePhaseSlack();
      MainRepairState main_state;
      main_state.opto_iteration = opto_iteration;
      main_state.initial_tns = initial_tns;
      main_state.prev_tns = initial_tns;
      main_state.phase_marker = phase_marker;
      ViolatingEnds violating_ends;
      if (initializeMainRepair(setup_slack_margin,
                               repair_tns_end_percent,
                               main_state,
                               violating_ends)) {
        runMainRepairLoop(violating_ends,
                          setup_slack_margin,
                          max_passes,
                          max_iterations,
                          verbose,
                          main_state);
      }
      committer_.printTrackerPhaseSummary(
          "LEGACY Phase Summary", "LEGACY Phase Endpoint Profiler", true);
      opto_iteration = main_state.opto_iteration;
      num_viols = main_state.num_viols;
      initial_tns = main_state.initial_tns;
      prev_tns = main_state.prev_tns;
      continue;
    }

    if (phase_name == "WNS" || phase_name == "WNS_PATH") {
      repairSetupWns(setup_slack_margin,
                     max_passes,
                     max_repairs_per_pass,
                     verbose,
                     opto_iteration,
                     initial_tns,
                     prev_tns,
                     false,
                     phase_marker,
                     rsz::ViolatorSortType::SORT_BY_LOAD_DELAY);
      committer_.printTrackerPhaseSummary(
          "WNS_PATH Phase Summary", "WNS_PATH Phase Endpoint Profiler", true);
      continue;
    }

    if (phase_name == "WNS_CONE") {
      repairSetupWns(setup_slack_margin,
                     max_passes,
                     max_repairs_per_pass,
                     verbose,
                     opto_iteration,
                     initial_tns,
                     prev_tns,
                     true,
                     phase_marker,
                     rsz::ViolatorSortType::SORT_BY_LOAD_DELAY);
      committer_.printTrackerPhaseSummary(
          "WNS_CONE Phase Summary", "WNS_CONE Phase Endpoint Profiler", true);
      continue;
    }

    if (phase_name == "TNS") {
      repairSetupTns(setup_slack_margin,
                     max_passes,
                     max_repairs_per_pass,
                     verbose,
                     opto_iteration,
                     phase_marker,
                     rsz::ViolatorSortType::SORT_BY_LOAD_DELAY);
      committer_.printTrackerPhaseSummary(
          "TNS Phase Summary", "TNS Phase Endpoint Profiler", true);
      continue;
    }

    if (phase_name == "ENDPOINT_FANIN") {
      repairSetupEndpointFanin(setup_slack_margin,
                               max_passes,
                               verbose,
                               opto_iteration,
                               phase_marker);
      committer_.printTrackerPhaseSummary(
          "ENDPOINT_FANIN Phase Summary",
          "ENDPOINT_FANIN Phase Endpoint Profiler",
          true);
      continue;
    }

    if (phase_name == "STARTPOINT_FANOUT") {
      repairSetupStartpointFanout(setup_slack_margin,
                                  max_passes,
                                  verbose,
                                  opto_iteration,
                                  phase_marker);
      committer_.printTrackerPhaseSummary(
          "STARTPOINT_FANOUT Phase Summary",
          "STARTPOINT_FANOUT Phase Startpoint Profiler",
          true);
      continue;
    }

    if (phase_name == "LAST_GASP") {
      if (!skip_last_gasp) {
        committer_.capturePrePhaseSlack();
        RepairSetupParams params = repair_setup_params;
        repairSetupLastGasp(params,
                            num_viols,
                            max_iterations,
                            opto_iteration,
                            initial_tns,
                            phase_marker);
        committer_.printTrackerPhaseSummary("LAST_GASP Phase Summary",
                                            "LAST_GASP Phase Endpoint Profiler",
                                            true);
      }
      continue;
    }

    logger_->error(utl::RSZ,
                   217,
                   "Unknown phase name '{}'. Valid phase names are: LEGACY, "
                   "WNS, WNS_PATH, WNS_CONE, TNS, ENDPOINT_FANIN, "
                   "STARTPOINT_FANOUT, LAST_GASP",
                   phase_name);
  }

  // Critical VT swap runs as a separate batch because it is endpoint-agnostic
  // once the critical instance set has been collected.
  if (!skip_crit_vt_swap && !config_.skip_vt_swap && hasVtSwapCells()) {
    committer_.capturePrePhaseSlack();
    RepairSetupParams params = repair_setup_params;
    if (swapVTCritCells(params, num_viols)) {
      estimate_parasitics_->updateParasitics();
      sta_->findRequireds();
    }
    committer_.printTrackerPhaseSummary(
        "VT Swap Phase Summary", nullptr, false);
  }

  printProgress(opto_iteration, true, true, false);
  committer_.printTrackerFinalReports(target_collector_->getViolatingPins());
  return reportRepairSummary(setup_slack_margin);
}

bool SetupLegacyPolicy::reportRepairSummary(const float setup_slack_margin)
{
  bool repaired = false;

  const int buffer_moves = committer_.summaryCommittedMoves(MoveType::kBuffer);
  const int size_up_moves = committer_.summaryCommittedMoves(MoveType::kSizeUp);
  const int size_down_moves
      = committer_.summaryCommittedMoves(MoveType::kSizeDown);
  const int swap_pins_moves
      = committer_.summaryCommittedMoves(MoveType::kSwapPins);
  const int clone_moves = committer_.summaryCommittedMoves(MoveType::kClone);
  const int split_load_moves
      = committer_.summaryCommittedMoves(MoveType::kSplitLoad);
  const int unbuffer_moves
      = committer_.summaryCommittedMoves(MoveType::kUnbuffer);
  const int vt_swap_moves = committer_.summaryCommittedMoves(MoveType::kVtSwap);
  const int size_up_match_moves
      = committer_.summaryCommittedMoves(MoveType::kSizeUpMatch);

  if (unbuffer_moves > 0) {
    repaired = true;
    logger_->info(utl::RSZ, 59, "Removed {} buffers.", unbuffer_moves);
  }
  if (buffer_moves > 0 || split_load_moves > 0) {
    repaired = true;
    if (split_load_moves == 0) {
      logger_->info(utl::RSZ, 40, "Inserted {} buffers.", buffer_moves);
    } else {
      logger_->info(utl::RSZ,
                    45,
                    "Inserted {} buffers, {} to split loads.",
                    buffer_moves + split_load_moves,
                    split_load_moves);
    }
  }
  logger_->metric("design__instance__count__setup_buffer",
                  buffer_moves + split_load_moves);
  if (size_up_moves + size_down_moves + size_up_match_moves + vt_swap_moves
      > 0) {
    repaired = true;
    logger_->info(
        utl::RSZ,
        51,
        "Resized {} instances: {} up, {} up match, {} down, {} VT",
        size_up_moves + size_up_match_moves + size_down_moves + vt_swap_moves,
        size_up_moves,
        size_up_match_moves,
        size_down_moves,
        vt_swap_moves);
  }
  if (swap_pins_moves > 0) {
    repaired = true;
    logger_->info(
        utl::RSZ, 43, "Swapped pins on {} instances.", swap_pins_moves);
  }
  if (clone_moves > 0) {
    repaired = true;
    logger_->info(utl::RSZ, 49, "Cloned {} instances.", clone_moves);
  }

  const sta::Slack worst_slack = sta_->worstSlack(max_);
  if (sta::fuzzyLess(worst_slack, setup_slack_margin)) {
    repaired = true;
    logger_->warn(utl::RSZ, 62, "Unable to repair all setup violations.");
  }
  if (resizer_.overMaxArea()) {
    logger_->error(utl::RSZ, 25, "max utilization reached.");
  }

  return repaired;
}

///////////////////////////////////////////////////////////////////////////////
// Path repair
///////////////////////////////////////////////////////////////////////////////

int SetupLegacyPolicy::fanout(sta::Vertex* vertex) const
{
  int fanout_count = 0;
  sta::VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (edge->isWire()) {
      fanout_count++;
    }
  }
  return fanout_count;
}

std::vector<std::pair<int, sta::Delay>> SetupLegacyPolicy::rankPathDrivers(
    sta::PathExpanded& expanded,
    const sta::Scene* corner,
    const int lib_ap) const
{
  vector<pair<int, sta::Delay>> load_delays;
  const int start_index = expanded.startIndex();
  const int path_length = expanded.size();

  // Rank driver stages by load-dependent delay so the noisiest points on the
  // expanded path get first crack at the repair budget.
  for (int i = start_index; i < path_length; i++) {
    const sta::Path* path_vertex_path = expanded.path(i);
    sta::Vertex* path_vertex = path_vertex_path->vertex(sta_);
    const sta::Pin* path_pin = path_vertex_path->pin(sta_);
    if (i > 0 && path_vertex->isDriver(network_)
        && !network_->isTopLevelPort(path_pin)) {
      const sta::TimingArc* prev_arc = path_vertex_path->prevArc(sta_);
      const sta::TimingArc* corner_arc = prev_arc->sceneArc(lib_ap);
      sta::Edge* prev_edge = path_vertex_path->prevEdge(sta_);
      const sta::Delay load_delay
          = graph_->arcDelay(
                prev_edge, prev_arc, corner->dcalcAnalysisPtIndex(max_))
            - corner_arc->intrinsicDelay();
      load_delays.emplace_back(i, load_delay);
    }
  }

  std::ranges::sort(
      load_delays, [](pair<int, sta::Delay> lhs, pair<int, sta::Delay> rhs) {
        return lhs.second > rhs.second
               || (lhs.second == rhs.second && lhs.first > rhs.first);
      });
  return load_delays;
}

int SetupLegacyPolicy::repairBudget(const sta::Slack path_slack) const
{
  int repairs_per_pass = 1;
  if (max_viol_ - min_viol_ != 0.0) {
    repairs_per_pass
        += std::round((max_repairs_per_pass_ - 1) * (-path_slack - min_viol_)
                      / (max_viol_ - min_viol_));
  }
  return fallback_ ? 1 : repairs_per_pass;
}

bool SetupLegacyPolicy::makePinTargetOnPath(const sta::Pin* pin,
                                            const sta::Path* path,
                                            const sta::Slack focus_slack,
                                            Target& target) const
{
  if (path == nullptr) {
    return false;
  }

  sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
  if (vertex == nullptr) {
    return false;
  }

  sta::PathExpanded expanded(path, sta_);
  for (int index = expanded.startIndex(); index < expanded.size(); index++) {
    const sta::Path* driver_path = expanded.path(index);
    sta::Vertex* path_vertex = driver_path->vertex(sta_);
    const sta::Pin* path_pin = driver_path->pin(sta_);
    if (path_vertex != vertex || path_pin != pin) {
      continue;
    }
    if (!path_vertex->isDriver(network_)
        || network_->isTopLevelPort(path_pin)) {
      return false;
    }

    makePathDriverTarget(path, expanded, index, focus_slack, target);
    return true;
  }

  return false;
}

bool SetupLegacyPolicy::makePinTarget(const sta::Pin* pin,
                                      const sta::Slack focus_slack,
                                      Target& target) const
{
  sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
  if (vertex == nullptr) {
    return false;
  }

  sta::Path* path = sta_->vertexWorstSlackPath(vertex, max_);
  return makePinTargetOnPath(pin, path, focus_slack, target);
}

bool SetupLegacyPolicy::repairPins(
    const std::vector<const sta::Pin*>& pins,
    const sta::Path* focus_path,
    const std::unordered_map<const sta::Pin*, std::unordered_set<MoveType>>*
        rejected_moves,
    std::vector<std::pair<const sta::Pin*, MoveType>>* chosen_moves)
{
  int changed = 0;

  if (pins.empty()) {
    return false;
  }

  int repairs_per_pass
      = target_collector_->repairsPerPass(max_repairs_per_pass_);
  if (fallback_) {
    repairs_per_pass = 1;
  }

  const sta::Slack focus_slack = target_collector_->getCurrentEndpointSlack();
  if (targetPrewarmEnabled()) {
    std::vector<Target> prewarm_targets;
    prewarm_targets.reserve(pins.size());
    for (const sta::Pin* driver_pin : pins) {
      Target target;
      const bool has_target
          = focus_path != nullptr
                ? makePinTargetOnPath(
                      driver_pin, focus_path, focus_slack, target)
                : makePinTarget(driver_pin, focus_slack, target);
      if (has_target) {
        prewarm_targets.push_back(target);
      }
    }
    prewarmTargets(prewarm_targets);
  }

  for (const sta::Pin* driver_pin : pins) {
    if (changed >= repairs_per_pass) {
      break;
    }

    Target target;
    const bool has_target
        = focus_path != nullptr
              ? makePinTargetOnPath(driver_pin, focus_path, focus_slack, target)
              : makePinTarget(driver_pin, focus_slack, target);
    if (!has_target) {
      continue;
    }

    committer_.trackViolatorWithTimingInfo(target.driver_pin,
                                           target.vertex(resizer_),
                                           focus_slack,
                                           *target_collector_);

    const std::unordered_set<MoveType>* rejected_types = nullptr;
    auto rejected_itr = rejected_moves->find(driver_pin);
    if (rejected_itr != rejected_moves->end()) {
      rejected_types = &rejected_itr->second;
    }

    std::optional<MoveType> accepted_type;
    logRepairTarget(target);
    if (tryRepairTarget(
            target, repairs_per_pass, changed, rejected_types, accepted_type)
        && accepted_type.has_value()) {
      chosen_moves->emplace_back(driver_pin, *accepted_type);
    }
  }

  return changed > 0;
}

void SetupLegacyPolicy::makePathDriverTarget(const sta::Path* path,
                                             sta::PathExpanded& expanded,
                                             const int drvr_index,
                                             const sta::Slack path_slack,
                                             Target& target) const
{
  const sta::Path* drvr_path = expanded.path(drvr_index);
  sta::Vertex* drvr_vertex = drvr_path->vertex(sta_);

  target = rsz::makePathDriverTarget(
      path, expanded, drvr_index, path_slack, resizer_);
  target.fanout = fanout(drvr_vertex);
}

void SetupLegacyPolicy::logRepairTarget(const Target& target) const
{
  sta::LibertyPort* drvr_port = network_->libertyPort(target.driver_pin);
  sta::LibertyCell* drvr_cell
      = drvr_port != nullptr ? drvr_port->libertyCell() : nullptr;
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "{} {} fanout = {} drvr_index = {}",
             network_->pathName(target.driver_pin),
             drvr_cell != nullptr ? drvr_cell->name() : "none",
             target.fanout,
             target.path_index);
}

int SetupLegacyPolicy::repairProgressIncrement(const MoveType type,
                                               const int repairs_per_pass)
{
  return type == MoveType::kUnbuffer ? repairs_per_pass : 1;
}

bool SetupLegacyPolicy::allowsBatchRepair(const MoveType type)
{
  return type == MoveType::kSizeDown;
}

bool SetupLegacyPolicy::tryCandidateSequence(
    MoveGenerator& generator,
    const Target& target,
    const int repairs_per_pass,
    int& changed,
    std::optional<MoveType>& accepted_type)
{
  auto candidates = generator.generate(target);
  for (auto& candidate : candidates) {
    const Estimate estimate = candidate->estimate();
    if (!estimate.legal) {
      continue;
    }

    committer_.trackMoveAttempt(target.driver_pin, generator.type());
    const MoveResult result = committer_.commit(*candidate);
    if (!result.accepted) {
      continue;
    }

    changed += repairProgressIncrement(result.type, repairs_per_pass);
    accepted_type = result.type;
    return true;
  }
  return false;
}

bool SetupLegacyPolicy::trySizeDownBatch(MoveGenerator& generator,
                                         const Target& target,
                                         const int repairs_per_pass,
                                         int& changed,
                                         std::optional<MoveType>& accepted_type)
{
  bool accepted_batch = false;
  while (tryCandidateSequence(
      generator, target, repairs_per_pass, changed, accepted_type)) {
    accepted_batch = true;
  }
  return accepted_batch;
}

bool SetupLegacyPolicy::tryRepairTarget(
    const Target& target,
    const int repairs_per_pass,
    int& changed,
    const std::unordered_set<MoveType>* rejected_types)
{
  std::optional<MoveType> accepted_type;
  return tryRepairTarget(
      target, repairs_per_pass, changed, rejected_types, accepted_type);
}

bool SetupLegacyPolicy::tryRepairTarget(
    const Target& target,
    const int repairs_per_pass,
    int& changed,
    const std::unordered_set<MoveType>* rejected_types,
    std::optional<MoveType>& accepted_type)
{
  Target live_target = target;
  sta::Vertex* live_vertex = live_target.vertex(resizer_);
  if (live_vertex != nullptr) {
    live_target.fanout = fanout(live_vertex);
  }

  for (const std::unique_ptr<MoveGenerator>& generator_ptr : move_generators_) {
    MoveGenerator& generator = *generator_ptr;
    const MoveType type = generator.type();
    if (rejected_types != nullptr && rejected_types->contains(type)) {
      continue;
    }
    if (!generator.isApplicable(live_target)) {
      continue;
    }

    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "Considering {} for {}",
               generator.name(),
               network_->pathName(live_target.driver_pin));

    if (allowsBatchRepair(type)) {
      return trySizeDownBatch(
          generator, live_target, repairs_per_pass, changed, accepted_type);
    }
    if (tryCandidateSequence(
            generator, live_target, repairs_per_pass, changed, accepted_type)) {
      return true;
    }
  }
  return false;
}

bool SetupLegacyPolicy::tryRepairPathTarget(const Target& target,
                                            const sta::Slack path_slack,
                                            const int repairs_per_pass,
                                            int& changed)
{
  committer_.trackViolatorWithTimingInfo(target.driver_pin,
                                         target.vertex(resizer_),
                                         path_slack,
                                         *target_collector_);
  logRepairTarget(target);
  return tryRepairTarget(target, repairs_per_pass, changed, nullptr);
}

bool SetupLegacyPolicy::repairPath(sta::Path* path, const sta::Slack path_slack)
{
  sta::PathExpanded expanded(path, sta_);
  int changed = 0;

  if (expanded.size() <= 1) {
    return false;
  }

  const sta::Scene* corner = path->scene(sta_);
  if (path->minMax(sta_) != resizer_.max_) {
    logger_->error(utl::RSZ,
                   kMsgRepairSetupExpectedMaxPath,
                   "repairSetup expects max delay path");
    return false;
  }

  const auto load_delays
      = rankPathDrivers(expanded, corner, corner->libertyIndex(resizer_.max_));
  const int repairs_per_pass = repairBudget(path_slack);

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "Path slack: {}, repairs: {}, load_delays: {}",
             delayAsString(path_slack, 3, sta_),
             repairs_per_pass,
             load_delays.size());

  // Construct target vector
  std::vector<Target> targets;
  targets.reserve(load_delays.size());
  for (const auto& [drvr_index, ignored] : load_delays) {
    static_cast<void>(ignored);
    Target target;
    makePathDriverTarget(path, expanded, drvr_index, path_slack, target);
    targets.push_back(target);
  }

  // Prewarm for legacy MT policy
  if (targetPrewarmEnabled()) {
    prewarmTargets(targets);
  }

  // Drive ranked targets through the configured generator list until this
  // pass reaches its repair budget.
  for (const Target& target : targets) {
    if (changed >= repairs_per_pass) {
      break;
    }
    tryRepairPathTarget(target, path_slack, repairs_per_pass, changed);
  }

  return changed > 0;
}

int SetupLegacyPolicy::committedMoves(const MoveType type) const
{
  return committer_.committedMoves(type);
}

int SetupLegacyPolicy::totalMoves(const MoveType type) const
{
  return committer_.totalMoves(type);
}

void SetupLegacyPolicy::printProgress(const int iteration,
                                      const bool force,
                                      const bool end,
                                      const bool last_gasp) const
{
  printProgress(iteration, force, end, last_gasp ? '+' : '*', false);
}

void SetupLegacyPolicy::printProgress(const int iteration,
                                      const bool force,
                                      const bool end,
                                      const char phase_marker,
                                      const bool use_startpoint_metrics) const
{
  const bool start = iteration == 0;

  if (start && !end) {
    logger_->report(
        "   Iter   | Removed | Resized | Inserted | Cloned |  Pin  |"
        "   Area   |    WNS   |   StTNS    |   EnTNS    |  Viol  |  Worst  ");
    logger_->report(
        "          | Buffers |  Gates  | Buffers  |  Gates | Swaps |"
        "          |          |            |            | Endpts | St/EnPt ");
    logger_->report(
        "---------------------------------------------------------------"
        "---------------------------------------------------------------");
  }

  if (iteration % print_interval_ != 0 && !force && !end) {
    return;
  }

  if (end) {
    target_collector_->collectViolatingEndpoints();
    target_collector_->collectViolatingStartpoints();
  }

  const sta::Slack wns = target_collector_->getWns();
  const sta::Slack st_tns = target_collector_->getTns(true);
  const sta::Slack en_tns = target_collector_->getTns(false);
  const bool show_startpoint_metrics = use_startpoint_metrics && !end;
  const sta::Pin* worst_pin
      = target_collector_->getWorstPin(show_startpoint_metrics);

  std::string itr_field = fmt::format("{}{}", iteration, phase_marker);
  if (end) {
    itr_field = "final";
  }

  const double design_area = resizer_.computeDesignArea();
  const double area_growth = design_area - initial_design_area_;
  double area_growth_percent = std::numeric_limits<double>::infinity();
  if (std::abs(initial_design_area_) > 0.0) {
    area_growth_percent = area_growth / initial_design_area_ * 100.0;
  }

  logger_->report(
      "{: >9s} | {: >7d} | {: >7d} | {: >8d} | {: >6d} | {: >5d} "
      "| {: >+7.1f}% | {: >8s} | {: >10s} | {: >10s} | {: >6d} | {}",
      itr_field,
      totalMoves(MoveType::kUnbuffer),
      totalMoves(MoveType::kSizeUp) + totalMoves(MoveType::kSizeDown)
          + totalMoves(MoveType::kSizeUpMatch) + totalMoves(MoveType::kVtSwap),
      totalMoves(MoveType::kBuffer) + totalMoves(MoveType::kSplitLoad),
      totalMoves(MoveType::kClone),
      totalMoves(MoveType::kSwapPins),
      area_growth_percent,
      delayAsString(wns, 3, sta_),
      delayAsString(st_tns, 1, sta_),
      delayAsString(en_tns, 1, sta_),
      std::max(0, target_collector_->getNumViolatingEndpoints()),
      worst_pin != nullptr ? network_->pathName(worst_pin) : "");

  debugPrint(logger_, RSZ, "memory", 1, "RSS = {}", utl::getCurrentRSS());
  if (end) {
    logger_->report(
        "---------------------------------------------------------------"
        "---------------------------------------------------------------");
  }
}

bool SetupLegacyPolicy::terminateProgress(const int iteration,
                                          const float initial_tns,
                                          float& prev_tns,
                                          float& fix_rate_threshold,
                                          const int endpt_index,
                                          const int num_endpts,
                                          const std::string_view phase_name,
                                          const char phase_marker)
{
  if (iteration % opto_large_interval_ == 0) {
    fix_rate_threshold *= 2.0;
  }
  if (iteration % opto_small_interval_ == 0) {
    float curr_tns = sta_->totalNegativeSlack(max_);
    float inc_fix_rate = (prev_tns - curr_tns) / initial_tns;
    prev_tns = curr_tns;
    if (iteration > 1000 && inc_fix_rate < fix_rate_threshold) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: Exiting at iteration {} because incr fix rate "
                 "{:0.2f}% is < {:0.2f}% [endpt {}/{}]",
                 phase_name,
                 phase_marker,
                 iteration,
                 inc_fix_rate * 100,
                 fix_rate_threshold * 100,
                 endpt_index,
                 num_endpts);
      return true;
    }
  }
  return false;
}

void SetupLegacyPolicy::reportCustomPhaseSetup() const
{
  logger_->info(
      utl::RSZ, 221, "Using custom phase sequence: {}", config_.phases);
}

void SetupLegacyPolicy::repairSetupWns(const float setup_slack_margin,
                                       const int max_passes_per_endpoint,
                                       const int max_repairs_per_pass,
                                       const bool verbose,
                                       int& opto_iteration,
                                       const float initial_tns,
                                       float& prev_tns,
                                       const bool use_cone_collection,
                                       const char phase_marker,
                                       const rsz::ViolatorSortType sort_type)
{
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      10,
      fmt::format("WNS{} Phase Time: {{}}", phase_marker));
  constexpr int max_no_progress = 4;
  committer_.capturePrePhaseSlack();
  overall_no_progress_count_ = 0;
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
  printProgress(opto_iteration, false, false, phase_marker);

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
    if (static_cast<int>(endpoint_pass_counts.size()) > max_end_repairs_) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "WNS{} Phase: Hit maximum endpoint repairs of {}",
                 phase_marker,
                 max_end_repairs_);
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
      printProgress(opto_iteration, true, false, phase_marker);
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
      overall_no_progress_count_++;
      if (overall_no_progress_count_ >= max_no_progress) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   1,
                   "WNS{} Phase: No TNS progress for {} cycles, exiting",
                   phase_marker,
                   overall_no_progress_count_);
        break;
      }
    }
    if (opto_iteration % opto_small_interval_ == 0) {
      overall_no_progress_count_ = 0;
    }

    sta::Slack prev_end_slack = 0.0;
    sta::Slack prev_worst_slack = 0.0;
    sta::Slack prev_tns_local = 0.0;
    if (total_decreasing_slack_passes == 0) {
      prev_end_slack = target_collector_->getCurrentEndpointSlack();
      prev_worst_slack = worst_slack;
      prev_tns_local = sta_->totalNegativeSlack(max_);
      fallback_ = false;
      committer_.beginJournal();
    } else {
      prev_end_slack = target_end_slack;
      prev_worst_slack = target_worst_slack;
      prev_tns_local = target_tns_local;
    }

    opto_iteration++;
    if (verbose || opto_iteration % print_interval_ == 0) {
      printProgress(opto_iteration, false, false, phase_marker);
    }

    std::vector<const sta::Pin*> viol_pins;
    if (use_cone_collection) {
      viol_pins = target_collector_->collectViolatorsByConeTraversal(
          current_endpoint, sort_type);
    } else {
      viol_pins = target_collector_->collectViolators(1, -1, sort_type);
    }
    sta::Path* focus_path = sta_->vertexWorstSlackPath(current_endpoint, max_);

    const int saved_max_repairs = max_repairs_per_pass_;
    if (use_cone_collection) {
      max_repairs_per_pass_ = viol_pins.size();
    } else {
      max_repairs_per_pass_ = max_repairs_per_pass;
    }

    std::vector<std::pair<const sta::Pin*, MoveType>> chosen_moves;
    const bool changed = repairPins(viol_pins,
                                    focus_path,
                                    &rejected_pin_moves_current_endpoint_,
                                    &chosen_moves);
    max_repairs_per_pass_ = saved_max_repairs;

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
      fallback_ = true;
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

  printProgress(opto_iteration, true, false, phase_marker);
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

void SetupLegacyPolicy::repairSetupTns(const float setup_slack_margin,
                                       const int max_passes_per_endpoint,
                                       const int max_repairs_per_pass,
                                       const bool verbose,
                                       int& opto_iteration,
                                       const char phase_marker,
                                       const rsz::ViolatorSortType sort_type)
{
  const utl::DebugScopedTimer timer(
      logger_,
      RSZ,
      "repair_setup",
      10,
      fmt::format("TNS{} Phase Time: {{}}", phase_marker));
  committer_.capturePrePhaseSlack();
  overall_no_progress_count_ = 0;
  target_collector_->resetEndpointPasses();
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "TNS{} Phase: Focusing on all violating endpoints...",
             phase_marker);
  printProgress(opto_iteration, false, false, phase_marker);

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
    if (endpoint_index >= max_end_repairs_) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "TNS{} Phase: Hit maximum endpoint repairs of {}",
                 phase_marker,
                 max_end_repairs_);
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
      const int saved_max_repairs = max_repairs_per_pass_;
      max_repairs_per_pass_ = max_repairs_per_pass;
      std::vector<std::pair<const sta::Pin*, MoveType>> chosen_moves;
      const bool changed = repairPins(viol_pins,
                                      focus_path,
                                      &rejected_pin_moves_current_endpoint_,
                                      &chosen_moves);
      max_repairs_per_pass_ = saved_max_repairs;

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
        fallback_ = true;
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
        printProgress(opto_iteration, true, true, phase_marker);
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
      printProgress(opto_iteration, false, false, phase_marker);
    }
  }

  printProgress(opto_iteration, true, false, phase_marker);
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

void SetupLegacyPolicy::repairSetupEndpointFanin(
    const float setup_slack_margin,
    const int max_passes_per_endpoint,
    const bool verbose,
    int& opto_iteration,
    const char phase_marker)
{
  repairSetupDirectional(false,
                         setup_slack_margin,
                         max_passes_per_endpoint,
                         verbose,
                         opto_iteration,
                         phase_marker);
}

void SetupLegacyPolicy::repairSetupStartpointFanout(
    const float setup_slack_margin,
    const int max_passes_per_startpoint,
    const bool verbose,
    int& opto_iteration,
    const char phase_marker)
{
  repairSetupDirectional(true,
                         setup_slack_margin,
                         max_passes_per_startpoint,
                         verbose,
                         opto_iteration,
                         phase_marker);
}

void SetupLegacyPolicy::repairSetupDirectional(const bool use_startpoints,
                                               const float setup_slack_margin,
                                               const int max_passes_per_point,
                                               const bool verbose,
                                               int& opto_iteration,
                                               const char phase_marker)
{
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
  printProgress(opto_iteration, false, false, phase_marker, use_startpoints);
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
    if (point_index >= max_end_repairs_) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: Hit maximum point repairs of {}",
                 phase_name,
                 phase_marker,
                 max_end_repairs_);
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
        printProgress(
            opto_iteration, false, false, phase_marker, use_startpoints);
      }

      std::vector<const sta::Pin*> viol_pins
          = target_collector_->collectViolatorsByDirectionalTraversal(
              use_startpoints,
              point_index,
              threshold,
              rsz::ViolatorSortType::SORT_BY_LOAD_DELAY);

      const int saved_max_repairs = max_repairs_per_pass_;
      max_repairs_per_pass_ = viol_pins.size();
      std::vector<std::pair<const sta::Pin*, MoveType>> chosen_moves;
      const bool changed = repairPins(viol_pins,
                                      nullptr,
                                      &rejected_pin_moves_current_endpoint_,
                                      &chosen_moves);
      max_repairs_per_pass_ = saved_max_repairs;
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

  printProgress(opto_iteration, true, false, phase_marker, use_startpoints);
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

bool SetupLegacyPolicy::initializeLastGaspRepair(
    const RepairSetupParams& params,
    const int opto_iteration,
    const float initial_tns,
    SetupLegacyPolicy::LastGaspState& last_gasp_state,
    SetupLegacyPolicy::ViolatingEnds& violating_ends)
{
  // Last-gasp intentionally narrows the move sequence to transforms that still
  // have a chance to improve slack without large topology changes.
  buildLastGaspMoveSequence(params);

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
  printProgress(last_gasp_state.opto_iteration,
                false,
                false,
                last_gasp_state.phase_marker);
  return true;
}

bool SetupLegacyPolicy::beginLastGaspEndpoint(
    const pair<sta::Vertex*, sta::Slack>& end_original_slack,
    SetupLegacyPolicy::LastGaspState& last_gasp_state,
    SetupLegacyPolicy::EndpointRepairState& endpoint_state)
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

bool SetupLegacyPolicy::lastGaspImproved(const sta::Slack worst_slack,
                                         const float curr_tns,
                                         const sta::Slack prev_worst_slack,
                                         const float prev_tns) const
{
  return sta::fuzzyGreaterEqual(worst_slack, prev_worst_slack)
         && sta::fuzzyGreaterEqual(curr_tns, prev_tns);
}

bool SetupLegacyPolicy::advanceLastGaspProgress(
    SetupLegacyPolicy::EndpointRepairState& endpoint_state,
    SetupLegacyPolicy::LastGaspState& last_gasp_state,
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
    fallback_ = true;
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

void SetupLegacyPolicy::repairLastGaspEndpoint(
    SetupLegacyPolicy::EndpointRepairState& endpoint_state,
    SetupLegacyPolicy::LastGaspState& last_gasp_state,
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
      printProgress(last_gasp_state.opto_iteration,
                    false,
                    false,
                    last_gasp_state.phase_marker);
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

bool SetupLegacyPolicy::shouldStopLastGasp(
    const SetupLegacyPolicy::LastGaspState& last_gasp_state,
    const int max_iterations) const
{
  return last_gasp_state.two_cons_terminations
         || reachedIterationLimit(last_gasp_state.opto_iteration,
                                  max_iterations);
}

void SetupLegacyPolicy::runLastGaspLoop(
    const SetupLegacyPolicy::ViolatingEnds& violating_ends,
    const RepairSetupParams& params,
    const int max_iterations,
    SetupLegacyPolicy::LastGaspState& last_gasp_state)
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
      printProgress(last_gasp_state.opto_iteration,
                    true,
                    false,
                    last_gasp_state.phase_marker);
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

void SetupLegacyPolicy::repairSetupLastGasp(const RepairSetupParams& params,
                                            int& num_viols,
                                            const int max_iterations,
                                            const int opto_iteration,
                                            const float initial_tns,
                                            const char phase_marker)
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
    return;
  }

  runLastGaspLoop(violating_ends, params, max_iterations, last_gasp_state);
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

bool SetupLegacyPolicy::swapVTCritCells(const RepairSetupParams& params,
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

void SetupLegacyPolicy::traverseFaninCone(
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

sta::Slack SetupLegacyPolicy::getInstanceSlack(sta::Instance* inst)
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

sta::Pin* SetupLegacyPolicy::worstOutputPin(sta::Instance* inst)
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
