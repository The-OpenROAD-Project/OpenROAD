// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"

namespace rsz {

// MT form of SwapPins.  On the main thread, prepareInputs() finds the
// equivalent inputs of the target gate and measures the arc delay through
// each.  generate() picks the fastest input on a worker thread.
class SwapPinsMtGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit SwapPinsMtGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kSwapPins; }
  const char* name() const override { return "SwapPinsMtMove"; }
  PrepareCacheMask prepareRequirements() const override
  {
    return kEstimatorContextCache;
  }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

  // The arc delays through the equivalent inputs of the target gate.  Leaves
  // target.swap_pins_input unset when the gate has none.
  void prepareInputs(Target& target) const override;
};

}  // namespace rsz
