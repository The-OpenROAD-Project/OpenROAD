// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SetupCritVtSwapPolicy.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "RepairTargetCollector.hh"
#include "VtSwapCandidate.hh"
#include "est/EstimateParasitics.h"
#include "rsz/Resizer.hh"
#include "sta/Fuzzy.hh"
#include "sta/GraphClass.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace rsz {

using std::pair;
using utl::RSZ;

namespace {
static constexpr size_t kMaxCritEndpoints = 100;
static constexpr int kMaxCritInstancesPerEndpoint = 50;

}  // namespace

void SetupCritVtSwapPolicy::iterate()
{
  int& num_viols = setup_context_.violation_count;
  // Critical VT swap runs as a separate phase because it is endpoint-agnostic.
  if (config_.skip_crit_vt_swap || config_.skip_vt_swap || !hasVtSwapCells()) {
    markRunComplete(true);
    return;
  }
  committer_.capturePrePhaseSlack();
  if (swapVTCritCells(num_viols)) {
    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();
  }
  committer_.printTrackerPhaseSummary("VT Swap Phase Summary", nullptr, false);
  markRunComplete(true);
}

bool SetupCritVtSwapPolicy::swapVTCritCells(int& num_viols)
{
  bool changed = false;
  ViolatingEnds violating_ends
      = collectViolatingEndpoints(config_.setup_slack_margin);
  if (violating_ends.size() > kMaxCritEndpoints) {
    violating_ends.resize(kMaxCritEndpoints);
  }
  // Collect a bounded fanin cone across the worst endpoints, then VT-swap that
  // deduplicated instance set in one committer batch.
  std::unordered_map<sta::Instance*, float> crit_insts;
  std::unordered_set<sta::Vertex*> visited;
  std::unordered_set<sta::Instance*> notSwappable;
  for (const auto& [endpoint, slack] : violating_ends) {
    traverseFaninCone(endpoint, crit_insts, visited, notSwappable);
  }
  debugPrint(logger_,
             RSZ,
             "swap_crit_vt",
             1,
             "identified {} critical instances",
             crit_insts.size());

  for (const std::pair<sta::Instance* const, float>& crit_inst_slack :
       crit_insts) {
    sta::Instance* inst = crit_inst_slack.first;
    sta::LibertyCell* best_cell = nullptr;
    if (!resizer_.checkAndMarkVTSwappable(inst, notSwappable, best_cell)) {
      continue;
    }
    sta::LibertyCell* current_cell = network_->libertyCell(inst);

    sta::Pin* output_pin = nullptr;
    if (committer_.moveTrackerEnabled(2)) {
      output_pin = outputPin(inst);
    }

    Target target;
    target.views = kInstanceView;
    target.driver_pin = output_pin;
    target.slack = crit_inst_slack.second;

    if (committer_.moveTrackerEnabled(2)) {
      committer_.setCurrentEndpoint(output_pin);
      committer_.trackViolatorWithTimingInfo(output_pin,
                                             graph_->pinDrvrVertex(output_pin),
                                             target.slack,
                                             *target_collector_);
    }

    VtSwapCandidate candidate(
        resizer_, target, output_pin, inst, current_cell, best_cell);
    committer_.trackMoveAttempt(output_pin, MoveType::kVtSwap);
    const MoveResult result = committer_.commit(candidate);
    if (result.accepted) {
      changed = true;
      debugPrint(logger_,
                 RSZ,
                 "swap_crit_vt",
                 1,
                 "inst {} did crit VT swap",
                 network_->pathName(inst));
    }
  }
  if (changed) {
    committer_.acceptPendingMoves();
    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();
    num_viols = collectViolatingEndpoints(config_.setup_slack_margin).size();
  } else {
    committer_.rejectPendingMoves();
  }

  return changed;
}

void SetupCritVtSwapPolicy::traverseFaninCone(
    sta::Vertex* endpoint,
    std::unordered_map<sta::Instance*, float>& crit_insts,
    std::unordered_set<sta::Vertex*>& visited,
    std::unordered_set<sta::Instance*>& notSwappable)
{
  if (visited.contains(endpoint)) {
    return;
  }

  visited.insert(endpoint);
  std::queue<sta::Vertex*> queue;
  queue.push(endpoint);
  int endpoint_insts = 0;
  sta::LibertyCell* best_lib_cell;

  // Walk backward only through violating fanin logic and cap the number of
  // instances contributed by each endpoint.
  while (!queue.empty() && endpoint_insts < kMaxCritInstancesPerEndpoint) {
    sta::Vertex* current = queue.front();
    queue.pop();

    sta::Pin* pin = current->pin();
    sta::Instance* inst = network_->instance(pin);

    if (inst) {
      if (resizer_.checkAndMarkVTSwappable(inst, notSwappable, best_lib_cell)) {
        const sta::Slack inst_slack = getInstanceSlack(inst);
        if (sta::fuzzyLess(inst_slack, config_.setup_slack_margin)) {
          auto it = crit_insts.find(inst);
          if (it == crit_insts.end()) {
            crit_insts[inst] = inst_slack;
            endpoint_insts++;
            debugPrint(logger_,
                       RSZ,
                       "swap_crit_vt",
                       1,
                       "swapVTCritCells: found crit inst {}: slack {}",
                       network_->name(inst),
                       float(inst_slack));
          }
        }
      }
    }

    sta::VertexInEdgeIterator edge_iter(current, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* fanin_vertex = edge->from(graph_);
      if (fanin_vertex->isRegClk()) {
        continue;
      }

      if (!visited.contains(fanin_vertex)) {
        const sta::Slack fanin_slack = sta_->slack(fanin_vertex, max_);
        if (sta::fuzzyLess(fanin_slack, config_.setup_slack_margin)) {
          queue.push(fanin_vertex);
          visited.insert(fanin_vertex);
        }
      }
    }
  }

  debugPrint(logger_,
             RSZ,
             "swap_crit_vt",
             1,
             "traverseFaninCone: endpoint {} has {} critical instances:",
             endpoint->name(network_),
             endpoint_insts);
  if (logger_->debugCheck(RSZ, "swap_crit_vt", 1)) {
    for (auto crit_inst_slack : crit_insts) {
      logger_->report(" {}", network_->pathName(crit_inst_slack.first));
    }
  }
}

sta::Slack SetupCritVtSwapPolicy::getInstanceSlack(sta::Instance* inst)
{
  sta::Slack worst_slack = std::numeric_limits<float>::max();
  sta::InstancePinIterator* pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    if (network_->direction(pin)->isAnyOutput()) {
      sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
      if (vertex) {
        const sta::Slack pin_slack = sta_->slack(vertex, max_);
        worst_slack = std::min(worst_slack, pin_slack);
      }
    }
  }
  delete pin_iter;

  return worst_slack;
}

sta::Pin* SetupCritVtSwapPolicy::outputPin(sta::Instance* inst)
{
  sta::InstancePinIterator* pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    if (!network_->direction(pin)->isAnyOutput()) {
      continue;
    }

    delete pin_iter;
    return pin;
  }
  delete pin_iter;

  return nullptr;
}

}  // namespace rsz
