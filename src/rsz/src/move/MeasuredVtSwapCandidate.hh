// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "sta/GraphClass.hh"
#include "sta/NetworkClass.hh"

namespace rsz {

// Candidate that measures a VT swap by temporarily applying the replacement,
// running incremental STA, recording the arrival improvement, and then
// rolling back the ECO journal.
//
// Unlike VtSwapCandidate and VtSwapMtCandidate (which use a local
// table-model delay estimate), this variant captures real post-swap timing
// including inter-cell coupling and path-reconvergence effects.  The cost
// is one full incremental STA per candidate.  Used exclusively by
// MeasuredVtSwapPolicy.  Single-threaded only (needs live STA state).
class MeasuredVtSwapCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  MeasuredVtSwapCandidate(Resizer& resizer,
                          const Target& target,
                          sta::Pin* driver_pin,
                          sta::Instance* inst,
                          sta::Vertex* driver_vertex,
                          const sta::Scene* scene,
                          sta::LibertyCell* current_cell,
                          sta::LibertyCell* candidate_cell);

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kVtSwap; }

 private:
  // === Measurement and journal helpers =====================================
  std::string logName() const;
  float arrivalDelay() const;
  void beginEstimateJournal() const;
  void restoreEstimateJournal(bool had_changes) const;

  // === Candidate state ======================================================
  sta::Pin* driver_pin_{nullptr};
  sta::Instance* inst_{nullptr};
  sta::Vertex* driver_vertex_{nullptr};
  const sta::Scene* scene_{nullptr};
  sta::LibertyCell* current_cell_{nullptr};
  sta::LibertyCell* candidate_cell_{nullptr};
};

}  // namespace rsz
