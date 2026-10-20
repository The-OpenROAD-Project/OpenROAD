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
#include "sta/TimingArc.hh"

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
      || !resizer_.replacementPreservesMaxCap(
          inst, replacement, scene != nullptr ? scene->mode() : nullptr)) {
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
  if (inst == nullptr || resizer_.dontTouch(inst) || resizer_.isFixed(inst)
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

  const sta::Path* prev_drvr_path = target.prevDriverPath(resizer_);
  const sta::Path* drvr_path = target.driverPath(resizer_);
  const sta::TimingArc* in_arc = drvr_path != nullptr
                                     ? drvr_path->prevArc(resizer_.staState())
                                     : nullptr;
  in_port = in_arc != nullptr ? in_arc->from() : nullptr;
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

enum class CostType
{
  kLeakage,
  kLefArea,
  kLibArea
};

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
  const sta::LibertyCellSeq swappable_cells = resizer_.getSwappableCells(cell);
  debugPrint(resizer_.logger(),
             utl::RSZ,
             "size_up_move",
             3,
             "upsizeCell {}: {} swappable cells",
             cell->name(),
             swappable_cells.size());
  if (swappable_cells.empty()) {
    return nullptr;
  }

  // Cost of a nominal cell under one metric; nullopt if unavailable.
  auto cost_of = [this](sta::LibertyCell* c,
                        const CostType type) -> std::optional<float> {
    switch (type) {
      case CostType::kLeakage: {
        const std::optional<float> leak = resizer_.cellLeakage(c);
        return (leak && *leak > 0.0f) ? leak : std::nullopt;
      }
      case CostType::kLefArea: {
        const odb::dbMaster* master = resizer_.dbNetwork()->staToDb(c);
        if (master != nullptr && master->getArea() > 0) {
          return static_cast<float>(master->getArea());
        }
        return std::nullopt;
      }
      case CostType::kLibArea:
        return c->area() > 0.0f ? std::optional<float>(c->area())
                                : std::nullopt;
    }
    return std::nullopt;
  };

  // new/old cost on the first metric BOTH cells support (never mixes units).
  auto cost_ratio = [&](sta::LibertyCell* old_cell,
                        sta::LibertyCell* new_cell) -> std::optional<float> {
    for (const CostType type :
         {CostType::kLeakage, CostType::kLefArea, CostType::kLibArea}) {
      const std::optional<float> old_cost = cost_of(old_cell, type);
      const std::optional<float> new_cost = cost_of(new_cell, type);
      if (old_cost && new_cost) {
        return *new_cost / *old_cost;
      }
    }
    return std::nullopt;
  };

  // gateDelay() takes the nominal port and maps arcs to the corner itself;
  // port capacitance has no corner mapping, so read it from the scene port.
  auto stage_delay = [&](const sta::LibertyCell* c) -> std::optional<float> {
    const sta::LibertyPort* drvr = c->findLibertyPort(drvr_port->name());
    const sta::LibertyPort* in = c->findLibertyPort(in_port->name());
    if (drvr == nullptr || in == nullptr) {
      return std::nullopt;
    }
    const sta::LibertyPort* scene_in = in->scenePort(lib_ap);
    if (scene_in == nullptr || drvr->scenePort(lib_ap) == nullptr) {
      return std::nullopt;
    }
    return resizer_.gateDelay(drvr, load_cap, scene, min_max)
           + prev_drive * scene_in->capacitance();
  };

  // Drive resistance of the driver output in the active corner; used to keep
  // selection to genuinely stronger cells (corner-robust, protects downstream
  // max-slew/max-cap) rather than cells that only improve single-corner stage
  // delay.
  auto drive_res = [&](const sta::LibertyCell* c) -> std::optional<float> {
    const sta::LibertyPort* drvr = c->findLibertyPort(drvr_port->name());
    if (drvr == nullptr) {
      return std::nullopt;
    }
    const sta::LibertyPort* scene_drvr = drvr->scenePort(lib_ap);
    if (scene_drvr == nullptr) {
      return std::nullopt;
    }
    return scene_drvr->driveResistance();
  };

  const std::optional<float> delay = stage_delay(cell);
  if (!delay) {
    return nullptr;
  }
  const std::optional<float> drive = drive_res(cell);

  sta::LibertyCell* best_cell = nullptr;
  float best_score = 0.0f;
  for (sta::LibertyCell* swappable : swappable_cells) {
    if (swappable == cell) {
      continue;
    }
    const std::optional<float> cand_delay = stage_delay(swappable);
    if (!cand_delay || *cand_delay >= *delay) {
      continue;
    }
    // Require a genuinely stronger driver (lower drive resistance).  A cell
    // that improves single-corner stage delay without strengthening drive tends
    // to leave worse max-slew/max-cap slack on the net and at other corners.
    const std::optional<float> cand_drive = drive_res(swappable);
    if (drive && cand_drive && *cand_drive >= *drive) {
      continue;
    }
    const std::optional<float> ratio = cost_ratio(cell, swappable);
    if (!ratio) {
      continue;
    }

    // score = delay gain / cost ratio; a ratio < 1.0 earns no extra credit.
    const float score = (*delay - *cand_delay) / std::max(1.0f, *ratio);
    debugPrint(resizer_.logger(),
               utl::RSZ,
               "size_up_move",
               3,
               "   cand {}: new_d={} old_d={} new_c/old_c={:.4g} score={:.4g}",
               swappable->name(),
               delayAsString(*cand_delay, 4, resizer_.staState()),
               delayAsString(*delay, 4, resizer_.staState()),
               *ratio,
               score);
    if (score > best_score) {
      best_cell = swappable;
      best_score = score;
    }
  }

  debugPrint(resizer_.logger(),
             utl::RSZ,
             "size_up_move",
             3,
             "   best cand {}: score={:.4g}",
             best_cell != nullptr ? best_cell->name() : "none",
             best_score);
  return best_cell;
}

}  // namespace rsz
