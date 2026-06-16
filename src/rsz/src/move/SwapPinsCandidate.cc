// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SwapPinsCandidate.hh"

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

SwapPinsCandidate::SwapPinsCandidate(Resizer& resizer,
                                     const Target& target,
                                     sta::Instance* drvr,
                                     sta::LibertyPort* drvr_port,
                                     sta::LibertyPort* input_port,
                                     sta::LibertyPort* swap_port,
                                     const float current_delay,
                                     const float swap_delay)
    : MoveCandidate(resizer, target),
      drvr_(drvr),
      drvr_port_(drvr_port),
      input_port_(input_port),
      swap_port_(swap_port),
      delay_state_{.current_delay = current_delay, .swap_delay = swap_delay}
{
}

Estimate SwapPinsCandidate::estimate()
{
  const float score = delay_state_.current_delay - delay_state_.swap_delay;
  return {.legal = score > 0.0f, .score = score};
}

MoveResult SwapPinsCandidate::apply()
{
  sta::Pin* drvr_pin = resizer_.network()->findPin(drvr_, drvr_port_);
  debugPrint(resizer_.logger(),
             RSZ,
             "swap_pins_move",
             1,
             "ACCEPT SwapPinsMove {}: Cell {}, pins {} <-> {}",
             resizer_.network()->name(drvr_pin),
             drvr_port_->libertyCell()->name(),
             input_port_->name(),
             swap_port_->name());

  if (!resizer_.swapPins(drvr_, input_port_, swap_port_)) {
    return rejectedMove();
  }
  return {
      .accepted = true,
      .type = MoveType::kSwapPins,
      .move_count = 1,
      .touched_instances = {drvr_},
  };
}

}  // namespace rsz
