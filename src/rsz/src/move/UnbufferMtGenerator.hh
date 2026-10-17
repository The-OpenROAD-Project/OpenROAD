// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"

namespace rsz {

// MT form of Unbuffer.  On the main thread, prepareInputs() checks that a
// buffer on the target path may go and estimates the delay its removal saves.
// generate() offers the removal when the estimate is a gain.
class UnbufferMtGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit UnbufferMtGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kUnbuffer; }
  const char* name() const override { return "UnbufferMtMove"; }
  PrepareCacheMask prepareRequirements() const override
  {
    return kUnbufferInputCache;
  }
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

  // kUnbufferInputCache: the live STA reads behind one removal.  Leaves
  // target.unbuffer_input unset when the buffer stays.
  static void prepareInputs(const GeneratorContext& context, Target& target);
};

}  // namespace rsz
