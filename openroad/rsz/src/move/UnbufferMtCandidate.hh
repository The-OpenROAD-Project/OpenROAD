// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace rsz {

// MT-safe removal of the target's buffer.  The batch ranks it by the gain
// prepareInputs() estimated and the batch STA judges the real effect.
// apply() merges the buffer's input and output nets and deletes the buffer.
class UnbufferMtCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  UnbufferMtCandidate(Resizer& resizer, const Target& target);

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kUnbuffer; }
  // The input net takes over the loads, the output net goes, and the upstream
  // driver and every load see new timing.
  MoveImpact impact() const override;
};

}  // namespace rsz
