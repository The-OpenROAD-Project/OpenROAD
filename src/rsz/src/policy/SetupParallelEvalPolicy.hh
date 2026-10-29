// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <deque>
#include <limits>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "BatchSelector.hh"
#include "BottleneckAnalysis.hh"
#include "OptimizationPolicy.hh"
#include "OptimizerTypes.hh"
#include "ParallelEvalState.hh"
#include "RepairSetupContext.hh"
#include "RepairSetupProfiler.hh"
#include "rsz/Resizer.hh"

namespace rsz {

class MoveCommitter;
class MoveGenerator;

// Batch setup repair.  The main thread prepares each target's live STA reads;
// generation and scoring run in parallel.  One conflict-free batch is then
// committed before a single parasitics and timing update.
class SetupParallelEvalPolicy : public OptimizationPolicy
{
 public:
  SetupParallelEvalPolicy(Resizer& resizer,
                          MoveCommitter& committer,
                          RepairSetupContext& setup_context,
                          const OptimizerRunConfig& config);
  ~SetupParallelEvalPolicy() override;

  const char* name() const override { return "SetupParallelEvalPolicy"; }
  bool start() override;
  // One batch: collect endpoints, evaluate them in chunks, select, commit,
  // update timing once and judge the checkpoint.
  void iterate() override;
  void finish() override;

  // === Batch rules ==========================================================
  // Pure functions of the batch flow.
  // The failing endpoint count clamped to [min_cap, max_cap].
  static int adaptiveBatchSize(size_t failing_endpoint_count,
                               int min_cap,
                               int max_cap);
  // Endpoints per chunk: a multiple of the batch cap, leaving room for
  // conflicts and targets without candidates.
  static size_t evaluationEndpointLimit(int batch_cap,
                                        int eval_endpoint_factor,
                                        int min_endpoints);
  // WNS better, or TNS better while WNS stays within wns_tolerance of the best
  // checkpoint WNS (a tie when the tolerance is 0).
  static bool batchImprovesCheckpoint(sta::Slack checkpoint_wns,
                                      sta::Slack checkpoint_tns,
                                      sta::Slack current_wns,
                                      sta::Slack current_tns,
                                      sta::Slack best_wns,
                                      sta::Slack wns_tolerance);

 private:
  // The unit tests (TestResizerMt) reach the internals through this peer.
  friend class ResizerTestPeer;

  // === Batch types ==========================================================
  // The violating endpoints this batch draws from and its move cap.
  struct BatchScope
  {
    size_t target_endpoints{0};
    int batch_cap{0};
  };

  // The candidate pool of one batch.  Candidates keep references to their
  // Target, so the targets live in a deque that never moves them.
  struct BatchPool
  {
    std::deque<Target> targets;
    std::vector<TargetEvaluation> evaluations;
    std::vector<BatchCandidate> candidates;
    BatchSelection selection;

    // Takes a chunk; the returned pointers stay valid for the whole batch.
    std::vector<Target*> adopt(std::vector<Target> chunk);
    void append(std::vector<TargetEvaluation> chunk_evaluations);
  };

  // WNS and TNS after a timing update, of the -path_group group when one is
  // selected.
  struct BatchQor
  {
    sta::Slack wns{0.0f};
    sta::Slack tns{0.0f};
  };

  // One target's generation: its generators in -sequence order and what each
  // produced.  Workers write only the task's own slots and profiler counts;
  // the main thread folds the task afterwards.
  struct GenerationTask
  {
    Target* target{nullptr};
    std::vector<MoveGenerator*> generators{};
    std::vector<CandidateVector> generated{};
    RepairSetupProfiler::DeferredCounts counts{};
    double worker_elapsed{0.0};
  };

  // A cell swap of the current batch with the cell and location it replaced,
  // so a salvage can revert single moves inside the open journal.
  struct SwapRecord
  {
    MoveCandidate* candidate{nullptr};
    sta::Instance* inst{nullptr};
    const sta::LibertyCell* before_cell{nullptr};
    int x{0};
    int y{0};
    MoveResult result{};
    bool reverted{false};
  };

  // === Run setup ============================================================
  // start() steps: the move set from -sequence, the run counters and the first
  // checkpoint.
  void selectMoveSequence();
  void resetRunState();
  // Capture initial QoR and reset checkpoint, stall, and work references.
  void saveInitialCheckpoint();
  // The shortest period of a clock with source pins in any mode, else of a
  // virtual clock, else the shortest set_max_delay; without any of them, the
  // magnitude of the initial WNS.
  float referencePeriod(sta::Slack initial_wns) const;
  void buildMoveGenerators(const std::vector<MoveType>& move_types,
                           const GeneratorContext& context) override;
  // A move type runs only when its -skip_* option and the library allow it.
  bool moveEnabled(MoveType type) const;

  // === Batch scope ==========================================================
  // Collects the violating endpoints, prints the progress row and sets the
  // batch scope.
  BatchScope collectTargetEndpoints();
  // Clamp the current failing-endpoint count to the active batch limits.
  int effectiveBatchCap(size_t failing_endpoint_count) const;
  // Restart a run whose batches kept no move with a smaller batch cap once.
  void startSmallBatchRetry();
  // The cone weights while the saved batches stall WNS; one analysis serves
  // several batches, and a pause starts the next one fresh.
  void analyzeBottlenecks(const BatchScope& scope);

  // === Evaluation ===========================================================
  // Evaluates the scope in chunks, worst slack first, until the batch is full.
  // The returned pool carries the selected batch.
  BatchPool evaluateTargets(const BatchScope& scope);
  // The path-driver targets of the violating endpoints [begin, end);
  // visited_pins dedups drivers across chunks.
  std::vector<Target> collectTargets(
      size_t begin_endpoint,
      size_t end_endpoint,
      std::unordered_set<sta::Pin*>& visited_pins) const;
  // Resolve the target driver's fanout for generation limits.
  int targetFanout(const Target& target) const;
  // Candidates in target order, each target's in -sequence order:
  // planGeneration() on the main thread, generate() on the pool, then
  // collectGenerated().
  std::vector<TargetEvaluation> generateCandidates(
      const std::vector<Target*>& targets);
  // One target's skip rules, rebuffer budget, work count and generation
  // inputs.
  GenerationTask planGeneration(Target& target);
  // Whether `generator` may run on a target of this batch.
  bool generatorAllowed(const MoveGenerator& generator) const;
  // The task's candidates in generator order; also replays the profiler
  // counts and the generation time the worker recorded for the task.
  TargetEvaluation collectGenerated(GenerationTask& task);
  // Score each target's generated candidates on the worker pool.
  void estimateTargets(std::vector<TargetEvaluation>& evaluations);
  // permissive (WNS tail) keeps estimator-rejected candidates with a positive
  // own gain; the batch STA judges them.
  static void estimateTarget(TargetEvaluation& evaluation, bool permissive);

  // === Selection ============================================================
  // Candidates know their Target, so the pool alone identifies each entry.
  std::vector<BatchCandidate> makeBatchCandidates(
      const std::vector<TargetEvaluation>& evaluations) const;
  // The conflict-free batch over the whole pool.
  void selectBatch(BatchPool& pool, int batch_cap);
  // The profiler records of the batch evaluateTargets() selected.
  void recordSelection(const BatchPool& pool) const;

  // === Commit and verdict ===================================================
  // Start the journal that owns all moves of the next batch.
  void openBatchJournal();
  // Capture the original cell and placement for selective salvage.
  std::optional<SwapRecord> recordSwap(MoveCandidate& candidate,
                                       sta::Instance* inst) const;
  // skip lists candidates left out of a re-commit after a salvage.
  int commitBatch(std::vector<TargetEvaluation>& evaluations,
                  const BatchSelection& selection,
                  const std::unordered_set<const MoveCandidate*>* skip
                  = nullptr);
  // A parasitics-and-timing update or journal restore the policy requests.
  void noteTimingUpdate();
  // The batch's parasitics-and-timing update, charged to the work meter.
  void updateTiming();
  // Read WNS and TNS after the batch timing update.  Under -path_group they
  // are the group's, as LEGACY judges: another group's worse paths at the
  // same endpoints would hide every gain in the group.
  BatchQor measureQor() const;
  // The verdict: saves a batch that beats the checkpoint, else salvages it
  // without its culprits, speculates on it in the WNS tail, or undoes it.
  // True when the batch or its salvage was saved.
  bool judgeBatch(BatchPool& pool, sta::Slack pre_batch_wns, int& batch_moves);
  bool withinMaxUtilization() const;
  sta::Slack checkpointWnsTolerance() const;
  // batchImprovesCheckpoint() plus the tail's limit on the TNS a WNS gain may
  // cost.
  bool acceptsBatch(sta::Slack wns_after, sta::Slack tns_after) const;
  // Keep the batch and advance the saved WNS/TNS reference.
  void saveBatchCheckpoint(sta::Slack wns, sta::Slack tns);
  // Charge a missed batch to the checkpoint-miss and stall counters.
  void noteMissedBatch(bool utilization_legal);
  // Speculates on a missed batch in the WNS tail, else undoes it.
  void settleMissedBatch(int batch_moves);
  // Undoes the current batch; a failed speculative run also puts all its
  // instances on the tabu.
  void undoMissedBatch();
  // Rolls back the current batch and puts its instances, and the
  // `speculative` instances of a failed run, on the tabu.
  void undoBatch(const std::vector<InstanceRecord>& speculative = {});
  // Restore the last saved design before another batch is attempted.
  void restoreBatchCheckpoint();
  // Restores the batch journal and takes back its moves.  The callers count the
  // timing update.
  void rollBackJournal();

  // === Salvage ==============================================================
  // Reverts only the culprits of a missed batch: cell swaps in place, else a
  // rollback and re-commit.  True when the result is saved as a checkpoint.
  bool salvageMissedBatch(std::vector<TargetEvaluation>& evaluations,
                          const BatchSelection& selection,
                          sta::Slack pre_batch_wns,
                          int& batch_moves);
  // Moves whose guarded side pins fell below the pre-batch design WNS, or
  // whose own endpoint got worse than the slack its target was collected with
  // (the group's under -path_group).  spared gets the endpoint-worse cell
  // swaps whose own stages did not slow down.
  std::vector<MoveCandidate*> findBatchCulprits(
      sta::Slack pre_batch_wns,
      std::vector<MoveCandidate*>& spared) const;
  // The change of a cell swap's own and fanin stage delays since prepare;
  // nullopt for other moves or when a stage arc is gone.
  std::optional<float> selfStageDelayDelta(
      const MoveCandidate& candidate) const;
  std::optional<float> graphStageDelay(const DelayStageState& stage) const;
  // Swaps the listed moves back to their old cells.  False, with nothing
  // touched, when a move is not a recorded cell swap.
  bool revertInPlace(const std::vector<MoveCandidate*>& moves,
                     int& batch_moves);

  // === Stop rules ===========================================================
  // Ends the run when no setup violation is left in what the verdict judges.
  bool finishIfNoSetupViolations();
  // Ends the run at the -max_utilization area, where nothing may run.
  bool finishAtMaxArea();
  // Ends the run at a hard limit with `reason`: the open journal rolls back to
  // the last checkpoint and a running tail stops.
  void stopAtCheckpoint(const char* reason);
  // Ends the run when the evaluated endpoints yield no target.
  void finishWithoutTargets();
  // Apply the batch-stage stall, miss, and work-budget stop rules.
  void applyStopRules(int batch_moves);
  // Hands a stalled run to the small-batch retry when no move was kept, else
  // to the WNS tail, or ends it with `reason`.
  void startTailOrFinish(const char* reason);
  void printTrackerBatchSummary() const;

  // === WNS tail =============================================================
  // Starts the tail if it is on and has not run.  False when the run should
  // stop instead.
  bool startWnsTail(const char* reason);
  // The generators the tail adds for moves the batches did not run.
  void addTailGenerators();
  // Whether the tail may add a generator for a move the batches did not run.
  bool tailMayAddMove(MoveType type) const;
  // Apply the WNS-tail stall and work-budget stop rules.
  void applyTailStopRules(int batch_moves);

  // === State ================================================================
  std::unique_ptr<BatchSelector> batch_selector_;
  // The cone weights of stalled batches; refreshed every few batches and when
  // the WNS tail starts.
  std::unique_ptr<BottleneckAnalysis> bottleneck_;
  int bottleneck_age_{std::numeric_limits<int>::max()};
  bool bottleneck_in_tail_{false};

  // Applied moves, including the current tentative batch until rollback.
  int committed_moves_{0};
  int batch_index_{0};
  // Moves committed since the last saved checkpoint.
  int tentative_moves_{0};
  // Consecutive batches not saved as checkpoints.
  int checkpoint_misses_{0};
  bool small_batch_retry_started_{false};
  // The -max_utilization warning prints once per run.
  bool max_utilization_warned_{false};
  bool batch_journal_open_{false};
  sta::Slack checkpoint_wns_{0.0f};
  sta::Slack checkpoint_tns_{0.0f};
  // Best WNS among saved checkpoints; bounds the tolerated WNS drift.
  sta::Slack best_checkpoint_wns_{0.0f};
  // Scales the WNS thresholds of the stall rules, the checkpoint tolerance
  // and the tail; fixed for the run (see OptimizationPolicyConfig.hh).
  float reference_period_{0.0f};
  // Endpoints a batch may target under -repair_tns.
  size_t max_end_repairs_{std::numeric_limits<size_t>::max()};
  // An unbuffer move in -sequence runs in every batch, not only in the tail's
  // single-move batches.
  bool unbuffer_in_sequence_{false};

  // Moves of the batch under evaluation.  The candidates and the swaps point
  // into the pool of iterate() and are cleared when it returns.
  std::vector<InstanceRecord> batch_instances_;
  std::vector<MoveCandidate*> batch_candidates_;
  // Cell swaps eligible for in-place salvage of the current batch.
  std::vector<SwapRecord> batch_swaps_;

  WorkMeter work_;
  StallMonitor stall_;
  WnsTail tail_;
  MissTabu tabu_;

  // RebufferMt targets planned in the current batch, against
  // rebuffer_max_targets.
  int rebuffer_targets_in_batch_{0};
};

}  // namespace rsz
