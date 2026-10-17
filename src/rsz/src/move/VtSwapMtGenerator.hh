// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

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
// prepareRequirements() asks for kEstimatorContextCache and
// kInputCapBudgetCache, so generate() runs on worker threads without live
// STA.
class VtSwapMtGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit VtSwapMtGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kVtSwap; }
  PrepareCacheMask prepareRequirements() const override
  {
    return kEstimatorContextCache | kInputCapBudgetCache;
  }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  // === Candidate-cell selection ============================================
  std::vector<sta::LibertyCell*> selectCandidateCells(
      sta::LibertyCell* current_cell) const;
};

}  // namespace rsz
