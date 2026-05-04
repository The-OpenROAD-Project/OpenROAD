// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "OptimizerTypes.hh"
#include "dispatch.hh"

namespace sta {
class dbSta;
class Graph;
class MinMax;
class Network;
}  // namespace sta

namespace utl {
class Logger;
class ThreadPool;
}  // namespace utl

namespace est {
class EstimateParasitics;
}  // namespace est

namespace rsz {

struct GeneratorContext;
class MoveCommitter;
class MoveGenerator;
class Resizer;

// Abstract base for one repair_setup strategy.
//
// Each concrete policy is responsible for:
//   - Target selection      : what violating endpoint/path/instance to fix next
//   - Move-type ordering    : which MoveType values to try and in what
//                             priority (legacy vs. MT sequences differ)
//   - Convergence           : when to stop (slack target, iteration cap,
//                             two-consecutive-termination rule, etc.)
//   - Prepare/evaluate loop : whether to fill prepared Target fields for MT
//                             generators, or drive generators directly on the
//                             main thread (legacy path)
//
// Optimizer (sequencer) creates one or more OptPolicy instances per run and
// drives each through start(config, ctx) -> iterate()* -> converged()/result()
// in that order.  When `ctx` is non-null the sequencer is sharing cross-phase
// accumulators with the policy; legacy single-policy callers may pass nullptr
// and the policy keeps its own accumulators.  hooks() lets a policy opt in to
// sequencer-side log/profiler scopes and pre-iteration slack sampling.
//
// Shared helpers below (makeGeneratorContext, buildMoveGenerators,
// accumulatePrepareRequirements, findGenerator, prepareTargets,
// generatorEnabled) are intentionally placed on the base so the policies do
// not re-implement wiring boilerplate.
class OptPolicy
{
 public:
  using GeneratorVector = std::vector<std::unique_ptr<MoveGenerator>>;

  // === Policy interface =====================================================
  OptPolicy(Resizer& resizer, MoveCommitter& committer);
  virtual ~OptPolicy();

  virtual const char* name() const = 0;
  virtual void start(const OptimizerRunConfig& config, PhaseRunContext* ctx);
  // Convenience overload for legacy callers that do not need cross-phase
  // accumulator sharing.  Forwards to start(config, nullptr).
  void start(const OptimizerRunConfig& config) { start(config, nullptr); }
  virtual void iterate() = 0;

  bool converged() const { return converged_; }
  bool result() const { return result_; }

  // Hooks the sequencer applies around iterate().  Default returns an empty
  // PhaseHooks (no pre-slack capture, no log/profiler scopes).
  virtual PhaseHooks hooks() const { return {}; }

 protected:
  // === Generator setup ======================================================
  GeneratorContext makeGeneratorContext(
      const OptimizerRunConfig& run_config) const;
  virtual void buildMoveGenerators(const std::vector<MoveType>& move_types,
                                   const GeneratorContext& context);

  PrepareCacheMask accumulatePrepareRequirements() const;
  MoveGenerator* findGenerator(MoveType type) const;

  // Main-thread prewarm stage: populate shared Resizer/dbNetwork lazy caches
  // before MT workers call generator code that reads those services.
  virtual void prewarmTargets(const std::vector<Target>& targets) const;
  virtual bool targetPrewarmEnabled() const;
  virtual bool generatorEnabled(MoveType type) const;

  // Main-thread prepare stage: capture per-target STA state before MT workers
  // generate or estimate candidates.
  virtual void prepareTargets(std::vector<Target>& targets) const;
  Target prepareTarget(const Target& target) const;

  // Prepare one target according to the aggregated generator requirements.
  void prepareTarget(Target& target, PrepareCacheMask mask) const;

  // Build the immutable arc-delay snapshot consumed by MT-safe candidates.
  void prepareArcDelayState(Target& target) const;

  // Prewarm Liberty-cell caches used by MT generator candidate selection.
  void prewarmTargetLibertyCaches(const std::vector<Target>& targets,
                                  bool prewarm_swappable_cells,
                                  bool prewarm_vt_equiv_cells) const;

  // Prewarm dbNetwork driver caches touched by replacement legality checks.
  void prewarmTargetDriverCaches(const std::vector<Target>& targets) const;

  // === Run lifecycle helpers ===============================================
  bool hasSetupViolations(const OptimizerRunConfig& config,
                          const sta::MinMax* max) const;
  sta::Slack totalNegativeSlack(const sta::MinMax* max) const;
  void prewarmStaForPrepareStage() const;
  std::unique_ptr<utl::ThreadPool> makeWorkerThreadPool() const;

  // Load every policy-tunable envar into policy_config_ once at start time.
  // Each subclass reads only the fields it actually consumes from
  // policy_config_; loading unused fields is harmless.
  void loadPolicyEnvars();

  // Reset run state at the beginning of start(); call once per run.
  void resetRun()
  {
    converged_ = false;
    result_ = false;
  }

  // Mark the run as terminated with the given outcome; call from each
  // subclass's terminal path (for example, finishRun()).
  void markRunComplete(const bool result)
  {
    result_ = result;
    converged_ = true;
  }

  // === Shared policy state ==================================================
  Resizer& resizer_;
  MoveCommitter& committer_;
  OptimizerRunConfig config_;
  // Sequencer-supplied accumulator state; nullptr for legacy callers.  Phases
  // that need cross-phase accumulators read/write through this pointer.
  PhaseRunContext* run_ctx_{nullptr};
  utl::Logger* logger_{nullptr};
  sta::dbSta* sta_{nullptr};
  sta::Network* network_{nullptr};
  sta::Graph* graph_{nullptr};
  est::EstimateParasitics* estimate_parasitics_{nullptr};
  const sta::MinMax* max_{nullptr};
  OptPolicyConfig policy_config_{};
  GeneratorVector move_generators_;
  std::vector<MoveType> move_sequence_;
  std::unique_ptr<utl::ThreadPool> thread_pool_;
  bool converged_{false};
  bool result_{false};
};

}  // namespace rsz
