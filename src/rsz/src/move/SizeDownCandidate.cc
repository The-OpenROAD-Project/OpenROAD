// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeDownCandidate.hh"

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

SizeDownCandidate::SizeDownCandidate(Resizer& resizer,
                                     const Target& target,
                                     sta::Pin* drvr_pin,
                                     sta::Instance* inst,
                                     sta::Pin* load_pin,
                                     sta::LibertyCell* current_cell,
                                     sta::LibertyCell* replacement,
                                     sta::Slack slack)
    : MoveCandidate(resizer, target),
      drvr_pin_(drvr_pin),
      inst_(inst),
      load_pin_(load_pin),
      current_cell_(current_cell),
      replacement_(replacement),
      slack_(slack)
{
}

MoveResult SizeDownCandidate::apply()
{
  if (!resizer_.replaceCell(inst_, replacement_)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "size_down_move",
               3,
               "REJECT SizeDownMove {} -> {}: ({} -> {}) slack={}",
               resizer_.network()->pathName(drvr_pin_),
               resizer_.network()->pathName(load_pin_),
               current_cell_->name(),
               replacement_->name(),
               delayAsString(slack_, 3, resizer_.staState()));
    return rejectedMove();
  }

  debugPrint(resizer_.logger(),
             RSZ,
             "size_down_move",
             3,
             "ACCEPT SizeDownMove {} -> {}: ({} -> {}) slack={}",
             resizer_.network()->pathName(drvr_pin_),
             resizer_.network()->pathName(load_pin_),
             current_cell_->name(),
             replacement_->name(),
             delayAsString(slack_, 3, resizer_.staState()));

  return {
      .accepted = true,
      .type = MoveType::kSizeDown,
      .move_count = 1,
      .touched_instances = {inst_},
  };
}

}  // namespace rsz
