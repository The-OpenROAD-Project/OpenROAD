// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "Optimizer.hh"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "MeasuredVtSwapPolicy.hh"
#include "OptimizationPolicy.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupContext.hh"
#include "SetupCritVtSwapPolicy.hh"
#include "SetupDirectionalPolicy.hh"
#include "SetupLastGaspPolicy.hh"
#include "SetupLegacyBase.hh"
#include "SetupLegacyMtPolicy.hh"
#include "SetupLegacyPolicy.hh"
#include "SetupMt1Policy.hh"
#include "SetupReroutePolicy.hh"
#include "SetupTnsPolicy.hh"
#include "SetupWnsPolicy.hh"
#include "est/EstimateParasitics.h"
#include "rsz/Resizer.hh"
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
  return phase_name == "LEGACY" || phase_name == "LEGACY_MT"
         || phase_name == "WNS" || phase_name == "WNS_PATH"
         || phase_name == "WNS_CONE" || phase_name == "TNS"
         || phase_name == "ENDPOINT_FANIN" || phase_name == "STARTPOINT_FANOUT"
         || phase_name == "LAST_GASP";
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
  if (phase_name == "LEGACY_MT") {
    return std::make_unique<SetupLegacyMtPolicy>(
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
  if (phase_name == "MT1") {
    return std::make_unique<SetupMt1Policy>(
        resizer_, committer_, setup_context, config_);
  }
  if (phase_name == "MEASURED_VT_SWAP") {
    return std::make_unique<MeasuredVtSwapPolicy>(
        resizer_, committer_, setup_context, config_);
  }
  // Only public phase names are listed; experimental top-level tokens
  // (LEGACY_MT, MT1, MEASURED_VT_SWAP) are accepted but undocumented.
  resizer_.logger()->error(
      utl::RSZ,
      217,
      "Unknown phase name '{}'. Valid phase names are: LEGACY, WNS, "
      "WNS_PATH, WNS_CONE, TNS, ENDPOINT_FANIN, STARTPOINT_FANOUT, "
      "LAST_GASP, CRIT_VT_SWAP, REROUTE",
      phase_name);
  return nullptr;
}

bool Optimizer::run()
{
  // Keep the footprint override local to this optimizer run.
  utl::SetAndRestore<bool> match_cell_footprint_override(
      resizer_.matchCellFootprint(), config_.match_cell_footprint);

  // Common initialize
  resizer_.runRepairSetupPreamble();
  committer_.init();

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
        "LAST_GASP, CRIT_VT_SWAP, REROUTE");
  }
  const int phase_count = phase_names.size();

  // Phase loop - Run multiple policies sequentially.
  std::unique_ptr<OptimizationPolicy> last_policy;
  RepairSetupContext setup_context(resizer_);
  setup_context.phase_pipeline_active = true;
  for (int i = 0; i < phase_count; ++i) {
    setup_context.phase_index = i;
    std::unique_ptr<OptimizationPolicy> policy
        = makePolicyForPhase(phase_names[i], setup_context);
    if (!policy->start()) {
      return false;
    }
    while (!policy->converged()) {
      policy->iterate();
    }
    last_policy = std::move(policy);
  }

  // Final report
  return last_policy->finalizeAndReport(setup_context.initial_design_area);
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
                     MoveType::kSizeDown,
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
