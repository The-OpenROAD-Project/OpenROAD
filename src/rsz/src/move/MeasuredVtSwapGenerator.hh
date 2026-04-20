// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/NetworkClass.hh"

namespace sta {
class LibertyCell;
class Vertex;
}  // namespace sta

namespace rsz {

// Builds VT-equivalent candidates for MeasuredVtSwapPolicy.
// (This is for experiment)
//
// selectCandidateCells() retrieves VT-equivalent Liberty cells for the current
// driver and caps the list at max_candidate_generation in library order.  This
// bounds the number of expensive incremental STA evaluations per target without
// reordering candidates.  Single-threaded only.
class MeasuredVtSwapGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit MeasuredVtSwapGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kVtSwap; }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  // === Target and candidate-cell selection =================================
  bool resolveTargetContext(const Target& target,
                            sta::Pin*& driver_pin,
                            sta::Instance*& inst,
                            sta::Vertex*& driver_vertex,
                            const sta::Scene*& scene,
                            sta::LibertyCell*& current_cell) const;
  std::vector<sta::LibertyCell*> selectCandidateCells(
      sta::LibertyCell* current_cell) const;
};

}  // namespace rsz
