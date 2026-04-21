// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeUpGenerator.hh"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SizeUpCandidate.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"

namespace rsz {

SizeUpGenerator::SizeUpGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
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
      || !resizer_.replacementPreservesMaxCap(inst, replacement)) {
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
      [this, &drvr_port_name, lib_ap](const sta::LibertyCell* cell1,
                                      const sta::LibertyCell* cell2) {
        return strongerCellLess(cell1, cell2, drvr_port_name, lib_ap);
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
