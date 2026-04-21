// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "VtSwapGenerator.hh"

#include <memory>
#include <unordered_set>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "VtSwapCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/LibertyClass.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"

namespace rsz {

VtSwapGenerator::VtSwapGenerator(
    const GeneratorContext& context,
    std::unordered_set<sta::Instance*>* not_swappable)
    : MoveGenerator(context), not_swappable_(not_swappable)
{
}

bool VtSwapGenerator::isApplicable(const Target& target) const
{
  if (!MoveGenerator::isApplicable(target)) {
    return false;
  }

  const bool path_target = target.canBePathDriver();
  const bool instance_target
      = not_swappable_ != nullptr && target.canBeInstance();
  return path_target || instance_target;
}

std::vector<std::unique_ptr<MoveCandidate>> VtSwapGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  sta::Pin* drvr_pin = nullptr;
  sta::Instance* inst = nullptr;
  sta::LibertyCell* curr_cell = nullptr;
  sta::LibertyCell* best_cell = nullptr;
  if (!selectBestCell(target, drvr_pin, inst, curr_cell, best_cell)) {
    return candidates;
  }

  candidates.push_back(std::make_unique<VtSwapCandidate>(
      resizer_, target, drvr_pin, inst, curr_cell, best_cell));
  return candidates;
}

bool VtSwapGenerator::selectBestCell(const Target& target,
                                     sta::Pin*& drvr_pin,
                                     sta::Instance*& inst,
                                     sta::LibertyCell*& curr_cell,
                                     sta::LibertyCell*& best_cell) const
{
  if (target.canBePathDriver()) {
    return selectPathBestCell(target, drvr_pin, inst, curr_cell, best_cell);
  }
  return selectInstanceBestCell(target, inst, curr_cell, best_cell);
}

bool VtSwapGenerator::selectPathBestCell(const Target& target,
                                         sta::Pin*& drvr_pin,
                                         sta::Instance*& inst,
                                         sta::LibertyCell*& curr_cell,
                                         sta::LibertyCell*& best_cell) const
{
  drvr_pin = target.resolvedPin(resizer_);
  if (drvr_pin == nullptr) {
    return false;
  }

  inst = target.inst(resizer_);

  return inst != nullptr && resizer_.vtCategoryCount() >= 2
         && !resizer_.dontTouch(inst) && resizer_.isLogicStdCell(inst)
         && resolvePathCurrentCell(drvr_pin, curr_cell)
         && selectBestEquivCell(curr_cell, best_cell);
}

bool VtSwapGenerator::selectInstanceBestCell(const Target& target,
                                             sta::Instance*& inst,
                                             sta::LibertyCell*& curr_cell,
                                             sta::LibertyCell*& best_cell) const
{
  inst = target.inst(resizer_);
  curr_cell = resizer_.network()->libertyCell(inst);
  return curr_cell != nullptr
         && resizer_.checkAndMarkVTSwappable(inst, *not_swappable_, best_cell)
         && best_cell != nullptr;
}

bool VtSwapGenerator::resolvePathCurrentCell(sta::Pin* drvr_pin,
                                             sta::LibertyCell*& curr_cell) const
{
  sta::LibertyPort* drvr_port = resizer_.network()->libertyPort(drvr_pin);
  curr_cell = drvr_port != nullptr ? drvr_port->libertyCell() : nullptr;
  return curr_cell != nullptr
         && resizer_.dbNetwork()->staToDb(curr_cell) != nullptr;
}

bool VtSwapGenerator::selectBestEquivCell(sta::LibertyCell* curr_cell,
                                          sta::LibertyCell*& best_cell) const
{
  sta::LibertyCellSeq equiv_cells = resizer_.getVTEquivCells(curr_cell);
  best_cell = equiv_cells.empty() ? nullptr : equiv_cells.back();
  if (best_cell == curr_cell) {
    best_cell = nullptr;
  }
  return best_cell != nullptr;
}

}  // namespace rsz
