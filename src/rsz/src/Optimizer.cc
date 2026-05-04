// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "Optimizer.hh"

#include <memory>
#include <string>
#include <vector>

#include "OptPolicy.hh"
#include "OptimizerTypes.hh"
#include "SetupLegacyMtPolicy.hh"
#include "SetupLegacyPolicy.hh"
#include "dispatch.hh"
#include "est/EstimateParasitics.h"
#include "rsz/Resizer.hh"
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

// Returns true when the token list contains a phase that needs a prepared
// LegacyRepairContext (target collector, move sequence, STA preambles).
// Top-level tokens (MT1, MEASURED_VT_SWAP) are self-contained and do not
// need it.
bool tokensNeedLegacyContext(const std::vector<PhaseStep>& steps)
{
  for (const PhaseStep& step : steps) {
    if (step.name != "MT1" && step.name != "MEASURED_VT_SWAP") {
      return true;
    }
  }
  return false;
}

// Returns true when any LEGACY_MT token appears in the list.  Selects
// SetupLegacyMtPolicy as the legacy context so virtual buildMoveGenerators
// / tryRepairTarget calls dispatch to the MT overrides.
bool tokensRequestMtMode(const std::vector<PhaseStep>& steps)
{
  for (const PhaseStep& step : steps) {
    if (step.name == "LEGACY_MT") {
      return true;
    }
  }
  return false;
}

}  // namespace

bool Optimizer::run()
{
  // Keep the footprint override local to this optimizer run.
  utl::SetAndRestore<bool> match_cell_footprint_override(
      resizer_.matchCellFootprint(), config_.match_cell_footprint);

  resizer_.runRepairSetupPreamble();
  committer_.init();

  // Token list: user-supplied -policy/-phases/-policies takes precedence,
  // otherwise the default ("LEGACY LAST_GASP").
  const std::string token_list
      = !config_.phases.empty() ? config_.phases : kDefaultPhases;
  const std::vector<PhaseStep> steps = parsePhases(token_list);

  // Pick the legacy context class.  LEGACY_MT in the token list selects
  // SetupLegacyMtPolicy so phase wrappers' virtual buildMoveGenerators /
  // tryRepairTarget calls land on the MT overrides.
  std::unique_ptr<SetupLegacyPolicy> legacy_ctx
      = tokensRequestMtMode(steps)
            ? std::make_unique<SetupLegacyMtPolicy>(resizer_, committer_)
            : std::make_unique<SetupLegacyPolicy>(resizer_, committer_);
  // Wire base members (logger_, sta_, network_, graph_, ...) but do not
  // run the phase pipeline yet  -  that prep is gated on whether any phase
  // token is in the dispatch list.
  legacy_ctx->start(config_, nullptr);

  const bool legacy_prepared = tokensNeedLegacyContext(steps);

  // Cross-phase accumulators, owned by the sequencer.  Phase wrappers
  // mutate these through PhaseRunContext refs.
  int opto_iteration = 0;
  int num_viols = 0;
  float initial_tns = 0.0f;
  float prev_tns = 0.0f;

  // Incremental parasitics guard must outlive every phase that performs
  // estimateParasitics-aware repairs (= every phase token).
  std::unique_ptr<est::IncrementalParasiticsGuard> parasitics_guard;
  if (legacy_prepared) {
    parasitics_guard = std::make_unique<est::IncrementalParasiticsGuard>(
        resizer_.estimateParasitics());
    if (!legacy_ctx->prepareForPhasePipeline()) {
      // No violations to repair  -  early return (matches legacy runSetup
      // behavior on total_violations == 0).
      return false;
    }
    initial_tns
        = resizer_.sta()->totalNegativeSlack(resizer_.maxAnalysisMode());
    prev_tns = initial_tns;
  }

  // Dispatch loop  -  each token maps to an OptPolicy via
  // makeOptPolicyByToken().  Phase wrappers delegate back to legacy_ctx;
  // top-level policies are self-contained.
  for (const PhaseStep& step : steps) {
    PhaseRunContext ctx{.opto_iteration = opto_iteration,
                        .initial_tns = initial_tns,
                        .prev_tns = prev_tns,
                        .num_viols = num_viols,
                        .step = step};
    std::unique_ptr<OptPolicy> policy = makeOptPolicyByToken(
        step.name, resizer_, committer_, legacy_ctx.get());
    if (policy == nullptr) {
      // Public phase names only.  LEGACY_MT, MT1, MEASURED_VT_SWAP are
      // experimental tokens still accepted by makeOptPolicyByToken; they
      // are intentionally omitted here so users do not depend on them.
      resizer_.logger()->error(
          utl::RSZ,
          217,
          "Unknown phase name '{}'. Valid phase names are: LEGACY, WNS, "
          "WNS_PATH, WNS_CONE, TNS, ENDPOINT_FANIN, STARTPOINT_FANOUT, "
          "LAST_GASP",
          step.name);
    }
    policy->start(config_, &ctx);
    while (!policy->converged()) {
      policy->iterate();
    }
  }

  if (legacy_prepared) {
    legacy_ctx->runPostPhaseVtSwap(num_viols);
    return legacy_ctx->finalizeAndReport(opto_iteration);
  }
  // Top-level-only run  -  self-contained policies already reported.
  return true;
}

bool Optimizer::repairSetup(const double setup_margin,
                            const double repair_tns_end_percent,
                            const int max_passes,
                            const int max_iterations,
                            const int max_repairs_per_pass,
                            const bool match_cell_footprint,
                            const bool verbose,
                            const std::vector<MoveType>& sequence,
                            const char* const phases,
                            const bool skip_pin_swap,
                            const bool skip_gate_cloning,
                            const bool skip_size_down,
                            const bool skip_buffering,
                            const bool skip_buffer_removal,
                            const bool skip_last_gasp,
                            const bool skip_vt_swap,
                            const bool skip_crit_vt_swap)
{
  configure(makeRepairSetupConfig(setup_margin,
                                  repair_tns_end_percent,
                                  max_passes,
                                  max_iterations,
                                  max_repairs_per_pass,
                                  match_cell_footprint,
                                  verbose,
                                  sequence,
                                  phases,
                                  skip_pin_swap,
                                  skip_gate_cloning,
                                  skip_size_down,
                                  skip_buffering,
                                  skip_buffer_removal,
                                  skip_last_gasp,
                                  skip_vt_swap,
                                  skip_crit_vt_swap));
  return run();
}

bool Optimizer::repairSetup(const sta::Pin* const end_pin)
{
  // Single-endpoint repair bypasses the phase pipeline entirely; uses the
  // legacy context only as a helper facade with a fixed move sequence.
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
  policy.start(config);
  committer_.init();
  return policy.repairSetupPin(end_pin);
}

OptimizerRunConfig Optimizer::makeRepairSetupConfig(
    const double setup_margin,
    const double repair_tns_end_percent,
    const int max_passes,
    const int max_iterations,
    const int max_repairs_per_pass,
    const bool match_cell_footprint,
    const bool verbose,
    const std::vector<MoveType>& sequence,
    const char* phases,
    const bool skip_pin_swap,
    const bool skip_gate_cloning,
    const bool skip_size_down,
    const bool skip_buffering,
    const bool skip_buffer_removal,
    const bool skip_last_gasp,
    const bool skip_vt_swap,
    const bool skip_crit_vt_swap)
{
  OptimizerRunConfig config;
  // Freeze all Tcl-facing knobs into one immutable policy config object.
  config.setup_slack_margin = setup_margin;
  config.repair_tns_end_percent = repair_tns_end_percent;
  config.max_passes = max_passes;
  config.max_iterations = max_iterations;
  config.max_repairs_per_pass = max_repairs_per_pass;
  config.match_cell_footprint = match_cell_footprint;
  config.verbose = verbose;
  config.sequence = sequence;
  config.phases = phases != nullptr ? phases : "";
  config.skip_pin_swap = skip_pin_swap;
  config.skip_gate_cloning = skip_gate_cloning;
  config.skip_size_down = skip_size_down;
  config.skip_buffering = skip_buffering;
  config.skip_buffer_removal = skip_buffer_removal;
  config.skip_last_gasp = skip_last_gasp;
  config.skip_vt_swap = skip_vt_swap;
  config.skip_crit_vt_swap = skip_crit_vt_swap;
  return config;
}

MoveCommitter& Optimizer::committer()
{
  return committer_;
}

}  // namespace rsz
