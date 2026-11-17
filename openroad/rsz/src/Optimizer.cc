// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "Optimizer.hh"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "GlobalSizingPolicy.hh"
#include "OptimizationPolicy.hh"
#include "OptimizerTypes.hh"
#include "ProgressReporter.hh"
#include "RepairSetupContext.hh"
#include "RepairSetupProfiler.hh"
#include "RepairTargetCollector.hh"
#include "SetupCritVtSwapPolicy.hh"
#include "SetupDirectionalPolicy.hh"
#include "SetupLastGaspPolicy.hh"
#include "SetupLegacyBase.hh"
#include "SetupLegacyPolicy.hh"
#include "SetupParallelEvalPolicy.hh"
#include "SetupReroutePolicy.hh"
#include "SetupTnsPolicy.hh"
#include "SetupWnsPolicy.hh"
#include "est/EstimateParasitics.h"
#include "rsz/Resizer.hh"
#include "sta/Fuzzy.hh"
#include "sta/StringUtil.hh"
#include "utl/Logger.h"
#include "utl/scope.h"

namespace rsz {

Optimizer::Optimizer(Resizer* resizer)
    : resizer_(*resizer), committer_(resizer_)
{
}

Optimizer::~Optimizer() = default;

void Optimizer::configure(const OptimizerRunConfig& config)
{
  config_ = config;
}

namespace {

bool isLegacyCompatiblePhase(const std::string_view phase_name)
{
  return phase_name == "LEGACY" || phase_name == "WNS"
         || phase_name == "WNS_PATH" || phase_name == "WNS_CONE"
         || phase_name == "TNS" || phase_name == "ENDPOINT_FANIN"
         || phase_name == "STARTPOINT_FANOUT" || phase_name == "LAST_GASP";
}

bool containsPhase(const std::vector<std::string>& phase_names,
                   const std::string_view phase_name)
{
  for (const std::string& candidate : phase_names) {
    if (candidate == phase_name) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> appendImplicitCritVtSwapForLegacyPhases(
    const std::vector<std::string>& requested_phase_names)
{
  std::vector<std::string> effective_phase_names = requested_phase_names;
  bool needs_implicit_crit_vt_swap = false;
  for (const std::string& phase_name : requested_phase_names) {
    if (isLegacyCompatiblePhase(phase_name)) {
      needs_implicit_crit_vt_swap = true;
      break;
    }
  }
  if (needs_implicit_crit_vt_swap
      && !containsPhase(effective_phase_names, "CRIT_VT_SWAP")) {
    // Preserve the legacy setup-repair contract: any legacy-compatible phase
    // pipeline gets one post-phase critical VT swap unless the user already
    // requested CRIT_VT_SWAP explicitly.
    effective_phase_names.emplace_back("CRIT_VT_SWAP");
  }
  return effective_phase_names;
}

}  // namespace

std::unique_ptr<OptimizationPolicy> Optimizer::makePolicyForPhase(
    const std::string_view phase_name,
    RepairSetupContext& setup_context)
{
  if (phase_name == "LEGACY") {
    return std::make_unique<SetupLegacyPolicy>(
        resizer_, committer_, setup_context, config_);
  }
  if (phase_name == "WNS" || phase_name == "WNS_PATH") {
    return std::make_unique<SetupWnsPolicy>(
        resizer_, committer_, setup_context, config_, /*use_cone=*/false);
  }
  if (phase_name == "WNS_CONE") {
    return std::make_unique<SetupWnsPolicy>(
        resizer_, committer_, setup_context, config_, /*use_cone=*/true);
  }
  if (phase_name == "TNS") {
    return std::make_unique<SetupTnsPolicy>(
        resizer_, committer_, setup_context, config_);
  }
  if (phase_name == "ENDPOINT_FANIN") {
    return std::make_unique<SetupDirectionalPolicy>(
        resizer_, committer_, setup_context, config_, /*use_starts=*/false);
  }
  if (phase_name == "STARTPOINT_FANOUT") {
    return std::make_unique<SetupDirectionalPolicy>(
        resizer_, committer_, setup_context, config_, /*use_starts=*/true);
  }
  if (phase_name == "LAST_GASP") {
    return std::make_unique<SetupLastGaspPolicy>(
        resizer_, committer_, setup_context, config_);
  }
  if (phase_name == "CRIT_VT_SWAP") {
    return std::make_unique<SetupCritVtSwapPolicy>(
        resizer_, committer_, setup_context, config_);
  }
  if (phase_name == "REROUTE") {
    return std::make_unique<SetupReroutePolicy>(
        resizer_, committer_, setup_context, config_);
  }

  if (phase_name == "PARALLEL_EVAL") {
    return std::make_unique<SetupParallelEvalPolicy>(
        resizer_, committer_, setup_context, config_);
  }
  if (phase_name == "GLOBAL_SIZING") {
    return std::make_unique<GlobalSizingPolicy>(
        resizer_, committer_, setup_context, config_);
  }
  // Only the phases of the README are listed; the experimental PARALLEL_EVAL
  // has a README section.
  resizer_.logger()->error(
      utl::RSZ,
      217,
      "Unknown phase name '{}'. Valid phase names are: LEGACY, WNS, "
      "WNS_PATH, WNS_CONE, TNS, ENDPOINT_FANIN, STARTPOINT_FANOUT, "
      "LAST_GASP, CRIT_VT_SWAP, REROUTE, GLOBAL_SIZING, "
      "PARALLEL_EVAL (experimental)",
      phase_name);
  return nullptr;
}

bool Optimizer::run()
{
  RepairSetupProfiler profile(
      resizer_,
      static_cast<RepairSetupProfiler::Mode>(config_.profile_mode),
      config_.profile_dir);

  const ProfileScope optimizer_scope("optimizer_overhead");
  profile.runConfig(config_);
  // Keep the footprint override local to this optimizer run.
  utl::SetAndRestore<bool> match_cell_footprint_override(
      resizer_.matchCellFootprint(), config_.match_cell_footprint);

  // Common initialize
  {
    const ProfileScope scope("init_onetime");
    resizer_.runRepairSetupPreamble();
    committer_.init();
  }

  // Keep incremental parasitics enabled across the full optimizer run so every
  // policy sees the same ECO invalidation/update behavior.
  est::IncrementalParasiticsGuard parasitics_guard(
      resizer_.estimateParasitics());

  // Get the requested phase string from -phases/-policy/-policies, then
  // restore the legacy implicit critical-VT-swap tail when needed.
  const std::string token_list
      = !config_.phases.empty() ? config_.phases : kDefaultPhases;
  const std::vector<std::string> requested_phase_names
      = sta::parseTokens(token_list);
  const std::vector<std::string> phase_names
      = appendImplicitCritVtSwapForLegacyPhases(requested_phase_names);
  if (phase_names.empty()) {
    resizer_.logger()->error(
        utl::RSZ,
        223,
        "No phase names specified. Valid phase names are: LEGACY, WNS, "
        "WNS_PATH, WNS_CONE, TNS, ENDPOINT_FANIN, STARTPOINT_FANOUT, "
        "LAST_GASP, CRIT_VT_SWAP, REROUTE, GLOBAL_SIZING, "
        "PARALLEL_EVAL (experimental)");
  }
  const int phase_count = phase_names.size();

  // Phase loop - Run multiple policies sequentially.
  RepairSetupContext setup_context(resizer_);
  setup_context.phase_pipeline_active = true;
  // The last policy picks the layout of the closing progress row when no
  // header was printed.
  bool show_startpoint_metrics = false;
  for (int i = 0; i < phase_count; ++i) {
    const std::string& phase_name = phase_names[i];
    profile.beginPhase(phase_name, i + 1);
    setup_context.phase_index = i;
    std::unique_ptr<OptimizationPolicy> policy
        = makePolicyForPhase(phase_name, setup_context);

    {
      const ProfileScope scope("init_onetime");
      if (!policy->start()) {
        profile.complete(false);
        return false;
      }
    }
    while (!policy->converged()) {
      policy->iterate();
    }
    policy->finish();
    show_startpoint_metrics = policy->showStartpointMetrics();
  }

  // Final report
  const ProfileScope report_scope("final_reporting");
  const bool result = reportRunResult(setup_context, show_startpoint_metrics);
  profile.complete(result);

  return result;
}

bool Optimizer::reportRunResult(RepairSetupContext& setup_context,
                                const bool show_startpoint_metrics)
{
  // The closing row and the RSZ-62 verdict share one final WNS.
  RepairTargetCollector final_targets(&resizer_);
  final_targets.init(config_.setup_slack_margin,
                     setup_context.progress_header_show_startpoint_metrics);
  setup_context.final_wns = final_targets.getWns();
  const ProgressReporter progress(resizer_, committer_, setup_context);
  progress.printFinal(final_targets, show_startpoint_metrics);
  committer_.printTrackerFinalReports(
      setup_context.target_collector.getViolatingPins());
  return reportRepairSummary(setup_context);
}

bool Optimizer::reportRepairSummary(
    const RepairSetupContext& setup_context) const
{
  const ProfileScope profile("reporting");
  utl::Logger* logger = resizer_.logger();
  bool repaired = false;

  const int buffer_moves = committer_.summaryCommittedMoves(MoveType::kBuffer);
  const int size_up_moves = committer_.summaryCommittedMoves(MoveType::kSizeUp);
  const int size_down_fanout_moves
      = committer_.summaryCommittedMoves(MoveType::kSizeDownFanout);
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
  const int reroute_moves
      = committer_.summaryCommittedMoves(MoveType::kReroute);
  const int buffer_to_inverters_moves
      = committer_.summaryCommittedMoves(MoveType::kBufferToInverters);

  if (unbuffer_moves > 0) {
    repaired = true;
    logger->info(utl::RSZ, 59, "Removed {} buffers.", unbuffer_moves);
  }
  if (buffer_moves > 0 || split_load_moves > 0) {
    repaired = true;
    if (split_load_moves == 0) {
      logger->info(utl::RSZ, 40, "Inserted {} buffers.", buffer_moves);
    } else {
      logger->info(utl::RSZ,
                   45,
                   "Inserted {} buffers, {} to split loads.",
                   buffer_moves + split_load_moves,
                   split_load_moves);
    }
  }
  if (buffer_to_inverters_moves > 0) {
    repaired = true;
    logger->info(utl::RSZ,
                 109,
                 "Replaced {} buffers with inverters.",
                 buffer_to_inverters_moves);
  }
  logger->metric("design__instance__count__setup_buffer",
                 buffer_moves + split_load_moves);
  if (size_up_moves + size_down_fanout_moves + size_up_match_moves
          + vt_swap_moves
      > 0) {
    repaired = true;
    logger->info(utl::RSZ,
                 51,
                 "Resized {} instances: {} up, {} up match, {} down, {} VT",
                 size_up_moves + size_up_match_moves + size_down_fanout_moves
                     + vt_swap_moves,
                 size_up_moves,
                 size_up_match_moves,
                 size_down_fanout_moves,
                 vt_swap_moves);
  }
  if (swap_pins_moves > 0) {
    repaired = true;
    logger->info(
        utl::RSZ, 43, "Swapped pins on {} instances.", swap_pins_moves);
  }
  if (clone_moves > 0) {
    repaired = true;
    logger->info(utl::RSZ, 49, "Cloned {} instances.", clone_moves);
  }
  if (reroute_moves > 0) {
    repaired = true;
    logger->info(
        utl::RSZ, 53, "Rerouted {} nets resistance-aware.", reroute_moves);
  }

  if (sta::fuzzyLess(setup_context.final_wns, config_.setup_slack_margin)) {
    repaired = true;
    logger->warn(utl::RSZ, 62, "Unable to repair all setup violations.");
  }
  if (resizer_.overMaxArea()) {
    logger->error(utl::RSZ, 25, "max utilization reached.");
  }

  return repaired;
}

// Single-endpoint setup repair (test/debug) - bypasses the phase pipeline.
bool Optimizer::repairSetup(const sta::Pin* const end_pin)
{
  if (end_pin == nullptr) {
    return false;
  }

  OptimizerRunConfig config;
  config.setup_slack_margin = 0.0;
  config.max_repairs_per_pass = 1;
  config.match_cell_footprint = resizer_.matchCellFootprint();
  config.sequence = {MoveType::kUnbuffer,
                     MoveType::kVtSwap,
                     MoveType::kSizeDownFanout,
                     MoveType::kSizeUp,
                     MoveType::kSwapPins,
                     MoveType::kBuffer,
                     MoveType::kClone,
                     MoveType::kSplitLoad};

  utl::SetAndRestore<bool> match_cell_footprint_override(
      resizer_.matchCellFootprint(), config.match_cell_footprint);
  resizer_.runRepairSetupPreamble();

  RepairSetupContext setup_context(resizer_);
  SetupLegacyBase policy(resizer_, committer_, setup_context, config);
  if (!policy.start()) {
    return false;
  }
  committer_.init();
  return policy.repairSetupPin(end_pin);
}

MoveCommitter& Optimizer::committer()
{
  return committer_;
}

}  // namespace rsz
