// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace rsz {

// Replace one buffer instance with a pair of cascaded inverters. The generator
// picks an inverter cell whose drive resistance is closest to the original
// buffer's, and emits a single BufferToInvertersCandidate when the buffer can
// be safely deleted.
class BufferToInvertersGenerator : public MoveGenerator
{
 public:
  explicit BufferToInvertersGenerator(const GeneratorContext& context);

  MoveType type() const override { return MoveType::kBufferToInverters; }

  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;
};

}  // namespace rsz
