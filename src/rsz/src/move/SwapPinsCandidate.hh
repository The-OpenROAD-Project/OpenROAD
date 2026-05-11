// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"

namespace sta {
class Instance;
class LibertyPort;
class Pin;
}  // namespace sta

namespace rsz {

// Candidate that swaps two functionally equivalent input pins on one gate.
//
// When a cell has symmetric input arcs (determined by Boolean functional
// equivalence), moving the critical-path signal to a faster arc can reduce
// delay without changing the netlist topology.  estimate() compares the
// pre-computed current_delay and swap_delay; apply() rewires the two input
// nets via Resizer::swapPins.
class SwapPinsCandidate : public MoveCandidate
{
 public:
  // === Delay comparison data ===============================================
  // Pre-computed delay pair.  current_delay is the arc delay through the
  // existing input pin; swap_delay is the delay through the alternative
  // symmetric pin.  The score is the positive difference (improvement).
  struct DelayState
  {
    float current_delay{0.0f};
    float swap_delay{0.0f};
  };

  // === Construction =========================================================
  SwapPinsCandidate(Resizer& resizer,
                    const Target& target,
                    sta::Instance* drvr,
                    sta::LibertyPort* drvr_port,
                    sta::LibertyPort* input_port,
                    sta::LibertyPort* swap_port,
                    float current_delay,
                    float swap_delay);

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kSwapPins; }

 private:
  // === Candidate state ======================================================
  sta::Instance* drvr_{nullptr};
  sta::LibertyPort* drvr_port_{nullptr};
  sta::LibertyPort* input_port_{nullptr};
  sta::LibertyPort* swap_port_{nullptr};
  DelayState delay_state_;
};

}  // namespace rsz
