// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "MeasuredVtSwapPolicy.hh"

#include <memory>
#include <string>
#include <vector>

#include "MeasuredVtSwapGenerator.hh"
#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "OptPolicy.hh"
#include "OptimizerTypes.hh"
#include "RepairTargetCollector.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"
#include "utl/env.h"

namespace rsz {

using utl::RSZ;

MeasuredVtSwapPolicy::MeasuredVtSwapPolicy(Resizer& resizer,
                                           MoveCommitter& committer)
    : OptPolicy(resizer, committer)
{
}

MeasuredVtSwapPolicy::~MeasuredVtSwapPolicy() = default;

void MeasuredVtSwapPolicy::start(const OptimizerRunConfig& config)
{
  // Reset the one-move-per-target policy state before the first pass begins.
  OptPolicy::start(config);
  policy_config_.max_candidate_generation
      = static_cast<int>(utl::readEnvarUint("RSZ_VTSWAP_CANDIDATES", 10));
  policy_config_.max_committed_moves
      = static_cast<int>(utl::readEnvarUint("RSZ_VTSWAP_MAX_MOVES", 100));
  generator_ = std::make_unique<MeasuredVtSwapGenerator>(
      makeGeneratorContext(config_));
  target_collector_ = std::make_unique<rsz::RepairTargetCollector>(&resizer_);
  committed_moves_ = 0;
  iteration_index_ = 0;
  attempt_index_ = 0;
  exhausted_endpoints_.clear();
  exhausted_instances_.clear();
  if (config_.skip_vt_swap || resizer_.vtCategoryCount() < 2) {
    // Skip the policy entirely when the library cannot provide a VT
    // alternative.
    finishRun(!hasSetupViolations(config_, max_));
  }
}

void MeasuredVtSwapPolicy::iterate()
{
  if (converged_) {
    return;
  }

  // Stop early when the design is already clean before any move is tried.
  if (!hasSetupViolations(config_, max_)) {
    finishRun(true);
    return;
  }

  const sta::Slack tns_before = totalNegativeSlack(max_);
  attempt_index_ = 0;
  exhausted_endpoints_.clear();
  exhausted_instances_.clear();

  // Repeatedly pick the current worst endpoint and try one VT-swap stage on it.
  while (hasSetupViolations(config_, max_)
         && committed_moves_ < policy_config_.max_committed_moves) {
    sta::Vertex* endpoint = findWorstViolatingEndpoint();
    if (endpoint == nullptr) {
      debugPrint(
          logger_,
          RSZ,
          "repair_setup",
          1,
          "MeasuredVtSwapPolicy pass exhausted all viable endpoints after "
          "{} committed moves (violating_endpoints={}, "
          "exhausted_endpoints={}, exhausted_instances={})",
          committed_moves_,
          countViolatingEndpoints(),
          exhausted_endpoints_.size(),
          exhausted_instances_.size());
      break;
    }

    sta::Path* path = findWorstSlackPath(endpoint);
    if (path == nullptr) {
      exhausted_endpoints_.insert(endpoint);
      ++attempt_index_;
      continue;
    }

    Target target;
    if (!selectLargestStageDelayTarget(path, target)) {
      exhausted_endpoints_.insert(endpoint);
      ++attempt_index_;
      continue;
    }

    if (!estimateAndCommitBestCandidate(target)) {
      exhausted_instances_.insert(target.inst(resizer_));
      ++attempt_index_;
      continue;
    }

    ++attempt_index_;
  }

  if (!hasSetupViolations(config_, max_)) {
    finishRun(true);
    return;
  }

  if (committed_moves_ >= policy_config_.max_committed_moves) {
    finishRun(false);
    return;
  }

  if (!sta::fuzzyLess(tns_before, totalNegativeSlack(max_))) {
    finishRun(false);
    return;
  }

  ++iteration_index_;
}

int MeasuredVtSwapPolicy::countViolatingEndpoints() const
{
  target_collector_->init(config_.setup_slack_margin);
  target_collector_->collectViolatingEndpoints();

  int count = 0;
  for (const auto& [endpoint_pin, endpoint_slack] :
       target_collector_->getViolatingEndpoints()) {
    sta::Vertex* endpoint = resizer_.graph()->pinLoadVertex(endpoint_pin);
    if (endpoint != nullptr
        && sta::fuzzyLess(endpoint_slack, config_.setup_slack_margin)) {
      ++count;
    }
  }
  return count;
}

sta::Vertex* MeasuredVtSwapPolicy::findWorstViolatingEndpoint() const
{
  target_collector_->init(config_.setup_slack_margin);
  target_collector_->collectViolatingEndpoints();

  sta::Vertex* worst_endpoint = nullptr;
  sta::Slack worst_slack = 0.0;
  for (const auto& [endpoint_pin, endpoint_slack] :
       target_collector_->getViolatingEndpoints()) {
    sta::Vertex* endpoint = resizer_.graph()->pinLoadVertex(endpoint_pin);
    if (endpoint == nullptr) {
      continue;
    }
    if (exhausted_endpoints_.contains(endpoint)) {
      continue;
    }

    if (sta::fuzzyLess(endpoint_slack, config_.setup_slack_margin)
        && (worst_endpoint == nullptr || endpoint_slack < worst_slack)) {
      worst_endpoint = endpoint;
      worst_slack = endpoint_slack;
    }
  }
  return worst_endpoint;
}

sta::Path* MeasuredVtSwapPolicy::findWorstSlackPath(sta::Vertex* endpoint) const
{
  return target_collector_->findWorstSlackPath(endpoint);
}

bool MeasuredVtSwapPolicy::selectLargestStageDelayTarget(sta::Path* path,
                                                         Target& target) const
{
  // Choose the swappable stage that contributes the largest delay on the path.
  const std::vector<Target> path_targets
      = target_collector_->collectPathDriverTargets(
          path, path->slack(resizer_.staState()));
  if (path_targets.empty()) {
    return false;
  }

  sta::PathExpanded expanded(path, resizer_.staState());
  const sta::Scene* scene = path->scene(resizer_.staState());
  const int dcalc_ap = scene->dcalcAnalysisPtIndex(max_);

  const Target* largest_target = nullptr;
  sta::Delay largest_stage_delay = 0.0;
  for (const Target& path_target : path_targets) {
    const int index = path_target.path_index;
    sta::Pin* pin = path_target.driver_pin;
    const sta::Path* driver_path = expanded.path(index);

    const sta::TimingArc* prev_arc = driver_path->prevArc(resizer_.staState());
    sta::Edge* prev_edge = driver_path->prevEdge(resizer_.staState());
    if (prev_arc == nullptr || prev_edge == nullptr) {
      continue;
    }

    sta::LibertyPort* driver_port = resizer_.network()->libertyPort(pin);
    sta::LibertyCell* current_cell
        = driver_port != nullptr ? driver_port->libertyCell() : nullptr;
    sta::Instance* inst = resizer_.network()->instance(pin);
    if (current_cell == nullptr || inst == nullptr
        || exhausted_instances_.contains(inst)
        || committer_.hasMoves(MoveType::kVtSwap, inst)
        || resizer_.getVTEquivCells(current_cell).size() <= 1) {
      continue;
    }

    const sta::Delay cell_delay
        = resizer_.graph()->arcDelay(prev_edge, prev_arc, dcalc_ap);
    sta::Delay stage_delay = cell_delay;

    if (index + 1 < expanded.size()) {
      const sta::Path* next_path = expanded.path(index + 1);
      sta::Edge* next_prev_edge = next_path->prevEdge(resizer_.staState());
      if (next_prev_edge != nullptr && next_prev_edge->isWire()) {
        sta::delayIncr(stage_delay,
                       sta::delayDiff(next_path->arrival(),
                                      driver_path->arrival(),
                                      resizer_.staState()),
                       resizer_.staState());
      }
    }

    if (largest_target == nullptr || stage_delay > largest_stage_delay) {
      largest_target = &path_target;
      largest_stage_delay = stage_delay;
    }
  }

  if (largest_target == nullptr) {
    return false;
  }

  target = *largest_target;
  return target.isValid() && target.vertex(resizer_) != nullptr;
}

bool MeasuredVtSwapPolicy::estimateAndCommitBestCandidate(const Target& target)
{
  auto candidates = generator_->generate(target);
  if (candidates.empty()) {
    return false;
  }

  MoveCandidate* best_candidate = nullptr;
  Estimate best_estimate;
  for (auto& candidate : candidates) {
    const Estimate estimate = candidate->estimate();
    if (!estimate.legal) {
      continue;
    }
    if (best_candidate == nullptr || estimate.score > best_estimate.score) {
      best_candidate = candidate.get();
      best_estimate = estimate;
    }
  }

  if (best_candidate == nullptr) {
    return false;
  }

  const MoveResult result = committer_.commit(*best_candidate);
  if (!result.accepted) {
    return false;
  }

  resizer_.updateParasiticsAndTiming();
  committer_.acceptPendingMoves();
  ++committed_moves_;
  logger_->info(utl::RSZ,
                kMsgPolicyCommittedMoves,
                "MeasuredVtSwapPolicy committed {} / {} moves.",
                committed_moves_,
                policy_config_.max_committed_moves);
  return true;
}

void MeasuredVtSwapPolicy::finishRun(const bool result)
{
  markRunComplete(result);
}

}  // namespace rsz
