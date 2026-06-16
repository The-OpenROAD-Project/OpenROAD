// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/NetworkClass.hh"

namespace sta {
class LibertyCell;
}

namespace rsz {

// Legacy single-candidate VT-swap generator.
//
// For kPathDriverView, selects the fastest VT-equivalent cell for
// the path driver via selectPathBestCell().  For kInstanceView,
// selects the fastest equivalent for a specific instance (used by the
// critical-cell fanin-cone VT sweep).  Produces at most one
// VtSwapCandidate.  The optional `not_swappable` set lets the caller
// exclude instances that have already been tried (e.g., during the
// swapVTCritCells phase).  Single-threaded only.
class VtSwapGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit VtSwapGenerator(const GeneratorContext& context,
                           std::unordered_set<sta::Instance*>* not_swappable
                           = nullptr);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kVtSwap; }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 protected:
  // === Target-view requirements ============================================
  TargetViewMask requiredViews() const override
  {
    return kPathDriverView | kInstanceView;
  }

 private:
  // === Candidate-cell selection ============================================
  bool selectBestCell(const Target& target,
                      sta::Pin*& drvr_pin,
                      sta::Instance*& inst,
                      sta::LibertyCell*& curr_cell,
                      sta::LibertyCell*& best_cell) const;
  bool selectPathBestCell(const Target& target,
                          sta::Pin*& drvr_pin,
                          sta::Instance*& inst,
                          sta::LibertyCell*& curr_cell,
                          sta::LibertyCell*& best_cell) const;
  bool selectInstanceBestCell(const Target& target,
                              sta::Instance*& inst,
                              sta::LibertyCell*& curr_cell,
                              sta::LibertyCell*& best_cell) const;
  bool resolvePathCurrentCell(sta::Pin* drvr_pin,
                              sta::LibertyCell*& curr_cell) const;
  bool selectBestEquivCell(sta::LibertyCell* curr_cell,
                           sta::LibertyCell*& best_cell) const;

  // === Exclusion state ======================================================
  std::unordered_set<sta::Instance*>* not_swappable_{nullptr};
};

}  // namespace rsz
