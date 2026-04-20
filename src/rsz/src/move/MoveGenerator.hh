// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"

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
//   1. prepareRequirements() -- declare which per-target PrepareCacheKind(s)
//      this type reads.  The policy aggregates flags from every active
//      generator into a single mask so OptPolicy only computes what the run
//      actually reads.  The default says "I need nothing"; override in types
//      that read prepared Target fields.
//   2. isApplicable() -- cheap yes/no filter the policy uses to skip this
//      type for a target before paying for generate().  Must not mutate state.
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
  virtual PrepareCacheMask prepareRequirements() const
  {
    return prepareCacheMask(PrepareCacheKind::kNone);
  }
  virtual bool isApplicable(const Target& target) const = 0;
  virtual std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target)
      = 0;

 protected:
  // === Shared generator dependencies =======================================
  Resizer& resizer_;
  MoveCommitter& committer_;
  const OptimizerRunConfig& run_config_;
  const OptPolicyConfig& policy_config_;
};

}  // namespace rsz
