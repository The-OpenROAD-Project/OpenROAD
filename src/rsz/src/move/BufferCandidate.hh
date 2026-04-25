// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"

namespace sta {
class Pin;
}  // namespace sta

namespace rsz {

// Candidate that invokes the legacy Rebuffer engine on one driver pin.
//
// This move uses the base legal placeholder estimate.  The Rebuffer engine
// decides internally whether buffering improves timing during apply(), which
// may insert, resize, or remove buffers along the net.
class BufferCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  BufferCandidate(Resizer& resizer, const Target& target, sta::Pin* driver_pin);

  // === MoveCandidate API ====================================================
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kBuffer; }

 private:
  // === Candidate state ======================================================
  sta::Pin* driver_pin_{nullptr};
};

}  // namespace rsz
