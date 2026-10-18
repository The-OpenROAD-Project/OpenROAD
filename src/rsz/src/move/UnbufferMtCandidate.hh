// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class Instance;
}  // namespace sta

namespace rsz {

// The buffer the main thread found removable for one target and the delay
// its removal is estimated to save.
struct UnbufferMtInput
{
  sta::Instance* buffer{nullptr};
  float estimated_gain{0.0f};
};

// MT-safe buffer removal.  The batch ranks it by the prepared gain and the
// batch STA judges the real effect.  apply() merges the buffer's input and
// output nets and deletes the buffer.
class UnbufferMtCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  UnbufferMtCandidate(Resizer& resizer,
                      const Target& target,
                      std::shared_ptr<const UnbufferMtInput> input);

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kUnbuffer; }
  // The input net takes over the loads, the output net goes, and the upstream
  // driver and every load see new timing.
  MoveImpact impact() const override;

 private:
  // === Candidate state ======================================================
  std::shared_ptr<const UnbufferMtInput> input_;
};

}  // namespace rsz
