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

// Builds downsizing candidates for the non-critical loads of the current
// driver.  For each load it takes a single drive step down -- the next-smaller
// same-family cell, mirroring how SizeUp takes the next-stronger cell rather
// than the family maximum -- and accepts it only if it stays within the load's
// delay budget and the max-cap/slew output limits.  High-fanout drivers are
// considered too: the driver speedup from a downsize is
// R_drvr * (shed load capacitance), so those drivers hold the most sheddable
// capacitance.  The checks are deliberately cheap and local; the full timing
// assessment is left to STA and the repair loop.  Single-threaded only.
class SizeDownFanoutGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit SizeDownFanoutGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kSizeDownFanout; }
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;
};

}  // namespace rsz
