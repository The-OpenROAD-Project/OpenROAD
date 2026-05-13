// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeUpCandidate.hh"

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

SizeUpCandidate::SizeUpCandidate(Resizer& resizer,
                                 const Target& target,
                                 sta::Pin* drvr_pin,
                                 sta::Instance* inst,
                                 sta::LibertyCell* replacement)
    : MoveCandidate(resizer, target),
      drvr_pin_(drvr_pin),
      inst_(inst),
      replacement_(replacement)
{
}

MoveResult SizeUpCandidate::apply()
{
  prepare();
  return applyReplacement();
}

void SizeUpCandidate::prepare()
{
  if (prepared_) {
    return;
  }

  // Resolve the pre-swap cell lazily so repeated apply paths share one lookup.
  prepared_ = true;
  resolveCurrentCell();
}

void SizeUpCandidate::resolveCurrentCell()
{
  sta::LibertyPort* drvr_port = resizer_.network()->libertyPort(drvr_pin_);
  current_cell_ = drvr_port->libertyCell();
}

MoveResult SizeUpCandidate::applyReplacement()
{
  // Replace the current cell only after the generator has already filtered
  // legal candidates.
  if (!resizer_.replaceCell(inst_, replacement_)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "size_up_move",
               2,
               "REJECT SizeUpMove {}: Couldn't replace cell {} with {}",
               resizer_.network()->pathName(drvr_pin_),
               current_cell_->name(),
               replacement_->name());
    return rejectedMove();
  }

  debugPrint(resizer_.logger(),
             RSZ,
             "size_up_move",
             1,
             "ACCEPT SizeUpMove {}: {} -> {}",
             resizer_.network()->pathName(drvr_pin_),
             current_cell_->name(),
             replacement_->name());
  return {
      .accepted = true,
      .type = MoveType::kSizeUp,
      .move_count = 1,
      .touched_instances = {inst_},
  };
}

}  // namespace rsz
