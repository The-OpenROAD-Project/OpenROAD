// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"

namespace sta {
class LibertyCell;
class Pin;
class Instance;
}  // namespace sta

namespace rsz {

// MT-safe size-up candidate scored from a prepared ArcDelayState.
//
// Unlike the legacy SizeUpCandidate which queries STA on the fly, this
// variant receives a state of the timing arc (ArcDelayState) built on
// the main thread and stored in Target. estimate() evaluates the Liberty table
// model purely from that snapshot (thread-safe; no STA mutation), so MT
// policies can run it on worker threads. apply() still
// executes on the main thread via Resizer::replaceCell.
class SizeUpMtCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  SizeUpMtCandidate(Resizer& resizer,
                    const Target& target,
                    sta::Pin* drvr_pin,
                    sta::Instance* inst,
                    sta::LibertyCell* replacement,
                    const ArcDelayState& arc_delay);

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kSizeUp; }

 private:
  std::string logName() const;

  // === Candidate state ======================================================
  sta::Pin* drvr_pin_{nullptr};
  sta::Instance* inst_{nullptr};
  sta::LibertyCell* current_cell_{nullptr};
  sta::LibertyCell* replacement_{nullptr};
  ArcDelayState arc_delay_;
};

}  // namespace rsz
