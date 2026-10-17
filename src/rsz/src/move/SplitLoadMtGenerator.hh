// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"

namespace rsz {

// MT form of SplitLoad.  On the main thread, prepareInputs() picks the less
// critical half of the loads and the buffer.  The candidate scores the move
// from prepared data.
class SplitLoadMtGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit SplitLoadMtGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kSplitLoad; }
  const char* name() const override { return "SplitLoadMtMove"; }
  PrepareCacheMask prepareRequirements() const override
  {
    return kEstimatorContextCache | kSplitLoadInputCache;
  }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

  // kSplitLoadInputCache: the live STA and placement reads behind one split.
  // Leaves target.split_load_input unset when the target has no split.
  static void prepareInputs(const GeneratorContext& context, Target& target);
};

}  // namespace rsz
