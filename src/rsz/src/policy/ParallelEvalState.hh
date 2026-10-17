// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "OptimizationPolicyConfig.hh"
#include "sta/Delay.hh"
#include "sta/NetworkClass.hh"

namespace rsz {

// Run state of SetupParallelEvalPolicy.  These classes hold counters and
// decisions only; the policy owns the journal, the STA and the generators.

// The work the run spent, in reference seconds (see "Work meter" in
// ParallelEvalPolicyConfig).  The stall rules read it instead of the wall
// clock, so every machine makes the same decisions.
class WorkMeter
{
 public:
  // Start reference-work accounting for this timing graph.
  void reset(size_t graph_vertices);
  // One parasitics-and-timing update or journal restore.
  void addTimingUpdate(size_t pending_nets);
  // Charge candidate evaluations to the deterministic work budget.
  void addEstimates(size_t estimates);
  // Charge one buffer-tree planning request to the work budget.
  void addPlanRequest();
  void addEndpoints(size_t endpoints) { endpoints_ += endpoints; }
  // Convert accumulated counts to reference seconds using policy weights.
  double seconds(const ParallelEvalPolicyConfig& config) const;

 private:
  double vertices_{0.0};
  uint64_t timing_updates_{0};
  uint64_t pending_nets_{0};
  uint64_t estimates_{0};
  uint64_t plan_requests_{0};
  uint64_t endpoints_{0};
};

// Stall detection of the batch stage.  A saved checkpoint is progress when
// it moves WNS or TNS by a visible part of its initial value.
class StallMonitor
{
 public:
  enum class Stall
  {
    kNone,
    kMoves,
    kBatches
  };

  explicit StallMonitor(const ParallelEvalPolicyConfig& config)
      : config_(config)
  {
  }

  // Set the initial QoR against which progress thresholds are measured.
  void reset(sta::Slack initial_wns, sta::Slack initial_tns);
  // A new batch: its cap and the failing endpoint count it started with.
  void beginBatch(int batch_cap, size_t fep)
  {
    last_cap_ = batch_cap;
    fep_ = fep;
  }
  void endBatch(int committed_moves) { last_batch_moves_ = committed_moves; }
  // A missed batch spent its moves without progress.  A later salvage of the
  // same batch resets the budget only if it progresses.
  void onMissed();
  // Update progress and the sliding stall window after a saved checkpoint.
  void onSaved(sta::Slack prev_tns,
               sta::Slack best_wns,
               sta::Slack wns,
               sta::Slack tns,
               double work);

  // The last saved batch did not move WNS past the best checkpoint.
  bool wnsStalled() const { return wns_stall_batches_ > 0; }
  // The cone weight ranks targets only after a saved batch left WNS
  // unchanged, and for a limited number of such batches.
  bool weightsPriority() const;
  bool windowStalled() const { return window_stalled_; }
  // The stall budget: too many moves or batches without progress.
  Stall stall() const;
  // Moves the batches may spend without progress: stall_batches full batches
  // at the current cap, so one grown batch cannot use it all.
  int moveBudget() const;

 private:
  // QoR and work at one saved checkpoint of the stall window.
  struct SavedCheckpoint
  {
    float wns{0.0f};
    size_t fep{0};
    double work{0.0};
  };

  // Test whether recent saved checkpoints made enough QoR progress.
  void updateWindow(sta::Slack wns, bool tns_progressed, double work);

  const ParallelEvalPolicyConfig& config_;
  sta::Slack initial_wns_{0.0f};
  sta::Slack initial_tns_{0.0f};
  // Batches without progress and the moves they spent.
  int stalled_batches_{0};
  int stalled_moves_{0};
  int last_batch_moves_{0};
  int last_cap_{0};
  // Saved batches in a row that did not move WNS.
  int wns_stall_batches_{0};
  // Failing endpoints at the last collection and the last saved checkpoints.
  size_t fep_{0};
  std::deque<SavedCheckpoint> window_;
  bool window_stalled_{false};
};

// The WNS tail: after the batches stall, the run works on the worst endpoints
// with small batches.  A stalled stage first retries with speculation, then
// widens; a work budget bounds the whole tail.
class WnsTail
{
 public:
  enum class Step
  {
    kContinue,
    kSpeculate,
    kWiden,
    kStop
  };

  explicit WnsTail(const ParallelEvalPolicyConfig& config) : config_(config) {}

  // Clear all tail and speculative state for a new policy run.
  void reset();
  // Starts the tail once; false when it is off or already ran.
  bool start(double work);
  void stop() { active_ = false; }
  bool active() const { return active_; }
  int endpoints() const { return endpoints_; }
  int batchCap() const { return batch_cap_; }
  int batches() const { return batches_; }
  double workStart() const { return work_start_; }
  double workBudget() const { return budget_work_; }
  // Buffer removal runs only in the single-move batches of a stalled greedy
  // stage, because a removal cannot be salvaged in place.
  bool unbufferAllowed() const
  {
    return active_ && batch_cap_ == 1 && greedy_stalled_;
  }

  // Bookkeeping after a batch: the stall counter and the next batch cap.
  void afterBatch(sta::Slack best_wns_before,
                  sta::Slack best_wns_after,
                  bool batch_saved);
  // The tail's decision after a batch; `reason` names the rule that fired.
  Step nextStep(int committed_moves, double work, const char*& reason) const;
  // Retry the stalled endpoints with speculation.
  void beginSpeculativeStage()
  {
    batches_without_gain_ = 0;
    greedy_stalled_ = true;
  }
  // Grow the endpoint set and the batch cap.
  // Increase the endpoint and batch-cap limits after a stalled stage.
  void widen();

  // Missed batches kept in the open journal since the last checkpoint.
  bool maySpeculate() const;
  void keepSpeculative(const std::vector<sta::Instance*>& instances);
  bool speculating() const { return speculative_batches_ > 0; }
  int speculativeBatches() const { return speculative_batches_; }
  // Ends the speculative run and returns the instances it committed.
  std::vector<sta::Instance*> endSpeculation();

 private:
  const ParallelEvalPolicyConfig& config_;
  bool active_{false};
  bool started_{false};
  // Batches so far and batches without a better WNS.
  int batches_{0};
  int batches_without_gain_{0};
  // Endpoints and batch caps of the stage; the cap doubles after a saved
  // batch and drops back to one move after a miss.
  int endpoints_{0};
  int max_batch_{0};
  int batch_cap_{0};
  // Work when the tail started and the work it may spend.
  double work_start_{0.0};
  double budget_work_{0.0};
  // Missed tail batches retained until a later checkpoint or rollback.
  int speculative_batches_{0};
  std::vector<sta::Instance*> speculative_instances_;
  bool greedy_stalled_{false};
};

// Instances the batches skip.  An undone batch puts its instances on the
// tabu for a few batches; an instance in too many undone batches is excluded
// for the rest of the run.
class MissTabu
{
 public:
  explicit MissTabu(const ParallelEvalPolicyConfig& config) : config_(config) {}

  // Clear the tabu and the exclusions for a new policy run.
  void reset();
  // Skip `inst` for miss_tabu_batches batches after batch `now`.
  void addTabu(sta::Instance* inst, int now);
  // addTabu() plus the undo count.  True once `inst` was in
  // max_undos_per_instance undone batches and is excluded for good.
  bool recordUndo(sta::Instance* inst, int now);
  // End the tabu of the instances whose tabu ends at batch `now`.
  void expire(int now);
  // Drop the tabu; the exclusions stay.
  void clearTemporary() { until_.clear(); }
  bool isExcluded(sta::Instance* inst) const
  {
    return !excluded_.empty() && excluded_.contains(inst);
  }
  bool isTabu(sta::Instance* inst) const
  {
    return !until_.empty() && until_.contains(inst);
  }

 private:
  const ParallelEvalPolicyConfig& config_;
  // Batch index until which each instance is temporarily skipped.
  std::unordered_map<sta::Instance*, int> until_;
  // Number of missed batches containing each instance.
  std::unordered_map<sta::Instance*, int> undo_counts_;
  // Instances excluded for the remainder of the policy run.
  std::unordered_set<sta::Instance*> excluded_;
};

}  // namespace rsz
