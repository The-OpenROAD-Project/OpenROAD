// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "src/rsz/src/move/BufferGenerator.hh"

#include <memory>
#include <vector>

#include "src/rsz/include/rsz/Resizer.hh"
#include "src/rsz/src/OptimizerTypes.hh"
#include "src/rsz/src/move/BufferCandidate.hh"
#include "src/rsz/src/move/MoveCandidate.hh"
#include "src/rsz/src/move/MoveGenerator.hh"

namespace rsz {

namespace {

constexpr int kRebufferMaxFanout = 20;

}  // namespace

BufferGenerator::BufferGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool BufferGenerator::isApplicable(const Target& target) const
{
  return MoveGenerator::isApplicable(target) && target.fanout > 1
         && target.fanout < kRebufferMaxFanout
         && resizer_.okToBufferNet(target.driver_pin);
}

std::vector<std::unique_ptr<MoveCandidate>> BufferGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  candidates.push_back(
      std::make_unique<BufferCandidate>(resizer_, target, target.driver_pin));
  return candidates;
}

}  // namespace rsz
