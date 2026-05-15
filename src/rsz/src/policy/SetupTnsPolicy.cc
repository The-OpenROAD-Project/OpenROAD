// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SetupTnsPolicy.hh"

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

void SetupTnsPolicy::iterate()
{
  buildMainMoveSequence(/*log_sequence=*/false);
  repairSetupTns(config_.setup_slack_margin,
                 config_.max_passes,
                 config_.max_repairs_per_pass,
                 config_.verbose,
                 rsz::ViolatorSortType::SORT_BY_LOAD_DELAY);
  committer_.printTrackerPhaseSummary(
      "TNS Phase Summary", "TNS Phase Endpoint Profiler", true);
  markRunComplete(true);
}

void SetupTnsPolicy::repairSetupTns(const float setup_slack_margin,
                                    const int max_passes_per_endpoint,
                                    const int max_repairs_per_pass,
                                    const bool verbose,
                                    const rsz::ViolatorSortType sort_type)
{
  int& opto_iteration = setup_context_.iteration;
  const char phase_marker = phaseMarkerForIndex(setup_context_.phase_index);
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
    bool force_single_repair = false;
    sta::Slack prev_endpoint_slack = endpoint_slack;

    committer_.beginJournal();
    bool journal_open = true;
    while (pass <= max_passes_per_endpoint) {
      std::vector<const sta::Pin*> viol_pins
          = target_collector_->collectViolators(1, -1, sort_type);
      sta::Path* focus_path = sta_->vertexWorstSlackPath(endpoint, max_);
      std::vector<std::pair<const sta::Pin*, MoveType>> chosen_moves;
      bool changed;
      {
        utl::SetAndRestore<int> override_max_repairs(
            setup_context_.max_repairs_per_pass, max_repairs_per_pass);
        changed = repairPins(viol_pins,
                             focus_path,
                             &rejected_pin_moves_current_endpoint_,
                             &chosen_moves,
                             force_single_repair);
      }

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
        force_single_repair = false;
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
        force_single_repair = true;
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

}  // namespace rsz
