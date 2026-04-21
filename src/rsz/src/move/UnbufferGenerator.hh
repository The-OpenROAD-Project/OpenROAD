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

// Builds buffer-removal candidates for buffer stages on the current repair
// path.
//
// Identifies single-output buffers on the target path, runs the legacy
// estimatedSlackOK legality check, and produces one UnbufferCandidate if
// removal is legal.  The committer also blocks removal
// when the buffer was inserted by an earlier move in the same endpoint pass
// (hasBlockingBufferRemovalMove guard).  Single-threaded only.
class UnbufferGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit UnbufferGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kUnbuffer; }
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;
};

}  // namespace rsz
