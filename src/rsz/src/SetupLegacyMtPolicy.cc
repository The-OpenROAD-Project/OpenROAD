// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SetupLegacyMtPolicy.hh"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BufferGenerator.hh"
#include "CloneGenerator.hh"
#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptPolicy.hh"
#include "OptimizerTypes.hh"
#include "SizeDownGenerator.hh"
#include "SizeUpMatchGenerator.hh"
#include "SizeUpMtGenerator.hh"
#include "SplitLoadGenerator.hh"
#include "SwapPinsGenerator.hh"
#include "UnbufferGenerator.hh"
#include "VtSwapMtGenerator.hh"
#include "rsz/Resizer.hh"
#include "utl/Logger.h"
#include "utl/ThreadPool.h"
#include "utl/env.h"

namespace rsz {

using utl::RSZ;

SetupLegacyMtPolicy::SetupLegacyMtPolicy(Resizer& resizer,
                                         MoveCommitter& committer)
    : SetupLegacyPolicy(resizer, committer)
{
}

SetupLegacyMtPolicy::~SetupLegacyMtPolicy() = default;

void SetupLegacyMtPolicy::start(const OptimizerRunConfig& config)
{
  SetupLegacyPolicy::start(config);
  policy_config_.max_candidate_generation
      = utl::readEnvarInt("RSZ_VTSWAP_CANDIDATES", 10);
  thread_pool_ = makeWorkerThreadPool();
}

void SetupLegacyMtPolicy::buildMoveGenerators(
    const std::vector<MoveType>& move_types,
    const GeneratorContext& context)
{
  // Keep SetupLegacyPolicy's move sequence, but use MT-safe generators only for
  // VtSwap and SizeUp.
  move_generators_.clear();
  move_generators_.reserve(move_types.size());
  for (const MoveType type : move_types) {
    std::unique_ptr<MoveGenerator> generator;
    switch (type) {
      case MoveType::kVtSwap:
        generator = std::make_unique<VtSwapMtGenerator>(context);
        break;
      case MoveType::kSizeUp:
        generator = std::make_unique<SizeUpMtGenerator>(context);
        break;
      case MoveType::kSizeUpMatch:
        generator = std::make_unique<SizeUpMatchGenerator>(context);
        break;
      case MoveType::kBuffer:
        generator = std::make_unique<BufferGenerator>(context);
        break;
      case MoveType::kClone:
        generator = std::make_unique<CloneGenerator>(context);
        break;
      case MoveType::kSplitLoad:
        generator = std::make_unique<SplitLoadGenerator>(context);
        break;
      case MoveType::kSizeDown:
        generator = std::make_unique<SizeDownGenerator>(context);
        break;
      case MoveType::kSwapPins:
        generator = std::make_unique<SwapPinsGenerator>(context);
        break;
      case MoveType::kUnbuffer:
        generator = std::make_unique<UnbufferGenerator>(context);
        break;
      case MoveType::kCount:
        break;
    }
    if (generator != nullptr) {
      move_generators_.push_back(std::move(generator));
    }
  }
}

bool SetupLegacyMtPolicy::canTryGenerator(
    const MoveGenerator& generator,
    const Target& target,
    const std::unordered_set<MoveType>* rejected_types) const
{
  const MoveType type = generator.type();
  if (rejected_types != nullptr && rejected_types->contains(type)) {
    return false;
  }
  return generator.isApplicable(target);
}

void SetupLegacyMtPolicy::logConsideringGenerator(
    const MoveGenerator& generator,
    const Target& target) const
{
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             1,
             "Considering {} for {}",
             generator.name(),
             network_->pathName(target.driver_pin));
}

bool SetupLegacyMtPolicy::usesMtCandidateScoring(const MoveType type) const
{
  return type == MoveType::kVtSwap || type == MoveType::kSizeUp;
}

MoveCandidate* SetupLegacyMtPolicy::estimateCandidatesMt(
    CandidateVector& candidates,
    std::vector<Estimate>& estimates,
    Estimate& best_estimate)
{
  if (candidates.empty()) {
    return nullptr;
  }

  estimates = thread_pool_->parallelMap(
      candidates,
      [](const std::unique_ptr<MoveCandidate>& candidate) -> Estimate {
        return candidate->estimate();
      });

  MoveCandidate* best_candidate = nullptr;
  best_estimate = {};
  for (size_t index = 0; index < estimates.size(); ++index) {
    const Estimate& estimate = estimates[index];
    if (!estimate.legal) {
      continue;
    }
    if (best_candidate == nullptr || estimate.score > best_estimate.score) {
      best_candidate = candidates[index].get();
      best_estimate = estimate;
    }
  }

  return best_candidate;
}

bool SetupLegacyMtPolicy::estimateAndCommitCandidates(
    const Target& target,
    const MoveType type,
    CandidateVector& candidates,
    const int repairs_per_pass,
    int& changed,
    std::optional<MoveType>& accepted_type)
{
  if (usesMtCandidateScoring(type)) {
    return estimateAndCommitMtCandidates(
        target, type, candidates, repairs_per_pass, changed, accepted_type);
  }
  return estimateAndCommitSerialCandidates(
      target, type, candidates, repairs_per_pass, changed, accepted_type);
}

bool SetupLegacyMtPolicy::estimateAndCommitMtCandidates(
    const Target& target,
    const MoveType type,
    CandidateVector& candidates,
    const int repairs_per_pass,
    int& changed,
    std::optional<MoveType>& accepted_type)
{
  std::vector<Estimate> estimates;
  Estimate best_estimate;
  MoveCandidate* best_candidate
      = estimateCandidatesMt(candidates, estimates, best_estimate);
  if (best_candidate == nullptr) {
    return false;
  }

  const MoveResult result = commitCandidate(target, type, *best_candidate);
  if (!result.accepted) {
    return false;
  }

  accepted_type = result.type;
  changed += repairProgressIncrement(result.type, repairs_per_pass);
  return true;
}

bool SetupLegacyMtPolicy::estimateAndCommitSerialCandidates(
    const Target& target,
    const MoveType type,
    CandidateVector& candidates,
    const int repairs_per_pass,
    int& changed,
    std::optional<MoveType>& accepted_type)
{
  bool accepted_batch = false;
  std::optional<MoveType> accepted_move_type;
  for (std::unique_ptr<MoveCandidate>& candidate : candidates) {
    const Estimate estimate = candidate->estimate();
    if (!estimate.legal) {
      continue;
    }

    const MoveResult result = commitCandidate(target, type, *candidate);
    if (!result.accepted) {
      continue;
    }

    accepted_batch = true;
    accepted_move_type = result.type;
    accepted_type = result.type;
    if (!allowsBatchRepair(result.type)) {
      changed += repairProgressIncrement(result.type, repairs_per_pass);
      return true;
    }
  }

  if (accepted_batch) {
    changed += repairProgressIncrement(*accepted_move_type, repairs_per_pass);
    return true;
  }
  return false;
}

bool SetupLegacyMtPolicy::estimateAndCommitSizeDownBatch(
    MoveGenerator& generator,
    const Target& target,
    const int repairs_per_pass,
    int& changed,
    std::optional<MoveType>& accepted_type)
{
  bool accepted_batch = false;
  std::optional<MoveType> accepted_move_type;
  while (true) {
    CandidateVector candidates = generator.generate(target);
    if (candidates.empty()) {
      break;
    }

    int batch_changed = 0;
    std::optional<MoveType> batch_accepted_type;
    if (!estimateAndCommitCandidates(target,
                                     generator.type(),
                                     candidates,
                                     repairs_per_pass,
                                     batch_changed,
                                     batch_accepted_type)) {
      break;
    }

    accepted_batch = true;
    accepted_move_type = *batch_accepted_type;
  }

  if (!accepted_batch) {
    return false;
  }

  changed += repairProgressIncrement(*accepted_move_type, repairs_per_pass);
  accepted_type = *accepted_move_type;
  return true;
}

MoveResult SetupLegacyMtPolicy::commitCandidate(const Target& target,
                                                const MoveType type,
                                                MoveCandidate& candidate)
{
  committer_.trackMoveAttempt(target.driver_pin, type);
  return committer_.commit(candidate);
}

bool SetupLegacyMtPolicy::tryRepairTarget(
    const Target& target,
    const int repairs_per_pass,
    int& changed,
    const std::unordered_set<MoveType>* rejected_types,
    std::optional<MoveType>& accepted_type)
{
  // Preserve SetupLegacyPolicy's single-target, move-sequence ordering.  Only
  // the VtSwap/SizeUp move types score their generated candidates in parallel.
  const Target prepared_target = OptPolicy::prepareTarget(target);

  for (const std::unique_ptr<MoveGenerator>& generator_ptr : move_generators_) {
    MoveGenerator& generator = *generator_ptr;
    if (!canTryGenerator(generator, prepared_target, rejected_types)) {
      continue;
    }

    logConsideringGenerator(generator, prepared_target);
    if (allowsBatchRepair(generator.type())) {
      if (estimateAndCommitSizeDownBatch(generator,
                                         prepared_target,
                                         repairs_per_pass,
                                         changed,
                                         accepted_type)) {
        return true;
      }
      continue;
    }

    CandidateVector candidates = generator.generate(prepared_target);
    if (estimateAndCommitCandidates(prepared_target,
                                    generator.type(),
                                    candidates,
                                    repairs_per_pass,
                                    changed,
                                    accepted_type)) {
      return true;
    }
  }
  return false;
}

}  // namespace rsz
