// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SetupLegacyBase.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizationPolicy.hh"
#include "OptimizerTypes.hh"
#include "Rebuffer.hh"
#include "RepairTargetCollector.hh"
#include "SizeUpGenerator.hh"
#include "SizeUpMatchGenerator.hh"
#include "SwapPinsGenerator.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"
#include "utl/mem_stats.h"
#include "utl/timer.h"

namespace rsz {

using std::pair;
using std::string;
using std::vector;

using utl::RSZ;

SetupLegacyBase::SetupLegacyBase(Resizer& resizer,
                                 MoveCommitter& committer,
                                 RepairSetupContext& setup_context,
                                 const OptimizerRunConfig& config)
    : OptimizationPolicy(resizer, committer, setup_context, config)
{
}

bool SetupLegacyBase::start()
{
  OptimizationPolicy::start();
  setup_context_.max_repairs_per_pass = config_.max_repairs_per_pass;
  if (!setup_context_.phase_pipeline_active) {
    return true;
  }
  if (!setup_context_.legacy_preamble_done) {
    setup_context_.legacy_preamble_done = true;
    setup_context_.legacy_preamble_has_violations = prepareForPhasePipeline();
  }
  return setup_context_.legacy_preamble_has_violations;
}

void SetupLegacyBase::iterate()
{
  // Used as the shared prepare/report policy; phases run via derived classes.
  markRunComplete(true);
}

bool SetupLegacyBase::repairSetupPin(const sta::Pin* end_pin)
{
  if (end_pin == nullptr) {
    return false;
  }

  initializeSetupServices();
  setup_context_.max_repairs_per_pass = 1;

  sta::Vertex* end_vertex = graph_->pinLoadVertex(end_pin);
  if (end_vertex == nullptr) {
    return false;
  }

  const sta::Slack end_slack = sta_->slack(end_vertex, max_);
  sta::Path* end_path = sta_->vertexWorstSlackPath(end_vertex, max_);
  if (end_path == nullptr) {
    return false;
  }
  buildMainMoveSequence(/*log_sequence=*/true);

  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  return repairPath(end_path, end_slack, /*force_single_repair=*/false);
}

void SetupLegacyBase::initializeSetupServices()
{
  resizer_.rebuffer_->init();
  resizer_.rebuffer_->initOnCorner(sta_->cmdScene());
}

bool SetupLegacyBase::hasVtSwapCells() const
{
  return resizer_.lib_data_->sorted_vt_categories.size() > 1;
}

///////////////////////////////////////////////////////////////////////////////
// Setup sequence helpers
///////////////////////////////////////////////////////////////////////////////

void SetupLegacyBase::pushMoveIfEnabled(const bool enabled, const MoveType type)
{
  if (enabled) {
    move_sequence_.push_back(type);
  }
}

void SetupLegacyBase::logMoveSequence() const
{
  string repair_moves = "Repair move sequence: ";
  for (const MoveType type : move_sequence_) {
    if (type == MoveType::kCount) {
      continue;
    }
    repair_moves += moveName(type);
    repair_moves += ' ';
  }
  logger_->info(utl::RSZ, 100, "{}", repair_moves);
}

void SetupLegacyBase::activateMoveSequence(const bool log_sequence)
{
  buildMoveGenerators(move_sequence_, makeGeneratorContext());
  if (log_sequence) {
    logMoveSequence();
  }
}

void SetupLegacyBase::buildMainMoveSequence(const bool log_sequence)
{
  move_sequence_.clear();

  if (!config_.sequence.empty()) {
    for (const MoveType move : config_.sequence) {
      switch (move) {
        case MoveType::kBuffer:
          pushMoveIfEnabled(!config_.skip_buffering, MoveType::kBuffer);
          break;
        case MoveType::kUnbuffer:
          pushMoveIfEnabled(!config_.skip_buffer_removal, MoveType::kUnbuffer);
          break;
        case MoveType::kSwapPins:
          pushMoveIfEnabled(!config_.skip_pin_swap, MoveType::kSwapPins);
          break;
        case MoveType::kSizeUp:
          move_sequence_.push_back(MoveType::kSizeUp);
          break;
        case MoveType::kSizeDown:
          pushMoveIfEnabled(!config_.skip_size_down, MoveType::kSizeDown);
          break;
        case MoveType::kClone:
          pushMoveIfEnabled(!config_.skip_gate_cloning, MoveType::kClone);
          break;
        case MoveType::kSplitLoad:
          pushMoveIfEnabled(!config_.skip_buffering, MoveType::kSplitLoad);
          break;
        case MoveType::kVtSwap:
          pushMoveIfEnabled(!config_.skip_vt_swap && hasVtSwapCells(),
                            MoveType::kVtSwap);
          break;
        case MoveType::kSizeUpMatch:
          move_sequence_.push_back(MoveType::kSizeUpMatch);
          break;
        case MoveType::kReroute:
          move_sequence_.push_back(MoveType::kReroute);
          break;
        case MoveType::kCount:
          break;
      }
    }
  } else {
    pushMoveIfEnabled(!config_.skip_buffer_removal, MoveType::kUnbuffer);
    pushMoveIfEnabled(!config_.skip_vt_swap && hasVtSwapCells(),
                      MoveType::kVtSwap);
    move_sequence_.push_back(MoveType::kSizeUp);
    if (!config_.skip_size_down) {
      // Disabled by default for legacy parity.
    }
    pushMoveIfEnabled(!config_.skip_pin_swap, MoveType::kSwapPins);
    pushMoveIfEnabled(!config_.skip_buffering, MoveType::kBuffer);
    pushMoveIfEnabled(!config_.skip_gate_cloning, MoveType::kClone);
    pushMoveIfEnabled(!config_.skip_buffering, MoveType::kSplitLoad);
  }

  activateMoveSequence(log_sequence);
}

SetupLegacyBase::ViolatingEnds SetupLegacyBase::collectViolatingEndpoints(
    const float setup_slack_margin) const
{
  target_collector_->init(setup_slack_margin);
  ViolatingEnds violating_ends;
  const auto& endpoint_pins = target_collector_->getViolatingEndpoints();
  violating_ends.reserve(endpoint_pins.size());
  for (const auto& [endpoint_pin, endpoint_slack] : endpoint_pins) {
    sta::Vertex* endpoint_vertex = graph_->pinLoadVertex(endpoint_pin);
    if (endpoint_vertex != nullptr) {
      violating_ends.emplace_back(endpoint_vertex, endpoint_slack);
    }
  }
  return violating_ends;
}

bool SetupLegacyBase::beginJournaledEndpointSearch(
    const pair<sta::Vertex*, sta::Slack>& end_original_slack,
    const int max_end_count,
    int& end_index,
    SetupLegacyBase::EndpointRepairState& endpoint_state)
{
  endpoint_state.end = end_original_slack.first;
  refreshEndpointSlacks(endpoint_state);
  target_collector_->useWorstEndpoint(endpoint_state.end);
  committer_.setCurrentEndpoint(endpoint_state.end->pin());
  ++end_index;
  if (end_index > max_end_count) {
    return false;
  }

  endpoint_state.pass = 1;
  endpoint_state.decreasing_slack_passes = 0;
  endpoint_state.force_single_repair = false;
  resizer_.journalBegin();
  endpoint_state.journal_open = true;
  return true;
}

void SetupLegacyBase::recordTermination(bool& prev_termination,
                                        bool& two_cons_terminations)
{
  if (prev_termination) {
    two_cons_terminations = true;
  } else {
    prev_termination = true;
  }
}

void SetupLegacyBase::acceptEndpointState(
    SetupLegacyBase::EndpointRepairState& endpoint_state)
{
  if (!endpoint_state.journal_open) {
    return;
  }

  resizer_.journalEnd();
  committer_.acceptPendingMoves();
  endpoint_state.journal_open = false;
}

void SetupLegacyBase::restoreEndpointState(
    SetupLegacyBase::EndpointRepairState& endpoint_state)
{
  if (!endpoint_state.journal_open) {
    return;
  }

  resizer_.journalRestore();
  committer_.rejectPendingMoves();
  endpoint_state.journal_open = false;
}

void SetupLegacyBase::finishEndpointSearch(
    SetupLegacyBase::EndpointRepairState& endpoint_state)
{
  if (endpoint_state.pass == 1) {
    acceptEndpointState(endpoint_state);
  } else {
    restoreEndpointState(endpoint_state);
  }
}

void SetupLegacyBase::saveImprovedCheckpoint(
    SetupLegacyBase::EndpointRepairState& endpoint_state)
{
  acceptEndpointState(endpoint_state);
  resizer_.journalBegin();
  endpoint_state.journal_open = true;
}

void SetupLegacyBase::refreshEndpointSlacks(
    SetupLegacyBase::EndpointRepairState& endpoint_state)
{
  endpoint_state.end_slack = sta_->slack(endpoint_state.end, max_);
  sta_->worstSlack(
      max_, endpoint_state.worst_slack, endpoint_state.worst_vertex);
}

bool SetupLegacyBase::reachedIterationLimit(const int iteration,
                                            const int max_iterations) const
{
  return max_iterations > 0 && iteration >= max_iterations;
}

bool SetupLegacyBase::prepareForPhasePipeline()
{
  initializeSetupServices();

  const float setup_slack_margin = config_.setup_slack_margin;
  const double repair_tns_end_percent = config_.repair_tns_end_percent;
  const int max_repairs_per_pass = config_.max_repairs_per_pass;

  setup_context_.max_repairs_per_pass = max_repairs_per_pass;

  target_collector_->init(setup_slack_margin);

  // Move sequence is logged before the violation summary so the move-sequence
  // line precedes "Found/Repairing" in the output.
  buildMainMoveSequence(/*log_sequence=*/true);

  const int total_violations = target_collector_->getTotalViolations();
  const sta::VertexSet& endpoints = sta_->endpoints();
  if (total_violations > 0) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "Violating endpoints {}/{} {}%",
               total_violations,
               endpoints.size(),
               int(total_violations / double(endpoints.size()) * 100));
  }
  if (total_violations == 0) {
    logger_->metric("design__instance__count__setup_buffer", 0);
    logger_->info(utl::RSZ, 98, "No setup violations found");
    return false;
  }
  logger_->info(utl::RSZ,
                94,
                "Found {} endpoints with setup violations.",
                total_violations);
  setup_context_.max_end_repairs
      = std::max(static_cast<int>(target_collector_->getMaxEndpointCount()
                                  * repair_tns_end_percent),
                 1);
  logger_->info(utl::RSZ,
                99,
                "Repairing {} out of {} ({:0.2f}%) violating endpoints...",
                setup_context_.max_end_repairs,
                total_violations,
                repair_tns_end_percent * 100.0);
  sta_->checkCapacitancesPreamble(sta_->scenes());
  sta_->checkSlewsPreamble();
  sta_->checkFanoutPreamble();

  if (committer_.moveTrackerEnabled()) {
    committer_.captureInitialSlackDistribution();
    committer_.captureOriginalEndpointSlack();
  }
  if (!config_.phases.empty()) {
    reportCustomPhaseSetup();
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Path repair
///////////////////////////////////////////////////////////////////////////////

int SetupLegacyBase::fanout(sta::Vertex* vertex) const
{
  int fanout_count = 0;
  sta::VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (edge->isWire()) {
      fanout_count++;
    }
  }
  return fanout_count;
}

std::vector<std::pair<int, sta::Delay>> SetupLegacyBase::rankPathDrivers(
    sta::PathExpanded& expanded,
    const sta::Scene* corner,
    const int lib_ap) const
{
  vector<pair<int, sta::Delay>> load_delays;
  const int start_index = expanded.startIndex();
  const int path_length = expanded.size();

  // Rank driver stages by load-dependent delay so the noisiest points on the
  // expanded path get first crack at the repair budget.
  for (int i = start_index; i < path_length; i++) {
    const sta::Path* path_vertex_path = expanded.path(i);
    sta::Vertex* path_vertex = path_vertex_path->vertex(sta_);
    const sta::Pin* path_pin = path_vertex_path->pin(sta_);
    if (i > 0 && path_vertex->isDriver(network_)
        && !network_->isTopLevelPort(path_pin)) {
      const sta::TimingArc* prev_arc = path_vertex_path->prevArc(sta_);
      const sta::TimingArc* corner_arc = prev_arc->sceneArc(lib_ap);
      sta::Edge* prev_edge = path_vertex_path->prevEdge(sta_);
      const sta::Delay load_delay
          = graph_->arcDelay(
                prev_edge, prev_arc, corner->dcalcAnalysisPtIndex(max_))
            - corner_arc->intrinsicDelay();
      load_delays.emplace_back(i, load_delay);
    }
  }

  std::ranges::sort(
      load_delays, [](pair<int, sta::Delay> lhs, pair<int, sta::Delay> rhs) {
        return lhs.second > rhs.second
               || (lhs.second == rhs.second && lhs.first > rhs.first);
      });
  return load_delays;
}

int SetupLegacyBase::repairBudget(const sta::Slack path_slack,
                                  const bool force_single_repair) const
{
  int repairs_per_pass = 1;
  if (setup_context_.max_viol - setup_context_.min_viol != 0.0) {
    repairs_per_pass
        += std::round((setup_context_.max_repairs_per_pass - 1)
                      * (-path_slack - setup_context_.min_viol)
                      / (setup_context_.max_viol - setup_context_.min_viol));
  }
  return force_single_repair ? 1 : repairs_per_pass;
}

bool SetupLegacyBase::makePinTargetOnPath(const sta::Pin* pin,
                                          const sta::Path* path,
                                          const sta::Slack focus_slack,
                                          Target& target) const
{
  if (path == nullptr) {
    return false;
  }

  sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
  if (vertex == nullptr) {
    return false;
  }

  sta::PathExpanded expanded(path, sta_);
  for (int index = expanded.startIndex(); index < expanded.size(); index++) {
    const sta::Path* driver_path = expanded.path(index);
    sta::Vertex* path_vertex = driver_path->vertex(sta_);
    const sta::Pin* path_pin = driver_path->pin(sta_);
    if (path_vertex != vertex || path_pin != pin) {
      continue;
    }
    if (!path_vertex->isDriver(network_)
        || network_->isTopLevelPort(path_pin)) {
      return false;
    }

    makePathDriverTarget(path, expanded, index, focus_slack, target);
    return true;
  }

  return false;
}

bool SetupLegacyBase::makePinTarget(const sta::Pin* pin,
                                    const sta::Slack focus_slack,
                                    Target& target) const
{
  sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
  if (vertex == nullptr) {
    return false;
  }

  sta::Path* path = sta_->vertexWorstSlackPath(vertex, max_);
  return makePinTargetOnPath(pin, path, focus_slack, target);
}

bool SetupLegacyBase::repairPins(
    const std::vector<const sta::Pin*>& pins,
    const sta::Path* focus_path,
    const std::unordered_map<const sta::Pin*, std::unordered_set<MoveType>>*
        rejected_moves,
    std::vector<std::pair<const sta::Pin*, MoveType>>* chosen_moves,
    const bool force_single_repair)
{
  int changed = 0;

  if (pins.empty()) {
    return false;
  }

  int repairs_per_pass
      = target_collector_->repairsPerPass(setup_context_.max_repairs_per_pass);
  if (force_single_repair) {
    repairs_per_pass = 1;
  }

  const sta::Slack focus_slack = target_collector_->getCurrentEndpointSlack();
  if (targetPrewarmEnabled()) {
    std::vector<Target> prewarm_targets;
    prewarm_targets.reserve(pins.size());
    for (const sta::Pin* driver_pin : pins) {
      Target target;
      const bool has_target
          = focus_path != nullptr
                ? makePinTargetOnPath(
                      driver_pin, focus_path, focus_slack, target)
                : makePinTarget(driver_pin, focus_slack, target);
      if (has_target) {
        prewarm_targets.push_back(target);
      }
    }
    prewarmTargets(prewarm_targets);
  }

  for (const sta::Pin* driver_pin : pins) {
    if (changed >= repairs_per_pass) {
      break;
    }

    Target target;
    const bool has_target
        = focus_path != nullptr
              ? makePinTargetOnPath(driver_pin, focus_path, focus_slack, target)
              : makePinTarget(driver_pin, focus_slack, target);
    if (!has_target) {
      continue;
    }

    committer_.trackViolatorWithTimingInfo(target.driver_pin,
                                           target.vertex(resizer_),
                                           focus_slack,
                                           *target_collector_);

    const std::unordered_set<MoveType>* rejected_types = nullptr;
    auto rejected_itr = rejected_moves->find(driver_pin);
    if (rejected_itr != rejected_moves->end()) {
      rejected_types = &rejected_itr->second;
    }

    std::optional<MoveType> accepted_type;
    logRepairTarget(target);
    if (tryRepairTarget(
            target, repairs_per_pass, changed, rejected_types, accepted_type)
        && accepted_type.has_value()) {
      chosen_moves->emplace_back(driver_pin, *accepted_type);
    }
  }

  return changed > 0;
}

void SetupLegacyBase::makePathDriverTarget(const sta::Path* path,
                                           sta::PathExpanded& expanded,
                                           const int drvr_index,
                                           const sta::Slack path_slack,
                                           Target& target) const
{
  const sta::Path* drvr_path = expanded.path(drvr_index);
  sta::Vertex* drvr_vertex = drvr_path->vertex(sta_);

  target = rsz::makePathDriverTarget(
      path, expanded, drvr_index, path_slack, resizer_);
  target.fanout = fanout(drvr_vertex);
}

void SetupLegacyBase::logRepairTarget(const Target& target) const
{
  sta::LibertyPort* drvr_port = network_->libertyPort(target.driver_pin);
  sta::LibertyCell* drvr_cell
      = drvr_port != nullptr ? drvr_port->libertyCell() : nullptr;
  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "{} {} fanout = {} drvr_index = {}",
             network_->pathName(target.driver_pin),
             drvr_cell != nullptr ? drvr_cell->name() : "none",
             target.fanout,
             target.path_index);
}

int SetupLegacyBase::repairProgressIncrement(const MoveType type,
                                             const int repairs_per_pass)
{
  return type == MoveType::kUnbuffer ? repairs_per_pass : 1;
}

bool SetupLegacyBase::allowsBatchRepair(const MoveType type)
{
  return type == MoveType::kSizeDown;
}

bool SetupLegacyBase::tryCandidateSequence(
    MoveGenerator& generator,
    const Target& target,
    const int repairs_per_pass,
    int& changed,
    std::optional<MoveType>& accepted_type)
{
  auto candidates = generator.generate(target);
  for (auto& candidate : candidates) {
    const Estimate estimate = candidate->estimate();
    if (!estimate.legal) {
      continue;
    }

    committer_.trackMoveAttempt(target.driver_pin, generator.type());
    const MoveResult result = committer_.commit(*candidate);
    if (!result.accepted) {
      continue;
    }

    changed += repairProgressIncrement(result.type, repairs_per_pass);
    accepted_type = result.type;
    return true;
  }
  return false;
}

bool SetupLegacyBase::trySizeDownBatch(MoveGenerator& generator,
                                       const Target& target,
                                       const int repairs_per_pass,
                                       int& changed,
                                       std::optional<MoveType>& accepted_type)
{
  bool accepted_batch = false;
  while (tryCandidateSequence(
      generator, target, repairs_per_pass, changed, accepted_type)) {
    accepted_batch = true;
  }
  return accepted_batch;
}

bool SetupLegacyBase::tryRepairTarget(
    const Target& target,
    const int repairs_per_pass,
    int& changed,
    const std::unordered_set<MoveType>* rejected_types)
{
  std::optional<MoveType> accepted_type;
  return tryRepairTarget(
      target, repairs_per_pass, changed, rejected_types, accepted_type);
}

bool SetupLegacyBase::tryRepairTarget(
    const Target& target,
    const int repairs_per_pass,
    int& changed,
    const std::unordered_set<MoveType>* rejected_types,
    std::optional<MoveType>& accepted_type)
{
  Target live_target = target;
  sta::Vertex* live_vertex = live_target.vertex(resizer_);
  if (live_vertex != nullptr) {
    live_target.fanout = fanout(live_vertex);
  }

  for (const std::unique_ptr<MoveGenerator>& generator_ptr : move_generators_) {
    MoveGenerator& generator = *generator_ptr;
    const MoveType type = generator.type();
    if (rejected_types != nullptr && rejected_types->contains(type)) {
      continue;
    }
    if (!generator.isApplicable(live_target)) {
      continue;
    }

    debugPrint(logger_,
               RSZ,
               "repair_setup",
               1,
               "Considering {} for {}",
               generator.name(),
               network_->pathName(live_target.driver_pin));

    if (allowsBatchRepair(type)) {
      if (trySizeDownBatch(generator,
                           live_target,
                           repairs_per_pass,
                           changed,
                           accepted_type)) {
        return true;
      }
      continue;
    }
    if (tryCandidateSequence(
            generator, live_target, repairs_per_pass, changed, accepted_type)) {
      return true;
    }
  }
  return false;
}

bool SetupLegacyBase::tryRepairPathTarget(const Target& target,
                                          const sta::Slack path_slack,
                                          const int repairs_per_pass,
                                          int& changed)
{
  committer_.trackViolatorWithTimingInfo(target.driver_pin,
                                         target.vertex(resizer_),
                                         path_slack,
                                         *target_collector_);
  logRepairTarget(target);
  return tryRepairTarget(target, repairs_per_pass, changed, nullptr);
}

bool SetupLegacyBase::repairPath(sta::Path* path,
                                 const sta::Slack path_slack,
                                 const bool force_single_repair)
{
  sta::PathExpanded expanded(path, sta_);
  int changed = 0;

  if (expanded.size() <= 1) {
    return false;
  }

  const sta::Scene* corner = path->scene(sta_);
  if (path->minMax(sta_) != resizer_.max_) {
    logger_->error(utl::RSZ,
                   kMsgRepairSetupExpectedMaxPath,
                   "repairSetup expects max delay path");
    return false;
  }

  const auto load_delays
      = rankPathDrivers(expanded, corner, corner->libertyIndex(resizer_.max_));
  const int repairs_per_pass = repairBudget(path_slack, force_single_repair);

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "Path slack: {}, repairs: {}, load_delays: {}",
             delayAsString(path_slack, 3, sta_),
             repairs_per_pass,
             load_delays.size());

  // Construct target vector
  std::vector<Target> targets;
  targets.reserve(load_delays.size());
  for (const auto& [drvr_index, ignored] : load_delays) {
    static_cast<void>(ignored);
    Target target;
    makePathDriverTarget(path, expanded, drvr_index, path_slack, target);
    targets.push_back(target);
  }

  // Prewarm for legacy MT policy
  if (targetPrewarmEnabled()) {
    prewarmTargets(targets);
  }

  // Drive ranked targets through the configured generator list until this
  // pass reaches its repair budget.
  for (const Target& target : targets) {
    if (changed >= repairs_per_pass) {
      break;
    }
    tryRepairPathTarget(target, path_slack, repairs_per_pass, changed);
  }

  return changed > 0;
}

int SetupLegacyBase::committedMoves(const MoveType type) const
{
  return committer_.committedMoves(type);
}

int SetupLegacyBase::totalMoves(const MoveType type) const
{
  return committer_.totalMoves(type);
}

void SetupLegacyBase::printProgress(const int iteration,
                                    const bool force,
                                    const char phase_marker,
                                    const bool use_startpoint_metrics) const
{
  const bool start = iteration == 0;

  if (start) {
    setup_context_.progress_header_printed = true;
    logger_->report(
        "   Iter   | Removed | Resized | Inserted | Cloned |  Pin  |"
        "   Area   |    WNS   |   StTNS    |   EnTNS    |  Viol  |  Worst  ");
    logger_->report(
        "          | Buffers |  Gates  | Buffers  |  Gates | Swaps |"
        "          |          |            |            | Endpts | St/EnPt ");
    logger_->report(
        "---------------------------------------------------------------"
        "---------------------------------------------------------------");
  }

  if (iteration % print_interval_ != 0 && !force) {
    return;
  }

  // StTNS is printed in every progress row, so collect startpoints only when a
  // row is actually emitted.
  target_collector_->collectViolatingStartpoints();

  const sta::Slack wns = target_collector_->getWns();
  const sta::Slack st_tns = target_collector_->getTns(true);
  const sta::Slack en_tns = target_collector_->getTns(false);
  const bool show_startpoint_metrics = use_startpoint_metrics;
  const sta::Pin* worst_pin
      = target_collector_->getWorstPin(show_startpoint_metrics);

  std::string itr_field = fmt::format("{}{}", iteration, phase_marker);

  const double design_area = resizer_.computeDesignArea();
  const double area_growth = design_area - setup_context_.initial_design_area;
  double area_growth_percent = std::numeric_limits<double>::infinity();
  if (std::abs(setup_context_.initial_design_area) > 0.0) {
    area_growth_percent
        = area_growth / setup_context_.initial_design_area * 100.0;
  }

  logger_->report(
      "{: >9s} | {: >7d} | {: >7d} | {: >8d} | {: >6d} | {: >5d} "
      "| {: >+7.1f}% | {: >8s} | {: >10s} | {: >10s} | {: >6d} | {}",
      itr_field,
      totalMoves(MoveType::kUnbuffer),
      totalMoves(MoveType::kSizeUp) + totalMoves(MoveType::kSizeDown)
          + totalMoves(MoveType::kSizeUpMatch) + totalMoves(MoveType::kVtSwap),
      totalMoves(MoveType::kBuffer) + totalMoves(MoveType::kSplitLoad),
      totalMoves(MoveType::kClone),
      totalMoves(MoveType::kSwapPins),
      area_growth_percent,
      delayAsString(wns, 3, sta_),
      delayAsString(st_tns, 1, sta_),
      delayAsString(en_tns, 1, sta_),
      std::max(0, target_collector_->getNumViolatingEndpoints()),
      worst_pin != nullptr ? network_->pathName(worst_pin) : "");

  debugPrint(logger_, RSZ, "memory", 1, "RSS = {}", utl::getCurrentRSS());
}

bool SetupLegacyBase::terminateProgress(const int iteration,
                                        const float initial_tns,
                                        float& prev_tns,
                                        float& fix_rate_threshold,
                                        const int endpt_index,
                                        const int num_endpts,
                                        const std::string_view phase_name,
                                        const char phase_marker)
{
  if (iteration % opto_large_interval_ == 0) {
    fix_rate_threshold *= 2.0;
  }
  if (iteration % opto_small_interval_ == 0) {
    float curr_tns = sta_->totalNegativeSlack(max_);
    float inc_fix_rate = (prev_tns - curr_tns) / initial_tns;
    prev_tns = curr_tns;
    if (iteration > 1000 && inc_fix_rate < fix_rate_threshold) {
      debugPrint(logger_,
                 RSZ,
                 "repair_setup",
                 1,
                 "{}{} Phase: Exiting at iteration {} because incr fix rate "
                 "{:0.2f}% is < {:0.2f}% [endpt {}/{}]",
                 phase_name,
                 phase_marker,
                 iteration,
                 inc_fix_rate * 100,
                 fix_rate_threshold * 100,
                 endpt_index,
                 num_endpts);
      return true;
    }
  }
  return false;
}

void SetupLegacyBase::reportCustomPhaseSetup() const
{
  logger_->info(
      utl::RSZ, 221, "Using custom phase sequence: {}", config_.phases);
}

}  // namespace rsz
