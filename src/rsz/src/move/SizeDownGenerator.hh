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

// Builds downsizing candidates for non-critical loads on the current driver.
// Iterates loads on the target net, finds smaller same-family cells, computes
// worst delay degradation, and produces a SizeDownCandidate if the degradation
// fits the available slack.  Single-threaded only.
class SizeDownGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit SizeDownGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kSizeDown; }
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;
};

}  // namespace rsz
