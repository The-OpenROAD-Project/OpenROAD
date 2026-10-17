// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

namespace rsz {

// Every tunable of the setup repair policies and their moves.  The default
// member initializer is the shipped value.

// Knobs shared by the policies and their moves.
struct OptimizationPolicyConfig
{
  // --- Delay estimator ------------------------------------------------------

  // Stages scored before and after the target stage; zero scores only the
  // target stage.
  int delay_estimation_levels{1};
  // Correct the table slew of the fanin driver stage with STA slews sampled
  // at three loads.  The accuracy report uses the same setting.
  bool sta_slew_bias{true};

  // Charge the slower fanin driver of a larger input pin to the other loads
  // of the fanin net.
  bool fanin_side_loss{false};
  float fanin_side_loss_weight{3.0f};
  // Weight of fanin loads off every violating worst path.
  float fanin_side_offpath_weight{0.0f};
  // A side path may fall below the worst slack by this fraction of the
  // candidate's own gain.
  float side_floor_margin{1.0f};
  // Also score the opposite output transition of a resized cell.
  bool dual_rf_eval{false};

  // --- VtSwapMt and SizeUpMt ------------------------------------------------

  // Drop replacements that do not drive at least as strong on rise and fall.
  bool dual_rf_drive_guard{true};

  // Weight of the SizeUpMt score in the shared pool; VtSwapMt keeps 1.
  float size_up_mt_score_scale{0.75f};

  // Largest drive-resistance ratio (current / replacement) of a size-up;
  // zero means no bound.
  float size_up_mt_max_drive_ratio{0.0f};

  // --- RebufferMt -----------------------------------------------------------

  // Weight of the RebufferMt score in the shared pool.
  float rebuffer_mt_score_scale{0.10f};

  // Minimum and maximum raw score of a buffer tree, in seconds; a maximum of
  // zero or less turns the upper gate off.
  float rebuffer_mt_min_score{5.0e-12f};
  float rebuffer_mt_max_score{45.0e-12f};

  // Buffer trees generated per target.
  int rebuffer_mt_max_candidates{3};

  // Let one-buffer trees compete in the score pool.
  bool allow_rebuffer_mt_single_buffer{false};
};

}  // namespace rsz
