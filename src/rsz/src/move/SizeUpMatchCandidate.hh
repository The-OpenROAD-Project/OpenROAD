// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"

namespace sta {
class LibertyCell;
}

namespace rsz {

// Candidate that replaces the current driver with the same cell family as
// the previous (upstream) stage's stronger driver.
//
// This exploits the observation that when two consecutive stages use the
// same cell family, matching the current stage's cell to the previous
// stage's stronger variant often yields a good size-up without a full
// drive-strength search.  This candidate follows legacy SizeUpMatchMove
// behavior: generation checks topology and cell-family compatibility, while
// apply() only verifies that Resizer::replaceCell succeeds.
class SizeUpMatchCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  SizeUpMatchCandidate(Resizer& resizer,
                       const Target& target,
                       sta::Pin* drvr_pin,
                       sta::Instance* inst,
                       sta::LibertyCell* replacement);

  // === MoveCandidate API ====================================================
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kSizeUpMatch; }

 private:
  // === Candidate preparation and apply helpers =============================
  void prepare();
  void loadDriverCell();
  MoveResult applyReplacement();

  // === Candidate state ======================================================
  bool prepared_{false};
  sta::Pin* drvr_pin_{nullptr};
  sta::Instance* inst_{nullptr};
  sta::LibertyCell* curr_cell_{nullptr};
  sta::LibertyCell* replacement_{nullptr};
};

}  // namespace rsz
