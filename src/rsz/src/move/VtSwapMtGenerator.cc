// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "VtSwapMtGenerator.hh"

#include <memory>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupProfiler.hh"
#include "VtSwapMtCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"

namespace rsz {

VtSwapMtGenerator::VtSwapMtGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

std::vector<std::unique_ptr<MoveCandidate>> VtSwapMtGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  const EstimatorContext& estimator_context = target.estimator_context.value();
  const SelectedArc& target_arc = estimator_context.target().arc;
  // The arc holds the cell of its scene.  The Resizer caches, warmed before
  // the workers start, key on the link cell.
  sta::LibertyCell* current_cell
      = resizer_.network()->libertyCell(target.inst(resizer_));
  const std::vector<sta::LibertyCell*>& candidate_cells
      = candidate_cells_cache_.at(current_cell).cells;

  // Fanin max-cap headroom is instance-level, captured once per target.
  const std::vector<Resizer::InputCapBudget>& budgets = inputCapBudgets(target);
  candidates.reserve(candidate_cells.size());
  for (sta::LibertyCell* candidate_cell : candidate_cells) {
    if (policy_config_.dual_rf_drive_guard) {
      sta::LibertyCell* candidate_scene
          = candidate_cell->sceneCell(target_arc.scene, target_arc.min_max);
      sta::LibertyPort* candidate_output
          = candidate_scene != nullptr ? candidate_scene->findLibertyPort(
                                             target_arc.outputPort()->name())
                                       : nullptr;
      if (candidate_output != nullptr
          && !driveResNoHigherOnBothRf(target_arc.outputPort(),
                                       candidate_output)) {
        RepairSetupProfiler::event(
            "dual_rf_drive_guard_rejects", 1, moveName(MoveType::kVtSwap));
        continue;
      }
    }

    // The same decision as replacementPreservesMaxCap(), from the captured
    // headroom.
    if (!resizer_.replacementFitsInputCapBudgets(budgets, candidate_cell)) {
      RepairSetupProfiler::event(
          "max_cap_rejects", 1, moveName(MoveType::kVtSwap));
      continue;
    }

    candidates.push_back(std::make_unique<VtSwapMtCandidate>(
        resizer_, target, target.inst(resizer_), candidate_cell));
  }
  return candidates;
}

bool VtSwapMtGenerator::isApplicable(const Target& target) const
{
  // Screen out targets that cannot legally change VT in the current library
  // set.
  if (!MoveGenerator::isApplicable(target)
      || !target.estimator_context.has_value()) {
    return false;
  }

  sta::Instance* inst = target.inst(resizer_);
  sta::LibertyCell* current_cell = resizer_.network()->libertyCell(inst);
  if (inst == nullptr || current_cell == nullptr || resizer_.dontTouch(inst)
      || !resizer_.isLogicStdCell(inst) || resizer_.vtCategoryCount() < 2) {
    return false;
  }

  if (resizer_.dbNetwork()->staToDb(current_cell) == nullptr) {
    return false;
  }

  std::map<sta::LibertyCell*, CandidateCells>::iterator cached
      = candidate_cells_cache_.find(current_cell);
  if (cached == candidate_cells_cache_.end()) {
    const sta::LibertyCellSeq equiv_cells
        = resizer_.getVTEquivCells(current_cell);
    CandidateCells entry;
    entry.applicable = equiv_cells.size() > 1;
    entry.cells.reserve(equiv_cells.size());
    // Enumerate all VT-equivalent cells except the current implementation.
    for (sta::LibertyCell* cell : equiv_cells) {
      if (cell != nullptr && cell != current_cell) {
        entry.cells.push_back(cell);
      }
    }
    cached
        = candidate_cells_cache_.emplace(current_cell, std::move(entry)).first;
  }
  return cached->second.applicable;
}

}  // namespace rsz
