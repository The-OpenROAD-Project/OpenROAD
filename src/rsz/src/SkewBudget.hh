// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <vector>

namespace rsz {

// A single setup-timing edge between register `i` (launch) and register `j`
// (capture). `slack` is the setup slack of the path at t = 0 (i.e., the
// initial setup slack from STA before any latency annotation).
// Constraint emitted: t_i - t_j + ε ≤ slack.
//
// `frozen` edges sit so far below the design's reported WNS that no
// schedule can help them (e.g. clock-gating checks, which report through
// a path group worstSlack doesn't cover). They get a pure don't-worsen
// row (t_i − t_j ≤ 0) and are excluded from both the phase-1 ε objective
// and the phase-2 TNS objective — otherwise one such edge drags ε
// hundreds of ps below the real WNS and every setup row goes vacuous.
struct SkewSetupEdge
{
  int i;
  int j;
  float slack;
  bool frozen = false;
};

// A single hold-timing edge between register `i` (launch) and register `j`
// (capture). `slack` is the hold slack at t = 0.
// Constraint emitted: t_i - t_j + slack ≥ hold_margin.
struct SkewHoldEdge
{
  int i;
  int j;
  float slack;
};

// Minimum-perturbation useful-skew LP solver. Solves
//   max ε                                    (phase 1)
//   s.t. t_i - t_j + ε ≤ slack_setup,ij                     ∀ setup edges
//        t_i - t_j     ≥ min(hold_margin - slack_hold,ij, 0) ∀ hold edges
//        |t_i| ≤ max_skew, t_i = 0 if pinned
// then with ε* := min(ε_phase1, target_wns):
//   min Σ |t_i|                              (phase 2)
//   s.t. ε ≥ ε* - wns_tolerance, original constraints
// and returns the resulting (ε*, t).
//
// The "min(·, 0)" on the hold lower bound implements preserve-hold
// semantics: post-skew hold slack ≥ min(initial slack, hold_margin). This
// guarantees the LP is feasible at t = 0 for any input (a design with
// pre-existing hold violations never makes the LP infeasible — useful skew
// just won't try to *fix* hold; that's repair_hold's job).
class SkewLPSolver
{
 public:
  struct Options
  {
    // CTS-achievable per-register skew, seconds.
    float max_skew = 200e-12f;
    // Delay-only schedules: t_i ∈ [0, max_skew] instead of [−max_skew,
    // +max_skew]. A built clock tree can only *add* delay to selected
    // sinks relative to the fastest sink, so a schedule meant for
    // physical realization must be one-sided — the base tree arrival is
    // the schedule's zero, which also keeps ground-pinned (I/O) edges
    // modeled consistently.
    bool one_sided = false;
    // Hold-side guardband, seconds. Subtracted from available hold margin.
    float hold_margin = 50e-12f;
    // Cap phase-2 ε from above. Default 0 = "close timing exactly, do not
    // burn headroom as skew." Set higher to push WNS positive at the cost
    // of more perturbation.
    float target_wns = 0.0f;
    // Phase-2 floor on ε relative to ε*: ε ≥ ε* - wns_tolerance.
    float wns_tolerance = 1e-12f;
    // Phase-2 objective is min Σ v_e + perturbation_weight · Σ u_i, where
    // v_e = max(0, −slack_new_e) (TNS) and u_i = |t_i|. The weight controls
    // the TNS-vs-perturbation trade: 0 means TNS-only (the LP will use as
    // much skew as needed); 1 means each ps of TNS gain costs 1 ps of
    // total perturbation. Default 1e-3 prioritizes TNS while still
    // pruning unneeded shifts.
    float perturbation_weight = 1e-3f;
  };

  enum class Status
  {
    kOptimal,          // ε* ≥ 0, hold feasible; budget closes timing.
    kSetupInfeasible,  // Hold feasible, but ε* < 0; best-effort schedule.
    kHoldInfeasible,   // No t satisfies all hold constraints.
    kSolverFailure,    // Solver returned non-OPTIMAL/FEASIBLE unexpectedly.
  };

  struct Result
  {
    Status status = Status::kSolverFailure;
    float wns = 0.0f;      // ε* (achieved WNS slack).
    float tns = 0.0f;      // − Σ max(0, −slack_new) (achieved setup TNS).
    std::vector<float> t;  // Per-register clock-arrival schedule.
    // t.size() == 0 if the LP failed before any schedule could be obtained.
  };

  SkewLPSolver(int num_registers, const Options& opts);

  void addSetupEdge(const SkewSetupEdge& edge);
  void addHoldEdge(const SkewHoldEdge& edge);
  // Forces t_i = 0 (e.g., for I/O-anchored registers).
  void pinRegister(int i);

  // For tests / diagnostics.
  int numRegisters() const { return n_; }
  const Options& options() const { return opts_; }

  Result solve();

 private:
  int n_;
  Options opts_;
  std::vector<SkewSetupEdge> setup_edges_;
  std::vector<SkewHoldEdge> hold_edges_;
  std::vector<bool> pinned_;
};

}  // namespace rsz
