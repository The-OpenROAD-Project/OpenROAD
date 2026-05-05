// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <string_view>
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
//   1. configure() freezes user options into `config_`.
//   2. run() parses `config_.phases` (or the default token list when the
//      user did not supply -phases/-policy/-policies), prepares a legacy
//      context only when a legacy phase is present, then dispatches each token
//      through makePolicyForPhase  -  phase wrappers delegate back to the
//      legacy context, top-level policies (MT1, MEASURED_VT_SWAP) are
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
  bool repairSetup(const sta::Pin* end_pin);

  // === Collaborator access ==================================================
  MoveCommitter& committer();

 private:
  // Default token list when the user did not supply -phases/-policy/-policies.
  static constexpr const char* kDefaultPhases = "LEGACY LAST_GASP";

  std::unique_ptr<OptPolicy> makePolicyForPhase(
      std::string_view token,
      SetupLegacyPolicy* legacy_parent);
  std::unique_ptr<SetupLegacyPolicy> makeLegacyContext(
      const std::vector<std::string>& phases);

  // === Run state ============================================================
  OptimizerRunConfig config_;
  Resizer& resizer_;
  MoveCommitter committer_;
};

}  // namespace rsz
