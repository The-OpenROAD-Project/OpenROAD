// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "VtSwapMtGenerator.hh"

#include <cstddef>
#include <memory>
#include <optional>
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

void VtSwapMtGenerator::prepareInputs(Target& target) const
{
  prepareInputCapBudgets(target);

  // Find the cell costs here, so generate() on a worker only reads the cache.
  sta::LibertyCell* current_cell
      = resizer_.network()->libertyCell(target.inst(resizer_));
  if (!vt_swap_options_.contains(current_cell)) {
    vt_swap_options_.emplace(current_cell, buildVtSwapOptions(current_cell));
  }
}

VtSwapMtGenerator::VtSwapOptions VtSwapMtGenerator::buildVtSwapOptions(
    sta::LibertyCell* current_cell) const
{
  VtSwapOptions options;
  // The current cell's costs, shared by every swap below.
  const CellCosts current_costs = cellCosts(current_cell);
  for (sta::LibertyCell* candidate_cell : selectCandidateCells(current_cell)) {
    // As for SizeUpMt, a cell without a cost on a metric both cells have is
    // not a candidate.
    const std::optional<float> cost_ratio
        = cellCostRatio(current_costs, candidate_cell);
    if (!cost_ratio) {
      continue;
    }
    options.cells.push_back(candidate_cell);
    options.cost_ratios.push_back(*cost_ratio);
  }
  return options;
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
  const VtSwapOptions& options = vt_swap_options_.at(current_cell);

  // Fanin max-cap headroom is instance-level, captured once per target.
  const std::vector<Resizer::InputCapBudget>& budgets = inputCapBudgets(target);
  std::vector<sta::LibertyCell*> fitting;
  std::vector<float> cost_ratios;
  for (size_t i = 0; i < options.cells.size(); ++i) {
    sta::LibertyCell* candidate_cell = options.cells[i];
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

    fitting.push_back(candidate_cell);
    cost_ratios.push_back(options.cost_ratios[i]);
  }

  // The cheapest faster VT keeps its full score; a jump over more VTs pays
  // only its extra cost.
  const std::vector<float> cost_divisors = relativeCellCosts(cost_ratios);
  candidates.reserve(fitting.size());
  for (size_t i = 0; i < fitting.size(); ++i) {
    candidates.push_back(std::make_unique<VtSwapMtCandidate>(
        resizer_, target, target.inst(resizer_), fitting[i], cost_divisors[i]));
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
