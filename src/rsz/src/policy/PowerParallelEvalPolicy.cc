// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "PowerParallelEvalPolicy.hh"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "PowerSwapCandidate.hh"
#include "PowerVtSwapGenerator.hh"
#include "RepairSetupContext.hh"
#include "RepairTargetCollector.hh"
#include "SetupParallelEvalPolicy.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/Search.hh"
#include "sta/SearchPred.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace rsz {

PowerParallelEvalPolicy::PowerParallelEvalPolicy(
    Resizer& resizer,
    MoveCommitter& committer,
    RepairSetupContext& setup_context,
    const OptimizerRunConfig& config)
    : SetupParallelEvalPolicy(resizer, committer, setup_context, config),
      vt_budget_(config.vt_budgets, config.vt_budget_run),
      // Past the setup contract the floor is the contract; a VT budget run
      // there spends the whole room.
      slack_ratio_(config.vt_budget_ws.has_value()
                       ? 1.0f
                       : PowerSwapCandidate::kSlackRatio)
{
  // Power moves never improve WNS, so the WNS tail, the cone weights, the score
  // floor and the worst-endpoint reserve have nothing to work on, and the
  // stall rules would end the run after a few saved batches.
  ParallelEvalPolicyConfig& pe = policy_config_.parallel_eval;
  pe.wns_tail_endpoints = 0;
  pe.priority.bottleneck_max_wns_stall_batches = 0;
  pe.selector.min_score_ratio = 0.0f;
  pe.selector.wns_reserve_pct = 0;
  pe.stall_batches = 100000;
  pe.stall_max_batches = 100000;
  pe.stall_window = 100000;
  // A missed batch holds few bad moves among thousands, and the run is over
  // in a few batches, so its instances come back soon.
  pe.miss_tabu_batches = 2;
  // Power swaps are many and each spends little slack, so a batch takes far
  // more of them than a setup batch.
  pe.max_batch_cap = 8192;
}

bool PowerParallelEvalPolicy::start()
{
  if (!SetupParallelEvalPolicy::start()) {
    return false;
  }
  start_contract_ = measureContract();
  // The tolerance is a share of the period the setup thresholds scale with;
  // without clocks or path delays there is no setup slack to spend.
  const float tolerance
      = reference_period_ < sta::INF
            ? config_.setup_tolerance / 100.0 * reference_period_
            : 0.0f;
  floors_ = SlackFloors(config_.setup_slack_margin, tolerance);
  if (floors_.tolerant()) {
    const sta::Unit* time_unit = sta_->units()->timeUnit();
    logger_->info(utl::RSZ,
                  3264,
                  "Setup tolerance {}{} ({}% of the clock period {}{}).",
                  time_unit->asString(tolerance),
                  time_unit->scaleAbbrevSuffix(),
                  config_.setup_tolerance,
                  time_unit->asString(reference_period_),
                  time_unit->scaleAbbrevSuffix());
  }
  if (config_.vt_budget_ws.has_value()) {
    setBudgetFloor(0.0);
  }
  if (move_sequence_.empty()) {
    logger_->warn(utl::RSZ,
                  3263,
                  "repair_power found no higher-VT cells; nothing to do.");
    markRunComplete(false);
    return true;
  }
  vt_budget_.count(resizer_);
  start_excess_ = vt_budget_.excess();
  if (!vt_budget_.lowering()) {
    power_model_.init();
  }
  return true;
}

void PowerParallelEvalPolicy::iterate()
{
  const int committed_before = committed_moves_;
  const int batch_before = batch_index_;
  batch_saved_ = 0.0f;
  SetupParallelEvalPolicy::iterate();
  vt_budget_.count(resizer_);
  if (config_.verbose && batch_index_ > batch_before && vt_budget_.lowering()) {
    logger_->report(
        "VT budget batch {:4}: kept {:6} moves, {:7} in total, {} "
        "missed",
        batch_index_,
        committed_moves_ - committed_before,
        committed_moves_,
        checkpoint_misses_);
  } else if (config_.verbose && batch_index_ > batch_before) {
    logger_->report(
        "Power batch {:4}: kept {:6} moves, {:7} in total, {} missed, "
        "saved {:.3e} W",
        batch_index_,
        committed_moves_ - committed_before,
        committed_moves_,
        checkpoint_misses_,
        batch_saved_);
  }
  // A missed batch is the miss rules' to judge, not the stall rule's.
  if (converged_ || checkpoint_misses_ > 0) {
    return;
  }
  if (vt_budget_.lowering()) {
    if (vt_budget_.met()
        || (committed_moves_ - committed_before <= kStallRatio * start_excess_
            && !lowerBudgetFloor())) {
      markRunComplete(false);
    }
    return;
  }

  total_saved_ += batch_saved_;
  saved_window_.push_back(batch_saved_);
  if (saved_window_.size() > kStallBatches) {
    saved_window_.pop_front();
  }
  const float window_saved
      = std::accumulate(saved_window_.begin(), saved_window_.end(), 0.0f);
  if (saved_window_.size() == kStallBatches
      && window_saved <= kStallRatio * total_saved_) {
    markRunComplete(false);
  }
}

SetupParallelEvalPolicy::BatchScope
PowerParallelEvalPolicy::collectTargetEndpoints()
{
  rebuffer_targets_in_batch_ = 0;
  power_model_.clear();
  if (batch_index_ > 0) {
    prewarmStaForPrepareStage();
  }

  // Every constrained endpoint, worst slack first.
  target_collector_->init(sta::INF);
  pre_batch_endpoints_ = target_collector_->getViolatingEndpoints();
  if (batch_index_ == 0) {
    for (const auto& [pin, slack] : pre_batch_endpoints_) {
      floors_.recordEndpoint(pin, sta::delayAsFloat(slack));
    }
  }
  savePreBatchSlews();

  collectSwappableDrivers();
  while (swappable_drivers_.empty() && lowerBudgetFloor()) {
    collectSwappableDrivers();
  }
  worst_paths_.clear();
  // PowerVtSwapGenerator reads slew checks.
  sta_->checkSlewsPreamble();

  const size_t scope = swappable_drivers_.size();
  const int batch_cap = effectiveBatchCap(scope);
  stall_.beginBatch(batch_cap, scope);
  return {.target_endpoints = scope, .batch_cap = batch_cap};
}

void PowerParallelEvalPolicy::collectSwappableDrivers()
{
  sta::Mode* mode = sta_->cmdMode();
  sta_->ensureClkNetwork(mode);
  if (floors_.tolerant()) {
    floor_required_ = floorRequired();
  }
  std::vector<std::pair<sta::Pin*, float>> drivers;
  std::unique_ptr<sta::LeafInstanceIterator> inst_iter(
      network_->leafInstanceIterator());
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();
    if (!resizer_.isEditableLogicStdCell(inst) || !canSwap(inst)) {
      continue;
    }
    std::unique_ptr<sta::InstancePinIterator> pin_iter(
        network_->pinIterator(inst));
    while (pin_iter->hasNext()) {
      sta::Pin* pin = pin_iter->next();
      sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
      if (vertex == nullptr || !network_->isDriver(pin)
          || sta_->isClock(pin, mode)) {
        continue;
      }
      const float slack = sta::delayAsFloat(sta_->slack(vertex, max_));
      // The room ignores timing exceptions, so a pin STA leaves
      // unconstrained keeps the default floor.
      if (floors_.tolerant() && slack < sta::INF) {
        const float room = floorRoom(vertex, floor_required_);
        if (room < sta::INF) {
          floors_.setDriver(pin, slack - room);
        }
      }
      const float floor = floors_.driver(pin);
      if (slack > floor) {
        // Most room first; the floor is the margin without a tolerance.
        drivers.emplace_back(pin, floors_.tolerant() ? slack - floor : slack);
      }
    }
  }
  std::ranges::stable_sort(drivers, [](const auto& a, const auto& b) {
    return a.second > b.second;
  });
  swappable_drivers_.clear();
  for (const auto& [pin, slack] : drivers) {
    swappable_drivers_.push_back(pin);
  }
}

PowerParallelEvalPolicy::FloorRequired PowerParallelEvalPolicy::floorRequired()
    const
{
  std::vector<sta::Vertex*> order;
  sta::VertexId max_id = 0;
  sta::VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    sta::Vertex* vertex = vertex_iter.next();
    order.push_back(vertex);
    max_id = std::max(max_id, graph_->id(vertex));
  }
  std::ranges::sort(order, std::greater{}, &sta::Vertex::level);

  const sta::SceneSeq& scenes = sta_->scenes();
  std::vector<sta::DcalcAPIndex> ap_indices;
  for (const sta::Scene* scene : scenes) {
    ap_indices.push_back(scene->dcalcAnalysisPtIndex(max_));
  }
  FloorRequired required(max_id + 1, {sta::INF, sta::INF});
  for (const auto& [pin, slack] : pre_batch_endpoints_) {
    sta::Vertex* vertex = graph_->pinLoadVertex(pin);
    if (vertex == nullptr) {
      continue;
    }
    for (const sta::RiseFall* rf : sta::RiseFall::range()) {
      required[graph_->id(vertex)][rf->index()]
          = sta::delayAsFloat(
                sta_->required(vertex, rf->asRiseFallBoth(), scenes, max_))
            - floors_.endpoint(pin);
    }
  }

  // Back from the endpoints by level.  An arc's delay is its largest over the
  // scenes, so the room errs low.
  const sta::SearchPred* pred = sta_->search()->evalPred();
  // Returns whether the edge lowered a required time of its from vertex.
  const auto pull = [&](sta::Edge* edge) {
    std::array<float, 2>& from = required[graph_->id(edge->from(graph_))];
    const std::array<float, 2>& to_required
        = required[graph_->id(edge->to(graph_))];
    bool lowered = false;
    for (const sta::TimingArc* arc : edge->timingArcSet()->arcs()) {
      const sta::RiseFall* from_rf = arc->fromEdge()->asRiseFall();
      const sta::RiseFall* to_rf = arc->toEdge()->asRiseFall();
      if (from_rf == nullptr || to_rf == nullptr) {
        continue;
      }
      float delay = 0.0f;
      for (const sta::DcalcAPIndex ap_index : ap_indices) {
        delay = std::max(
            delay, sta::delayAsFloat(graph_->arcDelay(edge, arc, ap_index)));
      }
      float& from_required = from[from_rf->index()];
      if (to_required[to_rf->index()] - delay < from_required) {
        from_required = to_required[to_rf->index()] - delay;
        lowered = true;
      }
    }
    return lowered;
  };
  // A transparent latch passes later data on to its output, but levels do not
  // order its D pin after its output: its D to Q edges pull after each pass,
  // and the pass repeats while they lower a required time.
  std::vector<sta::Edge*> latch_edges;
  for (int pass = 0; pass <= kLatchPasses; ++pass) {
    for (sta::Vertex* vertex : order) {
      sta::VertexOutEdgeIterator edge_iter(vertex, graph_);
      while (edge_iter.hasNext()) {
        sta::Edge* edge = edge_iter.next();
        sta::Vertex* to = edge->to(graph_);
        if (edge->role()->isTimingCheck() || !pred->searchThru(edge)) {
          continue;
        }
        if (to->level() > vertex->level()) {
          pull(edge);
        } else if (pass == 0 && edge->role() == sta::TimingRole::latchDtoQ()) {
          latch_edges.push_back(edge);
        }
      }
    }
    bool lowered = false;
    for (sta::Edge* edge : latch_edges) {
      lowered |= pull(edge);
    }
    if (!lowered) {
      break;
    }
  }
  return required;
}

float PowerParallelEvalPolicy::floorRoom(sta::Vertex* vertex,
                                         const FloorRequired& required) const
{
  const sta::SceneSeq& scenes = sta_->scenes();
  float room = sta::INF;
  if (graph_->id(vertex) >= required.size()) {
    return room;
  }
  for (const sta::RiseFall* rf : sta::RiseFall::range()) {
    const float shifted = required[graph_->id(vertex)][rf->index()];
    const float arrival = sta::delayAsFloat(
        sta_->arrival(vertex, rf->asRiseFallBoth(), scenes, max_));
    if (shifted < sta::INF && arrival > -sta::INF) {
      room = std::min(room, shifted - arrival);
    }
  }
  return room;
}

void PowerParallelEvalPolicy::setBudgetFloor(const double loss)
{
  budget_floor_loss_ = loss;
  const float below
      = reference_period_ < sta::INF ? loss / 100.0 * reference_period_ : 0.0f;
  const float floor = std::min(*config_.vt_budget_ws,
                               static_cast<float>(config_.setup_slack_margin))
                      - below;
  floors_ = SlackFloors::uniform(floor);
  const sta::Unit* time_unit = sta_->units()->timeUnit();
  logger_->info(utl::RSZ,
                3267,
                "VT budget: every endpoint may fall to setup slack {}{}.",
                time_unit->asString(floor),
                time_unit->scaleAbbrevSuffix());
}

bool PowerParallelEvalPolicy::lowerBudgetFloor()
{
  if (!config_.vt_budget_ws.has_value() || budget_floor_loss_ >= 100.0) {
    return false;
  }
  // Fine steps first, then half again each time.
  setBudgetFloor(
      budget_floor_loss_ == 0.0
          ? 1.0
          : std::min(100.0,
                     std::max(budget_floor_loss_ + 1.0,
                              std::round(budget_floor_loss_ * 1.5))));
  // The room grew, so the swaps over the old one may fit.
  rejected_.clear();
  return true;
}

void PowerParallelEvalPolicy::startTailOrFinish(const char* reason)
{
  if (vt_budget_.lowering() && !vt_budget_.met() && lowerBudgetFloor()) {
    checkpoint_misses_ = 0;
    return;
  }
  SetupParallelEvalPolicy::startTailOrFinish(reason);
}

bool PowerParallelEvalPolicy::canSwap(const sta::Instance* inst) const
{
  sta::LibertyCell* from = network_->libertyCell(inst);
  return std::ranges::any_of(higherVtCells(resizer_, from),
                             [&](const sta::LibertyCell* cell) {
                               return !isRejected(rejected_, inst, cell)
                                      && vt_budget_.admits(from, cell);
                             });
}

std::vector<Target> PowerParallelEvalPolicy::collectTargets(
    const size_t begin_endpoint,
    const size_t end_endpoint,
    std::unordered_set<sta::Pin*>& visited_pins) const
{
  std::vector<Target> targets;
  for (size_t index = begin_endpoint; index < end_endpoint; ++index) {
    sta::Pin* pin = swappable_drivers_[index];
    if (visited_pins.insert(pin).second) {
      Target target = instanceTarget(pin);
      if (target.driver_pin != nullptr) {
        target.fanout = targetFanout(target);
        targets.push_back(target);
      }
    }
  }
  return targets;
}

Target PowerParallelEvalPolicy::instanceTarget(sta::Pin* driver_pin) const
{
  sta::Vertex* vertex = graph_->pinDrvrVertex(driver_pin);
  const sta::Path* path = sta_->vertexWorstSlackPath(vertex, max_);
  // An unconstrained pin has no slack path; its arrival path still gives the
  // estimator the stages.
  if (path == nullptr) {
    path = sta_->vertexWorstArrivalPath(vertex, max_);
  }
  if (path == nullptr) {
    return {};
  }
  const sta::PathExpanded expanded(path, sta_);
  const int index = static_cast<int>(expanded.size()) - 1;
  const sta::Path* driver_path = expanded.path(index);
  if (index < static_cast<int>(expanded.startIndex())
      || driver_path->pin(sta_) != driver_pin
      || driver_path->prevArc(sta_) == nullptr) {
    return {};
  }
  return makePathDriverTarget(
      path, expanded, index, sta_->slack(vertex, max_), resizer_);
}

const std::vector<sta::Vertex*>& PowerParallelEvalPolicy::worstPathDrivers(
    sta::Vertex* driver) const
{
  const auto memo = worst_paths_.find(driver);
  if (memo != worst_paths_.end()) {
    return memo->second;
  }
  std::vector<sta::Vertex*>& drivers = worst_paths_[driver];
  if (const sta::Path* path = sta_->vertexWorstSlackPath(driver, max_)) {
    const sta::PathExpanded expanded(path, sta_);
    for (size_t i = expanded.startIndex(); i < expanded.size(); ++i) {
      sta::Vertex* vertex = expanded.path(i)->vertex(sta_);
      if (vertex->isDriver(network_)) {
        drivers.push_back(vertex);
      }
    }
  }
  if (drivers.empty() || drivers.back() != driver) {
    drivers.push_back(driver);
  }

  // The edges STA times forward raise the level, so the walk ends on a
  // combinational loop too.
  const sta::SearchPred* pred = sta_->search()->evalPred();
  sta::Vertex* vertex = driver;
  while (!sta_->search()->isEndpoint(vertex)) {
    sta::Vertex* next = nullptr;
    float next_slack = sta::INF;
    sta::VertexOutEdgeIterator edge_iter(vertex, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* to = edge->to(graph_);
      if (to->level() <= vertex->level() || !pred->searchThru(edge)) {
        continue;
      }
      const float slack = sta::delayAsFloat(sta_->slack(to, max_));
      if (slack < next_slack) {
        next = to;
        next_slack = slack;
      }
    }
    if (next == nullptr) {
      break;
    }
    vertex = next;
    if (vertex->isDriver(network_)) {
      drivers.push_back(vertex);
    }
  }
  return drivers;
}

void PowerParallelEvalPolicy::selectMoveSequence()
{
  move_sequence_.clear();
  if (moveEnabled(MoveType::kVtSwap)) {
    move_sequence_.push_back(MoveType::kVtSwap);
  }
  unbuffer_in_sequence_ = false;
}

void PowerParallelEvalPolicy::buildMoveGenerators(
    const std::vector<MoveType>& move_types,
    const GeneratorContext& context)
{
  move_generators_.clear();
  for (const MoveType type : move_types) {
    if (type == MoveType::kVtSwap) {
      move_generators_.push_back(std::make_unique<PowerVtSwapGenerator>(
          context, power_model_, rejected_, floors_, vt_budget_, slack_ratio_));
    }
  }
}

PowerParallelEvalPolicy::Contract PowerParallelEvalPolicy::measureContract()
    const
{
  const sta::MinMax* min = sta::MinMax::min();
  return {.setup_ws = sta_->worstSlack(max_),
          .setup_tns = sta_->totalNegativeSlack(max_),
          .hold_ws = sta_->worstSlack(min),
          .hold_tns = sta_->totalNegativeSlack(min)};
}

namespace {

// The side slack floor is the design WNS, which can fall during the run, so
// those rejects may clear.
bool staysRejected(const std::string_view reason)
{
  return reason != failReasonName(FailReason::kSideSlackFloorViolation)
         && reason != failReasonName(FailReason::kDualRfNonImproving);
}

// Float arrivals near 1 ns round at ~0.1 fs, and a re-estimated net moves
// an untouched path by a few of those.
constexpr float kSlackNoise = 1e-15f;

bool wsKept(const sta::Slack ws, const sta::Slack start, const float margin)
{
  return std::min(sta::delayAsFloat(ws) - margin, 0.0f)
         >= std::min(sta::delayAsFloat(start) - margin, 0.0f) - kSlackNoise;
}

bool tnsKept(const sta::Slack tns,
             const sta::Slack start,
             const float tolerance)
{
  return !sta::fuzzyLess(sta::delayAsFloat(tns),
                         sta::delayAsFloat(start) * (1.0f + tolerance));
}

}  // namespace

bool PowerParallelEvalPolicy::setupKept(const Contract& now) const
{
  if (floors_.tolerant()) {
    return brokenEndpoints(now).empty();
  }
  return wsKept(
             now.setup_ws, start_contract_.setup_ws, config_.setup_slack_margin)
         && tnsKept(now.setup_tns, start_contract_.setup_tns, kTnsTolerance);
}

bool PowerParallelEvalPolicy::holdKept(const Contract& now) const
{
  return wsKept(now.hold_ws, start_contract_.hold_ws, config_.hold_slack_margin)
         && tnsKept(now.hold_tns, start_contract_.hold_tns, kTnsTolerance);
}

bool PowerParallelEvalPolicy::acceptsBatch(sta::Slack, sta::Slack) const
{
  const Contract now = measureContract();
  return setupKept(now) && holdKept(now);
}

std::vector<sta::Vertex*> PowerParallelEvalPolicy::brokenEndpoints(
    const Contract& now) const
{
  if (floors_.tolerant()) {
    std::vector<sta::Vertex*> broken;
    for (const auto& [pin, before] : pre_batch_endpoints_) {
      sta::Vertex* vertex = graph_->pinLoadVertex(pin);
      if (vertex != nullptr
          && sta::delayAsFloat(sta_->slack(vertex, max_))
                 < floors_.endpoint(pin) - kSlackNoise) {
        broken.push_back(vertex);
      }
    }
    return broken;
  }
  const float margin = config_.setup_slack_margin;
  const float ws_bound
      = std::min(sta::delayAsFloat(start_contract_.setup_ws) - margin, 0.0f);
  float excess
      = sta::delayAsFloat(start_contract_.setup_tns) * (1.0f + kTnsTolerance)
        - sta::delayAsFloat(now.setup_tns);
  std::vector<sta::Vertex*> broken;
  std::vector<std::pair<float, sta::Vertex*>> drops;
  for (const auto& [pin, before] : pre_batch_endpoints_) {
    sta::Vertex* vertex = graph_->pinLoadVertex(pin);
    if (vertex == nullptr) {
      continue;
    }
    const float after = sta::delayAsFloat(sta_->slack(vertex, max_));
    const float drop
        = std::min(sta::delayAsFloat(before), 0.0f) - std::min(after, 0.0f);
    if (std::min(after - margin, 0.0f) < ws_bound - kSlackNoise) {
      broken.push_back(vertex);
      excess -= std::max(drop, 0.0f);
    } else if (drop > 0.0f) {
      drops.emplace_back(drop, vertex);
    }
  }

  std::ranges::stable_sort(
      drops, std::greater{}, [](const auto& drop) { return drop.first; });
  for (const auto& [drop, vertex] : drops) {
    if (excess <= 0.0f) {
      break;
    }
    broken.push_back(vertex);
    excess -= drop;
  }
  return broken;
}

void PowerParallelEvalPolicy::addPathMoves(
    sta::Vertex* endpoint,
    const std::unordered_map<const sta::Instance*, MoveCandidate*>& moves,
    std::unordered_set<MoveCandidate*>& on_path,
    std::unordered_set<MoveCandidate*>& beside) const
{
  const sta::Path* path = sta_->vertexWorstSlackPath(endpoint, max_);
  if (path == nullptr) {
    return;
  }
  const auto add
      = [&](const sta::Instance* inst, std::unordered_set<MoveCandidate*>& to) {
          const auto move = moves.find(inst);
          if (move != moves.end()) {
            to.insert(move->second);
          }
        };
  const auto add_net = [&](const sta::Net* net) {
    if (net == nullptr) {
      return;
    }
    std::unique_ptr<sta::NetConnectedPinIterator> pin_iter(
        network_->connectedPinIterator(net));
    while (pin_iter->hasNext()) {
      add(network_->instance(pin_iter->next()), beside);
    }
  };

  const sta::Mode* mode = sta_->cmdMode();
  // A path through a transparent latch starts at its D pin, which the moves
  // on the worst path to that pin delay too.
  for (int latch = 0; path != nullptr && latch <= kLatchPasses; ++latch) {
    const sta::PathExpanded expanded(path, sta_);
    for (size_t i = expanded.startIndex(); i < expanded.size(); ++i) {
      const sta::Pin* pin = expanded.path(i)->pin(sta_);
      const sta::Instance* inst = network_->instance(pin);
      if (inst == network_->topInstance()) {
        continue;
      }
      add(inst, on_path);
      const sta::LibertyCell* cell = network_->libertyCell(inst);
      const bool sequential = cell != nullptr && cell->isSequential();
      if (!sequential && !network_->isDriver(pin)) {
        continue;
      }
      if (network_->isDriver(pin)) {
        add_net(network_->net(pin));
      }
      std::unique_ptr<sta::InstancePinIterator> pin_iter(
          network_->pinIterator(inst));
      while (pin_iter->hasNext()) {
        const sta::Pin* inst_pin = pin_iter->next();
        if (network_->isLoad(inst_pin)
            && (network_->isDriver(pin) || sta_->isClock(inst_pin, mode))) {
          add_net(network_->net(inst_pin));
        }
      }
    }
    const sta::Pin* start
        = expanded.size() > 0 ? expanded.path(0)->pin(sta_) : nullptr;
    sta::Vertex* latch_d = start != nullptr && network_->isLatchData(start)
                               ? graph_->pinLoadVertex(start)
                               : nullptr;
    path = latch_d != nullptr ? sta_->vertexWorstArrivalPath(latch_d, max_)
                              : nullptr;
  }
}

std::array<float, 2> PowerParallelEvalPolicy::maxSlews(
    sta::Vertex* vertex) const
{
  std::array<float, 2> slews{-sta::INF, -sta::INF};
  for (const sta::Scene* scene : sta_->scenes()) {
    const sta::DcalcAPIndex ap = scene->dcalcAnalysisPtIndex(max_);
    for (const sta::RiseFall* rf : sta::RiseFall::range()) {
      slews[rf->index()] = std::max(
          slews[rf->index()], sta::delayAsFloat(graph_->slew(vertex, rf, ap)));
    }
  }
  return slews;
}

void PowerParallelEvalPolicy::savePreBatchSlews()
{
  sta::VertexId max_id = 0;
  sta::VertexIterator id_iter(graph_);
  while (id_iter.hasNext()) {
    max_id = std::max(max_id, graph_->id(id_iter.next()));
  }
  pre_batch_slews_.assign(max_id + 1, {sta::INF, sta::INF});
  sta::VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    sta::Vertex* vertex = vertex_iter.next();
    pre_batch_slews_[graph_->id(vertex)] = maxSlews(vertex);
  }
}

void PowerParallelEvalPolicy::addSlewSources(
    sta::Vertex* endpoint,
    const std::unordered_map<const sta::Instance*, MoveCandidate*>& moves,
    std::unordered_set<MoveCandidate*>& sources) const
{
  const sta::Path* path = sta_->vertexWorstSlackPath(endpoint, max_);
  if (path == nullptr) {
    return;
  }
  const auto slew_grew = [&](sta::Vertex* vertex) {
    const size_t id = graph_->id(vertex);
    if (id >= pre_batch_slews_.size()) {
      return false;
    }
    const std::array<float, 2> now = maxSlews(vertex);
    const std::array<float, 2>& before = pre_batch_slews_[id];
    return now[0] > before[0] + kSlackNoise || now[1] > before[1] + kSlackNoise;
  };
  const auto add = [&](const sta::Pin* pin) {
    const auto move = moves.find(network_->instance(pin));
    if (move == moves.end()) {
      return false;
    }
    sources.insert(move->second);
    return true;
  };
  const auto load_grew = [&](const sta::Pin* load) {
    const sta::LibertyPort* port = network_->libertyPort(load);
    const auto swap = std::ranges::find(
        batch_swaps_, network_->instance(load), &SwapRecord::inst);
    if (port == nullptr || swap == batch_swaps_.end() || swap->reverted) {
      return false;
    }
    const sta::LibertyPort* before
        = swap->before_cell->findLibertyPort(port->name());
    return before != nullptr && port->capacitance() > before->capacitance();
  };

  // The moves on the path and beside it are addPathMoves()'s.
  std::unordered_set<sta::Vertex*> path_vertices;
  std::unordered_set<sta::Vertex*> seen;
  std::vector<sta::Vertex*> stack;
  const sta::PathExpanded expanded(path, sta_);
  for (size_t i = 0; i < expanded.size(); ++i) {
    sta::Vertex* vertex = expanded.path(i)->vertex(sta_);
    path_vertices.insert(vertex);
    if (slew_grew(vertex) && seen.insert(vertex).second) {
      stack.push_back(vertex);
    }
  }
  while (!stack.empty()) {
    sta::Vertex* vertex = stack.back();
    stack.pop_back();
    const sta::Pin* pin = vertex->pin();
    if (!path_vertices.contains(vertex)) {
      if (add(pin)) {
        continue;
      }
      if (vertex->isDriver(network_)) {
        std::unique_ptr<sta::PinConnectedPinIterator> pin_iter(
            network_->connectedPinIterator(pin));
        while (pin_iter->hasNext()) {
          const sta::Pin* load = pin_iter->next();
          if (load != pin && network_->isLoad(load) && load_grew(load)) {
            add(load);
          }
        }
      }
    }
    // A timing check does not carry slew.
    sta::VertexInEdgeIterator edge_iter(vertex, graph_);
    while (edge_iter.hasNext()) {
      sta::Edge* edge = edge_iter.next();
      sta::Vertex* from = edge->from(graph_);
      if (!edge->role()->isTimingCheck() && slew_grew(from)
          && seen.insert(from).second) {
        stack.push_back(from);
      }
    }
  }
}

std::vector<MoveCandidate*> PowerParallelEvalPolicy::findBatchCulprits(
    const sta::Slack pre_batch_wns,
    std::vector<MoveCandidate*>& spared) const
{
  // A side pin below the pre-batch WNS made a new worst path; with a
  // tolerance, one past its room broke an endpoint's floor.
  const float side_floor
      = std::min(static_cast<float>(config_.setup_slack_margin),
                 sta::delayAsFloat(pre_batch_wns));
  const auto side_broke = [&](const sta::Pin* pin) {
    if (!floors_.tolerant()) {
      return setupBelow(pin, side_floor);
    }
    sta::Vertex* vertex = pin != nullptr ? graph_->pinLoadVertex(pin) : nullptr;
    return vertex != nullptr
           && floorRoom(vertex, floor_required_) < -kSlackNoise;
  };
  // A swap seldom speeds up a hold path, so a hold failure is charged to the
  // swaps that own a pin below the hold margin.
  const Contract now = measureContract();
  const bool hold_failed = !holdKept(now);
  const float hold_floor
      = std::min(static_cast<float>(config_.hold_slack_margin),
                 sta::delayAsFloat(start_contract_.hold_ws));

  std::unordered_set<MoveCandidate*> on_path;
  std::unordered_set<MoveCandidate*> beside;
  std::unordered_set<MoveCandidate*> slew_sources;
  size_t broken_endpoints = 0;
  const bool setup_failed = !setupKept(now);
  if (setup_failed) {
    std::unordered_map<const sta::Instance*, MoveCandidate*> moves;
    for (MoveCandidate* candidate : batch_candidates_) {
      moves[candidate->target().inst(resizer_)] = candidate;
    }
    const std::vector<sta::Vertex*> broken = brokenEndpoints(now);
    broken_endpoints = broken.size();
    for (sta::Vertex* endpoint : broken) {
      addPathMoves(endpoint, moves, on_path, beside);
      addSlewSources(endpoint, moves, slew_sources);
    }
  }

  // The moves beside a path may be innocent, so they go back only when the
  // culprits alone do not restore the contract.
  std::vector<MoveCandidate*> culprits;
  std::vector<MoveCandidate*> lasting;
  for (MoveCandidate* candidate : batch_candidates_) {
    const Target& target = candidate->target();
    const bool slew_source = slew_sources.contains(candidate);
    const bool broke = on_path.contains(candidate) || slew_source
                       || (setup_failed
                           && (driverFell(target)
                               || std::ranges::any_of(
                                   candidate->guardedSidePins(), side_broke)))
                       || (hold_failed && holdBelow(target, hold_floor));
    if (broke) {
      culprits.push_back(candidate);
      if (slew_source || misestimated(*candidate)) {
        lasting.push_back(candidate);
      }
    } else if (beside.contains(candidate)) {
      spared.push_back(candidate);
    }
  }
  if (culprits.empty()) {
    culprits = std::move(spared);
    spared.clear();
  }
  debugPrint(logger_,
             utl::RSZ,
             "repair_power",
             1,
             "batch {} missed: setup WS {:.6e} -> {:.6e}, {} culprits ({} "
             "rejected for the run) and {} spared of {} moves, {} broken "
             "endpoints",
             batch_index_,
             sta::delayAsFloat(start_contract_.setup_ws),
             sta::delayAsFloat(now.setup_ws),
             culprits.size(),
             lasting.size(),
             spared.size(),
             batch_candidates_.size(),
             broken_endpoints);
  // A swap whose estimate was short, or whose slew growth reached a broken
  // path its estimate does not see, would break it again; the others broke
  // the contract together and come back after the tabu.  Culprits that cover
  // the batch are not salvaged.
  if (culprits.size() < batch_candidates_.size()) {
    for (const MoveCandidate* culprit : lasting) {
      rejectSwap(*culprit);
    }
  }
  return culprits;
}

bool PowerParallelEvalPolicy::misestimated(const MoveCandidate& candidate) const
{
  const std::optional<float> slowdown = selfStageDelayDelta(candidate);
  return !slowdown.has_value()
         || *slowdown
                > static_cast<const PowerSwapCandidate&>(candidate).cost();
}

bool PowerParallelEvalPolicy::setupBelow(const sta::Pin* pin,
                                         const float floor) const
{
  sta::Vertex* vertex = pin != nullptr ? graph_->pinLoadVertex(pin) : nullptr;
  return vertex != nullptr
         && sta::fuzzyLess(sta::delayAsFloat(sta_->slack(vertex, max_)), floor);
}

bool PowerParallelEvalPolicy::driverFell(const Target& target) const
{
  sta::Vertex* vertex = graph_->pinDrvrVertex(target.driver_pin);
  if (vertex == nullptr) {
    return false;
  }
  const float slack = sta::delayAsFloat(sta_->slack(vertex, max_));
  return sta::fuzzyLess(slack, floors_.driver(target.driver_pin))
         && sta::fuzzyLess(slack, sta::delayAsFloat(target.slack));
}

bool PowerParallelEvalPolicy::holdBelow(const Target& target,
                                        const float floor) const
{
  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network_->pinIterator(target.inst(resizer_)));
  while (pin_iter->hasNext()) {
    sta::Vertex* vertex = graph_->pinLoadVertex(pin_iter->next());
    if (vertex != nullptr
        && sta::fuzzyLess(
            sta::delayAsFloat(sta_->slack(vertex, sta::MinMax::min())),
            floor)) {
      return true;
    }
  }
  return false;
}

void PowerParallelEvalPolicy::noteBudgetReject(const MoveCandidate& candidate)
{
  const auto& swap = static_cast<const PowerSwapCandidate&>(candidate);
  const sta::Instance* inst = swap.target().inst(resizer_);
  const sta::LibertyCell* cell = network_->libertyCell(inst);
  BudgetReject& reject = budget_rejects_[inst];
  if (reject.from_cell != cell) {
    reject = {.from_cell = cell, .gain = 0.0f};
  }
  reject.gain = std::max(reject.gain, swap.powerGain());
}

void PowerParallelEvalPolicy::finish()
{
  SetupParallelEvalPolicy::finish();
  // The power the slack budget left: instances still on the cell a budget
  // reject would have left.  Summed in hash order, so in double for printed
  // digits that do not change from run to run.
  double left = 0.0;
  size_t stayed = 0;
  for (const auto& [inst, reject] : budget_rejects_) {
    if (network_->libertyCell(inst) == reject.from_cell) {
      left += reject.gain;
      ++stayed;
    }
  }
  debugPrint(logger_,
             utl::RSZ,
             "repair_power",
             1,
             "slack budget left {:.3e} W on {} instances ({:.1f}% of the "
             "{:.3e} W saved); {} more instances swapped after their reject",
             left,
             stayed,
             total_saved_ > 0.0f ? 100.0f * left / total_saved_ : 0.0f,
             total_saved_,
             budget_rejects_.size() - stayed);
}

void PowerParallelEvalPolicy::selectBatch(BatchPool& pool, const int batch_cap)
{
  // The budgets turn some of the best scores away, so the selection takes
  // twice the cap and the cap applies to the admitted moves.
  for (const TargetEvaluation& evaluation : pool.evaluations) {
    for (size_t i = 0; i < evaluation.candidates.size(); ++i) {
      const Estimate& estimate = evaluation.estimates[i];
      if (!estimate.legal && staysRejected(estimate.reason)) {
        rejectSwap(*evaluation.candidates[i]);
        if (estimate.reason == PowerSwapCandidate::kBudgetReason) {
          noteBudgetReject(*evaluation.candidates[i]);
        }
      }
    }
  }
  SetupParallelEvalPolicy::selectBatch(pool, 2 * batch_cap);
  // The cost admitted at each driver, the admitted moves whose worst path
  // covers each driver, and what each admitted move has left to spend.
  std::unordered_map<sta::Vertex*, float> cost_at;
  std::unordered_map<sta::Vertex*, std::vector<size_t>> covering;
  std::vector<float> remaining;
  std::vector<size_t> admitted;
  // The VT budgets with the admitted swaps.
  VtBudgetCounts vt_budget = vt_budget_;
  for (const size_t index : pool.selection.evaluation_indices) {
    if (static_cast<int>(admitted.size()) >= batch_cap) {
      break;
    }
    const auto& candidate = static_cast<const PowerSwapCandidate&>(
        pool.evaluations[index].bestCandidate());
    const Target& target = candidate.target();
    sta::Vertex* driver = graph_->pinDrvrVertex(target.driver_pin);
    const std::vector<sta::Vertex*>& path = worstPathDrivers(driver);
    const float cost = candidate.cost();
    float spent = 0.0f;
    for (sta::Vertex* vertex : path) {
      const auto at = cost_at.find(vertex);
      if (at != cost_at.end()) {
        spent += at->second;
      }
    }
    const float limit = slack_ratio_
                        * (sta::delayAsFloat(target.slack)
                           - floors_.driver(target.driver_pin));
    if (spent + cost > limit) {
      continue;
    }
    std::vector<size_t>& covered_by = covering[driver];
    if (std::ranges::any_of(covered_by, [&](const size_t move) {
          return remaining[move] < cost;
        })) {
      continue;
    }
    const sta::LibertyCell* from = network_->libertyCell(target.inst(resizer_));
    if (!vt_budget.admits(from, candidate.candidateCell())) {
      continue;
    }
    vt_budget.move(from, candidate.candidateCell());
    for (const size_t move : covered_by) {
      remaining[move] -= cost;
    }
    cost_at[driver] += cost;
    const size_t move = remaining.size();
    remaining.push_back(limit - spent - cost);
    for (sta::Vertex* vertex : path) {
      covering[vertex].push_back(move);
    }
    admitted.push_back(index);
  }
  pool.selection.evaluation_indices = std::move(admitted);
}

void PowerParallelEvalPolicy::updateTiming()
{
  resizer_.estimateParasitics()->finishIncrementalRoutes();
  SetupParallelEvalPolicy::updateTiming();
}

void PowerParallelEvalPolicy::rejectSwap(const MoveCandidate& candidate) const
{
  const auto& swap = static_cast<const PowerSwapCandidate&>(candidate);
  const sta::Instance* inst = swap.target().inst(resizer_);
  if (!isRejected(rejected_, inst, swap.candidateCell())) {
    rejected_[inst].push_back(swap.candidateCell());
  }
}

void PowerParallelEvalPolicy::saveBatchCheckpoint(const sta::Slack wns,
                                                  const sta::Slack tns)
{
  std::unordered_set<const MoveCandidate*> reverted;
  for (const SwapRecord& swap : batch_swaps_) {
    if (swap.reverted) {
      reverted.insert(swap.candidate);
    }
  }
  for (const MoveCandidate* candidate : batch_candidates_) {
    if (!reverted.contains(candidate)) {
      batch_saved_
          += static_cast<const PowerSwapCandidate*>(candidate)->powerGain();
    }
  }
  SetupParallelEvalPolicy::saveBatchCheckpoint(wns, tns);
}

}  // namespace rsz
