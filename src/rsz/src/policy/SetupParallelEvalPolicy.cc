// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SetupParallelEvalPolicy.hh"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BatchSelector.hh"
#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "PathGroupFilter.hh"
#include "RebufferMtCandidate.hh"
#include "RebufferMtGenerator.hh"
#include "RepairSetupProfiler.hh"
#include "RepairTargetCollector.hh"
#include "SizeDownFanoutMtGenerator.hh"
#include "SizeUpMtCandidate.hh"
#include "SizeUpMtGenerator.hh"
#include "SplitLoadMtGenerator.hh"
#include "SwapPinsMtGenerator.hh"
#include "UnbufferMtGenerator.hh"
#include "VtSwapMtCandidate.hh"
#include "VtSwapMtGenerator.hh"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/ExceptionPath.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/Sdc.hh"
#include "utl/Logger.h"
#include "utl/ThreadPool.h"

namespace rsz {

SetupParallelEvalPolicy::SetupParallelEvalPolicy(
    Resizer& resizer,
    MoveCommitter& committer,
    RepairSetupContext& setup_context,
    const OptimizerRunConfig& config)
    : OptimizationPolicy(
          resizer,
          committer,
          setup_context,
          config,
          OptimizationPolicyConfig::forParallelEval(config.effort)),
      stall_(policy_config_.parallel_eval),
      tail_(policy_config_.parallel_eval),
      tabu_(policy_config_.parallel_eval)
{
  is_experimental = true;
}

namespace {

// The wall time of one worker task for the profiler; a null sink measures
// nothing.
class WorkerTimer
{
 public:
  explicit WorkerTimer(double* elapsed) : elapsed_(elapsed)
  {
    if (elapsed_ != nullptr) {
      start_ = std::chrono::steady_clock::now();
    }
  }
  ~WorkerTimer()
  {
    if (elapsed_ != nullptr) {
      *elapsed_ = std::chrono::duration<double>(std::chrono::steady_clock::now()
                                                - start_)
                      .count();
    }
  }

 private:
  double* elapsed_;
  std::chrono::steady_clock::time_point start_;
};

// The PARALLEL_EVAL form of each move type.
std::unique_ptr<MoveGenerator> makeParallelEvalGenerator(
    const MoveType type,
    const GeneratorContext& context)
{
  switch (type) {
    case MoveType::kSizeUp:
      return std::make_unique<SizeUpMtGenerator>(context);
    case MoveType::kBuffer:
      return std::make_unique<RebufferMtGenerator>(context);
    case MoveType::kVtSwap:
      return std::make_unique<VtSwapMtGenerator>(context);
    case MoveType::kSwapPins:
      // generate() picks the input with the fastest prepared arc delay, and
      // estimate() charges the displaced signal's side loss.
      return std::make_unique<SwapPinsMtGenerator>(context);
    // SplitLoadMt chooses like the serial move in prepareInputs() and scores
    // the choice from the prepared path window.  UnbufferMt decides there
    // whether the buffer can go and estimates the gain that estimate() uses.
    // SizeDownFanoutMt measures there and chooses in generate().
    case MoveType::kSplitLoad:
      return std::make_unique<SplitLoadMtGenerator>(context);
    case MoveType::kSizeDownFanout:
      return std::make_unique<SizeDownFanoutMtGenerator>(context);
    case MoveType::kUnbuffer:
      return std::make_unique<UnbufferMtGenerator>(context);
    case MoveType::kClone:
    case MoveType::kSizeUpMatch:
    case MoveType::kReroute:
    case MoveType::kBufferToInverters:
    case MoveType::kCount:
      break;
  }

  return nullptr;
}

// Keeps the shortest positive value seen so far in shortest.
void keepShortest(float& shortest, const float value)
{
  if (value > 0.0f && (shortest == 0.0f || value < shortest)) {
    shortest = value;
  }
}

}  // namespace

SetupParallelEvalPolicy::~SetupParallelEvalPolicy() = default;

bool SetupParallelEvalPolicy::start()
{
  OptimizationPolicy::start();
  target_collector_->setRecordCritPathLoadPins(policy_config_.fanin_side_loss);

  selectMoveSequence();
  buildMoveGenerators(move_sequence_, makeGeneratorContext());
  bottleneck_ = std::make_unique<BottleneckAnalysis>(sta_, max_);
  // The reserve follows the margin the targets are collected with.  The
  // selector copies its config, so the margin goes in first.
  policy_config_.parallel_eval.selector.slack_margin
      = config_.setup_slack_margin;
  batch_selector_
      = std::make_unique<BatchSelector>(policy_config_.parallel_eval.selector);
  resetRunState();

  // The worker threads also run the collector's endpoint scans.
  thread_pool_ = makeWorkerThreadPool();
  target_collector_->setScanThreadPool(thread_pool_.get());

  // Collect the endpoints before the first QoR sample, so the sample follows
  // the same required-time update as in LEGACY.
  target_collector_->init(config_.setup_slack_margin);

  // -repair_tns below 100% limits every batch to as many worst endpoints as
  // LEGACY would repair now, and at least one.
  max_end_repairs_ = std::numeric_limits<size_t>::max();
  if (config_.repair_tns_end_percent < 1.0) {
    max_end_repairs_ = static_cast<size_t>(
        std::max(static_cast<int>(target_collector_->getMaxEndpointCount()
                                  * config_.repair_tns_end_percent),
                 1));
  }

  saveInitialCheckpoint();
  committer_.tracker().captureInitialSlackDistribution();
  committer_.tracker().captureOriginalEndpointSlack();
  // The generators set up their state for the phase's worker threads.
  for (const std::unique_ptr<MoveGenerator>& generator : move_generators_) {
    generator->startPhase(thread_pool_->threadCount());
  }
  progress_.printHeader(showStartpointMetrics());

  return true;
}

void SetupParallelEvalPolicy::selectMoveSequence()
{
  // -sequence is the move set; without it the default runs.  The candidates
  // of a target compete on score, and the sequence order only breaks exact
  // ties; SizeUpMt scores carry the 0.75 size_up_mt_score_scale.  The default
  // goes through the same filter, so -skip_vt_swap and a single-VT library
  // still remove VtSwapMt.
  std::vector<MoveType> requested_sequence = config_.sequence;
  if (requested_sequence.empty()) {
    requested_sequence = {MoveType::kUnbuffer,
                          MoveType::kVtSwap,
                          MoveType::kSizeUp,
                          MoveType::kBuffer,
                          MoveType::kSplitLoad};
  }

  move_sequence_.clear();
  for (const MoveType type : requested_sequence) {
    if (!moveEnabled(type)) {
      continue;
    }

    // BufferToInverters has no gain estimate to rank it.  Clone, SizeUpMatch
    // and Reroute have no Mt form.
    if (type == MoveType::kBufferToInverters || type == MoveType::kClone
        || type == MoveType::kSizeUpMatch || type == MoveType::kReroute) {
      logger_->warn(utl::RSZ,
                    2028,
                    "PARALLEL_EVAL has no form of move '{}'; it is skipped.",
                    moveName(type));
      continue;
    }

    move_sequence_.push_back(type);
  }

  unbuffer_in_sequence_ = std::ranges::find(move_sequence_, MoveType::kUnbuffer)
                          != move_sequence_.end();
}

void SetupParallelEvalPolicy::resetRunState()
{
  committed_moves_ = 0;
  batch_index_ = 0;
  tentative_moves_ = 0;
  checkpoint_misses_ = 0;
  small_batch_retry_started_ = false;
  max_utilization_warned_ = false;
  batch_journal_open_ = false;
  batch_instances_.clear();
  batch_candidates_.clear();
  batch_swaps_.clear();
  rebuffer_targets_in_batch_ = 0;
  tail_.reset();
  tabu_.reset(resizer_.dbNetwork());
}

void SetupParallelEvalPolicy::saveInitialCheckpoint()
{
  const BatchQor qor = measureQor();
  checkpoint_wns_ = qor.wns;
  checkpoint_tns_ = qor.tns;
  best_checkpoint_wns_ = checkpoint_wns_;
  reference_period_ = referencePeriod(checkpoint_wns_);
  stall_.reset(checkpoint_tns_, reference_period_);
  work_.reset(graph_->vertexCount());
  if (RepairSetupProfiler::enabled()) {
    RepairSetupProfiler::current()->metadata(
        "graph_vertices", std::to_string(graph_->vertexCount()));
    RepairSetupProfiler::current()->metadata(
        "reference_period", fmt::format("{:.6e}", reference_period_));
  }
  debugPrint(logger_,
             utl::RSZ,
             "repair_setup",
             1,
             "PARALLEL_EVAL reference clock period {:.3e}",
             reference_period_);

  RepairSetupProfiler::qor(
      "checkpoint_initial", checkpoint_wns_, checkpoint_tns_);
}

float SetupParallelEvalPolicy::referencePeriod(
    const sta::Slack initial_wns) const
{
  // Every mode times the design, so the shortest clock of any mode sets the
  // scale.  A virtual clock times only IO constraints, whose budgets are often
  // rough, so it sets the scale only when no clock has source pins.
  float real_period = 0.0f;
  float virtual_period = 0.0f;
  for (const sta::Mode* mode : sta_->modes()) {
    for (const sta::Clock* clk : mode->sdc()->clocks()) {
      keepShortest(clk->isVirtual() ? virtual_period : real_period,
                   clk->period());
    }
  }
  if (real_period > 0.0f) {
    return real_period;
  }
  if (virtual_period > 0.0f) {
    return virtual_period;
  }

  // set_max_delay sets the scale only when no mode has a clock.
  float max_delay = 0.0f;
  for (const sta::Mode* mode : sta_->modes()) {
    for (const sta::ExceptionPath* exception : mode->sdc()->exceptions()) {
      if (exception->isPathDelay()
          && exception->minMax()->matches(sta::MinMax::max())) {
        keepShortest(max_delay, exception->delay());
      }
    }
  }
  if (max_delay > 0.0f) {
    return max_delay;
  }

  // Without clocks or max delays the initial WNS is the only time scale.
  return std::abs(sta::delayAsFloat(initial_wns));
}

void SetupParallelEvalPolicy::buildMoveGenerators(
    const std::vector<MoveType>& move_types,
    const GeneratorContext& context)
{
  move_generators_.clear();
  move_generators_.reserve(move_types.size());
  for (const MoveType type : move_types) {
    std::unique_ptr<MoveGenerator> generator
        = makeParallelEvalGenerator(type, context);
    if (generator != nullptr) {
      move_generators_.push_back(std::move(generator));
    }
  }
}

void SetupParallelEvalPolicy::iterate()
{
  if (converged_) {
    return;
  }

  const ProfileIteration iteration("batch_iter");
  tabu_.expire(batch_index_);
  if (finishIfNoSetupViolations() || finishAtMaxArea()) {
    return;
  }

  // Collect, weigh, evaluate: the pool comes back with its batch selected,
  // because the chunk loop needs the selection to know when to stop.
  const BatchScope scope = collectTargetEndpoints();
  analyzeBottlenecks(scope);
  // Design wide even under -path_group: the side pins of a missed batch may
  // lie on any group's paths.
  const sta::Slack pre_batch_wns = sta_->worstSlack(max_);
  BatchPool pool = evaluateTargets(scope);
  if (pool.targets.empty()) {
    finishWithoutTargets();
    return;
  }

  // Commit, judge, stop.
  openBatchJournal();
  int batch_moves = commitBatch(pool.evaluations, pool.selection);
  if (batch_moves > 0) {
    updateTiming();
  }
  ++batch_index_;

  const sta::Slack best_wns_before = best_checkpoint_wns_;
  const bool batch_saved = judgeBatch(pool, pre_batch_wns, batch_moves);
  if (tail_.active()) {
    tail_.afterBatch(best_wns_before, best_checkpoint_wns_, batch_saved);
  }

  applyStopRules(batch_moves);
  printTrackerBatchSummary();

  // The candidates belong to the pool, which ends here.
  batch_candidates_.clear();
  batch_swaps_.clear();
}

bool SetupParallelEvalPolicy::judgeBatch(BatchPool& pool,
                                         const sta::Slack pre_batch_wns,
                                         int& batch_moves)
{
  const BatchQor after = measureQor();
  RepairSetupProfiler::qor("batch_after", after.wns, after.tns);
  stall_.endBatch(batch_moves);
  const bool utilization_legal = withinMaxUtilization();
  if (utilization_legal && acceptsBatch(after.wns, after.tns)) {
    if (!sta::fuzzyLess(checkpoint_wns_, after.wns)) {
      RepairSetupProfiler::event("checkpoints_saved_wns_tolerance");
    }

    saveBatchCheckpoint(after.wns, after.tns);
    return true;
  }

  noteMissedBatch(utilization_legal);
  if (salvageMissedBatch(
          pool.evaluations, pool.selection, pre_batch_wns, batch_moves)) {
    return true;
  }

  settleMissedBatch(batch_moves);
  return false;
}

bool SetupParallelEvalPolicy::finishIfNoSetupViolations()
{
  // The verdict's slack decides.  Under -path_group a clean group ends the run
  // here; the empty target list would roll back the batch that cleaned it.
  const BatchQor qor = measureQor();
  if (sta::fuzzyLess(qor.wns, config_.setup_slack_margin)) {
    return false;
  }

  if (batch_journal_open_) {
    saveBatchCheckpoint(qor.wns, qor.tns);
  }

  RepairSetupProfiler::stop("no_setup_violations");
  markRunComplete(true);
  return true;
}

bool SetupParallelEvalPolicy::finishAtMaxArea()
{
  // Like LEGACY, nothing runs at the -max_utilization area.  The verdict keeps
  // batches below the limit, so normally only a design that starts there stops
  // here.
  if (withinMaxUtilization()) {
    return false;
  }

  stopAtCheckpoint("max_area");
  return true;
}

void SetupParallelEvalPolicy::stopAtCheckpoint(const char* reason)
{
  // An open speculative run has not reached a checkpoint, so it is undone.
  RepairSetupProfiler::stop(reason);
  restoreBatchCheckpoint();
  if (tail_.active()) {
    RepairSetupProfiler::event("wns_tail_batches", tail_.batches());
    tail_.stop();
  }
  markRunComplete(false);
}

void SetupParallelEvalPolicy::finishWithoutTargets()
{
  RepairSetupProfiler::stop("no_targets");
  restoreBatchCheckpoint();
  markRunComplete(false);
}

void SetupParallelEvalPolicy::analyzeBottlenecks(const BatchScope& scope)
{
  // The weights apply only while the saved batches stall WNS; after a pause
  // the next analysis starts fresh.
  if (!stall_.weightsPriority()) {
    bottleneck_age_ = std::numeric_limits<int>::max();
    return;
  }

  // One analysis serves bottleneck_refresh_batches batches; the tail's narrower
  // endpoint set gets a fresh one.
  const BatchPriorityConfig& priority = policy_config_.parallel_eval.priority;
  const bool tail_changed = tail_.active() != bottleneck_in_tail_;
  if (bottleneck_age_ < std::numeric_limits<int>::max()) {
    ++bottleneck_age_;
  }
  if (!tail_changed && bottleneck_age_ < priority.bottleneck_refresh_batches) {
    return;
  }
  bottleneck_age_ = 0;
  bottleneck_in_tail_ = tail_.active();

  const ProfileScope profile("bottleneck_analysis");
  const size_t endpoint_limit
      = std::min(static_cast<size_t>(priority.bottleneck_max_endpoints),
                 scope.target_endpoints);
  bottleneck_->analyze(target_collector_->getViolatingEndpoints(),
                       static_cast<float>(config_.setup_slack_margin),
                       static_cast<int>(endpoint_limit),
                       priority.bottleneck_max_startpoints);
  RepairSetupProfiler::event("bottleneck_analyses");
  RepairSetupProfiler::event("bottleneck_cone_vertices",
                             bottleneck_->coneSize());
}

SetupParallelEvalPolicy::BatchScope
SetupParallelEvalPolicy::collectTargetEndpoints()
{
  rebuffer_targets_in_batch_ = 0;

  {
    const ProfileScope profile("init_per_iter");
    committer_.tracker().capturePrePhaseSlack();
    if (batch_index_ > 0) {
      prewarmStaForPrepareStage();
    }
  }

  target_collector_->init(config_.setup_slack_margin);

  // The row shows the state this collection saw: after batch_index_
  // batches.  As in LEGACY, -verbose prints every row; without it only the
  // first row and the final one appear.
  if (config_.verbose || batch_index_ == 0) {
    progress_.printRow(
        fmt::format("{}{}",
                    batch_index_,
                    phaseMarkerForIndex(setup_context_.phase_index)),
        *target_collector_,
        nullptr,
        showStartpointMetrics(),
        /*worst_by_startpoint=*/false);
  }
  const size_t failing_endpoint_count
      = target_collector_->getViolatingEndpoints().size();

  // -repair_tns and the WNS tail visit only the worst endpoints, so the batch
  // and the evaluation chunks are sized on that subset.
  size_t target_endpoints = std::min(failing_endpoint_count, max_end_repairs_);
  if (tail_.active()) {
    target_endpoints
        = std::min(target_endpoints, static_cast<size_t>(tail_.endpoints()));
  }

  const int batch_cap = effectiveBatchCap(target_endpoints);
  stall_.beginBatch(batch_cap, failing_endpoint_count);
  RepairSetupProfiler::event("batch_fep_at_collect", failing_endpoint_count);
  RepairSetupProfiler::event("batch_effective_cap", batch_cap);

  return {.target_endpoints = target_endpoints, .batch_cap = batch_cap};
}

SetupParallelEvalPolicy::BatchPool SetupParallelEvalPolicy::evaluateTargets(
    const BatchScope& scope)
{
  // Why chunks: a batch commits at most batch_cap moves per timing update,
  // but evaluation cost grows with every endpoint.  So the endpoints are
  // evaluated worst slack first, one chunk at a time, and the next chunk runs
  // only while the batch is not full.  The batch is built from the most
  // critical endpoints first.
  const size_t chunk_endpoints = evaluationEndpointLimit(
      scope.batch_cap,
      policy_config_.parallel_eval.eval_endpoint_factor,
      policy_config_.parallel_eval.eval_min_endpoints);

  BatchPool pool;
  std::unordered_set<sta::Pin*> visited_pins;
  size_t next_endpoint = 0;
  while (next_endpoint < scope.target_endpoints) {
    const size_t chunk_end
        = std::min(scope.target_endpoints, next_endpoint + chunk_endpoints);
    std::vector<Target> chunk
        = collectTargets(next_endpoint, chunk_end, visited_pins);
    next_endpoint = chunk_end;
    if (!chunk.empty()) {
      // Prepare reads live STA on the main thread; generation and estimation
      // then run on the worker pool.
      prewarmTargets(chunk);
      prepareTargets(chunk);

      const std::vector<Target*> targets = pool.adopt(std::move(chunk));
      std::vector<TargetEvaluation> evaluations = generateCandidates(targets);
      // One worker owns each TargetEvaluation, so the pool is never nested.
      estimateTargets(evaluations);

      pool.append(std::move(evaluations));
    }

    // The selection so far decides whether another chunk is needed; the last
    // selection is the batch iterate() commits.  A batch cut short by the score
    // floor still takes another chunk: later endpoints can bring candidates
    // above the floor.
    selectBatch(pool, scope.batch_cap);
    if (next_endpoint >= scope.target_endpoints
        || static_cast<int>(pool.selection.evaluation_indices.size())
               >= scope.batch_cap) {
      break;
    }

    RepairSetupProfiler::event("batch_eval_chunks_extended");
  }

  work_.addEndpoints(next_endpoint);
  RepairSetupProfiler::event("batch_endpoints_evaluated", next_endpoint);
  if (!pool.targets.empty()) {
    recordSelection(pool);
  }

  return pool;
}

std::vector<Target*> SetupParallelEvalPolicy::BatchPool::adopt(
    std::vector<Target> chunk)
{
  std::vector<Target*> adopted;
  adopted.reserve(chunk.size());
  for (Target& target : chunk) {
    targets.push_back(std::move(target));
    adopted.push_back(&targets.back());
  }

  return adopted;
}

void SetupParallelEvalPolicy::BatchPool::append(
    std::vector<TargetEvaluation> chunk_evaluations)
{
  std::ranges::move(chunk_evaluations, std::back_inserter(evaluations));
}

void SetupParallelEvalPolicy::selectBatch(BatchPool& pool, const int batch_cap)
{
  const ProfileScope profile("batch_selection");
  pool.candidates = makeBatchCandidates(pool.evaluations);
  pool.selection = batch_selector_->select(
      pool.candidates, batch_cap, stall_.wnsStalled());
}

void SetupParallelEvalPolicy::recordSelection(const BatchPool& pool) const
{
  if (!RepairSetupProfiler::enabled()) {
    return;
  }

  for (const size_t index : pool.selection.evaluation_indices) {
    RepairSetupProfiler::moveSelected(pool.evaluations[index].bestCandidate());
  }

  const BatchSelectionStats& stats = pool.selection.stats;
  RepairSetupProfiler::event("batch_considered", stats.considered);
  RepairSetupProfiler::event("batch_limited", stats.batch_limited);
  RepairSetupProfiler::event("score_limited", stats.score_limited);
  for (size_t index = 0; index < stats.deferred.size(); ++index) {
    const BatchConflictReason reason = static_cast<BatchConflictReason>(index);
    RepairSetupProfiler::event(
        std::string("conflict_") + batchConflictReasonName(reason),
        stats.deferred[index]);
  }
}

SetupParallelEvalPolicy::BatchQor SetupParallelEvalPolicy::measureQor() const
{
  const ProfileScope profile("qor_collection");
  sta::Slack wns = 0.0;
  sta::Vertex* worst_vertex = nullptr;
  target_collector_->worstSlack(wns, worst_vertex);
  return {.wns = wns, .tns = target_collector_->totalNegativeSlack()};
}

bool SetupParallelEvalPolicy::withinMaxUtilization() const
{
  // Only the -max_utilization area limit of repair_timing applies, as in
  // LEGACY; without it any area is legal.
  return !resizer_.overMaxArea();
}

void SetupParallelEvalPolicy::noteMissedBatch(const bool utilization_legal)
{
  // The batch spent its moves without progress, so they count against the
  // stall budget.  A later salvage resets the budget only if it progresses.
  ++checkpoint_misses_;
  stall_.onMissed();
  RepairSetupProfiler::event("checkpoint_misses");
  if (!utilization_legal) {
    RepairSetupProfiler::event("checkpoint_reject_utilization");
    // Without a warning the user sees no sign of the limit: the undone
    // batches leave the design below it.
    if (!max_utilization_warned_) {
      max_utilization_warned_ = true;
      const double core_area = resizer_.coreArea();
      logger_->warn(utl::RSZ,
                    2033,
                    "PARALLEL_EVAL reached the -max_utilization limit of "
                    "{:.2f}%: batch {} would grow the design area to {:.1f} "
                    "um^2, over the {:.1f} um^2 limit. Batches over the "
                    "limit are undone.",
                    100.0 * resizer_.maxArea() / core_area,
                    batch_index_,
                    progress_.designArea() * 1e12,
                    resizer_.maxArea() * 1e12);
    }
  }
}

void SetupParallelEvalPolicy::settleMissedBatch(const int batch_moves)
{
  // A batch over the utilization limit is never built on.
  if (tail_.active() && batch_moves > 0 && withinMaxUtilization()
      && tail_.maySpeculate()) {
    // Keep the missed batch and let the next batches build on it, like the
    // serial loop's non-improving passes.  A later save keeps the whole run; a
    // spent budget rolls it all back.
    tail_.keepSpeculative(batch_instances_);
    RepairSetupProfiler::event(
        "wns_tail_speculate",
        {.metric = "wns_tail_speculative_batches",
         .detail = std::to_string(tail_.speculativeBatches()),
         .trace = true});
  } else {
    undoMissedBatch();
  }
}

void SetupParallelEvalPolicy::undoMissedBatch()
{
  std::vector<InstanceRecord> speculative;
  if (tail_.speculating()) {
    // Every batch of the failed run goes on the tabu list, or the first one
    // would be picked again at once.  Only the last batch counts toward the
    // undo limit.
    RepairSetupProfiler::event("wns_tail_speculation_failed");
    speculative = tail_.endSpeculation();
  }

  undoBatch(speculative);
}

std::vector<Target> SetupParallelEvalPolicy::collectTargets(
    const size_t begin_endpoint,
    const size_t end_endpoint,
    std::unordered_set<sta::Pin*>& visited_pins) const
{
  std::vector<Target> targets
      = target_collector_->collectCritPathDriverPinTargets(
          [this](sta::Pin* pin) {
            return resizer_.isEditableLogicStdCell(network_->instance(pin));
          },
          begin_endpoint,
          end_endpoint,
          visited_pins);
  for (Target& target : targets) {
    target.fanout = targetFanout(target);
  }

  RepairSetupProfiler::event("targets_collected", targets.size());
  return targets;
}

size_t SetupParallelEvalPolicy::evaluationEndpointLimit(
    const int batch_cap,
    const int eval_endpoint_factor,
    const int min_endpoints)
{
  // The floor keeps small batches and small designs fully evaluated.
  return std::max(static_cast<size_t>(batch_cap)
                      * static_cast<size_t>(eval_endpoint_factor),
                  static_cast<size_t>(min_endpoints));
}

int SetupParallelEvalPolicy::targetFanout(const Target& target) const
{
  sta::Vertex* vertex = target.vertex(resizer_);
  int fanout = 0;
  sta::VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    if (edge_iter.next()->isWire()) {
      ++fanout;
    }
  }

  return fanout;
}

std::vector<TargetEvaluation> SetupParallelEvalPolicy::generateCandidates(
    const std::vector<Target*>& targets)
{
  const ProfileScope profile("candidate_generation");
  // The main thread applies the skip rules and prepares the generation
  // inputs of each target.
  std::vector<GenerationTask> tasks;
  tasks.reserve(targets.size());
  for (Target* target : targets) {
    tasks.push_back(planGeneration(*target));
  }

  // Every generator runs on the pool; a task without generators stays out.
  std::vector<GenerationTask*> worker_tasks;
  for (GenerationTask& task : tasks) {
    if (!task.generators.empty()) {
      worker_tasks.push_back(&task);
    }
  }

  const bool profiling = RepairSetupProfiler::enabled();
  thread_pool_->parallelFor(
      worker_tasks, [profiling](GenerationTask* const& task) {
        const WorkerTimer timer(profiling ? &task->worker_elapsed : nullptr);
        // Workers have no profiler; collectGenerated() replays their counts.
        const DeferredCountScope counts(profiling ? &task->counts : nullptr);
        for (size_t index = 0; index < task->generators.size(); ++index) {
          task->generated[index]
              = task->generators[index]->generate(*task->target);
        }
      });

  // The main thread folds the tasks back in target order.
  std::vector<TargetEvaluation> evaluations;
  evaluations.reserve(tasks.size());
  for (GenerationTask& task : tasks) {
    evaluations.push_back(collectGenerated(task));
  }

  return evaluations;
}

SetupParallelEvalPolicy::GenerationTask SetupParallelEvalPolicy::planGeneration(
    Target& target)
{
  GenerationTask task{.target = &target};
  sta::Instance* inst = target.inst(resizer_);
  if (tabu_.isExcluded(inst)) {
    RepairSetupProfiler::event("skip_undo_blocked");
    return task;
  }

  if (tabu_.isTabu(inst)) {
    RepairSetupProfiler::event("skip_miss_tabu");
    return task;
  }

  for (const std::unique_ptr<MoveGenerator>& generator : move_generators_) {
    if (!generatorAllowed(*generator)) {
      continue;
    }

    if (!generator->isApplicable(target)) {
      RepairSetupProfiler::event(
          "skip_not_applicable", 1, moveName(generator->type()));
      continue;
    }

    if (generator->type() == MoveType::kBuffer) {
      ++rebuffer_targets_in_batch_;
      work_.addPlanRequest();
    }

    // Prepare the generator's live STA reads here, so its generate() can run
    // on a worker.
    generator->prepareInputs(target);
    task.generators.push_back(generator.get());
  }

  task.generated.resize(task.generators.size());
  return task;
}

bool SetupParallelEvalPolicy::generatorAllowed(
    const MoveGenerator& generator) const
{
  switch (generator.type()) {
    case MoveType::kBuffer: {
      // Targets come worst slack first, so the budget keeps the trees that
      // matter.
      const int max_targets = policy_config_.parallel_eval.rebuffer_max_targets;
      if (rebuffer_targets_in_batch_ >= max_targets) {
        RepairSetupProfiler::event("skip_rebuffer_target_budget");
        return false;
      }
      return true;
    }
    case MoveType::kUnbuffer:
      // The tail's own removal generator runs only in single-move batches; a
      // requested unbuffer move runs in every batch.
      if (!unbuffer_in_sequence_ && !tail_.unbufferAllowed()) {
        RepairSetupProfiler::event("skip_unbuffer_batch");
        return false;
      }
      return true;
    case MoveType::kClone:
    case MoveType::kSizeUp:
    case MoveType::kSizeUpMatch:
    case MoveType::kSizeDownFanout:
    case MoveType::kSwapPins:
    case MoveType::kVtSwap:
    case MoveType::kSplitLoad:
    case MoveType::kReroute:
    case MoveType::kBufferToInverters:
    case MoveType::kCount:
      return true;
  }
  logger_->critical(utl::RSZ, 2030, "unhandled MoveType in generatorAllowed");
}

TargetEvaluation SetupParallelEvalPolicy::collectGenerated(GenerationTask& task)
{
  const bool profiling = RepairSetupProfiler::enabled();
  TargetEvaluation evaluation;
  for (size_t index = 0; index < task.generators.size(); ++index) {
    CandidateVector& generated = task.generated[index];
    RepairSetupProfiler::movesGenerated(task.generators[index]->type(),
                                        generated);

    evaluation.candidates.reserve(evaluation.candidates.size()
                                  + generated.size());
    std::ranges::move(generated, std::back_inserter(evaluation.candidates));
  }

  if (profiling && !task.generators.empty()) {
    RepairSetupProfiler::replay(task.counts);
    RepairSetupProfiler::workerTime("generation", task.worker_elapsed);
  }

  return evaluation;
}

void SetupParallelEvalPolicy::estimateTargets(
    std::vector<TargetEvaluation>& evaluations)
{
  const ProfileScope profile("candidate_estimation");
  const bool profiling = RepairSetupProfiler::enabled();
  // The WNS tail keeps estimator-rejected candidates with a positive own
  // gain; the batch STA judges them.
  const bool permissive = tail_.active();

  std::vector<TargetEvaluation*> tasks;
  tasks.reserve(evaluations.size());
  for (TargetEvaluation& evaluation : evaluations) {
    tasks.push_back(&evaluation);
  }

  thread_pool_->parallelFor(
      tasks, [profiling, permissive](TargetEvaluation* const& evaluation) {
        const WorkerTimer timer(profiling ? &evaluation->worker_elapsed
                                          : nullptr);
        estimateTarget(*evaluation, permissive);
      });

  // Fold the candidate counters into the profiler on the main thread, after
  // the workers finish.
  const ProfileScope reporting("reporting");
  for (TargetEvaluation& evaluation : evaluations) {
    work_.addEstimates(evaluation.candidates.size());
    if (profiling) {
      RepairSetupProfiler::workerTime("estimation", evaluation.worker_elapsed);
      for (size_t index = 0; index < evaluation.candidates.size(); ++index) {
        RepairSetupProfiler::moveEstimated(*evaluation.candidates[index],
                                           evaluation.estimates[index]);
      }

      if (evaluation.hasBestCandidate()) {
        RepairSetupProfiler::event(
            "winners", 1, moveName(evaluation.bestCandidate().type()));
      } else {
        RepairSetupProfiler::event("skip_no_legal_candidate");
      }
    }
  }
}

void SetupParallelEvalPolicy::estimateTarget(TargetEvaluation& evaluation,
                                             const bool permissive)
{
  evaluation.estimates.reserve(evaluation.candidates.size());
  for (std::unique_ptr<MoveCandidate>& candidate : evaluation.candidates) {
    Estimate estimate = candidate->estimate();
    if (permissive && !estimate.legal && estimate.tail_admissible) {
      // Trust the own-path gain and leave the side cost to the batch STA.  The
      // adjusted score keeps clean estimates ahead: it is zero or less, or
      // below the RebufferMt score floor, and ranks these by the smallest
      // modelled loss.  A side floor violation stays rejected, except
      // SizeUpMt's opposite-transition reject: its score is the positive
      // own-path gain, so it can rank ahead of clean estimates, and only the
      // batch STA judges the opposite transition.
      estimate.legal = true;
      estimate.reason = "wns_tail_permissive";
    }

    evaluation.estimates.push_back(estimate);
    if (!estimate.legal) {
      continue;
    }

    // The first candidate with the highest score wins.
    if (!evaluation.hasBestCandidate()
        || estimate.score > evaluation.best_estimate.score) {
      evaluation.best_candidate = candidate.get();
      evaluation.best_estimate = estimate;
    }
  }
}

std::vector<BatchCandidate> SetupParallelEvalPolicy::makeBatchCandidates(
    const std::vector<TargetEvaluation>& evaluations) const
{
  // A stalled batch ranks the targets by their share of the violating cone.
  const bool weighted = stall_.weightsPriority();

  std::vector<BatchCandidate> candidates;
  candidates.reserve(evaluations.size());
  for (size_t index = 0; index < evaluations.size(); ++index) {
    const TargetEvaluation& evaluation = evaluations[index];
    if (!evaluation.hasBestCandidate()) {
      continue;
    }

    const MoveCandidate& best = evaluation.bestCandidate();
    const float local_score = evaluation.best_estimate.score;
    candidates.push_back(
        {.evaluation_index = index,
         .local_score = local_score,
         .priority_score = weighted ? bottleneck_->bottleneckPriorityScore(
                                          local_score,
                                          best.target().vertex(resizer_),
                                          policy_config_.parallel_eval.priority)
                                    : local_score,
         .endpoint_slack = sta::delayAsFloat(best.target().slack),
         .impact = best.impact()});
  }

  return candidates;
}

int SetupParallelEvalPolicy::commitBatch(
    std::vector<TargetEvaluation>& evaluations,
    const BatchSelection& selection,
    const std::unordered_set<const MoveCandidate*>* skip)
{
  int batch_moves = 0;
  batch_instances_.clear();
  batch_candidates_.clear();
  batch_swaps_.clear();
  for (const size_t evaluation_index : selection.evaluation_indices) {
    TargetEvaluation& evaluation = evaluations[evaluation_index];
    MoveCandidate& candidate = evaluation.bestCandidate();
    if (skip != nullptr && skip->contains(&candidate)) {
      continue;
    }

    const Target& target = candidate.target();
    sta::Instance* inst = target.inst(resizer_);
    // The name is read before the move, which may delete the instance.
    InstanceRecord record{
        .inst = inst, .name = resizer_.dbNetwork()->staToDb(inst)->getName()};

    std::optional<SwapRecord> swap = recordSwap(candidate, inst);
    const MoveResult result = committer_.commit(candidate);
    if (!result.accepted) {
      continue;
    }

    batch_instances_.push_back(std::move(record));
    batch_candidates_.push_back(&candidate);
    if (swap.has_value() && result.move_count == 1
        && result.touched_instances.size() == 1
        && result.touched_instances.front() == inst) {
      swap->result = result;
      batch_swaps_.push_back(std::move(*swap));
    }

    committer_.trackMoveAttempt(
        candidate, target.driver_pin, target.endpointPin(resizer_));
    ++batch_moves;
    ++committed_moves_;
    ++tentative_moves_;
  }

  return batch_moves;
}

std::optional<SetupParallelEvalPolicy::SwapRecord>
SetupParallelEvalPolicy::recordSwap(MoveCandidate& candidate,
                                    sta::Instance* inst) const
{
  // Remember the cell a size or VT swap replaces, so a salvage can revert it
  // alone.  replaceCell finds masters by name, so a cell whose name maps to
  // another master than the instance has is not recorded.  The target
  // collection already checked that the instance has a Liberty cell and a
  // master.
  if (candidate.type() != MoveType::kSizeUp
      && candidate.type() != MoveType::kVtSwap) {
    return std::nullopt;
  }

  const sta::LibertyCell* cell = network_->libertyCell(inst);
  odb::dbInst* physical = resizer_.dbNetwork()->staToDb(inst);
  if (resizer_.block()->getDb()->findMaster(cell->name().c_str())
      != physical->getMaster()) {
    return std::nullopt;
  }

  SwapRecord swap{.candidate = &candidate, .inst = inst, .before_cell = cell};
  physical->getLocation(swap.x, swap.y);
  return swap;
}

bool SetupParallelEvalPolicy::batchImprovesCheckpoint(
    const sta::Slack checkpoint_wns,
    const sta::Slack checkpoint_tns,
    const sta::Slack current_wns,
    const sta::Slack current_tns,
    const sta::Slack best_wns,
    const sta::Slack wns_tolerance)
{
  if (sta::fuzzyLess(checkpoint_wns, current_wns)) {
    return true;
  }

  // TNS progress counts while WNS stays in the tolerated band below the best
  // checkpoint, so tiny WNS jitter cannot discard whole batches.
  const bool wns_within_band
      = sta::fuzzyEqual(checkpoint_wns, current_wns)
        || sta::fuzzyGreaterEqual(current_wns, best_wns - wns_tolerance);
  return wns_within_band && sta::fuzzyLess(checkpoint_tns, current_tns);
}

bool SetupParallelEvalPolicy::acceptsBatch(const sta::Slack wns_after,
                                           const sta::Slack tns_after) const
{
  if (!batchImprovesCheckpoint(checkpoint_wns_,
                               checkpoint_tns_,
                               wns_after,
                               tns_after,
                               best_checkpoint_wns_,
                               checkpointWnsTolerance())) {
    return false;
  }

  // The tail also takes estimator-rejected moves, so a WNS gain may cost
  // only a bounded multiple of itself in TNS.
  if (tail_.active()) {
    const float wns_gain = std::max(
        0.0f,
        sta::delayAsFloat(wns_after) - sta::delayAsFloat(checkpoint_wns_));
    if (sta::fuzzyLess(
            tns_after,
            checkpoint_tns_
                - wns_gain * policy_config_.parallel_eval.wns_tail_tns_ratio)) {
      RepairSetupProfiler::event("wns_tail_reject_tns");
      return false;
    }
  }

  return true;
}

sta::Slack SetupParallelEvalPolicy::checkpointWnsTolerance() const
{
  // The tail works on the worst path, so it allows no WNS drift.
  if (tail_.active()) {
    return 0.0f;
  }
  return reference_period_
         * policy_config_.parallel_eval.checkpoint_wns_tolerance_period_pct
         / 100.0f;
}

int SetupParallelEvalPolicy::adaptiveBatchSize(
    const size_t failing_endpoint_count,
    const int min_cap,
    const int max_cap)
{
  // Each batch pays a full timing update, so the configured cap is a floor;
  // the failing endpoint count raises it up to the bound.
  return static_cast<int>(
      std::min(std::max(failing_endpoint_count, static_cast<size_t>(min_cap)),
               static_cast<size_t>(max_cap)));
}

void SetupParallelEvalPolicy::startSmallBatchRetry()
{
  RepairSetupProfiler::event(
      "retry", {.metric = "retries", .detail = "small_batch", .trace = true});
  small_batch_retry_started_ = true;
  checkpoint_misses_ = 0;

  // The missed batches spent the stall budget at the full cap, and the small
  // budget of the retry would end it after one batch.  The retry also tries
  // again the targets that the undone batches left on the tabu list.
  stall_.reset(checkpoint_tns_, reference_period_);
  tabu_.clearTemporary();

  // The retry keeps the conflict rules but drops the score floor and the
  // worst-path reserve.
  BatchSelectorConfig selector = policy_config_.parallel_eval.selector;
  selector.min_score_ratio = 0.0f;
  selector.wns_reserve_pct = 0;
  batch_selector_ = std::make_unique<BatchSelector>(std::move(selector));
}

int SetupParallelEvalPolicy::effectiveBatchCap(
    const size_t failing_endpoint_count) const
{
  if (tail_.active()) {
    return tail_.batchCap();
  }

  // The low-progress retry keeps its small fixed batches.
  if (small_batch_retry_started_) {
    return policy_config_.parallel_eval.retry_batch_cap;
  }

  return adaptiveBatchSize(failing_endpoint_count,
                           policy_config_.parallel_eval.min_batch_cap,
                           policy_config_.parallel_eval.max_batch_cap);
}

void SetupParallelEvalPolicy::noteTimingUpdate()
{
  work_.addTimingUpdate(resizer_.estimateParasitics()->pendingNetCount());
}

void SetupParallelEvalPolicy::updateTiming()
{
  noteTimingUpdate();
  resizer_.updateParasiticsAndTiming();
}

void SetupParallelEvalPolicy::openBatchJournal()
{
  if (!batch_journal_open_) {
    committer_.beginJournal();
    batch_journal_open_ = true;
  }
}

void SetupParallelEvalPolicy::saveBatchCheckpoint(const sta::Slack wns,
                                                  const sta::Slack tns)
{
  RepairSetupProfiler::event("checkpoints_saved");
  RepairSetupProfiler::qor("checkpoint_saved", wns, tns);
  if (tail_.speculating()) {
    RepairSetupProfiler::event("wns_tail_speculation_paid");
    RepairSetupProfiler::event("wns_tail_speculation_paid_batches",
                               tail_.speculativeBatches());
    tail_.endSpeculation();
  }

  if (batch_journal_open_) {
    // Timing is already current from the batch-level update.
    committer_.commitJournal(false);
    batch_journal_open_ = false;
  }

  stall_.onSaved(checkpoint_tns_,
                 best_checkpoint_wns_,
                 wns,
                 tns,
                 work_.seconds(policy_config_.parallel_eval));
  checkpoint_wns_ = wns;
  checkpoint_tns_ = tns;
  best_checkpoint_wns_ = std::max(best_checkpoint_wns_, wns);
  tentative_moves_ = 0;
  checkpoint_misses_ = 0;
}

std::vector<MoveCandidate*> SetupParallelEvalPolicy::findBatchCulprits(
    const sta::Slack pre_batch_wns,
    std::vector<MoveCandidate*>& spared) const
{
  const ProfileScope profile("qor_collection");
  std::vector<MoveCandidate*> culprits;
  std::vector<MoveCandidate*> victims;
  sta::Graph* graph = sta_->graph();

  // A target carries the slack its endpoint was collected with, the group's
  // under -path_group, so the endpoint is read the same way.  One group query
  // covers the batch; a query per endpoint would test every path against the
  // group_path exception again.
  const PathGroupFilter path_group(&resizer_);
  std::unordered_map<const sta::Pin*, sta::Slack> group_slacks;
  if (path_group.enabled()) {
    sta::Slack slack_max = -sta::INF;
    for (const MoveCandidate* candidate : batch_candidates_) {
      slack_max = std::max(slack_max, candidate->target().slack);
    }
    const PathGroupFilter::GroupSlacks slacks
        = path_group.groupEndpointSlacks(max_, slack_max);
    group_slacks.insert(slacks.begin(), slacks.end());
  }
  const auto endpoint_slack = [&](const sta::Pin* pin, sta::Vertex* vertex) {
    if (!path_group.enabled()) {
      return sta_->slack(vertex, max_);
    }
    // A missing endpoint is above every target slack, so it got no worse.
    return group_slacks.contains(pin) ? group_slacks.at(pin)
                                      : sta::Slack(sta::INF);
  };

  for (MoveCandidate* candidate : batch_candidates_) {
    bool culprit = false;
    bool victim = false;

    // The move's own endpoint must not end up worse than before the batch.
    const sta::Pin* endpoint = candidate->target().endpointPin(resizer_);
    sta::Vertex* endpoint_vertex
        = endpoint != nullptr ? graph->pinLoadVertex(endpoint) : nullptr;
    if (endpoint_vertex != nullptr
        && sta::fuzzyLess(endpoint_slack(endpoint, endpoint_vertex),
                          candidate->target().slack)) {
      culprit = true;

      // Many moves share an endpoint, so one bad estimate marks every move on
      // that path.  A swap whose own stages did not slow down lost its slack to
      // another move, so it stays in the batch.
      const std::optional<float> self_delta = selfStageDelayDelta(*candidate);
      if (self_delta.has_value() && *self_delta <= 0.0f) {
        victim = true;
        culprit = false;
      }
    }

    // A guarded side pin below the pre-batch WNS created a new worst path.
    if (!culprit) {
      culprit = std::ranges::any_of(
          candidate->guardedSidePins(), [&](const sta::Pin* pin) {
            sta::Vertex* vertex = graph->pinLoadVertex(pin);
            return vertex != nullptr
                   && sta::fuzzyLess(sta_->slack(vertex, max_), pre_batch_wns);
          });
    }

    // A move is one or the other: a spared swap that also sank a side pin is
    // a culprit, so the two lists never revert the same move twice.
    if (culprit) {
      culprits.push_back(candidate);
    } else if (victim) {
      victims.push_back(candidate);
    }
  }

  if (!victims.empty()) {
    RepairSetupProfiler::event("batch_victims_spared", victims.size());
    if (culprits.empty()) {
      // No move slowed its own stages, so the endpoint rule is all there is.
      RepairSetupProfiler::event("batch_culprits_endpoint_fallback");
      return victims;
    }

    spared = std::move(victims);
  }

  return culprits;
}

std::optional<float> SetupParallelEvalPolicy::selfStageDelayDelta(
    const MoveCandidate& candidate) const
{
  const MoveType type = candidate.type();
  if (type != MoveType::kSizeUp && type != MoveType::kVtSwap) {
    return std::nullopt;
  }

  const std::optional<EstimatorContext>& state
      = candidate.target().estimator_context;
  if (!state.has_value() || state->path_stages.empty()) {
    return std::nullopt;
  }

  // The swap changes its own arc and, through the input capacitance, the
  // fanin arc that drives it; the prepared window holds both baselines.
  float delta = 0.0f;
  float baseline = 0.0f;
  const int first = std::max(state->target_stage_index - 1, 0);
  for (int index = first; index <= state->target_stage_index; ++index) {
    const DelayStageState& stage = state->path_stages[index];
    const std::optional<float> delay = graphStageDelay(stage);
    if (!delay.has_value()) {
      return std::nullopt;
    }

    delta += *delay - stage.current_delay;
    baseline += stage.current_delay;
  }

  // Only a visible fraction of the stage delay counts as a slowdown.
  return delta
         - policy_config_.parallel_eval.culprit_delay_ratio
               * std::abs(baseline);
}

std::optional<float> SetupParallelEvalPolicy::graphStageDelay(
    const DelayStageState& stage) const
{
  const sta::TimingArc* reference = stage.arc.ref_arc;
  if (stage.driver_pin == nullptr || reference == nullptr) {
    return std::nullopt;
  }

  sta::Vertex* driver = graph_->pinDrvrVertex(stage.driver_pin);
  if (driver == nullptr) {
    return std::nullopt;
  }

  const int dcalc_ap = stage.arc.scene->dcalcAnalysisPtIndex(stage.arc.min_max);
  sta::VertexInEdgeIterator edge_iter(driver, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (edge->isWire()) {
      continue;
    }

    // The swapped cell carries a new arc set; match it by ports and edges.
    const sta::TimingArc* arc
        = findMatchingTimingArc(reference, edge->timingArcSet());
    if (arc != nullptr) {
      return sta::delayAsFloat(graph_->arcDelay(edge, arc, dcalc_ap));
    }
  }

  return std::nullopt;
}

bool SetupParallelEvalPolicy::salvageMissedBatch(
    std::vector<TargetEvaluation>& evaluations,
    const BatchSelection& selection,
    const sta::Slack pre_batch_wns,
    int& batch_moves)
{
  if (!batch_journal_open_ || batch_candidates_.empty()) {
    return false;
  }

  // A removed buffer comes back as a new instance when the journal rolls
  // back, so a batch with a removal is not re-committed in part.
  if (std::ranges::any_of(batch_candidates_, [](const MoveCandidate* move) {
        return move->type() == MoveType::kUnbuffer;
      })) {
    RepairSetupProfiler::event("batch_salvage_skipped_removal");
    return false;
  }

  const size_t batch_size = batch_candidates_.size();
  std::vector<MoveCandidate*> victims;
  const std::vector<MoveCandidate*> culprits
      = findBatchCulprits(pre_batch_wns, victims);
  if (culprits.empty() || culprits.size() >= batch_size) {
    return false;
  }

  RepairSetupProfiler::event("batch_salvage_attempts");
  RepairSetupProfiler::event("batch_culprits", culprits.size());

  std::unordered_set<const MoveCandidate*> skip;
  const auto tabu_moves = [&](const std::vector<MoveCandidate*>& moves) {
    for (MoveCandidate* move : moves) {
      skip.insert(move);
      tabu_.addTabu(move->target().inst(resizer_), batch_index_);
    }
  };
  tabu_moves(culprits);

  // Revert only the culprits and keep the other moves.  Cell-swap culprits go
  // back to their old cells in place; any other culprit rolls the batch back
  // and re-commits the rest.  Spared moves that still miss get a second pass.
  bool in_place = true;
  std::vector<MoveCandidate*> blocked = culprits;
  for (int pass = 0; pass < 2; ++pass) {
    if (in_place && revertInPlace(blocked, batch_moves)) {
      RepairSetupProfiler::event("batch_salvage_in_place");
    } else {
      // The roll-back would also drop the speculative batches kept in the
      // same journal; the caller keeps or rolls back all of them together.
      if (tail_.speculating()) {
        RepairSetupProfiler::event("batch_salvage_skipped_speculating");
        return false;
      }

      // Once the batch is rolled back, later passes re-commit as well.  The
      // restore updates timing, so it counts like an undo.
      in_place = false;
      noteTimingUpdate();
      rollBackJournal();
      openBatchJournal();
      batch_moves = commitBatch(evaluations, selection, &skip);
      if (batch_moves == 0) {
        return false;
      }
    }

    updateTiming();
    const BatchQor after = measureQor();
    RepairSetupProfiler::qor("batch_salvaged_after", after.wns, after.tns);
    if (withinMaxUtilization() && acceptsBatch(after.wns, after.tns)) {
      RepairSetupProfiler::event("batches_salvaged");
      saveBatchCheckpoint(after.wns, after.tns);
      return true;
    }

    if (victims.empty() || skip.size() + victims.size() >= batch_size) {
      break;
    }

    RepairSetupProfiler::event("batch_salvage_second_pass");
    tabu_moves(victims);
    blocked = std::move(victims);
    victims.clear();
  }

  RepairSetupProfiler::event("batch_salvage_failed");
  return false;
}

bool SetupParallelEvalPolicy::revertInPlace(
    const std::vector<MoveCandidate*>& moves,
    int& batch_moves)
{
  std::vector<SwapRecord*> swaps;
  swaps.reserve(moves.size());
  for (const MoveCandidate* move : moves) {
    // Only a recorded cell swap can be reverted in place.
    const std::vector<SwapRecord>::iterator swap
        = std::ranges::find(batch_swaps_, move, &SwapRecord::candidate);
    if (swap == batch_swaps_.end()) {
      return false;
    }

    // A move reverted by an earlier pass has nothing left to undo.
    if (!swap->reverted) {
      swaps.push_back(&*swap);
    }
  }

  const ProfileScope profile("apply");
  for (SwapRecord* swap : swaps) {
    RepairSetupProfiler::beginRevert(*swap->candidate);

    // The inverse swap stays in the open batch journal, so a later restore of
    // the whole batch still lands on the pre-batch state.
    if (!resizer_.replaceCell(swap->inst, swap->before_cell)) {
      logger_->error(
          utl::RSZ, 2037, "PARALLEL_EVAL salvage could not revert a cell swap");
    }

    resizer_.dbNetwork()->staToDb(swap->inst)->setLocation(swap->x, swap->y);

    // Drop the move's pending accounting so the journal commit does not
    // accept it and a later restore does not reject it twice.
    if (!committer_.rejectPendingCellMove(
            swap->result, swap->candidate->target().driver_pin)) {
      logger_->error(
          utl::RSZ, 2038, "PARALLEL_EVAL salvage lost its pending cell move");
    }

    RepairSetupProfiler::endRevert("salvage");
    swap->reverted = true;
  }

  const int reverted = static_cast<int>(swaps.size());
  batch_moves -= reverted;
  committed_moves_ -= reverted;
  tentative_moves_ -= reverted;
  RepairSetupProfiler::event("batch_salvage_in_place_reverts", reverted);

  return true;
}

void SetupParallelEvalPolicy::undoBatch(
    const std::vector<InstanceRecord>& speculative)
{
  if (!batch_journal_open_) {
    return;
  }

  RepairSetupProfiler::event("batches_undone");
  RepairSetupProfiler::event("moves_undone", tentative_moves_);
  noteTimingUpdate();
  rollBackJournal();

  // The rollback deleted the instances the open journal created, such as the
  // buffers of a speculative run, so only the instances it kept go on the
  // tabu.
  const sta::dbNetwork* db_network = resizer_.dbNetwork();
  for (sta::Instance* inst : keptInstances(db_network, speculative)) {
    tabu_.addTabu(inst, batch_index_);
  }

  // The same targets would win again from this state, so skip them for a
  // few batches.
  for (sta::Instance* inst : keptInstances(db_network, batch_instances_)) {
    if (tabu_.recordUndo(inst, batch_index_)) {
      RepairSetupProfiler::event("undo_blocked_instances");
    }
  }

  batch_instances_.clear();
}

void SetupParallelEvalPolicy::restoreBatchCheckpoint()
{
  if (!batch_journal_open_) {
    return;
  }

  RepairSetupProfiler::event("checkpoints_restored");
  noteTimingUpdate();
  rollBackJournal();
}

void SetupParallelEvalPolicy::rollBackJournal()
{
  committer_.restoreJournal();
  // The restore leaves the undone edits in the tracked area, which later
  // phases and the final report read through overMaxArea().
  resizer_.recountDesignAreaAfterUndo();
  batch_journal_open_ = false;
  committed_moves_ -= tentative_moves_;
  tentative_moves_ = 0;
}

void SetupParallelEvalPolicy::applyStopRules(const int batch_moves)
{
  if (finishIfNoSetupViolations()) {
    return;
  }

  // -max_iterations ends the run: it starts no WNS tail and stops a running
  // one.
  if (config_.max_iterations > 0 && batch_index_ >= config_.max_iterations) {
    stopAtCheckpoint("max_iterations");
    return;
  }

  if (tail_.active()) {
    applyTailStopRules(batch_moves);
    return;
  }

  // The journal is closed here: the verdict saved or undid the batch.
  if (batch_moves == 0) {
    startTailOrFinish("no_committed_moves");
    return;
  }

  if (stall_.windowStalled()) {
    startTailOrFinish("stalled_window");
    return;
  }

  const StallMonitor::Stall stall = stall_.stall();
  if (stall != StallMonitor::Stall::kNone) {
    startTailOrFinish(stall == StallMonitor::Stall::kMoves ? "stalled_moves"
                                                           : "stalled_batches");
    return;
  }

  if (checkpoint_misses_
      >= policy_config_.parallel_eval.max_checkpoint_misses) {
    startTailOrFinish("max_checkpoint_misses");
  }
}

void SetupParallelEvalPolicy::startTailOrFinish(const char* reason)
{
  // Whichever rule ends the batches, a run that has kept no move retries once
  // with small batches first.  Full batches spend the stall budget before
  // they reach the miss limit, so the miss rule alone would never retry.
  if (!small_batch_retry_started_ && committed_moves_ == 0) {
    startSmallBatchRetry();
    return;
  }

  if (startWnsTail(reason)) {
    return;
  }

  RepairSetupProfiler::stop(reason);
  markRunComplete(false);
}

bool SetupParallelEvalPolicy::startWnsTail(const char* reason)
{
  if (!tail_.start(work_.seconds(policy_config_.parallel_eval),
                   reference_period_)) {
    return false;
  }

  if (RepairSetupProfiler::enabled()) {
    RepairSetupProfiler::current()->metadata(
        "wns_tail_work_start", fmt::format("{:.3f}", tail_.workStart()));
    RepairSetupProfiler::current()->metadata(
        "wns_tail_budget_work", fmt::format("{:.3f}", tail_.workBudget()));
  }

  checkpoint_misses_ = 0;

  // The stalled main loop left the worst path's instances on the tabu list;
  // the tail exists to work on exactly those.
  tabu_.clearTemporary();
  addTailGenerators();

  RepairSetupProfiler::event(
      "wns_tail_begin",
      {.metric = "wns_tail_starts", .detail = reason, .trace = true});
  RepairSetupProfiler::qor("wns_tail_begin", checkpoint_wns_, checkpoint_tns_);
  debugPrint(logger_,
             utl::RSZ,
             "repair_setup",
             1,
             "PARALLEL_EVAL WNS tail after {} at batch {} (WNS {:.3e})",
             reason,
             batch_index_,
             sta::delayAsFloat(checkpoint_wns_));

  return true;
}

void SetupParallelEvalPolicy::addTailGenerators()
{
  // Pin swaps, buffer removal and size down join the tail when the default
  // sequence left them out.  Buffer removal runs only in single-move batches.
  for (const MoveType type :
       {MoveType::kSwapPins, MoveType::kUnbuffer, MoveType::kSizeDownFanout}) {
    if (tailMayAddMove(type)) {
      move_generators_.push_back(
          makeParallelEvalGenerator(type, makeGeneratorContext()));
      move_generators_.back()->startPhase(thread_pool_->threadCount());
      RepairSetupProfiler::event("wns_tail_generator", 1, moveName(type));
    }
  }
}

bool SetupParallelEvalPolicy::moveEnabled(const MoveType type) const
{
  switch (type) {
    case MoveType::kVtSwap:
      return !config_.skip_vt_swap && resizer_.vtCategoryCount() > 1;
    case MoveType::kSizeUp:
    case MoveType::kSizeUpMatch:
    case MoveType::kReroute:
      return true;
    case MoveType::kBuffer:
      return !config_.skip_buffering;
    case MoveType::kSwapPins:
      return !config_.skip_pin_swap;
    // The topology moves follow the same skip options as the serial
    // sequence.
    case MoveType::kClone:
      return !config_.skip_gate_cloning;
    case MoveType::kSplitLoad:
      return !config_.skip_buffering;
    case MoveType::kUnbuffer:
      return !config_.skip_buffer_removal;
    case MoveType::kSizeDownFanout:
      return !config_.skip_size_down_fanout;
    case MoveType::kBufferToInverters:
      return !config_.skip_buffer_to_inverters;
    // parseMoveSequence() never yields kCount.
    case MoveType::kCount:
      break;
  }

  return false;
}

bool SetupParallelEvalPolicy::tailMayAddMove(const MoveType type) const
{
  // An explicit -sequence is the complete move set of the run, and the
  // -skip_* options apply to the tail as they do to the batches.
  return config_.sequence.empty() && moveEnabled(type)
         && findGenerator(type) == nullptr;
}

void SetupParallelEvalPolicy::applyTailStopRules(const int batch_moves)
{
  const char* reason = nullptr;
  switch (tail_.nextStep(
      batch_moves, work_.seconds(policy_config_.parallel_eval), reason)) {
    case WnsTail::Step::kContinue:
      return;
    case WnsTail::Step::kSpeculate:
      // The stalled greedy stage undid its last batch, so the journal is
      // closed.
      tail_.beginSpeculativeStage();
      tabu_.clearTemporary();
      RepairSetupProfiler::event("wns_tail_speculative_stage",
                                 {.metric = "wns_tail_speculative_stages",
                                  .detail = std::to_string(tail_.endpoints()),
                                  .trace = true});
      return;
    case WnsTail::Step::kWiden:
      // The restore also drops an open speculative run.
      restoreBatchCheckpoint();
      tail_.widen();
      tabu_.clearTemporary();
      RepairSetupProfiler::event(
          "wns_tail_grow",
          {.detail = std::to_string(tail_.endpoints()), .trace = true});
      return;
    case WnsTail::Step::kStop:
      break;
  }

  RepairSetupProfiler::event("wns_tail_batches", tail_.batches());
  RepairSetupProfiler::stop(reason);
  restoreBatchCheckpoint();
  tail_.stop();
  markRunComplete(false);
}

void SetupParallelEvalPolicy::printTrackerBatchSummary() const
{
  if (!batch_journal_open_) {
    committer_.printTrackerPhaseSummary("PARALLEL_EVAL Batch Summary",
                                        "PARALLEL_EVAL Batch Endpoint Profiler",
                                        true);
  }
}

void SetupParallelEvalPolicy::finish()
{
  // Only this policy reads the crit-path load pins of every endpoint.
  target_collector_->setRecordCritPathLoadPins(false);
  // The worker threads and the generators' state for them end with the phase.
  target_collector_->setScanThreadPool(nullptr);
  for (const std::unique_ptr<MoveGenerator>& generator : move_generators_) {
    generator->finishPhase();
  }
  thread_pool_.reset();

  if (RepairSetupProfiler::enabled()) {
    // The work the run spent, for checking the meter against the wall clock.
    RepairSetupProfiler::current()->metadata(
        "work_seconds",
        fmt::format("{:.3f}", work_.seconds(policy_config_.parallel_eval)));
  }
}

}  // namespace rsz
