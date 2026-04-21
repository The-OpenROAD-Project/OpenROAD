// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"

namespace sta {
class LibertyCell;
class LibertyPort;
}  // namespace sta

namespace rsz {

class Resizer;
class MoveCommitter;

// === Generator input bundles ===============================================

// Deliver necessary information from policy to MoveGenerator.
//
// Passing references through GeneratorContext replaces the pre-refactor
// pattern where each generator held a direct pointer to Resizer and reached
// into global state for options.  This keeps unit-level coupling explicit:
//   - resizer       : the shared rsz::Resizer (OpenDB + STA access)
//   - committer     : the run-scoped MoveCommitter (journal + accounting)
//   - run_config    : frozen user options (skip flags, margins, sequence)
//   - policy_config : policy-selected tunables (max_candidate_generation, ...)
// All four references outlive every generator, so generators may store
// references directly and do not need to copy or own any of these.
struct GeneratorContext
{
  Resizer& resizer;
  MoveCommitter& committer;  // For move conflict check in pending moves.
  const OptimizerRunConfig& run_config;
  const OptPolicyConfig& policy_config;
};

// === Move generator interface ==============================================

// Base class for one move type (Buffer, Clone, SizeUp, VtSwap, ...).
//
// Responsibilities:
//   1. prepareRequirements() -- declare which per-target PrepareCacheMask bits
//      this type reads.  The policy computes flags per target so OptPolicy
//      only prepares data for generators that can consume that target.  The
//      default says "I need nothing"; override in types that read prepared
//      Target fields.
//   2. isApplicable() -- caller-facing target filter.  The base method checks
//      requiredViews(); derived generators call it first, then add
//      move-specific legality checks.
//   3. generate() -- expand one Target into zero or more concrete
//      MoveCandidate objects.  Candidates are owned by unique_ptr and
//      returned up to the policy which decides which to estimate/commit.
//
// Thread-safety: generate()/isApplicable() run on worker threads for MT
// policies.  Implementations should read prepared Target fields and avoid STA
// analysis state that requires the main thread.
class MoveGenerator
{
 public:
  // === Construction and identity ===========================================
  explicit MoveGenerator(const GeneratorContext& context)
      : resizer_(context.resizer),
        committer_(context.committer),
        run_config_(context.run_config),
        policy_config_(context.policy_config)
  {
  }
  virtual ~MoveGenerator() = default;

  virtual MoveType type() const = 0;
  virtual const char* name() const { return moveName(type()); }

  // === Target preparation and generation ===================================
  virtual bool isApplicable(const Target& target) const
  {
    const TargetViewMask views = requiredViews();
    return ((views & kPathDriverView) != 0 && target.canBePathDriver())
           || ((views & kInstanceView) != 0 && target.canBeInstance());
  }

  virtual PrepareCacheMask prepareRequirements() const
  {
    return kNoPrepareCache;
  }
  virtual std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target)
      = 0;

 protected:
  // === Target-view requirements ============================================

  // Since kPathDriver is the most common, set it as the default required view.
  // If a derived MoveGenerator requires other views, override this.
  virtual TargetViewMask requiredViews() const { return kPathDriverView; }

  // === Shared Liberty-cell ordering helpers ================================
  const sta::LibertyPort* findScenePort(const sta::LibertyCell* cell,
                                        const std::string& port_name,
                                        int lib_ap) const;
  bool strongerCellLess(const sta::LibertyCell* lhs,
                        const sta::LibertyCell* rhs,
                        const std::string& drvr_port_name,
                        int lib_ap) const;

  // === Shared generator dependencies =======================================
  Resizer& resizer_;
  MoveCommitter& committer_;
  const OptimizerRunConfig& run_config_;
  const OptPolicyConfig& policy_config_;
};

}  // namespace rsz
