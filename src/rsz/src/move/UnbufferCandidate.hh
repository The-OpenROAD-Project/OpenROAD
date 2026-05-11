// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"

namespace sta {
class Instance;
}

namespace rsz {

// Candidate that removes one existing buffer instance (buffer removal /
// bypass).
//
// The generator pre-checks that the buffer has a single output, that the
// downstream capacitance stays within max-cap limits, and that local slack
// is sufficient.  apply() calls Resizer::removeBuffer to short-circuit the
// buffer's input and output nets and delete the instance.
class UnbufferCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  UnbufferCandidate(Resizer& resizer,
                    const Target& target,
                    sta::Instance* drvr);

  // === MoveCandidate API ====================================================
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kUnbuffer; }

 private:
  // === Candidate state ======================================================
  sta::Instance* drvr_{nullptr};
};

}  // namespace rsz
