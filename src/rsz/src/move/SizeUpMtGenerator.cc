// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeUpMtGenerator.hh"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SizeUpMtCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"

namespace rsz {

SizeUpMtGenerator::SizeUpMtGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool SizeUpMtGenerator::isApplicable(const Target& target) const
{
  return MoveGenerator::isApplicable(target) && target.inst(resizer_) != nullptr
         && target.isPrepared(kArcDelayStateCache);
}

std::vector<std::unique_ptr<MoveCandidate>> SizeUpMtGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  if (!isApplicable(target)) {
    return candidates;
  }

  sta::Instance* target_inst = target.inst(resizer_);
  const ArcDelayState& arc_delay = target.arc_delay.value();
  const std::vector<sta::LibertyCell*> replacements = findSizeUpOptions(
      arc_delay.arc.output_port, arc_delay.arc.scene, arc_delay.arc.min_max);
  candidates.reserve(replacements.size());
  for (sta::LibertyCell* replacement : replacements) {
    if (!resizer_.replacementPreservesMaxCap(target_inst, replacement)) {
      continue;
    }

    candidates.push_back(std::make_unique<SizeUpMtCandidate>(resizer_,
                                                             target,
                                                             target.driver_pin,
                                                             target_inst,
                                                             replacement,
                                                             arc_delay));
  }
  return candidates;
}

std::vector<sta::LibertyCell*> SizeUpMtGenerator::findSizeUpOptions(
    const sta::LibertyPort* drvr_port,
    const sta::Scene* scene,
    const sta::MinMax* min_max) const
{
  std::vector<sta::LibertyCell*> replacements;
  if (drvr_port == nullptr || scene == nullptr || min_max == nullptr) {
    return replacements;
  }

  const int lib_ap = scene->libertyIndex(min_max);
  sta::LibertyCell* cell = drvr_port->libertyCell();
  sta::LibertyCellSeq swappable_cells = resizer_.getSwappableCells(cell);
  if (swappable_cells.empty()) {
    return replacements;
  }

  // Rank equivalent cells from strongest to weakest so the first legal win is
  // the best gain.
  const std::string& drvr_port_name = drvr_port->name();
  std::ranges::sort(
      swappable_cells.begin(),
      swappable_cells.end(),
      [this, &drvr_port_name, lib_ap](const sta::LibertyCell* cell1,
                                      const sta::LibertyCell* cell2) {
        return strongerCellLess(cell1, cell2, drvr_port_name, lib_ap);
      });

  const sta::LibertyPort* scene_drvr_port = drvr_port->scenePort(lib_ap);
  if (scene_drvr_port == nullptr) {
    return replacements;
  }

  const float drive = scene_drvr_port->driveResistance();
  // Keep every stronger equivalent cell and let candidate::estimate do the
  // exact timing check.
  odb::dbMaster* current_master = resizer_.dbNetwork()->staToDb(cell);
  for (sta::LibertyCell* swappable : swappable_cells) {
    odb::dbMaster* swappable_master = resizer_.dbNetwork()->staToDb(swappable);
    if (current_master == nullptr || swappable_master == nullptr) {
      continue;
    }

    // Keep size-up in the same VT class so VT changes stay owned by kVtSwap.
    if (resizer_.cellVTType(current_master).vt_index
        != resizer_.cellVTType(swappable_master).vt_index) {
      continue;
    }

    sta::LibertyCell* swappable_corner = swappable->sceneCell(lib_ap);
    if (swappable_corner == nullptr) {
      continue;
    }
    sta::LibertyPort* swappable_drvr
        = swappable_corner->findLibertyPort(drvr_port_name);
    if (swappable_drvr == nullptr) {
      continue;
    }
    const float swappable_drive = swappable_drvr->driveResistance();
    if (swappable_drive < drive) {
      replacements.push_back(swappable);
    }
  }
  return replacements;
}

}  // namespace rsz
