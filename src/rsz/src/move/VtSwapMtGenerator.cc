// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "VtSwapMtGenerator.hh"

#include <memory>
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
  if (!target.isPrepared(kEstimatorContextCache)) {
    return candidates;
  }

  const EstimatorContext& estimator_context = target.estimator_context.value();
  const SelectedArc& target_arc = estimator_context.target().arc;
  sta::LibertyCell* current_cell = estimator_context.target().arc.currentCell();
  const std::vector<sta::LibertyCell*> candidate_cells
      = selectCandidateCells(current_cell);

  // Fanin max-cap headroom is instance-level, captured once per target.
  const std::vector<Resizer::InputCapBudget> budgets = inputCapBudgets(target);
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
      || !target.isPrepared(kEstimatorContextCache)) {
    return false;
  }

  sta::Instance* inst = target.inst(resizer_);
  sta::LibertyCell* current_cell = resizer_.network()->libertyCell(inst);
  if (inst == nullptr || current_cell == nullptr || resizer_.dontTouch(inst)
      || !resizer_.isLogicStdCell(inst) || resizer_.vtCategoryCount() < 2) {
    return false;
  }

  if (resizer_.dbNetwork()->staToDb(current_cell) == nullptr
      || resizer_.getVTEquivCells(current_cell).size() <= 1) {
    return false;
  }
  return true;
}

std::vector<sta::LibertyCell*> VtSwapMtGenerator::selectCandidateCells(
    sta::LibertyCell* current_cell) const
{
  // Enumerate all VT-equivalent cells except the current implementation.
  sta::LibertyCellSeq equiv_cells = resizer_.getVTEquivCells(current_cell);
  std::vector<sta::LibertyCell*> candidates;
  candidates.reserve(equiv_cells.size());
  for (sta::LibertyCell* cell : equiv_cells) {
    if (cell != nullptr && cell != current_cell) {
      candidates.push_back(cell);
    }
  }
  return candidates;
}

}  // namespace rsz
