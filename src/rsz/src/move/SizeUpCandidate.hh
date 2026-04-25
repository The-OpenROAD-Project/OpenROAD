// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "sta/NetworkClass.hh"

namespace sta {
class LibertyCell;
}  // namespace sta

namespace rsz {

// Candidate that replaces a path driver with a stronger same-family cell.
//
// The legacy SizeUpGenerator picks one replacement using drive-strength
// heuristics.  This move uses the base legal placeholder estimate; apply()
// performs Resizer::replaceCell and reports the touched instance.  Unlike
// SizeUpMtCandidate, this variant runs single-threaded and resolves its driver
// cell lazily via prepare().
class SizeUpCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  SizeUpCandidate(Resizer& resizer,
                  const Target& target,
                  sta::Pin* drvr_pin,
                  sta::Instance* inst,
                  sta::LibertyCell* replacement);

  // === MoveCandidate API ====================================================
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kSizeUp; }

 private:
  // === Candidate preparation and apply helpers =============================
  void prepare();
  void resolveCurrentCell();
  MoveResult applyReplacement();

  // === Candidate state ======================================================
  bool prepared_{false};
  sta::Pin* drvr_pin_{nullptr};
  sta::Instance* inst_{nullptr};
  sta::LibertyCell* current_cell_{nullptr};
  sta::LibertyCell* replacement_{nullptr};
};

}  // namespace rsz
