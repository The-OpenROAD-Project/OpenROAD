// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "MeasuredVtSwapCandidate.hh"

#include <string>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Scene.hh"
#include "sta/Sta.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

MeasuredVtSwapCandidate::MeasuredVtSwapCandidate(
    Resizer& resizer,
    const Target& target,
    sta::Pin* driver_pin,
    sta::Instance* inst,
    sta::Vertex* driver_vertex,
    const sta::Scene* scene,
    sta::LibertyCell* current_cell,
    sta::LibertyCell* candidate_cell)
    : MoveCandidate(resizer, target),
      driver_pin_(driver_pin),
      inst_(inst),
      driver_vertex_(driver_vertex),
      scene_(scene),
      current_cell_(current_cell),
      candidate_cell_(candidate_cell)
{
}

Estimate MeasuredVtSwapCandidate::estimate()
{
  if (!resizer_.replacementPreservesMaxCap(inst_, candidate_cell_)) {
    return {.legal = false, .score = 0.0f};
  }

  // Evaluate the swap on a temporary journaled edit so the database can be
  // restored afterward.
  beginEstimateJournal();
  const float before_delay = arrivalDelay();
  if (!resizer_.replaceCell(inst_, candidate_cell_)) {
    restoreEstimateJournal(false);
    return {.legal = false, .score = 0.0f};
  }

  resizer_.updateParasiticsAndTiming();
  const float after_delay = arrivalDelay();
  restoreEstimateJournal(true);

  const float score = before_delay - after_delay;
  // Keep the raw score even for non-improving swaps so policy ranking can
  // compare rejected candidates consistently.
  return {.legal = score > 0.0f, .score = score};
}

MoveResult MeasuredVtSwapCandidate::apply()
{
  // Apply the chosen VT replacement after the one-stage policy commits it.
  if (!resizer_.replaceCell(inst_, candidate_cell_)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "opt_moves",
               1,
               "REJECT measured_vt_swap {}: {} -> {} swap failed",
               logName(),
               current_cell_->name(),
               candidate_cell_->name());
    return {
        .accepted = false,
        .type = MoveType::kVtSwap,
        .touched_instances = {},
    };
  }

  debugPrint(resizer_.logger(),
             RSZ,
             "opt_moves",
             1,
             "ACCEPT measured_vt_swap {}: {} -> {}",
             logName(),
             current_cell_->name(),
             candidate_cell_->name());
  return {
      .accepted = true,
      .type = MoveType::kVtSwap,
      .move_count = 1,
      .touched_instances = {inst_},
  };
}

std::string MeasuredVtSwapCandidate::logName() const
{
  return resizer_.network()->pathName(driver_pin_);
}

float MeasuredVtSwapCandidate::arrivalDelay() const
{
  const sta::SceneSeq scenes
      = resizer_.sta()->makeSceneSeq(const_cast<sta::Scene*>(scene_));
  return resizer_.sta()->arrival(driver_vertex_,
                                 sta::RiseFallBoth::riseFall(),
                                 scenes,
                                 resizer_.maxAnalysisMode());
}

void MeasuredVtSwapCandidate::beginEstimateJournal() const
{
  odb::dbDatabase::beginEco(resizer_.block());
}

void MeasuredVtSwapCandidate::restoreEstimateJournal(
    const bool had_changes) const
{
  resizer_.initForJournalRestore();
  odb::dbDatabase::undoEco(resizer_.block());
  if (had_changes) {
    resizer_.updateParasiticsAndTiming();
  }
}

}  // namespace rsz
