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

// Builds split-load buffering candidates for high-fanout path drivers.
// Partitions loads by timing criticality and produces one
// SplitLoadCandidate that places a buffer to isolate the non-critical
// subset from the driver.  Single-threaded only.
class SplitLoadGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit SplitLoadGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kSplitLoad; }
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;
};

}  // namespace rsz
