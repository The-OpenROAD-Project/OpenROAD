// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "VtSwapMtGenerator.hh"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "VtSwapMtCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"

namespace rsz {

VtSwapMtGenerator::VtSwapMtGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

std::vector<std::unique_ptr<MoveCandidate>> VtSwapMtGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  if (!target.isPrepared(PrepareCacheKind::kArcDelayState)) {
    return candidates;
  }

  const ArcDelayState& arc_delay = *target.arc_delay;
  sta::LibertyCell* current_cell
      = const_cast<sta::LibertyPort*>(arc_delay.arc.output_port)->libertyCell();
  const std::vector<sta::LibertyCell*> candidate_cells
      = selectCandidateCells(current_cell);
  candidates.reserve(candidate_cells.size());
  for (sta::LibertyCell* candidate_cell : candidate_cells) {
    candidates.push_back(
        std::make_unique<VtSwapMtCandidate>(resizer_,
                                            target,
                                            target.driver_pin,
                                            target.inst(resizer_),
                                            current_cell,
                                            candidate_cell,
                                            arc_delay));
  }
  return candidates;
}

bool VtSwapMtGenerator::isApplicable(const Target& target) const
{
  // Screen out targets that cannot legally change VT in the current library
  // set.
  if (!target.isKind(TargetKind::kPathDriver) || !target.isValid()
      || !target.isPrepared(PrepareCacheKind::kArcDelayState)) {
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

  if (candidates.size()
      <= static_cast<size_t>(policy_config_.max_candidate_generation)) {
    return candidates;
  }

  // Keep the leakage-ascending order returned by getVTEquivCells() and trim
  // only the suffix.
  candidates.resize(policy_config_.max_candidate_generation);
  return candidates;
}

}  // namespace rsz
