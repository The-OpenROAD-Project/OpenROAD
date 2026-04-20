// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "MoveCommitter.hh"
#include "OptPolicy.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class MinMax;
class Path;
class Pin;
class Vertex;
}  // namespace sta

namespace utl {
class Logger;
}

namespace rsz {

class MoveCandidate;
class MoveGenerator;

// Experimental fully-MT batched setup-repair policy (RSZ_POLICY=mt1).
//
// Unlike SetupLegacyMtPolicy (which preserves the legacy per-endpoint loop),
// SetupMt1Policy processes a *flat batch* of bottleneck targets per iteration:
//   1. findBottleneckTargets(): collect the worst-violating path-driver
//      instances across all endpoints.
//   2. prepareTargets(): snapshot delay context / load cap on each Target
//      on the main thread.
//   3. evaluateTargets(): walk prepared targets serially while dispatching
//      move-type and candidate fanout for each target to the ThreadPool.
//      Target-level parallelism is intentionally avoided because some
//      generator-side STA/network queries are not thread-safe.
//   4. commitEvaluatedTargets(): apply the best candidate for each target
//      on the main thread, sequentially, under one ECO journal per target.
//
// Convergence: stops when TNS stops improving, committed_moves_ reaches
// max_committed_moves, or no legal candidate is found.
//
// Currently supports VtSwapMt and SizeUpMt types only.
class SetupMt1Policy : public OptPolicy
{
 public:
  // === OptPolicy entry points ==============================================
  SetupMt1Policy(Resizer& resizer, MoveCommitter& committer);
  ~SetupMt1Policy() override;

  const char* name() const override { return "SetupMt1Policy"; }
  void start(const OptimizerRunConfig& config) override;
  void iterate() override;

 private:
  // === Run lifecycle and convergence =======================================
  bool finishIfNoValidTargetPin(const std::vector<Target>& targets);
  bool finishIfStopConditionReached(sta::Slack tns_before,
                                    sta::Slack tns_after);
  void finishRun(bool result);

  // === Move type configuration ============================================
  void buildMoveGenerators(const std::vector<MoveType>& move_types,
                           const GeneratorContext& context) override;

  // === Target selection =====================================================
  std::vector<Target> findBottleneckTargets() const;

  // === Parallel generation and scoring =====================================
  std::vector<TargetEvaluation> generateAndEstimateTargets(
      const std::vector<Target>& targets);
  TargetEvaluation generateAndEstimateTarget(const Target& target);
  CandidateVector generateCandidates(const Target& target);
  MoveCandidate* estimateCandidates(CandidateVector& candidates,
                                    std::vector<Estimate>& estimates,
                                    Estimate& best_estimate);

  // === Sequential commit stage =============================================
  int commitAndUpdateTiming(const std::vector<Target>& targets,
                            std::vector<TargetEvaluation>& evaluations);
  bool commitBestCandidate(MoveCandidate& best_candidate, const Target& target);

  // === MoveTracker reporting ===============================================
  void trackPreparedTargets(const std::vector<Target>& targets);
  void printTrackerIterationSummary();
  void printTrackerFinalReports();

  // === Iteration progress ===================================================
  int committed_moves_{0};
  int iteration_index_{0};
};

}  // namespace rsz
