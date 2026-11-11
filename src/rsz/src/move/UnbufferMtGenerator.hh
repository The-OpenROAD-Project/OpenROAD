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
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

  // The live STA reads behind one removal.  Leaves target.unbuffer_gain
  // unset when the buffer stays.
  void prepareInputs(Target& target) const override;
};

}  // namespace rsz
