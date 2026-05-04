// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCommitter.hh"
#include "OptPolicy.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace rsz {

class SetupLegacyPolicy;

// Top-level driver and sequencer for one repair_setup run.
//
// Lifecycle per call:
//   1. configure()/repairSetup() freezes user options into `config_`.
//   2. run() parses `config_.phases` (or the default token list when the
//      user did not supply -policy/-phases/-policies), picks the legacy
//      context class based on the tokens (single-thread by default,
//      SetupLegacyMtPolicy when LEGACY_MT appears), then dispatches each
//      token through makeOptPolicyByToken  -  phase wrappers delegate back to
//      the legacy context, top-level policies (MT1, MEASURED_VT_SWAP) are
//      self-contained.
//
// The Optimizer owns `committer_` for the full run; the legacy context and
// any phase-OptPolicy / top-level OptPolicy live for the duration of the
// dispatch loop only.
class Optimizer
{
 public:
  // === Run lifecycle ========================================================
  explicit Optimizer(Resizer* resizer);
  ~Optimizer();

  void configure(const OptimizerRunConfig& config);
  bool run();

  // === repair_setup API =====================================================
  bool repairSetup(double setup_margin,
                   double repair_tns_end_percent,
                   int max_passes,
                   int max_iterations,
                   int max_repairs_per_pass,
                   bool match_cell_footprint,
                   bool verbose,
                   const std::vector<MoveType>& sequence,
                   const char* phases,
                   bool skip_pin_swap,
                   bool skip_gate_cloning,
                   bool skip_size_down,
                   bool skip_buffering,
                   bool skip_buffer_removal,
                   bool skip_last_gasp,
                   bool skip_vt_swap,
                   bool skip_crit_vt_swap);
  bool repairSetup(const sta::Pin* end_pin);

  // === Collaborator access ==================================================
  MoveCommitter& committer();

 private:
  // Default token list when the user did not supply -policy/-phases/-policies.
  static constexpr const char* kDefaultPhases = "LEGACY LAST_GASP";

  // === Run configuration builders ==========================================
  static OptimizerRunConfig makeRepairSetupConfig(
      double setup_margin,
      double repair_tns_end_percent,
      int max_passes,
      int max_iterations,
      int max_repairs_per_pass,
      bool match_cell_footprint,
      bool verbose,
      const std::vector<MoveType>& sequence,
      const char* phases,
      bool skip_pin_swap,
      bool skip_gate_cloning,
      bool skip_size_down,
      bool skip_buffering,
      bool skip_buffer_removal,
      bool skip_last_gasp,
      bool skip_vt_swap,
      bool skip_crit_vt_swap);

  // === Run state ============================================================
  OptimizerRunConfig config_;
  Resizer& resizer_;
  MoveCommitter committer_;
};

}  // namespace rsz
