// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "OptimizerTypes.hh"

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
// Optimizer creates exactly one OptPolicy per run and calls
// start() -> iterate()* -> converged()/result() in that order.  Shared
// helpers below (makeGeneratorContext, buildMoveGenerators,
// accumulatePrepareRequirements, findGenerator, prepareTargets,
// generatorEnabled) are intentionally placed on the base so the four
// policies do not re-implement wiring boilerplate.
class OptPolicy
{
 public:
  using GeneratorVector = std::vector<std::unique_ptr<MoveGenerator>>;

  // === Policy interface =====================================================
  OptPolicy(Resizer& resizer, MoveCommitter& committer);
  virtual ~OptPolicy();

  virtual const char* name() const = 0;
  virtual void start(const OptimizerRunConfig& config);
  virtual void iterate() = 0;

  bool converged() const { return converged_; }
  bool result() const { return result_; }

 protected:
  // === Generator setup ======================================================
  GeneratorContext makeGeneratorContext(
      const OptimizerRunConfig& run_config) const;
  virtual void buildMoveGenerators(const std::vector<MoveType>& move_types,
                                   const GeneratorContext& context);

  PrepareCacheMask accumulatePrepareRequirements() const;
  MoveGenerator* findGenerator(MoveType type) const;
  virtual void prepareTargets(std::vector<Target>& targets) const;
  Target prepareTarget(const Target& target) const;
  virtual void prewarmTargets(const std::vector<Target>& targets) const;
  virtual bool targetPrewarmEnabled() const;
  virtual bool generatorEnabled(MoveType type) const;

  void prepareTarget(Target& target, PrepareCacheMask mask) const;
  void prepareArcDelayState(Target& target) const;
  void prewarmTargetLibertyCaches(const std::vector<Target>& targets,
                                  bool prewarm_swappable_cells,
                                  bool prewarm_vt_equiv_cells) const;
  void prewarmTargetDriverCaches(const std::vector<Target>& targets) const;

  // === Run lifecycle helpers ===============================================
  bool hasSetupViolations(const OptimizerRunConfig& config,
                          const sta::MinMax* max) const;
  sta::Slack totalNegativeSlack(const sta::MinMax* max) const;
  void prewarmStaForPrepareStage() const;
  std::unique_ptr<utl::ThreadPool> makeWorkerThreadPool() const;

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
