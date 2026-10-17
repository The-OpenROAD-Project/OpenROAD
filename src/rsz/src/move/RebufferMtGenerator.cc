// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RebufferMtGenerator.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "Rebuffer.hh"
#include "RebufferMtCandidate.hh"
#include "RebufferPlannerPool.hh"
#include "RepairSetupProfiler.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Scene.hh"
#include "sta/StaState.hh"
#include "utl/Logger.h"

namespace rsz {

namespace {

constexpr int kRebufferMaxFanout = 20;

// Side contexts without a downstream window for the plan's other loads.  The
// main tree window scores the target load path.
std::vector<BufferTreeSidePathContext> makeSideContexts(
    const RebufferTreePlan& plan,
    const RebufferLoadTimingMap& load_timing)
{
  std::vector<BufferTreeSidePathContext> contexts;
  if (load_timing.empty()) {
    return contexts;
  }

  for (const BufferTreeLoadPath& load_path : plan.load_paths) {
    if (load_path.load_pin == nullptr
        || load_path.load_pin == plan.target_load_pin) {
      continue;
    }

    const RebufferLoadTimingMap::const_iterator timing
        = load_timing.find(load_path.load_pin);
    if (timing == load_timing.end()) {
      continue;
    }

    contexts.push_back(
        {.load_pin = load_path.load_pin,
         .current_driver_to_load_delay = timing->second.wire_delay,
         .current_slack = timing->second.slack,
         .weight = timing->second.weight});
  }

  return contexts;
}

}  // namespace

RebufferMtGenerator::RebufferMtGenerator(const GeneratorContext& context)
    : MoveGenerator(context), planners_(context.resizer)
{
}

RebufferLoadTimingMap RebufferMtGenerator::collectLoadTiming(
    const Target& target) const
{
  const ProfileScope profile("prepare_rebuffer_side_loads");
  RebufferLoadTimingMap load_timing;
  const SelectedArc& arc = target.estimator_context->target().arc;
  sta::dbSta* sta = resizer_.sta();
  sta::Graph* graph = sta->graph();
  sta::Vertex* driver_vertex = graph->pinDrvrVertex(target.driver_pin);
  if (driver_vertex == nullptr) {
    return load_timing;
  }

  const int dcalc_ap = arc.scene->dcalcAnalysisPtIndex(arc.min_max);

  // Wire edges from the driver reach every load on the net.
  sta::VertexOutEdgeIterator edge_iter(driver_vertex, graph);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (!edge->isWire()) {
      continue;
    }

    sta::Vertex* load_vertex = edge->to(graph);
    const sta::Slack slack = sta->slack(load_vertex, arc.min_max);
    if (sta::delayInf(slack, sta)) {
      continue;
    }

    // Loads off every violating worst path only lose pin slack, not TNS.
    load_timing[load_vertex->pin()]
        = {.slack = sta::delayAsFloat(slack),
           .wire_delay = sta::delayAsFloat(
               graph->wireArcDelay(edge, arc.outputRiseFall(), dcalc_ap)),
           .weight = sideLoadWeight(load_vertex->pin(),
                                    critical_load_pins_,
                                    policy_config_.fanin_side_offpath_weight)};
  }

  RepairSetupProfiler::event("rebuffer_side_loads_collected",
                             load_timing.size());
  return load_timing;
}

bool RebufferMtGenerator::isApplicable(const Target& target) const
{
  return MoveGenerator::isApplicable(target) && target.fanout > 1
         && target.fanout < kRebufferMaxFanout
         && resizer_.okToBufferNet(target.driver_pin)
         && target.estimator_context.has_value();
}

void RebufferMtGenerator::prepareInputs(Target& target) const
{
  target.rebuffer_net = resizer_.rebuffer().makeBufferedNet(target.driver_pin);
  // Without the side-load model the loads behind a new buffer cost nothing.
  target.rebuffer_load_timing.clear();
  if (policy_config_.rebuffer_mt_side_loads) {
    target.rebuffer_load_timing = collectLoadTiming(target);
  }
}

std::vector<std::unique_ptr<MoveCandidate>> RebufferMtGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  if (committer_.hasMoves(MoveType::kBuffer, target.inst(resizer_))) {
    return candidates;
  }

  if (target.load_pin == nullptr) {
    return candidates;
  }

  // The net and load timing prepareInputs() staged on the main thread, planned
  // with a worker planner leased for this call.  Without worker planners the
  // lease hands out the Resizer's planner.
  const RebufferPlannerPool::Lease lease(planners_);
  const std::vector<RebufferTreePlan> plans
      = makeTreePlans(lease.planner(), target);
  addTreePlanCandidates(target, plans, candidates);
  return candidates;
}

void RebufferMtGenerator::startPhase(const size_t worker_count)
{
  // prepareInputs() stages nets with the Resizer's planner, which also plans
  // when the phase has no worker threads.  Each worker thread plans with a
  // planner of its own.
  const ProfileScope profile("rebuffer_planner_init");
  sta::Scene* scene = resizer_.sta()->cmdScene();
  resizer_.rebuffer().initForPlanning(scene);
  planners_.reset(scene, worker_count);
}

void RebufferMtGenerator::finishPhase()
{
  // The worker planners counted their warnings, so the log does not depend
  // on the thread schedule.  One line reports the counts by message ID.
  const std::map<int, int> warning_counts = planners_.workerWarningCounts();
  if (!warning_counts.empty()) {
    int total = 0;
    std::string counts;
    for (const std::pair<const int, int>& id_count : warning_counts) {
      counts += fmt::format("{}RSZ-{:04d} x{}",
                            counts.empty() ? "" : ", ",
                            id_count.first,
                            id_count.second);
      total += id_count.second;
    }
    resizer_.logger()->warn(utl::RSZ,
                            2034,
                            "{} RebufferMt planning warnings on worker threads "
                            "were not logged: {}. Run with set_thread_count 1 "
                            "to log them.",
                            total,
                            counts);
  }

  planners_.reset(resizer_.sta()->cmdScene(), 0);
}

std::vector<RebufferTreePlan> RebufferMtGenerator::makeTreePlans(
    Rebuffer& rebuffer_planner,
    const Target& target) const
{
  std::vector<RebufferTreePlan> plans = rebuffer_planner.makeRebufferTreePlans(
      target.driver_pin,
      target.rebuffer_net,
      target.load_pin,
      std::max(policy_config_.rebuffer_mt_max_candidates, 1));

  // The plan gates: one-buffer trees only when the policy asks for them, and
  // no tree past the score cap.
  std::erase_if(plans, [&](const RebufferTreePlan& plan) {
    if (plan.buffer_count == 0) {
      return true;
    }
    if (plan.buffer_count == 1 && !plan.anchor_tree
        && !policy_config_.allow_rebuffer_mt_single_buffer) {
      return true;
    }
    return !plan.anchor_tree && policy_config_.rebuffer_mt_max_score > 0.0f
           && std::fabs(plan.driver_slack_gain)
                  > policy_config_.rebuffer_mt_max_score;
  });
  return plans;
}

void RebufferMtGenerator::addTreePlanCandidates(
    const Target& target,
    const std::vector<RebufferTreePlan>& plans,
    std::vector<std::unique_ptr<MoveCandidate>>& candidates) const
{
  for (const RebufferTreePlan& plan : plans) {
    candidates.push_back(std::make_unique<RebufferMtCandidate>(
        resizer_,
        target,
        plan,
        policy_config_.rebuffer_mt_score_scale,
        policy_config_.rebuffer_mt_min_score,
        makeSideContexts(plan, target.rebuffer_load_timing)));
  }
}

}  // namespace rsz
