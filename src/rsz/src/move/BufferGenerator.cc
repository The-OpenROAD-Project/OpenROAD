// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "BufferGenerator.hh"

#include <memory>
#include <vector>

#include "BufferCandidate.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/StaState.hh"

namespace rsz {

namespace {

constexpr int kRebufferMaxFanout = 20;

sta::Pin* resolveDriverPin(const Target& target, const sta::StaState* sta)
{
  if (!target.isKind(TargetKind::kPathDriver)
      || target.endpoint_path == nullptr) {
    return nullptr;
  }

  sta::Pin* drvr_pin = target.driver_pin;
  if (drvr_pin == nullptr) {
    drvr_pin = target.endpoint_path->pin(sta);
  }
  return drvr_pin;
}

}  // namespace

BufferGenerator::BufferGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool BufferGenerator::isApplicable(const Target& target) const
{
  return target.isKind(TargetKind::kPathDriver)
         && target.endpoint_path != nullptr;
}

std::vector<std::unique_ptr<MoveCandidate>> BufferGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  sta::Pin* drvr_pin = resolveDriverPin(target, resizer_.staState());
  if (drvr_pin == nullptr || target.fanout <= 1
      || target.fanout >= kRebufferMaxFanout
      || !resizer_.okToBufferNet(drvr_pin)) {
    return candidates;
  }

  candidates.push_back(
      std::make_unique<BufferCandidate>(resizer_, target, drvr_pin));
  return candidates;
}

}  // namespace rsz
