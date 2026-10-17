// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"

namespace rsz {

class Rebuffer;
class Resizer;

// MT form of Rebuffer.  It plans several buffer trees for one driver, and a
// winning candidate commits exactly the tree it scored.
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
  // Plans with the Resizer's planner on the main thread.
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  // The candidates of one target: tree plans of `net` from `rebuffer_planner`.
  std::vector<std::unique_ptr<MoveCandidate>> generateWith(
      const Target& target,
      Rebuffer& rebuffer_planner,
      BufferedNetPtr net) const;
  // One candidate per tree plan that passes the plan gates.
  void addTreePlanCandidates(
      const Target& target,
      const std::vector<RebufferTreePlan>& plans,
      std::vector<std::unique_ptr<MoveCandidate>>& candidates) const;
};

}  // namespace rsz
