// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SetupWnsPolicy.hh"

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
#include "policy/OptimizationPolicy.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/GraphClass.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"
#include "utl/scope.h"
#include "utl/timer.h"

namespace rsz {

using std::pair;
using utl::RSZ;

namespace {
static constexpr int kDelayDigits = 3;

}  // namespace

void SetupWnsPolicy::iterate()
{
  buildMainMoveSequence(/*log_sequence=*/false);
  repairSetupWns(config_.setup_slack_margin,
                 config_.max_passes,
                 config_.max_repairs_per_pass,
                 config_.verbose,
                 /*use_cone_collection=*/use_cone_,
                 rsz::ViolatorSortType::SORT_BY_LOAD_DELAY);
  if (use_cone_) {
    committer_.printTrackerPhaseSummary(
        "WNS_CONE Phase Summary", "WNS_CONE Phase Endpoint Profiler", true);
  } else {
    committer_.printTrackerPhaseSummary(
        "WNS_PATH Phase Summary", "WNS_PATH Phase Endpoint Profiler", true);
  }
  markRunComplete(true);
}

void SetupWnsPolicy::repairSetupWns(const float setup_slack_margin,
                                    const int max_passes_per_endpoint,
                                    const int max_repairs_per_pass,
                                    const bool verbose,
                                    const bool use_cone_collection,
                                    const rsz::ViolatorSortType sort_type)
{
  int& opto_iteration = setup_context_.iteration;
  const float initial_tns = setup_context_.initial_tns;
  float& prev_tns = setup_context_.previous_tns;
  const char phase_marker = phaseMarkerForIndex(setup_context_.phase_index);
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
    if (std::cmp_greater(endpoint_pass_counts.size(),
                         setup_context_.max_end_repairs)) {
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

    std::vector<std::pair<const sta::Pin*, MoveType>> chosen_moves;
    bool changed;
    {
      const int new_max_repairs = use_cone_collection
                                      ? static_cast<int>(viol_pins.size())
                                      : max_repairs_per_pass;
      const bool force_single_repair = total_decreasing_slack_passes > 0;
      utl::SetAndRestore<int> override_max_repairs(
          setup_context_.max_repairs_per_pass, new_max_repairs);
      changed = repairPins(viol_pins,
                           focus_path,
                           &rejected_pin_moves_current_endpoint_,
                           &chosen_moves,
                           force_single_repair);
    }

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

}  // namespace rsz
