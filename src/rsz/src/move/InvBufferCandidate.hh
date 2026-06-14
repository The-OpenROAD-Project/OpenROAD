// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class Instance;
class LibertyCell;
class Pin;
}  // namespace sta

namespace rsz {

// This move replaces a buffer with two inverters evenly placed along
// the fanout wire.
class InvBufferCandidate : public MoveCandidate
{
 public:
  InvBufferCandidate(Resizer& resizer,
                     const Target& target,
                     sta::Instance* buffer,
                     sta::LibertyCell* inv_cell);

  MoveResult apply() override;
  MoveType type() const override { return MoveType::kInvBuffer; }

 private:
  sta::Instance* buffer_{nullptr};
  sta::LibertyCell* inv_cell_{nullptr};
};

}  // namespace rsz
