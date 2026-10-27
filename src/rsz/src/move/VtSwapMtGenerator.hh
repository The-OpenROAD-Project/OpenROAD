// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <vector>

#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class LibertyCell;
class Scene;
}  // namespace sta

namespace rsz {

// MT-safe generator that enumerates VT-equivalent cells and produces
// multiple VtSwapMtCandidate instances for parallel evaluation.
//
// prepareRequirements() asks for the estimator context and the flip-flop
// input-side checks, and prepareInputs() captures the input-cap budgets, so
// generate() runs on worker threads without live STA.
class VtSwapMtGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit VtSwapMtGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kVtSwap; }
  PrepareCacheMask prepareRequirements() const override
  {
    return kEstimatorContextCache | kSequentialEstimatorCache;
  }
  bool isApplicable(const Target& target) const override;
  void prepareInputs(Target& target) const override
  {
    prepareInputCapBudgets(target);
  }
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  // === Candidate-cell selection ============================================
  struct CandidateCells
  {
    bool applicable{false};
    std::vector<sta::LibertyCell*> cells;
  };

  // Filled by isApplicable() on the main thread before generate() workers.
  mutable std::map<sta::LibertyCell*, CandidateCells> candidate_cells_cache_;
};

}  // namespace rsz
