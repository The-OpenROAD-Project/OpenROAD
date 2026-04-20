// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeUpGenerator.hh"

#include <algorithm>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "SizeUpCandidate.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
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

SizeUpGenerator::SizeUpGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool SizeUpGenerator::isApplicable(const Target& target) const
{
  return target.isKind(TargetKind::kPathDriver)
         && target.endpoint_path != nullptr;
}

std::vector<std::unique_ptr<MoveCandidate>> SizeUpGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  // Build one conventional size-up candidate from the active timing stage.
  sta::Pin* drvr_pin = nullptr;
  sta::Instance* inst = nullptr;
  sta::LibertyPort* drvr_port = nullptr;
  if (!resolveDriverContext(target, drvr_pin, inst, drvr_port)) {
    return candidates;
  }

  const sta::Scene* scene = nullptr;
  const sta::MinMax* min_max = nullptr;
  float load_cap = 0.0f;
  float prev_drive = 0.0f;
  sta::LibertyPort* in_port = nullptr;
  if (!loadStageContext(
          target, drvr_pin, scene, min_max, load_cap, in_port, prev_drive)) {
    return candidates;
  }

  sta::LibertyCell* replacement = selectReplacement(
      in_port, drvr_port, load_cap, prev_drive, scene, min_max);
  if (replacement == nullptr
      || !replacementPreservesMaxCap(inst, replacement)) {
    return candidates;
  }

  candidates.push_back(std::make_unique<SizeUpCandidate>(
      resizer_, target, drvr_pin, inst, replacement));
  return candidates;
}

bool SizeUpGenerator::resolveDriverContext(const Target& target,
                                           sta::Pin*& drvr_pin,
                                           sta::Instance*& inst,
                                           sta::LibertyPort*& drvr_port) const
{
  drvr_pin = target.driver_pin;
  if (drvr_pin == nullptr) {
    drvr_pin = target.endpoint_path->pin(resizer_.staState());
  }
  if (drvr_pin == nullptr) {
    return false;
  }

  inst = resizer_.network()->instance(drvr_pin);
  if (inst == nullptr || resizer_.dontTouch(inst)
      || !resizer_.isLogicStdCell(inst)) {
    return false;
  }

  drvr_port = resizer_.network()->libertyPort(drvr_pin);
  return drvr_port != nullptr;
}

bool SizeUpGenerator::loadStageContext(const Target& target,
                                       sta::Pin* drvr_pin,
                                       const sta::Scene*& scene,
                                       const sta::MinMax*& min_max,
                                       float& load_cap,
                                       sta::LibertyPort*& in_port,
                                       float& prev_drive) const
{
  // Use the current path stage and the upstream driver to estimate delay gain.
  scene = target.activeScene(resizer_);
  min_max = target.minMax(resizer_);
  load_cap
      = resizer_.sta()->graphDelayCalc()->loadCap(drvr_pin, scene, min_max);

  const sta::Path* in_path = target.inputPath(resizer_);
  const sta::Path* prev_drvr_path = target.prevDriverPath(resizer_);
  if (in_path == nullptr) {
    return false;
  }

  sta::Pin* in_pin = in_path->pin(resizer_.staState());
  in_port
      = in_pin != nullptr ? resizer_.network()->libertyPort(in_pin) : nullptr;
  if (in_port == nullptr) {
    return false;
  }

  prev_drive = 0.0f;
  if (prev_drvr_path != nullptr) {
    sta::Pin* prev_drvr_pin = prev_drvr_path->pin(resizer_.staState());
    sta::LibertyPort* prev_drvr_port
        = prev_drvr_pin != nullptr
              ? resizer_.network()->libertyPort(prev_drvr_pin)
              : nullptr;
    if (prev_drvr_port != nullptr) {
      prev_drive = prev_drvr_port->driveResistance();
    }
  }
  return true;
}

sta::LibertyCell* SizeUpGenerator::selectReplacement(
    sta::LibertyPort* in_port,
    sta::LibertyPort* drvr_port,
    const float load_cap,
    const float prev_drive,
    const sta::Scene* scene,
    const sta::MinMax* min_max) const
{
  return upsizeCell(in_port, drvr_port, load_cap, prev_drive, scene, min_max);
}

bool SizeUpGenerator::replacementPreservesMaxCap(
    sta::Instance* inst,
    const sta::LibertyCell* replacement) const
{
  return replacement != nullptr && checkMaxCapViolation(inst, replacement);
}

float SizeUpGenerator::getInputPinCapacitance(
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

bool SizeUpGenerator::checkMaxCapOK(const sta::Pin* drvr_pin,
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

bool SizeUpGenerator::checkMaxCapViolation(
    sta::Instance* inst,
    const sta::LibertyCell* replacement) const
{
  // Reject replacements that overload any fanin net of the resized instance.
  sta::LibertyCell* current_cell = resizer_.network()->libertyCell(inst);
  if (current_cell == nullptr) {
    return true;
  }

  auto pin_iter = std::unique_ptr<sta::InstancePinIterator>(
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

sta::LibertyCell* SizeUpGenerator::upsizeCell(sta::LibertyPort* in_port,
                                              sta::LibertyPort* drvr_port,
                                              const float load_cap,
                                              const float prev_drive,
                                              const sta::Scene* scene,
                                              const sta::MinMax* min_max) const
{
  if (scene == nullptr || min_max == nullptr || in_port == nullptr
      || drvr_port == nullptr) {
    return nullptr;
  }

  const int lib_ap = scene->libertyIndex(min_max);
  sta::LibertyCell* cell = drvr_port->libertyCell();
  sta::LibertyCellSeq swappable_cells = resizer_.getSwappableCells(cell);
  if (swappable_cells.empty()) {
    return nullptr;
  }

  // Evaluate stronger equivalent cells first so the search stops on the best
  // local replacement.
  const std::string& in_port_name = in_port->name();
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
  const sta::LibertyPort* scene_input_port
      = static_cast<const sta::LibertyPort*>(in_port)->scenePort(lib_ap);
  if (scene_drvr_port == nullptr || scene_input_port == nullptr) {
    return nullptr;
  }

  const float drive = scene_drvr_port->driveResistance();
  const float delay = resizer_.gateDelay(drvr_port, load_cap, scene, min_max)
                      + prev_drive * scene_input_port->capacitance();

  // Accept the first cell that improves both drive resistance and estimated
  // stage delay.
  for (sta::LibertyCell* swappable : swappable_cells) {
    sta::LibertyCell* swappable_corner = swappable->sceneCell(lib_ap);
    if (swappable_corner == nullptr) {
      continue;
    }
    sta::LibertyPort* swappable_drvr
        = swappable_corner->findLibertyPort(drvr_port_name);
    sta::LibertyPort* swappable_input
        = swappable_corner->findLibertyPort(in_port_name);
    if (swappable_drvr == nullptr || swappable_input == nullptr) {
      continue;
    }
    const float swappable_drive = swappable_drvr->driveResistance();
    const float swappable_delay
        = resizer_.gateDelay(swappable_drvr, load_cap, scene, min_max)
          + prev_drive * swappable_input->capacitance();
    if (swappable_drive < drive && swappable_delay < delay) {
      return swappable;
    }
  }

  return nullptr;
}

}  // namespace rsz
