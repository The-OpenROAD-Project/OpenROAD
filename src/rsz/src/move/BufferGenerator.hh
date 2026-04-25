// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"

namespace rsz {

// Builds legacy rebuffer candidates for moderate-fanout path drivers.
// isApplicable() gates on minimum fanout and checks that the net is
// allowed to be buffered (okToBuffer guard in Resizer).  Single-threaded
// only; the Rebuffer engine mutates STA state internally.
class BufferGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit BufferGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kBuffer; }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;
};

}  // namespace rsz
