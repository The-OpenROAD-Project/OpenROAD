// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"

namespace rsz {

// MT form of SizeDownFanout.  On the main thread, prepareInputs() measures
// each fanout load and the cell one drive step down: the delay budget, the
// output limits and the delay change.  generate() keeps the steps within that
// budget and those limits, and the candidate scores the lighter target load.
class SizeDownFanoutMtGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit SizeDownFanoutMtGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kSizeDownFanout; }
  const char* name() const override { return "SizeDownFanoutMtMove"; }
  PrepareCacheMask prepareRequirements() const override
  {
    return kEstimatorContextCache;
  }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

  // The main-thread measurements behind the size-downs of one target.
  // Leaves target.size_down_fanout_loads empty when no load has a smaller cell.
  void prepareInputs(Target& target) const override;
};

}  // namespace rsz
