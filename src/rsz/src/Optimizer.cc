// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "Optimizer.hh"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "MeasuredVtSwapPolicy.hh"
#include "OptPolicy.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupContext.hh"
#include "SetupCritVtSwapPolicy.hh"
#include "SetupDirectionalPolicy.hh"
#include "SetupLastGaspPolicy.hh"
#include "SetupLegacyBase.hh"
#include "SetupLegacyMtPolicy.hh"
#include "SetupLegacyPolicy.hh"
#include "SetupMt1Policy.hh"
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

// Returns true when the token list contains a phase that needs the legacy
// setup-repair preamble before phase dispatch. MT-only tokens still share the
// setup context, but they do their own target collection.
bool hasLegacyPhase(const std::vector<std::string>& phase_names)
{
  return std::any_of(phase_names.begin(),
                     phase_names.end(),
                     [](const std::string& phase_name) {
                       return phase_name != "MT1"
                              && phase_name != "MEASURED_VT_SWAP";
                     });
}

// Returns true when any LEGACY_MT token appears in the list.  Selects
// SetupLegacyMtPolicy as the legacy context so virtual buildMoveGenerators
// / tryRepairTarget calls dispatch to the MT overrides.
bool hasLegacyMtPhase(const std::vector<std::string>& phase_names)
{
  return std::any_of(
      phase_names.begin(),
      phase_names.end(),
      [](const std::string& phase_name) { return phase_name == "LEGACY_MT"; });
}

}  // namespace

std::unique_ptr<OptPolicy> Optimizer::makePolicyForPhase(
    const std::string_view phase_name,
    RepairSetupContext& setup_context)
{
  if (phase_name == "LEGACY") {
    return std::make_unique<SetupLegacyPolicy>(
        resizer_, committer_, setup_context);
  }
  if (phase_name == "LEGACY_MT") {
    return std::make_unique<SetupLegacyMtPolicy>(
        resizer_, committer_, setup_context);
  }
  if (phase_name == "WNS" || phase_name == "WNS_PATH") {
    return std::make_unique<SetupWnsPolicy>(
        resizer_, committer_, setup_context, /*use_cone=*/false);
  }
  if (phase_name == "WNS_CONE") {
    return std::make_unique<SetupWnsPolicy>(
        resizer_, committer_, setup_context, /*use_cone=*/true);
  }
  if (phase_name == "TNS") {
    return std::make_unique<SetupTnsPolicy>(
        resizer_, committer_, setup_context);
  }
  if (phase_name == "ENDPOINT_FANIN") {
    return std::make_unique<SetupDirectionalPolicy>(
        resizer_, committer_, setup_context, /*use_starts=*/false);
  }
  if (phase_name == "STARTPOINT_FANOUT") {
    return std::make_unique<SetupDirectionalPolicy>(
        resizer_, committer_, setup_context, /*use_starts=*/true);
  }
  if (phase_name == "LAST_GASP") {
    return std::make_unique<SetupLastGaspPolicy>(
        resizer_, committer_, setup_context);
  }
  if (phase_name == "CRIT_VT_SWAP") {
    return std::make_unique<SetupCritVtSwapPolicy>(
        resizer_, committer_, setup_context);
  }
  if (phase_name == "MT1") {
    return std::make_unique<SetupMt1Policy>(
        resizer_, committer_, setup_context);
  }
  if (phase_name == "MEASURED_VT_SWAP") {
    return std::make_unique<MeasuredVtSwapPolicy>(
        resizer_, committer_, setup_context);
  }
  // Only public phase names are listed; experimental top-level tokens
  // (LEGACY_MT, MT1, MEASURED_VT_SWAP) are accepted but undocumented.
  resizer_.logger()->error(
      utl::RSZ,
      217,
      "Unknown phase name '{}'. Valid phase names are: LEGACY, WNS, "
      "WNS_PATH, WNS_CONE, TNS, ENDPOINT_FANIN, STARTPOINT_FANOUT, "
      "LAST_GASP, CRIT_VT_SWAP",
      phase_name);
  return nullptr;
}

std::unique_ptr<SetupLegacyBase> Optimizer::makeLegacyContext(
    const std::vector<std::string>& phase_names,
    RepairSetupContext& setup_context)
{
  if (hasLegacyMtPhase(phase_names)) {
    return std::make_unique<SetupLegacyMtPolicy>(
        resizer_, committer_, setup_context);
  }
  return std::make_unique<SetupLegacyBase>(resizer_, committer_, setup_context);
}

bool Optimizer::run()
{
  // Keep the footprint override local to this optimizer run.
  utl::SetAndRestore<bool> match_cell_footprint_override(
      resizer_.matchCellFootprint(), config_.match_cell_footprint);

  // Common initialize
  resizer_.runRepairSetupPreamble();
  committer_.init();

  // Token list: user-supplied -phases/-policy/-policies takes precedence,
  // otherwise the default ("LEGACY LAST_GASP CRIT_VT_SWAP").
  const std::string token_list
      = !config_.phases.empty() ? config_.phases : kDefaultPhases;
  const std::vector<std::string> phase_names = sta::parseTokens(token_list);

  const bool has_legacy_phase = hasLegacyPhase(phase_names);
  RepairSetupContext setup_context(resizer_);
  setup_context.initial_design_area = resizer_.computeDesignArea();
  std::unique_ptr<SetupLegacyBase> setup_prepare_policy;
  if (has_legacy_phase) {
    setup_prepare_policy = makeLegacyContext(phase_names, setup_context);
    // Wire base members (logger_, sta_, network_, graph_, ...) but do not
    // run the phase pipeline yet; that prep is gated below.
    setup_prepare_policy->start(config_, nullptr);
  }

  // Keep incremental parasitics enabled across the full optimizer run so every
  // policy sees the same ECO invalidation/update behavior.
  est::IncrementalParasiticsGuard parasitics_guard(
      resizer_.estimateParasitics());
  if (setup_prepare_policy != nullptr
      && !setup_prepare_policy->prepareForPhasePipeline()) {
    // No violations to repair - early return
    return false;
  }

  // Shared optimization progress, owned by the sequencer and updated by
  // policies that participate in multi-phase repair.
  OptimizerProgress progress;
  progress.initial_tns
      = resizer_.sta()->totalNegativeSlack(resizer_.maxAnalysisMode());
  progress.previous_tns = progress.initial_tns;

  // Phase loop - Run multiple policies sequentially
  std::unique_ptr<OptPolicy> last_policy;
  const int phase_count = phase_names.size();
  for (int phase_index = 0; phase_index < phase_count; ++phase_index) {
    PhaseRunContext ctx{progress, phase_index};
    std::unique_ptr<OptPolicy> policy
        = makePolicyForPhase(phase_names[phase_index], setup_context);
    policy->start(config_, &ctx);
    while (!policy->converged()) {
      policy->iterate();
    }
    last_policy = std::move(policy);
  }

  // Final report
  OptPolicy* report_policy = setup_prepare_policy != nullptr
                                 ? setup_prepare_policy.get()
                                 : last_policy.get();
  bool include_progress_header = (setup_prepare_policy == nullptr);
  return report_policy->finalizeAndReport(setup_context.initial_design_area,
                                          include_progress_header);
}

// Single-endpoint setup repair for test/debug purpose
// - It bypasses the phase pipeline entirely
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
  setup_context.initial_design_area = resizer_.computeDesignArea();
  SetupLegacyBase policy(resizer_, committer_, setup_context);
  policy.start(config, nullptr);
  committer_.init();
  return policy.repairSetupPin(end_pin);
}

MoveCommitter& Optimizer::committer()
{
  return committer_;
}

}  // namespace rsz
