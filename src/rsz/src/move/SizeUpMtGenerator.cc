// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeUpMtGenerator.hh"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupProfiler.hh"
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
  sta::Instance* target_inst = target.inst(resizer_);
  return MoveGenerator::isApplicable(target) && target_inst != nullptr
         && !resizer_.dontTouch(target_inst) && !resizer_.isFixed(target_inst)
         && target.isPrepared(kEstimatorContextCache);
}

std::vector<std::unique_ptr<MoveCandidate>> SizeUpMtGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  sta::Instance* target_inst = target.inst(resizer_);
  const EstimatorContext& estimator_context = target.estimator_context.value();
  const SelectedArc& target_arc = estimator_context.target().arc;
  const float score_scale = policy_config_.size_up_mt_score_scale;
  const SizeUpOptions& options = findSizeUpOptions(
      target_arc.outputPort(), target_arc.scene, target_arc.min_max);
  if (options.dual_rf_rejects > 0) {
    RepairSetupProfiler::event("dual_rf_drive_guard_rejects",
                               options.dual_rf_rejects,
                               moveName(MoveType::kSizeUp));
  }

  const std::vector<sta::LibertyCell*>& replacements = options.cells;

  // Fanin max-cap headroom is instance-level, captured once per target.
  const std::vector<Resizer::InputCapBudget>& budgets = inputCapBudgets(target);
  candidates.reserve(replacements.size());
  for (sta::LibertyCell* replacement : replacements) {
    if (!resizer_.replacementFitsInputCapBudgets(budgets, replacement)) {
      RepairSetupProfiler::event(
          "max_cap_rejects", 1, moveName(MoveType::kSizeUp));
      continue;
    }

    candidates.push_back(std::make_unique<SizeUpMtCandidate>(
        resizer_, target, target_inst, replacement, score_scale));
  }
  return candidates;
}

const SizeUpMtGenerator::SizeUpOptions& SizeUpMtGenerator::findSizeUpOptions(
    const sta::LibertyPort* drvr_port,
    const sta::Scene* scene,
    const sta::MinMax* min_max) const
{
  const SizeUpOptionKey key{drvr_port, scene, min_max};
  const std::lock_guard<std::mutex> lock(size_up_options_mutex_);
  std::map<SizeUpOptionKey, SizeUpOptions>::iterator cached
      = size_up_options_.find(key);
  if (cached == size_up_options_.end()) {
    cached = size_up_options_
                 .emplace(key, buildSizeUpOptions(drvr_port, scene, min_max))
                 .first;
  }

  return cached->second;
}

SizeUpMtGenerator::SizeUpOptions SizeUpMtGenerator::buildSizeUpOptions(
    const sta::LibertyPort* drvr_port,
    const sta::Scene* scene,
    const sta::MinMax* min_max) const
{
  SizeUpOptions options;
  std::vector<sta::LibertyCell*>& replacements = options.cells;
  if (drvr_port == nullptr || scene == nullptr || min_max == nullptr) {
    return options;
  }

  const int lib_ap = scene->libertyIndex(min_max);
  sta::LibertyCell* cell = drvr_port->libertyCell();
  sta::LibertyCellSeq swappable_cells = resizer_.getSwappableCells(cell);
  if (swappable_cells.empty()) {
    return options;
  }

  const std::string& drvr_port_name = drvr_port->name();

  const sta::LibertyPort* scene_drvr_port = drvr_port->scenePort(lib_ap);
  if (scene_drvr_port == nullptr) {
    return options;
  }

  const float drive_r = scene_drvr_port->driveResistance();
  // A cell more than size_up_mt_max_drive_ratio times stronger grows the input
  // capacitance by about the same factor, so the jump is bounded.
  const float max_ratio = policy_config_.size_up_mt_max_drive_ratio;
  const bool limit_ratio = max_ratio > 0.0f && drive_r > 0.0f;
  const float min_drive_r = limit_ratio ? drive_r / max_ratio : 0.0f;
  odb::dbMaster* current_master = resizer_.dbNetwork()->staToDb(cell);
  for (sta::LibertyCell* swappable : swappable_cells) {
    if (swappable == cell) {
      continue;
    }
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
    if (policy_config_.dual_rf_drive_guard
        && !driveResNoHigherOnBothRf(scene_drvr_port, swappable_drvr)) {
      ++options.dual_rf_rejects;
      continue;
    }
    // Keep every equivalent cell that does not weaken drive resistance and let
    // candidate::estimate require a strict delay improvement.
    const float swappable_drive_r = swappable_drvr->driveResistance();
    if (swappable_drive_r <= drive_r
        && (!limit_ratio || swappable_drive_r >= min_drive_r)) {
      replacements.push_back(swappable);
    }
  }
  return options;
}

}  // namespace rsz
