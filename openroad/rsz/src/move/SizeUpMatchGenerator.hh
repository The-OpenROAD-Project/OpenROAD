// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class Instance;
class LibertyCell;
class Pin;
}  // namespace sta

namespace rsz {

// Builds size-up-match candidates by selecting the previous stage's
// stronger cell in the same family, but only when the inter-stage topology
// is simple (single fanout from the previous driver).
//
// This is a lightweight alternative to SizeUpGenerator that avoids the full
// drive-strength search and produces one SizeUpMatchCandidate per target.
// Single-threaded only.
class SizeUpMatchGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit SizeUpMatchGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kSizeUpMatch; }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  // === Driver and previous-stage helpers ===================================
  bool resolveDriverTarget(const Target& target,
                           sta::Pin*& drvr_pin,
                           sta::Instance*& inst,
                           sta::LibertyCell*& curr_cell) const;
  bool loadPreviousDriverPin(const Target& target,
                             sta::Pin*& prev_drvr_pin) const;
  bool hasSingleStageFanout(sta::Pin* prev_drvr_pin, int& fanout) const;
  sta::LibertyCell* selectReplacement(sta::LibertyCell* curr_cell,
                                      sta::Pin* prev_drvr_pin) const;
  bool isSameFamilyStrongerDriver(sta::LibertyCell* curr_cell,
                                  sta::LibertyCell* prev_cell) const;
};

}  // namespace rsz
