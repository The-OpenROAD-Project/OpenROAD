// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SetupCritVtSwapPolicy.hh"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "PathGroupFilter.hh"
#include "RepairSetupContext.hh"
#include "RepairSetupProfiler.hh"
#include "RepairTargetCollector.hh"
#include "SequentialTimingEstimator.hh"
#include "VtSwapCandidate.hh"
#include "est/EstimateParasitics.h"
#include "rsz/Resizer.hh"
#include "sta/ClkNetwork.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PocvMode.hh"
#include "sta/PortDirection.hh"
#include "sta/Scene.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace rsz {

using std::pair;
using utl::RSZ;

namespace {
static constexpr size_t kMaxCritEndpoints = 100;
static constexpr int kMaxCritInstancesPerEndpoint = 50;

}  // namespace

SetupCritVtSwapPolicy::SetupCritVtSwapPolicy(Resizer& resizer,
                                             MoveCommitter& committer,
                                             RepairSetupContext& setup_context,
                                             const OptimizerRunConfig& config,
                                             const bool capture_only)
    : SetupLegacyBase(resizer, committer, setup_context, config),
      capture_only_(capture_only)
{
}

void SetupCritVtSwapPolicy::iterate()
{
  int& num_viols = setup_context_.violation_count;
  // Critical VT swap runs as a separate phase because it is endpoint-agnostic.
  const bool skip = capture_only_
                        ? config_.skip_vt_swap
                        : config_.skip_crit_vt_swap || config_.skip_vt_swap;
  if (skip || !hasVtSwapCells()) {
    RepairSetupProfiler::event("skip_crit_vt_swap");
    markRunComplete(true);
    return;
  }

  // The timing the sweep starts from, which no earlier phase may have sampled.
  if (RepairSetupProfiler::enabled()) {
    const ProfileScope collection("qor_collection");
    sta::Slack wns;
    sta::Vertex* worst_vertex;
    target_collector_->worstSlack(wns, worst_vertex);
    RepairSetupProfiler::qor(
        "phase_before", wns, target_collector_->totalNegativeSlack());
  }

  const ProfileIteration iteration("crit_instance_sweep");
  committer_.tracker().capturePrePhaseSlack();
  if (swapVTCritCells(num_viols)) {
    resizer_.updateParasiticsAndTiming();
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
    RepairSetupProfiler::event("skip_endpoint_limit",
                               violating_ends.size() - kMaxCritEndpoints);
    violating_ends.resize(kMaxCritEndpoints);
  }
  // Collect a bounded fanin cone across the worst endpoints, then VT-swap that
  // deduplicated instance set in one committer batch.
  // The instances are kept in the order they are found: a swap can be
  // rejected for max capacitance depending on the swaps before it.
  const size_t max_crit_insts
      = violating_ends.size() * kMaxCritInstancesPerEndpoint;
  std::vector<std::pair<sta::Instance*, float>> crit_insts;
  crit_insts.reserve(max_crit_insts);
  std::unordered_set<sta::Instance*> crit_inst_set;
  crit_inst_set.reserve(max_crit_insts);
  std::unordered_set<sta::Vertex*> visited;
  std::unordered_set<sta::Instance*> notSwappable;
  // Capture-only registers and the VT that shortens their setup check.
  std::unordered_map<sta::Instance*, sta::LibertyCell*> capture_cells;

  {
    const ProfileScope profile("target_collection");
    const std::optional<std::unordered_set<sta::Vertex*>> group_fanout
        = findGroupFanout();
    for (const auto& [endpoint, slack] : violating_ends) {
      traverseFaninCone(endpoint,
                        group_fanout.has_value() ? &*group_fanout : nullptr,
                        crit_insts,
                        crit_inst_set,
                        visited,
                        notSwappable,
                        capture_cells);
    }

    RepairSetupProfiler::event("targets_collected", crit_insts.size());
  }
  debugPrint(logger_,
             RSZ,
             "swap_crit_vt",
             1,
             "identified {} critical instances",
             crit_insts.size());

  for (const auto& [inst, inst_slack] : crit_insts) {
    sta::LibertyCell* best_cell = nullptr;
    if (!resizer_.checkAndMarkVTSwappable(inst, notSwappable, best_cell)) {
      RepairSetupProfiler::event(
          "skip_not_swappable", 1, moveName(MoveType::kVtSwap));
      continue;
    }
    const auto capture_cell = capture_cells.find(inst);
    if (capture_cell != capture_cells.end()) {
      best_cell = capture_cell->second;
    } else if (capture_only_) {
      continue;
    }
    sta::LibertyCell* current_cell = network_->libertyCell(inst);

    sta::Pin* output_pin = nullptr;
    if (committer_.moveTrackerEnabled(2) || RepairSetupProfiler::enabled()) {
      output_pin = outputPin(inst);
    }

    Target target;
    target.views = kInstanceView;
    target.driver_pin = output_pin;
    target.slack = inst_slack;

    if (committer_.moveTrackerEnabled(2)) {
      committer_.setCurrentEndpoint(output_pin);
      committer_.trackViolatorWithTimingInfo(output_pin,
                                             graph_->pinDrvrVertex(output_pin),
                                             target.slack,
                                             *target_collector_);
    }

    VtSwapCandidate candidate(
        resizer_, target, output_pin, inst, current_cell, best_cell);

    // CRIT applies constructed candidates without invoking estimate().
    RepairSetupProfiler::moveGenerated(candidate);
    RepairSetupProfiler::event(
        "estimate_skipped", 1, moveName(candidate.type()));
    RepairSetupProfiler::event("winners", 1, moveName(candidate.type()));
    RepairSetupProfiler::moveSelected(candidate);
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
    resizer_.updateParasiticsAndTiming();
    num_viols = collectViolatingEndpoints(config_.setup_slack_margin).size();
  } else {
    RepairSetupProfiler::stop("no_change");
    committer_.rejectPendingMoves();
  }

  return changed;
}

std::optional<std::unordered_set<sta::Vertex*>>
SetupCritVtSwapPolicy::findGroupFanout()
{
  const PathGroupFilter path_group_filter(&resizer_);
  if (!path_group_filter.enabled()) {
    return std::nullopt;
  }
  std::unordered_set<sta::Vertex*> reached;
  std::queue<sta::Vertex*> queue;
  sta::VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    sta::Vertex* vertex = vertex_iter.next();
    const sta::Pin* pin = vertex->pin();
    if (sta_->isClock(pin, sta_->cmdMode())) {
      continue;
    }
    const sta::PortDirection* direction = network_->direction(pin);
    if (((network_->isTopLevelPort(pin) && direction->isAnyInput())
         || (resizer_.isRegister(vertex) && direction->isAnyOutput()))
        && path_group_filter.startpointInGroup(vertex)
        && reached.insert(vertex).second) {
      queue.push(vertex);
    }
  }
  while (!queue.empty()) {
    sta::Vertex* vertex = queue.front();
    queue.pop();
    sta::VertexOutEdgeIterator edge_iter(vertex, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* fanout = edge->to(graph_);
      // A register clock pin launches paths of its own rather than
      // continuing this one.
      if (edge->role()->isTimingCheck() || fanout->isRegClk()) {
        continue;
      }
      if (reached.insert(fanout).second) {
        queue.push(fanout);
      }
    }
  }
  return reached;
}

void SetupCritVtSwapPolicy::traverseFaninCone(
    sta::Vertex* endpoint,
    const std::unordered_set<sta::Vertex*>* group_fanout,
    std::vector<std::pair<sta::Instance*, float>>& crit_insts,
    std::unordered_set<sta::Instance*>& crit_inst_set,
    std::unordered_set<sta::Vertex*>& visited,
    std::unordered_set<sta::Instance*>& notSwappable,
    std::unordered_map<sta::Instance*, sta::LibertyCell*>& capture_cells)
{
  if (visited.contains(endpoint)) {
    return;
  }

  visited.insert(endpoint);
  using SlackVertex = std::pair<sta::Slack, sta::Vertex*>;
  auto cmp = [](const SlackVertex& a, const SlackVertex& b) {
    return a.first > b.first;
  };
  std::priority_queue<SlackVertex, std::vector<SlackVertex>, decltype(cmp)>
      queue(cmp);
  queue.emplace(sta_->slack(endpoint, max_), endpoint);
  int endpoint_insts = 0;
  sta::LibertyCell* best_lib_cell;

  // Walk backward through violating fanin logic ordered by worst slack first,
  // capping the number of instances contributed by each endpoint.
  while (!queue.empty() && endpoint_insts < kMaxCritInstancesPerEndpoint) {
    sta::Vertex* current = queue.top().second;
    queue.pop();

    sta::Pin* pin = current->pin();
    sta::Instance* inst = network_->instance(pin);

    // Under -path_group a cell counts only through an output on the group's
    // paths into the endpoint, where its own delay lies on them; the
    // endpoint's register is reached through its data pin, not its delay.
    if (inst && (group_fanout == nullptr || current->isDriver(network_))) {
      if (resizer_.checkAndMarkVTSwappable(inst, notSwappable, best_lib_cell)) {
        sta::Slack inst_slack = getInstanceSlack(inst);
        // A register reached through its data pin is critical by its setup
        // check; swap it only when the new VT shortens that check.
        if (!sta::fuzzyLess(inst_slack, config_.setup_slack_margin)
            && group_fanout == nullptr) {
          const sta::Slack capture_slack = captureSlack(inst);
          if (sta::fuzzyLess(capture_slack, config_.setup_slack_margin)) {
            sta::LibertyCell* capture_cell = bestCaptureVtCell(inst);
            if (capture_cell != nullptr) {
              capture_cells[inst] = capture_cell;
              inst_slack = capture_slack;
            }
          }
        }
        if (sta::fuzzyLess(inst_slack, config_.setup_slack_margin)) {
          if (crit_inst_set.insert(inst).second) {
            crit_insts.emplace_back(inst, inst_slack);
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

      // Logic no group startpoint reaches carries none of the group's paths.
      if (group_fanout != nullptr && !group_fanout->contains(fanin_vertex)) {
        continue;
      }

      if (!visited.contains(fanin_vertex)) {
        const sta::Slack fanin_slack = sta_->slack(fanin_vertex, max_);
        if (sta::fuzzyLess(fanin_slack, config_.setup_slack_margin)) {
          queue.emplace(fanin_slack, fanin_vertex);
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
    for (size_t i = crit_insts.size() - endpoint_insts; i < crit_insts.size();
         ++i) {
      logger_->report(" {}", network_->pathName(crit_insts[i].first));
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

sta::Slack SetupCritVtSwapPolicy::captureSlack(sta::Instance* inst)
{
  // Any timing-checked data input counts: the cell need not be classified
  // as a register (a statetable MBFF may not be), only checked.
  sta::Slack worst_slack = std::numeric_limits<float>::max();
  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    if (!network_->direction(pin)->isAnyInput()
        || sta_->isClock(pin, sta_->cmdMode())) {
      continue;
    }
    sta::Vertex* vertex = graph_->pinLoadVertex(pin);
    if (vertex && vertex->hasChecks()) {
      worst_slack = std::min(worst_slack, sta_->slack(vertex, max_));
    }
  }
  return worst_slack;
}

float SetupCritVtSwapPolicy::captureSetupGain(sta::Instance* inst,
                                              sta::LibertyCell* cell)
{
  sta::LibertyCell* current = network_->libertyCell(inst);
  if (current == nullptr || cell == nullptr || cell == current) {
    return 0.0f;
  }

  // Worst margin increase per checked pin, check type and data transition,
  // over every scene and clock edge: the slack each check loses.
  struct CheckKey
  {
    sta::Vertex* vertex;
    bool setup;
    bool operator==(const CheckKey&) const = default;
  };
  struct CheckKeyHash
  {
    size_t operator()(const CheckKey& key) const
    {
      return std::hash<sta::Vertex*>()(key.vertex) * 2 + key.setup;
    }
  };
  using RfDelta = std::array<float, sta::RiseFall::index_count>;
  std::unordered_map<CheckKey, RfDelta, CheckKeyHash> deltas;

  for (sta::TimingArcSet* arc_set : current->timingArcSets()) {
    const sta::TimingRole* role = arc_set->role()->genericRole();
    const bool setup = role == sta::TimingRole::setup();
    if (!setup && role != sta::TimingRole::hold()) {
      continue;
    }
    sta::Pin* clk_pin = network_->findPin(inst, arc_set->from());
    sta::Pin* data_pin = network_->findPin(inst, arc_set->to());
    sta::LibertyPort* cand_from
        = cell->findLibertyPort(arc_set->from()->name());
    sta::LibertyPort* cand_to = cell->findLibertyPort(arc_set->to()->name());
    if (clk_pin == nullptr || data_pin == nullptr || cand_from == nullptr
        || cand_to == nullptr) {
      return 0.0f;
    }
    sta::Vertex* data_vertex = graph_->pinLoadVertex(data_pin);
    sta::Vertex* clk_vertex = graph_->pinLoadVertex(clk_pin);
    if (data_vertex == nullptr || clk_vertex == nullptr) {
      return 0.0f;
    }
    const sta::MinMax* min_max
        = setup ? sta::MinMax::max() : sta::MinMax::min();
    auto [entry, inserted]
        = deltas.try_emplace(CheckKey{.vertex = data_vertex, .setup = setup},
                             RfDelta{
                                 -std::numeric_limits<float>::infinity(),
                                 -std::numeric_limits<float>::infinity(),
                             });

    for (const sta::Scene* scene : sta_->scenes()) {
      const sta::Pvt* pvt = sta_->pvt(inst, min_max, scene->sdc());
      const sta::DcalcAPIndex ap_index = scene->dcalcAnalysisPtIndex(min_max);
      const sta::ClkNetwork* clk_network = scene->mode()->clkNetwork();
      for (sta::TimingArc* arc : arc_set->arcs()) {
        const sta::RiseFall* clk_rf = arc->fromEdge()->asRiseFall();
        const sta::RiseFall* data_rf = arc->toEdge()->asRiseFall();
        if (clk_rf == nullptr || data_rf == nullptr) {
          continue;
        }
        sta::TimingArc* cand_arc = nullptr;
        for (sta::TimingArcSet* cand_set :
             cell->timingArcSets(cand_from, cand_to)) {
          if (cand_set->role()->genericRole() != role) {
            continue;
          }
          for (sta::TimingArc* candidate : cand_set->arcs()) {
            if (candidate->fromEdge() == arc->fromEdge()
                && candidate->toEdge() == arc->toEdge()) {
              cand_arc = candidate;
            }
          }
        }
        const sta::CheckTimingModel* model = arc->checkModel(scene, min_max);
        const sta::CheckTimingModel* cand_model
            = cand_arc ? cand_arc->checkModel(scene, min_max) : nullptr;
        if (model == nullptr || cand_model == nullptr) {
          return 0.0f;
        }
        const float clk_slew
            = clk_network->isIdealClock(clk_pin)
                  ? clk_network->idealClkSlew(clk_pin, clk_rf, min_max)
                  : sta::delayAsFloat(
                        graph_->slew(clk_vertex, clk_rf, ap_index));
        const float data_slew
            = sta::delayAsFloat(graph_->slew(data_vertex, data_rf, ap_index));
        const float margin = sta::delayAsFloat(model->checkDelay(
            pvt, clk_slew, data_slew, 0.0f, min_max, sta::PocvMode::scalar));
        const float cand_margin = sta::delayAsFloat(cand_model->checkDelay(
            pvt, clk_slew, data_slew, 0.0f, min_max, sta::PocvMode::scalar));
        float& delta = entry->second[data_rf->index()];
        delta = std::max(delta, cand_margin - margin);
      }
    }
  }

  // Legal when no check is pushed into, or deeper into, violation: a longer
  // margin on a check with slack to spare is fine (e.g. an SE fall setup).
  // The gain is the summed setup TNS recovered at the violating pins.
  float gain = 0.0f;
  for (const auto& [key, rf_delta] : deltas) {
    const sta::MinMax* min_max = key.setup ? max_ : sta::MinMax::min();
    float before = std::numeric_limits<float>::infinity();
    float after = std::numeric_limits<float>::infinity();
    for (const sta::RiseFall* rf : sta::RiseFall::range()) {
      const float delta = rf_delta[rf->index()];
      if (!std::isfinite(delta)) {
        continue;
      }
      const float slack
          = sta::delayAsFloat(sta_->slack(key.vertex, rf, min_max));
      if (!std::isfinite(slack)) {
        continue;
      }
      before = std::min(before, slack);
      after = std::min(after, slack - delta);
    }
    if (!std::isfinite(before)) {
      continue;
    }
    if (sta::fuzzyLess(after, std::min(before, 0.0f))) {
      return 0.0f;
    }
    if (key.setup) {
      gain += std::min(after, 0.0f) - std::min(before, 0.0f);
    }
  }
  return gain;
}

sta::LibertyCell* SetupCritVtSwapPolicy::bestCaptureVtCell(sta::Instance* inst)
{
  sta::LibertyCell* current = network_->libertyCell(inst);
  if (current == nullptr) {
    return nullptr;
  }
  sta::LibertyCell* best = nullptr;
  float best_gain = 0.0f;
  for (sta::LibertyCell* candidate : resizer_.getVTEquivCells(current)) {
    const float gain = captureSetupGain(inst, candidate);
    if (gain > best_gain) {
      best_gain = gain;
      best = candidate;
    }
  }
  return best;
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
