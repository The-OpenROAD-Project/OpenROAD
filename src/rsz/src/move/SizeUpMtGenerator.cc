// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeUpMtGenerator.hh"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "SizeUpMtCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Transition.hh"

namespace rsz {

namespace {

const sta::LibertyPort* findScenePort(const sta::LibertyCell* cell,
                                      const std::string& port_name,
                                      const int lib_ap)
{
  if (cell == nullptr) {
    return nullptr;
  }

  const sta::LibertyPort* port = cell->findLibertyPort(port_name);
  return port != nullptr ? port->scenePort(lib_ap) : nullptr;
}

}  // namespace

SizeUpMtGenerator::SizeUpMtGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool SizeUpMtGenerator::isApplicable(const Target& target) const
{
  return target.isKind(TargetKind::kPathDriver) && target.isValid()
         && target.inst(resizer_) != nullptr
         && target.isPrepared(PrepareCacheKind::kArcDelayState);
}

std::vector<std::unique_ptr<MoveCandidate>> SizeUpMtGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  if (!isApplicable(target)) {
    return candidates;
  }

  sta::Instance* target_inst = target.inst(resizer_);
  const ArcDelayState& arc_delay = *target.arc_delay;
  const std::vector<sta::LibertyCell*> replacements = findSizeUpOptions(
      arc_delay.arc.output_port, arc_delay.arc.scene, arc_delay.arc.min_max);
  candidates.reserve(replacements.size());
  for (sta::LibertyCell* replacement : replacements) {
    if (!replacementPreservesMaxCap(target_inst, replacement)) {
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
      [=, this](const sta::LibertyCell* cell1, const sta::LibertyCell* cell2) {
        const sta::LibertyPort* port1
            = findScenePort(cell1, drvr_port_name, lib_ap);
        const sta::LibertyPort* port2
            = findScenePort(cell2, drvr_port_name, lib_ap);
        if ((port1 != nullptr) != (port2 != nullptr)) {
          return port1 != nullptr;
        }
        if (port1 == nullptr) {
          return cell1->name() < cell2->name();
        }
        const float drive1 = port1->driveResistance();
        const float drive2 = port2->driveResistance();
        const sta::ArcDelay intrinsic1
            = port1->intrinsicDelay(resizer_.staState());
        const sta::ArcDelay intrinsic2
            = port2->intrinsicDelay(resizer_.staState());
        const float capacitance1 = port1->capacitance();
        const float capacitance2 = port2->capacitance();
        return std::tie(drive2, intrinsic1, capacitance1)
               < std::tie(drive1, intrinsic2, capacitance2);
      });

  const sta::LibertyPort* scene_drvr_port
      = static_cast<const sta::LibertyPort*>(drvr_port)->scenePort(lib_ap);
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

bool SizeUpMtGenerator::replacementPreservesMaxCap(
    sta::Instance* inst,
    const sta::LibertyCell* replacement) const
{
  return replacement != nullptr && checkMaxCapViolation(inst, replacement);
}

float SizeUpMtGenerator::getInputPinCapacitance(
    sta::Pin* pin,
    const sta::LibertyCell* cell) const
{
  sta::LibertyPort* port = resizer_.network()->libertyPort(pin);
  if (port == nullptr) {
    return 0.0f;
  }

  sta::LibertyPort* cell_port = cell->findLibertyPort(port->name());
  if (cell_port == nullptr) {
    return 0.0f;
  }

  float cap = 0.0f;
  for (const sta::RiseFall* rf : sta::RiseFall::range()) {
    cap = std::max(cap, cell_port->capacitance(rf, resizer_.maxAnalysisMode()));
  }
  return cap;
}

bool SizeUpMtGenerator::checkMaxCapOK(const sta::Pin* drvr_pin,
                                      const float cap_delta) const
{
  float cap;
  float max_cap;
  float cap_slack;
  const sta::Scene* corner;
  const sta::RiseFall* tr;
  resizer_.sta()->checkCapacitance(drvr_pin,
                                   resizer_.sta()->scenes(),
                                   resizer_.maxAnalysisMode(),
                                   cap,
                                   max_cap,
                                   cap_slack,
                                   tr,
                                   corner);
  if (max_cap <= 0.0f || corner == nullptr) {
    return true;
  }

  const float new_cap = cap + cap_delta;
  if (cap_slack < 0.0f) {
    return new_cap <= cap;
  }
  return new_cap <= max_cap;
}

bool SizeUpMtGenerator::checkMaxCapViolation(
    sta::Instance* inst,
    const sta::LibertyCell* replacement) const
{
  // Reject replacements that would push any fanin net past its max capacitance
  // limit.
  sta::LibertyCell* current_cell = resizer_.network()->libertyCell(inst);
  if (current_cell == nullptr) {
    return true;
  }

  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      resizer_.network()->pinIterator(inst));
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    if (!resizer_.network()->direction(pin)->isAnyInput()) {
      continue;
    }

    sta::PinSet* drivers = resizer_.network()->drivers(pin);
    if (drivers == nullptr) {
      continue;
    }

    const float old_cap = getInputPinCapacitance(pin, current_cell);
    const float new_cap = getInputPinCapacitance(pin, replacement);
    const float cap_delta = new_cap - old_cap;
    if (cap_delta <= 0.0f) {
      continue;
    }

    for (const sta::Pin* driver_pin : *drivers) {
      if (!checkMaxCapOK(driver_pin, cap_delta)) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace rsz
