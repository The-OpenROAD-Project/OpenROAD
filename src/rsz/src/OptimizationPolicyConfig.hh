// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "rsz/Resizer.hh"

namespace rsz {

// The tunables of the setup repair policies and their moves; estimator and
// planner calibration constants stay in their files.  The default member
// initializer is the shipped value; forParallelEval() sets the PARALLEL_EVAL
// values.  Times are seconds and "pct" fields are percent.  "period_pct"
// fields are percent of the reference clock period of a PARALLEL_EVAL run:
// the shortest period of a clock with source pins in any mode, fixed for the
// run.  A share of |WNS| would vanish as WNS approaches zero.

// Controls how stalled batches favor targets in shared violating cones.
// The rank is score * (1 + bottleneck_scale *
// severity_ratio^bottleneck_exponent).
struct BatchPriorityConfig
{
  // Strength of the shared-bottleneck priority bonus.
  float bottleneck_scale{2.0f};
  // Emphasis on the most shared bottlenecks.
  float bottleneck_exponent{2.0f};
  // The bonus applies only after a saved batch left WNS unchanged, and for at
  // most this many such batches in a row.  Undone batches do not count.
  int bottleneck_max_wns_stall_batches{8};
  // Maximum endpoints weighed, worst slack first; each costs one bit per
  // cone vertex.
  int bottleneck_max_endpoints{1024};
  // Maximum distinct cone startpoints analyzed, with the same per-vertex cost.
  int bottleneck_max_startpoints{1024};
  // Batches one analysis serves; the start of the WNS tail refreshes it.
  int bottleneck_refresh_batches{4};
};

// Sets the score floor and worst-endpoint reserve of batch admission.
struct BatchSelectorConfig
{
  // The score floor applies once the batch holds this many moves.
  int score_floor_start{16};
  // A candidate joins only when its score is at least this fraction of the
  // best score.
  float min_score_ratio{0.0f};
  // Percent of the batch kept for the worst-path targets, which the score
  // order alone would not reach.
  int wns_reserve_pct{0};
  // Endpoints at or above this slack (the -setup_slack_margin of the run)
  // take no reserved slot.
  float slack_margin{0.0f};
};

// Controls batch evaluation, checkpoints, and stopping in PARALLEL_EVAL.
struct ParallelEvalPolicyConfig
{
  // --- Batch size ---------------------------------------------------------

  // Smallest batch cap; also the floor of the stall move budget.
  int min_batch_cap{64};

  // The cap is the failing endpoint count, clamped to the two caps.
  int max_batch_cap{512};

  // Batch cap of the one-time retry of a run whose batches kept no move.
  int retry_batch_cap{8};

  // --- Evaluation funnel ----------------------------------------------------

  // Endpoints evaluated per chunk, worst slack first: eval_endpoint_factor
  // times the batch cap, at least eval_min_endpoints.  The next chunk runs
  // only while the batch is not full (see evaluateTargets()).
  int eval_endpoint_factor{2};
  // Minimum endpoint count evaluated in each chunk.
  int eval_min_endpoints{256};

  // Targets per batch that may plan buffer trees, worst slack first.
  int rebuffer_max_targets{128};

  // --- Batch selection ------------------------------------------------------

  // Ranking across targets in stalled batches.  Legality and each target's
  // winner always use the local score.
  BatchPriorityConfig priority;

  // Batch admission.  The small-batch retry drops the floor and the reserve.
  BatchSelectorConfig selector{.min_score_ratio = 0.05f, .wns_reserve_pct = 10};

  // --- Checkpoint, salvage and undo -----------------------------------------

  // Consecutive batches that may miss the checkpoint before the batch stage
  // gives up.  As after every rule that ends the batches, a run that has kept
  // no move retries once with small batches; otherwise the WNS tail starts,
  // or the run ends when there is none.
  int max_checkpoint_misses{8};

  // A stage counts as slowed by the batch only when its delay grows by more
  // than this fraction of its total; slew changes elsewhere in the batch move
  // untouched arcs by a hair.
  float culprit_delay_ratio{0.01f};
  // A batch that improves TNS is also saved while its WNS stays within this
  // percent of the reference clock period below the best checkpoint WNS.
  float checkpoint_wns_tolerance_period_pct{0.02f};
  // Batches that skip the instances of an undone batch.
  int miss_tabu_batches{8};

  // Undone batches an instance may be in before it is blocked for the run.
  int max_undos_per_instance{3};

  // --- Stall rules ----------------------------------------------------------

  // The batches stop after stall_max_batches batches without progress, or
  // once those batches spent stall_batches times the current cap in moves.
  // Progress is a saved checkpoint that improves WNS by
  // progress_wns_period_pct percent of the reference clock period or TNS by
  // progress_tns_pct percent of its initial value.
  int stall_batches{6};
  // Absolute cap on consecutive stalled batches.
  int stall_max_batches{24};
  // Minimum TNS improvement relative to the initial magnitude.
  float progress_tns_pct{0.5f};
  // Minimum WNS improvement in percent of the reference clock period.
  float progress_wns_period_pct{0.04f};

  // The batches also stop when the last stall_window saved batches, after
  // spending stall_window_min_work, moved neither WNS by stall_wns_period_pct
  // nor the failing endpoint count by stall_failing_endpoint_pct percent per
  // batch and TNS did not progress.
  int stall_window{6};
  // Average WNS gain per saved batch, in percent of the reference clock
  // period, that keeps the window moving.  The rule also needs a flat TNS
  // and failing endpoint count, so it ends only batch stages that barely move
  // WNS.
  float stall_wns_period_pct{0.2f};
  // Average failing endpoint drop per saved batch, in percent of the current
  // count, that keeps the window moving.
  float stall_failing_endpoint_pct{1.0f};
  // Minimum reference work before the window stall check applies.
  float stall_window_min_work{30.0f};

  // --- WNS tail -------------------------------------------------------------

  // After the batches stall, the run works on the worst wns_tail_endpoints
  // endpoints with small batches.  Zero turns the tail off.
  int wns_tail_endpoints{8};
  // Tail batch cap.  A tail batch starts at one move, doubles after each
  // saved batch and drops back to one after a miss.
  int wns_tail_batch{8};
  // A tail stage ends after wns_tail_stall batches without a better WNS; the
  // tail ends after wns_tail_max_batches batches.
  int wns_tail_stall{6};
  int wns_tail_max_batches{300};
  // A stalled tail grows its endpoints and batch cap by wns_tail_grow, up to
  // wns_tail_max_endpoints endpoints.
  int wns_tail_grow{2};
  int wns_tail_max_endpoints{64};
  // A tail batch may cost at most this multiple of its WNS gain in TNS.
  float wns_tail_tns_ratio{10.0f};
  // A tail batch is WNS progress only when it raises the best WNS by this
  // percent of the reference clock period.
  float wns_tail_min_gain_period_pct{0.1f};
  // Tail work budget: this percent of the batch stage's work, at least
  // wns_tail_min_work.
  float wns_tail_work_pct{400.0f};
  // Absolute floor on the tail work budget, in reference seconds.
  float wns_tail_min_work{60.0f};
  // Missed tail batches that may stay in the open journal.  A later save
  // keeps them all; otherwise they all roll back.
  int wns_tail_speculate{6};

  // --- Work meter -----------------------------------------------------------

  // The stall window and the tail budget count work, not wall-clock time,
  // so a run makes the same decisions on every machine.  Work is in
  // reference seconds:
  //   work_sta_per_vertex * graph_vertices * timing_updates
  //   + work_per_pending_net * nets_with_invalid_parasitics
  //   + work_per_estimate * candidate_estimates
  //   + work_per_plan * buffer_tree_plan_requests
  //   + work_per_endpoint * endpoints_evaluated
  // A timing update is each parasitics-and-timing update the policy asks
  // for, and each batch undo, checkpoint restore or salvage rollback.  The
  // pending nets are read before the update or the rollback, so a rollback
  // after the batch STA adds none.  The weights are a non-negative
  // least-squares fit of profiled batch wall times on the reference machine.
  float work_sta_per_vertex{5.0e-7f};
  float work_per_pending_net{1.93e-3f};
  float work_per_estimate{1.35e-5f};
  float work_per_plan{1.14e-3f};
  float work_per_endpoint{5.65e-4f};
};

// Knobs of the Mt moves and the policies that run them.  forParallelEval()
// returns the PARALLEL_EVAL values on top of the defaults.
struct OptimizationPolicyConfig
{
  static OptimizationPolicyConfig forParallelEval(RepairEffort effort);

  // --- Delay estimator ------------------------------------------------------

  // Stages scored before and after the target stage; zero scores only the
  // target stage.
  int delay_estimation_levels{1};
  // Correct the table slew of the fanin driver stage with STA slews sampled
  // at three loads.  The accuracy report uses the same setting.
  bool sta_slew_bias{true};

  // Charge the slower fanin driver of a larger input pin to the other loads
  // of the fanin net.  PARALLEL_EVAL turns it on.
  bool fanin_side_loss{false};
  // The side loss counts this many times against the gain.  Empirically
  // tuned: a smaller weight lets through moves that degrade the side paths,
  // which costs QoR overall.
  float fanin_side_loss_weight{3.0f};
  // Weight of fanin loads off every violating worst path.
  float fanin_side_offpath_weight{0.0f};
  // A side path may fall below the worst slack by this fraction of the
  // candidate's own gain.
  float side_floor_margin{1.0f};
  // Also score the opposite output transition of a resized cell.
  // PARALLEL_EVAL turns it on.
  bool dual_rf_eval{false};

  // --- VtSwapMt and SizeUpMt ------------------------------------------------

  // Drop replacements that do not drive at least as strong on rise and fall.
  // PARALLEL_EVAL turns the guard off.
  bool dual_rf_drive_guard{true};

  // Weight of the SizeUpMt score in the shared pool; VtSwapMt keeps 1.
  // Empirically tuned.  A size-up costs more than a VT swap of similar gain
  // (area, input pin cap, legalizer displacement, side paths), so it ranks
  // lower.
  float size_up_mt_score_scale{0.75f};

  // Largest drive-resistance ratio (current / replacement) of a size-up;
  // zero means no bound.  PARALLEL_EVAL sets one.
  float size_up_mt_max_drive_ratio{0.0f};

  // --- RebufferMt -----------------------------------------------------------

  // Weight of the RebufferMt score in the shared pool.
  float rebuffer_mt_score_scale{0.10f};

  // Minimum and maximum raw score of a buffer tree, in seconds; a maximum of
  // zero or less turns the upper gate off, as PARALLEL_EVAL does.
  float rebuffer_mt_min_score{5.0e-12f};
  float rebuffer_mt_max_score{45.0e-12f};

  // Buffer trees generated per target.
  int rebuffer_mt_max_candidates{3};

  // Let one-buffer trees compete in the score pool.  PARALLEL_EVAL turns it on.
  bool allow_rebuffer_mt_single_buffer{false};

  // Charge the buffer-tree side loads: the loads a new buffer moves behind it
  // lose slack.  PARALLEL_EVAL turns it on.
  bool rebuffer_mt_side_loads{false};

  // --- PARALLEL_EVAL --------------------------------------------------------

  ParallelEvalPolicyConfig parallel_eval;

  // --- SplitLoadMt and SizeDownFanoutMt -------------------------------------

  // Weights of their scores in the shared pool, below SizeUpMt because their
  // model ignores the wire change.  SwapPinsMt keeps 1.
  float split_load_mt_score_scale{0.50f};
  float size_down_fanout_mt_score_scale{0.75f};
};

}  // namespace rsz
