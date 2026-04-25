// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "sta/NetworkClass.hh"

namespace sta {
class LibertyCell;
}

namespace rsz {

// Candidate that replaces one instance with a faster VT-equivalent Liberty
// cell (e.g., HVT → SVT or SVT → LVT within the same footprint).
//
// Used by the legacy single-threaded VtSwapGenerator.  This move uses the base
// legal placeholder estimate; the legacy path relies on the generator's
// best-cell selection being legal.  apply() performs Resizer::replaceCell.
class VtSwapCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  VtSwapCandidate(Resizer& resizer,
                  const Target& target,
                  sta::Pin* drvr_pin,
                  sta::Instance* inst,
                  sta::LibertyCell* curr_cell,
                  sta::LibertyCell* best_cell);

  // === MoveCandidate API ====================================================
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kVtSwap; }

 private:
  // === Apply helpers ========================================================
  std::string logName() const;
  MoveResult applyReplacement();

  // === Candidate state ======================================================
  sta::Pin* drvr_pin_{nullptr};
  sta::Instance* inst_{nullptr};
  sta::LibertyCell* curr_cell_{nullptr};
  sta::LibertyCell* best_cell_{nullptr};
};

}  // namespace rsz
