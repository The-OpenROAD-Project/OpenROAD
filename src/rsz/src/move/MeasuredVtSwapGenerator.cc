// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "MeasuredVtSwapGenerator.hh"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include "MeasuredVtSwapCandidate.hh"
#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"

namespace rsz {

MeasuredVtSwapGenerator::MeasuredVtSwapGenerator(
    const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool MeasuredVtSwapGenerator::isApplicable(const Target& target) const
{
  return target.isKind(TargetKind::kPathDriver)
         && target.endpoint_path != nullptr
         && target.resolvedPin(resizer_) != nullptr
         && target.vertex(resizer_) != nullptr;
}

std::vector<std::unique_ptr<MoveCandidate>> MeasuredVtSwapGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;

  // Resolve the chosen stage once, then instantiate one candidate per
  // VT-equivalent cell.
  sta::Pin* driver_pin = nullptr;
  sta::Instance* inst = nullptr;
  sta::Vertex* driver_vertex = nullptr;
  const sta::Scene* scene = nullptr;
  sta::LibertyCell* current_cell = nullptr;
  if (!resolveTargetContext(
          target, driver_pin, inst, driver_vertex, scene, current_cell)) {
    return candidates;
  }

  for (sta::LibertyCell* candidate_cell : selectCandidateCells(current_cell)) {
    candidates.push_back(
        std::make_unique<MeasuredVtSwapCandidate>(resizer_,
                                                  target,
                                                  driver_pin,
                                                  inst,
                                                  driver_vertex,
                                                  scene,
                                                  current_cell,
                                                  candidate_cell));
  }
  return candidates;
}

bool MeasuredVtSwapGenerator::resolveTargetContext(
    const Target& target,
    sta::Pin*& driver_pin,
    sta::Instance*& inst,
    sta::Vertex*& driver_vertex,
    const sta::Scene*& scene,
    sta::LibertyCell*& current_cell) const
{
  // Filter out stages that are not swappable or already changed by this move
  // type.
  if (target.endpoint_path == nullptr) {
    return false;
  }

  driver_pin = target.resolvedPin(resizer_);
  driver_vertex = target.vertex(resizer_);
  scene = target.activeScene(resizer_);
  if (driver_pin == nullptr || driver_vertex == nullptr) {
    return false;
  }

  inst = target.inst(resizer_);
  if (inst == nullptr || resizer_.dontTouch(inst)
      || !resizer_.isLogicStdCell(inst) || resizer_.vtCategoryCount() < 2
      || committer_.hasMoves(MoveType::kVtSwap, inst)) {
    return false;
  }

  sta::LibertyPort* driver_port = resizer_.network()->libertyPort(driver_pin);
  current_cell = driver_port != nullptr ? driver_port->libertyCell() : nullptr;
  if (current_cell == nullptr
      || resizer_.dbNetwork()->staToDb(current_cell) == nullptr) {
    return false;
  }
  return scene != nullptr;
}

std::vector<sta::LibertyCell*> MeasuredVtSwapGenerator::selectCandidateCells(
    sta::LibertyCell* current_cell) const
{
  // Enumerate alternate VT implementations in deterministic library order.
  sta::LibertyCellSeq equiv_cells = resizer_.getVTEquivCells(current_cell);
  std::vector<sta::LibertyCell*> candidates;
  candidates.reserve(
      std::min(equiv_cells.size(),
               static_cast<size_t>(policy_config_.max_candidate_generation)));
  for (sta::LibertyCell* cell : equiv_cells) {
    if (cell != nullptr && cell != current_cell) {
      candidates.push_back(cell);
      if (candidates.size()
          >= static_cast<size_t>(policy_config_.max_candidate_generation)) {
        break;
      }
    }
  }

  return candidates;
}

}  // namespace rsz
