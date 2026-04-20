// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "sta/NetworkClass.hh"

namespace sta {
class LibertyCell;
}  // namespace sta

namespace rsz {

// MT-safe VT-swap candidate scored from a prepared ArcDelayState.
//
// Same concept as VtSwapCandidate but estimate() reads from a prepared
// ArcDelayState (built on the main thread and stored on Target) and
// evaluates the Liberty table model locally, making it thread-safe for
// parallel evaluation on worker threads.  apply() still runs on the main
// thread via Resizer::replaceCell.
class VtSwapMtCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  VtSwapMtCandidate(Resizer& resizer,
                    const Target& target,
                    sta::Pin* driver_pin,
                    sta::Instance* inst,
                    sta::LibertyCell* current_cell,
                    sta::LibertyCell* candidate_cell,
                    const ArcDelayState& arc_delay);

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kVtSwap; }

 private:
  std::string getDrvPinName() const;

  // === Candidate state ======================================================
  sta::Pin* driver_pin_{nullptr};
  sta::Instance* inst_{nullptr};
  sta::LibertyCell* current_cell_{nullptr};
  sta::LibertyCell* candidate_cell_{nullptr};
  ArcDelayState arc_delay_;
};

}  // namespace rsz
