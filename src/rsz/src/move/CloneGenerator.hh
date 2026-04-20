// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace rsz {

// Builds gate-clone candidates for high-fanout combinational drivers.
// isApplicable() checks that the driver has enough fanout to benefit and
// that skip_gate_cloning is not set.  The generator partitions loads by
// slack criticality and produces one CloneCandidate.  Single-threaded only.
class CloneGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit CloneGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kClone; }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;
};

}  // namespace rsz
