// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "VtSwapCandidate.hh"

#include <string>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

VtSwapCandidate::VtSwapCandidate(Resizer& resizer,
                                 const Target& target,
                                 sta::Pin* drvr_pin,
                                 sta::Instance* inst,
                                 sta::LibertyCell* curr_cell,
                                 sta::LibertyCell* best_cell)
    : MoveCandidate(resizer, target),
      drvr_pin_(drvr_pin),
      inst_(inst),
      curr_cell_(curr_cell),
      best_cell_(best_cell)
{
}

MoveResult VtSwapCandidate::apply()
{
  return applyReplacement();
}

std::string VtSwapCandidate::logName() const
{
  if (target().kind == TargetKind::kInstance) {
    return resizer_.network()->pathName(inst_);
  }
  return resizer_.network()->pathName(drvr_pin_);
}

MoveResult VtSwapCandidate::applyReplacement()
{
  if (!resizer_.replacementPreservesMaxCap(inst_, best_cell_)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "vt_swap_move",
               2,
               "REJECT VTSwapMove {}: {} -> {} violates max capacitance",
               logName(),
               curr_cell_->name(),
               best_cell_->name());
    return rejectedMove();
  }

  if (!resizer_.replaceCell(inst_, best_cell_)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "vt_swap_move",
               2,
               "REJECT VTSwapMove {}: Failed to swap {} -> {}",
               logName(),
               curr_cell_->name(),
               best_cell_->name());
    return rejectedMove();
  }

  debugPrint(resizer_.logger(),
             RSZ,
             "vt_swap_move",
             1,
             "ACCEPT VTSwapMove {}: {} -> {}",
             logName(),
             curr_cell_->name(),
             best_cell_->name());
  return {
      .accepted = true,
      .type = MoveType::kVtSwap,
      .move_count = 1,
      .touched_instances = {inst_},
  };
}

}  // namespace rsz
