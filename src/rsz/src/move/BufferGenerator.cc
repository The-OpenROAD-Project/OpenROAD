// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "BufferGenerator.hh"

#include <memory>
#include <vector>

#include "BufferCandidate.hh"
#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

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
