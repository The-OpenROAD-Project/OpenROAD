// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "Optimizer.hh"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "MeasuredVtSwapPolicy.hh"
#include "OptPolicy.hh"
#include "OptimizerTypes.hh"
#include "PhasePolicies.hh"
#include "SetupLegacyMtPolicy.hh"
#include "SetupLegacyPolicy.hh"
#include "SetupMt1Policy.hh"
#include "est/EstimateParasitics.h"
#include "rsz/Resizer.hh"
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

// Whitespace-tokenize the phase string.
std::vector<std::string> parsePhases(const std::string_view phases)
{
  std::vector<std::string> steps;
  std::istringstream stream{std::string(phases)};
  std::string token;
  while (stream >> token) {
    steps.push_back(token);
  }
  return steps;
}

// Returns true when the token list contains a phase that needs a prepared
// LegacyRepairContext (target collector, move sequence, STA preambles).
// Top-level tokens (MT1, MEASURED_VT_SWAP) are self-contained and do not
// need it.
bool hasLegacyPhase(const std::vector<std::string>& steps)
{
  return std::any_of(steps.begin(), steps.end(), [](const std::string& step) {
    return step != "MT1" && step != "MEASURED_VT_SWAP";
  });
}

// Returns true when any LEGACY_MT token appears in the list.  Selects
// SetupLegacyMtPolicy as the legacy context so virtual buildMoveGenerators
// / tryRepairTarget calls dispatch to the MT overrides.
bool hasLegacyMtPhase(const std::vector<std::string>& steps)
{
  return std::any_of(steps.begin(), steps.end(), [](const std::string& step) {
    return step == "LEGACY_MT";
  });
}

}  // namespace

std::unique_ptr<OptPolicy> Optimizer::makePolicyForPhase(
    const std::string_view token,
    SetupLegacyPolicy* const legacy_parent)
{
  // Single-thread vs MT behavior for LEGACY/LEGACY_MT is determined by the
  // legacy_parent class (SetupLegacyPolicy vs SetupLegacyMtPolicy) selected
  // by the sequencer before dispatch starts.
  if (token == "LEGACY" || token == "LEGACY_MT") {
    return std::make_unique<MainRepairPhasePolicy>(
        resizer_, committer_, legacy_parent);
  }
  if (token == "WNS" || token == "WNS_PATH") {
    return std::make_unique<WnsPhasePolicy>(
        resizer_, committer_, legacy_parent, /*use_cone=*/false);
  }
  if (token == "WNS_CONE") {
    return std::make_unique<WnsPhasePolicy>(
        resizer_, committer_, legacy_parent, /*use_cone=*/true);
  }
  if (token == "TNS") {
    return std::make_unique<TnsPhasePolicy>(
        resizer_, committer_, legacy_parent);
  }
  if (token == "ENDPOINT_FANIN") {
    return std::make_unique<DirectionalPhasePolicy>(
        resizer_, committer_, legacy_parent, /*use_starts=*/false);
  }
  if (token == "STARTPOINT_FANOUT") {
    return std::make_unique<DirectionalPhasePolicy>(
        resizer_, committer_, legacy_parent, /*use_starts=*/true);
  }
  if (token == "LAST_GASP") {
    return std::make_unique<LastGaspPhasePolicy>(
        resizer_, committer_, legacy_parent);
  }
  if (token == "CRIT_VT_SWAP") {
    return std::make_unique<CritVtSwapPhasePolicy>(
        resizer_, committer_, legacy_parent);
  }
  if (token == "MT1") {
    return std::make_unique<SetupMt1Policy>(resizer_, committer_);
  }
  if (token == "MEASURED_VT_SWAP") {
    return std::make_unique<MeasuredVtSwapPolicy>(resizer_, committer_);
  }
  // Only public phase names are listed; experimental top-level tokens
  // (LEGACY_MT, MT1, MEASURED_VT_SWAP) are accepted but undocumented.
  resizer_.logger()->error(
      utl::RSZ,
      217,
      "Unknown phase name '{}'. Valid phase names are: LEGACY, WNS, "
      "WNS_PATH, WNS_CONE, TNS, ENDPOINT_FANIN, STARTPOINT_FANOUT, "
      "LAST_GASP, CRIT_VT_SWAP",
      token);
  return nullptr;
}

std::unique_ptr<SetupLegacyPolicy> Optimizer::makeLegacyContext(
    const std::vector<std::string>& phases)
{
  if (hasLegacyMtPhase(phases)) {
    return std::make_unique<SetupLegacyMtPolicy>(resizer_, committer_);
  }
  return std::make_unique<SetupLegacyPolicy>(resizer_, committer_);
}

bool Optimizer::run()
{
  // Keep the footprint override local to this optimizer run.
  utl::SetAndRestore<bool> match_cell_footprint_override(
      resizer_.matchCellFootprint(), config_.match_cell_footprint);

  // Common initialize
  resizer_.runRepairSetupPreamble();
  committer_.init();
  const double initial_design_area = resizer_.computeDesignArea();

  // Token list: user-supplied -phases/-policy/-policies takes precedence,
  // otherwise the default ("LEGACY LAST_GASP CRIT_VT_SWAP").
  const std::string token_list
      = !config_.phases.empty() ? config_.phases : kDefaultPhases;
  const std::vector<std::string> steps = parsePhases(token_list);

  std::unique_ptr<SetupLegacyPolicy> legacy_ctx;
  if (hasLegacyPhase(steps)) {
    legacy_ctx = makeLegacyContext(steps);
    // Wire base members (logger_, sta_, network_, graph_, ...) but do not
    // run the phase pipeline yet; that prep is gated below.
    legacy_ctx->start(config_, nullptr);
  }

  // Keep incremental parasitics enabled across the full optimizer run so every
  // policy sees the same ECO invalidation/update behavior.
  est::IncrementalParasiticsGuard parasitics_guard(
      resizer_.estimateParasitics());
  if (legacy_ctx != nullptr && !legacy_ctx->prepareForPhasePipeline()) {
    // No violations to repair  -  early return (matches legacy runSetup
    // behavior on total_violations == 0).
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
  for (std::size_t phase_index = 0; phase_index < steps.size(); ++phase_index) {
    const std::string& step = steps[phase_index];
    PhaseRunContext ctx{.progress = progress,
                        .phase_index = static_cast<int>(phase_index)};
    std::unique_ptr<OptPolicy> policy
        = makePolicyForPhase(step, legacy_ctx.get());
    policy->start(config_, &ctx);
    while (!policy->converged()) {
      policy->iterate();
    }
    last_policy = std::move(policy);
  }

  OptPolicy* report_policy
      = legacy_ctx != nullptr ? legacy_ctx.get() : last_policy.get();
  return report_policy->finalizeAndReport(initial_design_area,
                                          /*include_progress_header=*/
                                          legacy_ctx == nullptr);
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

  SetupLegacyPolicy policy(resizer_, committer_);
  policy.start(config, nullptr);
  committer_.init();
  return policy.repairSetupPin(end_pin);
}

MoveCommitter& Optimizer::committer()
{
  return committer_;
}

}  // namespace rsz
