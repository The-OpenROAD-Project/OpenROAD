// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "OptimizationPolicyConfig.hh"

namespace rsz {

OptimizationPolicyConfig OptimizationPolicyConfig::forParallelEval(
    const RepairEffort effort)
{
  OptimizationPolicyConfig config;

  // A batch commits on estimates alone, so the estimates also charge the side
  // effects: fanin loads, buffer-tree side loads and the opposite transition.
  config.fanin_side_loss = true;
  config.rebuffer_mt_side_loads = true;
  config.dual_rf_eval = true;

  // The estimate is less accurate for big size-up jumps, so the jump is
  // bounded.  The rise/fall guard is off; it leaves buffer chains with no
  // candidate.
  config.size_up_mt_max_drive_ratio = 2.5f;
  config.dual_rf_drive_guard = false;

  // RebufferMt is the only buffering move here: one-buffer trees join the
  // pool and the upper score gate is off.
  config.allow_rebuffer_mt_single_buffer = true;
  config.rebuffer_mt_max_score = 0.0f;
  config.rebuffer_mt_score_scale = 0.40f;

  ParallelEvalPolicyConfig& parallel_eval = config.parallel_eval;
  // -effort trades runtime for the last gains after the batches converge.
  switch (effort) {
    case RepairEffort::kLow:
      // No WNS tail: the batch-stage stall ends the run.
      parallel_eval.wns_tail_endpoints = 0;
      break;
    case RepairEffort::kMedium:
      // Both stall rules fire sooner and the tail widens less.
      parallel_eval.stall_max_batches = 12;
      parallel_eval.progress_tns_pct = 1.0f;
      parallel_eval.progress_wns_period_pct = 0.08f;
      parallel_eval.wns_tail_max_endpoints = 16;
      parallel_eval.wns_tail_work_pct = 100.0f;
      parallel_eval.wns_tail_min_work = 15.0f;
      break;
    case RepairEffort::kHigh:
      break;
  }

  return config;
}

}  // namespace rsz
