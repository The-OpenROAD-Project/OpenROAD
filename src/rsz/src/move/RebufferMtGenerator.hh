// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "RebufferPlannerPool.hh"

namespace rsz {

class Rebuffer;
class Resizer;

// MT form of Rebuffer.  The main thread stages the driver net and its load
// timing.  A worker plans buffer trees with its own planner, and a winning
// candidate commits exactly the tree it scored.
//
// generate() is the one exception to the worker rules of MoveGenerator: the
// worker planner reads STA results, namely the worst-slack paths of the
// loads, the input slews of the driver, clock arrivals and slew limits.  The
// reads are safe because the main thread keeps the timing fixed while the
// workers generate, startPhase() creates the slew-limit checker on the main
// thread, and these queries never update the search.  The planner writes
// only its own state and the slack marks on the net copy that prepareInputs()
// built for the target.  The planner's debug prints (set_debug_level RSZ
// rebuffer) also come from the workers.
class RebufferMtGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit RebufferMtGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kBuffer; }
  const char* name() const override { return "RebufferMtMove"; }
  PrepareCacheMask prepareRequirements() const override
  {
    return kEstimatorContextCache;
  }
  bool isApplicable(const Target& target) const override;
  // Plans the prepared net with a worker planner from the pool, or with the
  // Resizer's planner when the phase has no worker threads.
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

  // The driver net as a buffered-net tree and, with rebuffer_mt_side_loads,
  // the current timing of its loads.
  void prepareInputs(Target& target) const override;

  // Readies the Resizer's planner for prepareInputs() and makes one planner
  // per worker thread.
  void startPhase(size_t worker_count) override;
  // Logs one line for the warnings the worker planners counted, then releases
  // them.
  void finishPhase() override;

 private:
  // Main-thread STA reads for every load of the target driver net.
  RebufferLoadTimingMap collectLoadTiming(const Target& target) const;
  // The tree plans of the target's prepared net that become candidates.
  std::vector<RebufferTreePlan> makeTreePlans(Rebuffer& rebuffer_planner,
                                              const Target& target) const;
  // One candidate per tree plan.
  void addTreePlanCandidates(
      const Target& target,
      const std::vector<RebufferTreePlan>& plans,
      std::vector<std::unique_ptr<MoveCandidate>>& candidates) const;

  // Empty outside a phase with worker threads.
  RebufferPlannerPool planners_;
};

}  // namespace rsz
