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

// Top-level driver for one repair_setup run.
//
// Lifecycle per call:
//   1. configure()/repairSetup() freezes user options into `config_`.
//   2. selectActivePolicy() picks exactly one OptPolicy instance based on the
//      RSZ_POLICY environment variable (falling back to SetupLegacyPolicy).
//   3. runActivePolicy() calls OptPolicy::start()/iterate() until the policy
//      reports converged(), then returns its result().
//
// The Optimizer owns `committer_` and `opt_policy_` for the full run; the
// policy reaches into the committer for ECO journaling and tracking, and the
// committer outlives the policy so final reports can be emitted after the
// policy object has been destroyed.  The Optimizer itself does not perform
// any timing analysis or ECO work -- all move logic lives in OptPolicy and
// its MoveGenerator/MoveCandidate collaborators.
class Optimizer
{
 public:
  // === Run lifecycle ========================================================
  explicit Optimizer(Resizer* resizer);
  ~Optimizer();

  void configure(const OptimizerRunConfig& config);
  bool run();
  bool converged() const;

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
  enum class PolicySelection
  {
    kLegacy,
    kLegacyMt,
    kMeasuredVtSwap,
    kVtSwapMt1
  };

  // === Policy orchestration =================================================
  PolicySelection selectPolicy() const;
  void selectActivePolicy();
  bool runActivePolicy();

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
  std::unique_ptr<OptPolicy> opt_policy_;
};

}  // namespace rsz
