// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "MoveCommitter.hh"
#include "OptimizationPolicy.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace rsz {

struct RepairSetupContext;

// Top-level driver and sequencer for one repair_setup run.
//
// Lifecycle per call:
//   1. configure() freezes user options into `config_`.
//   2. run() parses `config_.phases` (or the default token list when the
//      user did not supply -phases/-policy/-policies), prepares one shared
//      setup-repair context, then dispatches each token through
//      makePolicyForPhase. Every policy sees the same target collector and
//      tracker-report context.
//   3. reportRunResult() prints the final report after the last phase.
//
// The Optimizer owns `committer_` for the full run; the shared setup context
// lives for one run() call and each OptimizationPolicy for its own phase
// only.
class Optimizer
{
 public:
  // === Run lifecycle ========================================================
  explicit Optimizer(Resizer* resizer);
  ~Optimizer();

  void configure(const OptimizerRunConfig& config);
  bool run();

  // === repair_setup API =====================================================
  bool repairSetup(const sta::Pin* end_pin);

  // === Collaborator access ==================================================
  MoveCommitter& committer();

 private:
  // Default token list when the user did not supply -phases/-policy/-policies.
  static constexpr const char* kDefaultPhases = "LEGACY LAST_GASP";

  std::unique_ptr<OptimizationPolicy> makePolicyForPhase(
      std::string_view phase_name,
      RepairSetupContext& setup_context);

  // === Final report =========================================================
  // The report of the run, once after the last phase: the closing progress
  // row, the tracker reports and the move summary.  show_startpoint_metrics
  // picks the row layout when no progress header was printed.
  bool reportRunResult(RepairSetupContext& setup_context,
                       bool show_startpoint_metrics);
  // The committed moves and the RSZ-62 verdict.  Returns true when the run
  // committed a move or left a setup violation.
  bool reportRepairSummary(const RepairSetupContext& setup_context) const;

  // === Run state ============================================================
  OptimizerRunConfig config_;
  Resizer& resizer_;
  MoveCommitter committer_;
};

}  // namespace rsz
