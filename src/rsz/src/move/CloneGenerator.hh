// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "src/rsz/include/rsz/Resizer.hh"
#include "src/rsz/src/OptimizerTypes.hh"
#include "src/rsz/src/move/MoveCandidate.hh"
#include "src/rsz/src/move/MoveGenerator.hh"

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
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;
};

}  // namespace rsz
