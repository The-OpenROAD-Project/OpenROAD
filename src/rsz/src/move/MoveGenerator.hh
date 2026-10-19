// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

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
//   - policy_config : policy-selected tunables
// All four references outlive every generator, so generators may store
// references directly and do not need to copy or own any of these.
struct GeneratorContext
{
  Resizer& resizer;
  MoveCommitter& committer;  // For move conflict check in pending moves.
  const OptimizerRunConfig& run_config;
  const OptimizationPolicyConfig& policy_config;
  // Load pins on the worst paths of violating endpoints, or null.  Side-load
  // models weight only these pins fully.
  const std::unordered_set<const sta::Pin*>* critical_load_pins{nullptr};
  // Equivalent input ports found so far in the run, for SwapPinsMt.
  EquivPinCache* equiv_pin_cache{nullptr};
};

// === Move generator interface ==============================================

// Base class for one move type (Buffer, Clone, SizeUp, VtSwap, ...).
//
// Responsibilities:
//   1. prepareRequirements() -- declare which per-target PrepareCacheMask bits
//      this type reads.  The policy computes flags per target so
//      OptimizationPolicy only prepares data for generators that can consume
//      that target.  The default says "I need nothing"; override in types that
//      read prepared Target fields.
//   2. isApplicable() -- cheap target filter, on the main thread before
//      prepareInputs().  The base method checks requiredViews(); derived
//      generators call it first, then add move-specific checks on the Target,
//      the committed moves and dont-touch state, so prepareInputs() runs only
//      for targets the move can use.
//   3. prepareInputs() -- the generator's own live STA reads for one target,
//      stored on the Target.  The policy calls it on the main thread right
//      before generate().  The default reads nothing.
//   4. generate() -- expand one Target into zero or more concrete
//      MoveCandidate objects.  Candidates are owned by unique_ptr and
//      returned up to the policy which decides which to estimate/commit.
//   5. startPhase()/finishPhase() -- set up and release the generator's
//      state for one phase, such as a planner per worker thread.  The default
//      keeps none.
//
// Thread-safety: generate() runs on worker threads in PARALLEL_EVAL, so it
// reads prepared Target fields, the netlist and Liberty but no live STA, and
// does not log.  isApplicable() and prepareInputs() run on the main thread.
// RebufferMtGenerator is the one exception to the generate() rules; its class
// comment gives the reason.
class MoveGenerator
{
 public:
  // === Construction and identity ===========================================
  explicit MoveGenerator(const GeneratorContext& context)
      : resizer_(context.resizer),
        committer_(context.committer),
        run_config_(context.run_config),
        policy_config_(context.policy_config),
        critical_load_pins_(context.critical_load_pins),
        equiv_pin_cache_(context.equiv_pin_cache)
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
  // Main thread, right before generate(): the live STA reads generate()
  // needs, stored on the target so generate() can run on a worker.
  virtual void prepareInputs(Target& target) const {}
  virtual std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) = 0;

  // === Phase lifecycle =====================================================
  // Main thread, from the policy's start(): until finishPhase(), up to
  // `worker_count` worker threads call generate() at once; with zero, the
  // main thread calls it.
  virtual void startPhase(size_t worker_count) {}
  // Main thread, from the policy's finish(), after the last generate().
  virtual void finishPhase() {}

 protected:
  // === Target-view requirements ============================================

  // Since kPathDriver is the most common, set it as the default required view.
  // If a derived MoveGenerator requires other views, override this.
  virtual TargetViewMask requiredViews() const { return kPathDriverView; }

  // === Shared Liberty-cell ordering helpers ================================
  const sta::LibertyPort* findScenePort(const sta::LibertyCell* cell,
                                        const std::string& port_name,
                                        int lib_ap_index) const;
  // Compare equivalent cells by drive strength in the requested library.
  bool weakerCellFirst(const sta::LibertyCell* lhs,
                       const sta::LibertyCell* rhs,
                       const std::string& drvr_port_name,
                       int lib_ap_index) const;

  // === Shared fanin max-cap helper =========================================
  // The cell-swap generators' fanin max-cap headroom, captured by their
  // prepareInputs(): the STA capacitance check behind it is main-thread only.
  // The second cell-swap generator of a target reuses it.
  void prepareInputCapBudgets(Target& target) const
  {
    if (!target.input_cap_budgets.has_value()) {
      target.input_cap_budgets
          = resizer_.inputCapBudgets(target.inst(resizer_));
    }
  }
  const std::vector<Resizer::InputCapBudget>& inputCapBudgets(
      const Target& target) const
  {
    return *target.input_cap_budgets;
  }

  // === Shared generator dependencies =======================================
  Resizer& resizer_;
  MoveCommitter& committer_;
  const OptimizerRunConfig& run_config_;
  const OptimizationPolicyConfig& policy_config_;
  // See GeneratorContext.
  const std::unordered_set<const sta::Pin*>* critical_load_pins_;
  EquivPinCache* equiv_pin_cache_;
};

}  // namespace rsz
