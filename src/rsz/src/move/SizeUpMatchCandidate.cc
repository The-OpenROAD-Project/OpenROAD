// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "src/rsz/src/move/SizeUpMatchCandidate.hh"

#include "src/rsz/include/rsz/Resizer.hh"
#include "src/rsz/src/OptimizerTypes.hh"
#include "src/rsz/src/move/MoveCandidate.hh"
#include "src/sta/include/sta/Liberty.hh"
#include "src/sta/include/sta/Network.hh"
#include "src/sta/include/sta/NetworkClass.hh"
#include "src/utl/include/utl/Logger.h"

namespace rsz {

using utl::RSZ;

SizeUpMatchCandidate::SizeUpMatchCandidate(Resizer& resizer,
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

MoveResult SizeUpMatchCandidate::apply()
{
  prepare();
  return applyReplacement();
}

void SizeUpMatchCandidate::prepare()
{
  if (prepared_) {
    return;
  }

  prepared_ = true;
  loadDriverCell();
}

void SizeUpMatchCandidate::loadDriverCell()
{
  sta::LibertyPort* drvr_port = resizer_.network()->libertyPort(drvr_pin_);
  curr_cell_ = drvr_port->libertyCell();
}

MoveResult SizeUpMatchCandidate::applyReplacement()
{
  if (!resizer_.replacementPreservesMaxCap(inst_, replacement_)
      || !resizer_.replaceCell(inst_, replacement_)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "size_up_match_move",
               2,
               "REJECT SizeUpMatchMove {}: Couldn't replace {} -> {}",
               resizer_.network()->pathName(drvr_pin_),
               curr_cell_->name(),
               replacement_->name());
    return rejectedMove();
  }

  debugPrint(resizer_.logger(),
             RSZ,
             "size_up_match_move",
             1,
             "ACCEPT SizeUpMatchMove {}: Replaced {} -> {}",
             resizer_.network()->pathName(drvr_pin_),
             curr_cell_->name(),
             replacement_->name());
  return {
      .accepted = true,
      .type = MoveType::kSizeUpMatch,
      .move_count = 1,
      .touched_instances = {inst_},
  };
}

}  // namespace rsz
