// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RebufferMtGenerator.hh"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "Rebuffer.hh"
#include "RebufferMtCandidate.hh"
#include "rsz/Resizer.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/StaState.hh"

namespace rsz {

namespace {

constexpr int kRebufferMaxFanout = 20;

const sta::Pin* targetLoadPin(const Target& target, Resizer& resizer)
{
  if (target.endpoint_path == nullptr || target.path_index < 0) {
    return nullptr;
  }

  sta::PathExpanded expanded(target.endpoint_path, resizer.staState());
  const int next_path_index = target.path_index + 1;
  if (next_path_index >= static_cast<int>(expanded.size())) {
    return nullptr;
  }

  const sta::Path* next_path = expanded.path(next_path_index);
  if (next_path == nullptr) {
    return nullptr;
  }

  return next_path->pin(resizer.staState());
}

}  // namespace

RebufferMtGenerator::RebufferMtGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool RebufferMtGenerator::isApplicable(const Target& target) const
{
  return MoveGenerator::isApplicable(target) && target.fanout > 1
         && target.fanout < kRebufferMaxFanout
         && resizer_.okToBufferNet(target.driver_pin)
         && target.isPrepared(kEstimatorContextCache);
}

std::vector<std::unique_ptr<MoveCandidate>> RebufferMtGenerator::generate(
    const Target& target)
{
  // The main thread reads the net itself and charges no side loads.
  Rebuffer& rebuffer_planner = resizer_.rebuffer();
  return generateWith(target,
                      rebuffer_planner,
                      rebuffer_planner.makeBufferedNet(target.driver_pin));
}

std::vector<std::unique_ptr<MoveCandidate>> RebufferMtGenerator::generateWith(
    const Target& target,
    Rebuffer& rebuffer_planner,
    BufferedNetPtr net) const
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;

  if (committer_.hasMoves(MoveType::kBuffer, target.inst(resizer_))) {
    return candidates;
  }

  const sta::Pin* load_pin = targetLoadPin(target, resizer_);
  if (load_pin == nullptr) {
    return candidates;
  }

  const int max_tree_candidates
      = std::max(policy_config_.rebuffer_mt_max_candidates, 1);
  const std::vector<RebufferTreePlan> plans
      = rebuffer_planner.makeRebufferTreePlans(
          target.driver_pin, std::move(net), load_pin, max_tree_candidates);
  addTreePlanCandidates(target, plans, candidates);
  return candidates;
}

void RebufferMtGenerator::addTreePlanCandidates(
    const Target& target,
    const std::vector<RebufferTreePlan>& plans,
    std::vector<std::unique_ptr<MoveCandidate>>& candidates) const
{
  for (const RebufferTreePlan& plan : plans) {
    if (plan.buffer_count == 0) {
      continue;
    }

    // One-buffer trees compete in the pool only when the policy asks for
    // them.
    const bool single_buffer_plan = plan.buffer_count == 1;
    if (single_buffer_plan && !plan.anchor_tree
        && !policy_config_.allow_rebuffer_mt_single_buffer) {
      continue;
    }

    const float tree_score = std::fabs(plan.driver_slack_gain);
    if (!plan.anchor_tree && policy_config_.rebuffer_mt_max_score > 0.0f
        && tree_score > policy_config_.rebuffer_mt_max_score) {
      continue;
    }

    candidates.push_back(std::make_unique<RebufferMtCandidate>(
        resizer_,
        target,
        target.estimator_context.value(),
        plan,
        policy_config_.rebuffer_mt_score_scale,
        policy_config_.rebuffer_mt_min_score));
  }
}

}  // namespace rsz
