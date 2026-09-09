// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

namespace rsz {

// Tunables for the Lagrangian-Relaxation global sizing driver. Configured via
// the `set_global_sizing_config` Tcl command (read out of dbProperties by
// Resizer::initBlock()) and consumed by GlobalSizingPolicy.
struct GlobalSizingConfig
{
  // Optional pre-LR initialization: Replace every editable instance with
  // the smallest (kMinSizeMaxVt) or largest (kMaxSizeMinVt) leakage equivalent
  // cell before LR runs.
  enum class PresizeMode
  {
    kDisabled = 0,
    kMinSizeMaxVt = 1,
    kMaxSizeMinVt = 2,
  };
  PresizeMode presize_mode = PresizeMode::kDisabled;
  // Optional clock network sizing: Global sizing excludes clock network
  // instances by default. Can be enabled for post-CTS timing repair for better
  // clock performance.
  bool include_clock_network = false;
  float setup_slack_margin = 0.0f;
  int max_iterations = 20;
  // Step size α for the dual-subgradient update on λ.
  //   λ_e ← max(floor, λ_e · (1 + α · g_e_norm))
  // with g_e_norm ∈ [-1, 0]. Tight arcs (g=0) are unchanged; arcs at full
  // slack (g=-1) shrink to (1-α)·λ. Halved on pass rejection.
  float beta = 0.6f;
  // Endpoint seed exponent: mu_k ~ max(0, margin - slack_k)^p.
  float mu_exponent = 2.0f;
  // Floor for multipliers (subgradient floor so unused arcs can re-enter).
  float lambda_floor = 1e-12f;
  // Dimensionless balance between timing pressure and leakage cost.
  // bias = 1.0 keeps Σλ·d (scaled) ≈ leakage cost on the median gate.
  float timing_bias = 64.0f;
  // Safety derate (<= 1) on the per-gate distributed downsize budget. The
  // depth-normalized distribution already guarantees per-path budget sums
  // <= path slack, so 1.0 is feasible in theory; a value < 1 adds margin for
  // the un-modeled slew cascade / estimated-vs-routed parasitic gap.
  float budget_safety_factor = 1.0f;
};

}  // namespace rsz
