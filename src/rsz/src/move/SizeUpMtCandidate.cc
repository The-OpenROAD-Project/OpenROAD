// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeUpMtCandidate.hh"

#include <string>
#include <utility>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

SizeUpMtCandidate::SizeUpMtCandidate(Resizer& resizer,
                                     const Target& target,
                                     sta::Pin* drvr_pin,
                                     sta::Instance* inst,
                                     sta::LibertyCell* replacement,
                                     const ArcDelayState& arc_delay)
    : MoveCandidate(resizer, target),
      drvr_pin_(drvr_pin),
      inst_(inst),
      current_cell_(resizer.network()->libertyCell(inst)),
      replacement_(replacement),
      arc_delay_(arc_delay)
{
}

Estimate SizeUpMtCandidate::estimate()
{
  // Score one size-up replacement from immutable inputs prepared before the
  // worker pool runs candidate estimation.
  const DelayEstimate delay_est
      = DelayEstimator::estimate(arc_delay_, replacement_);
  return {.legal = delay_est.legal, .score = delay_est.arrival_impr};
}

MoveResult SizeUpMtCandidate::apply()
{
  // Commit the chosen replacement directly after the generator has screened
  // legality.
  if (!resizer_.replacementPreservesMaxCap(inst_, replacement_)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "opt_moves",
               1,
               "REJECT size_up_mt1 {}: {} -> {} violates max capacitance",
               logName(),
               current_cell_->name(),
               replacement_->name());
    return {
        .accepted = false,
        .type = MoveType::kSizeUp,
        .touched_instances = {},
    };
  }

  const bool accepted = resizer_.replaceCell(inst_, replacement_);
  if (accepted) {
    const std::string& current_cell_name = current_cell_->name();
    const std::string& replacement_name = replacement_->name();
    debugPrint(resizer_.logger(),
               RSZ,
               "opt_moves",
               1,
               "ACCEPT size_up_mt1 {}: {} -> {}",
               logName(),
               current_cell_name,
               replacement_name);
    return {
        .accepted = true,
        .type = MoveType::kSizeUp,
        .move_count = 1,
        .touched_instances = {inst_},
    };
  }

  debugPrint(resizer_.logger(),
             RSZ,
             "opt_moves",
             1,
             "REJECT size_up_mt1 {}: {} -> {} replace failed",
             logName(),
             current_cell_->name(),
             replacement_->name());
  return {
      .accepted = false,
      .type = MoveType::kSizeUp,
      .touched_instances = {},
  };
}

std::string SizeUpMtCandidate::logName() const
{
  return drvr_pin_ != nullptr ? resizer_.network()->pathName(drvr_pin_) : "";
}

}  // namespace rsz
