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
// input-side checks, and prepareInputs() captures the input-cap budgets and
// the cell costs, so generate() runs on worker threads without live STA.
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
  void prepareInputs(Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  // The VT-equivalent cells of one current cell and their new/old cell cost
  // ratios.  The library does not change in a run, so the options are cached
  // for the run: prepareInputs() fills the cache on the main thread and
  // generate() only reads it.
  struct VtSwapOptions
  {
    std::vector<sta::LibertyCell*> cells;
    std::vector<float> cost_ratios;
  };

  // === Candidate-cell selection ============================================
  std::vector<sta::LibertyCell*> selectCandidateCells(
      sta::LibertyCell* current_cell) const;
  VtSwapOptions buildVtSwapOptions(sta::LibertyCell* current_cell) const;

  mutable std::map<const sta::LibertyCell*, VtSwapOptions> vt_swap_options_;
};

}  // namespace rsz
