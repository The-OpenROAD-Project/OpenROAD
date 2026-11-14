// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "VtSwapMtGenerator.hh"

#include <cstddef>
#include <memory>
#include <optional>
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
  // Reuse the VT alternatives cached for this target's link cell.
  // The timing arc holds a scene cell, while the cache key uses the link cell
  // captured before workers start.
  sta::LibertyCell* current_cell
      = resizer_.network()->libertyCell(target.inst(resizer_));
  const CandidateCells& options = candidate_cells_cache_.at(current_cell);

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

  if (resizer_.dbNetwork()->staToDb(current_cell) == nullptr) {
    return false;
  }

  // Populate the candidate cache once for each current link cell.
  const std::pair<std::map<sta::LibertyCell*, CandidateCells>::iterator, bool>
      cached = candidate_cells_cache_.try_emplace(current_cell);
  if (cached.second) {  // New cache entry.
    const sta::LibertyCellSeq equiv_cells
        = resizer_.getVTEquivCells(current_cell);
    CandidateCells& entry = cached.first->second;  // Map iterator's value.
    entry.cells.reserve(equiv_cells.size());
    entry.cost_ratios.reserve(equiv_cells.size());
    // The current cell's costs, shared by every swap below.
    const CellCosts current_costs = cellCosts(current_cell);
    // Enumerate all VT-equivalent cells except the current implementation.
    for (sta::LibertyCell* cell : equiv_cells) {
      if (cell == nullptr || cell == current_cell) {
        continue;
      }
      // As for SizeUpMt, a cell without a cost on a metric both cells have is
      // not a candidate.  cellLeakage() fills a cache, so the costs are found
      // here on the main thread.
      const std::optional<float> cost_ratio
          = cellCostRatio(current_costs, cell);
      if (!cost_ratio) {
        continue;
      }
      entry.cells.push_back(cell);
      entry.cost_ratios.push_back(*cost_ratio);
    }
    entry.applicable = !entry.cells.empty();
  }
  return cached.first->second.applicable;  // Cached CandidateCells flag.
}

}  // namespace rsz
