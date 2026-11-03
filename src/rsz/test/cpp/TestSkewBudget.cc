// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <cmath>

#include "SkewBudget.hh"
#include "gtest/gtest.h"

namespace rsz {
namespace {

// Tolerance for LP solution comparisons (1 fs).
constexpr float kTol = 1e-15f;
// Relaxed tolerance for phase-2 ε comparisons (1 ps — wns_tolerance).
constexpr float kEpsTol = 5e-12f;

SkewLPSolver::Options DefaultOptions()
{
  SkewLPSolver::Options o;
  o.max_skew = 200e-12f;
  o.hold_margin = 0.0f;  // off by default in tests for cleaner math
  o.target_wns = 0.0f;
  o.wns_tolerance = 1e-12f;
  return o;
}

// 1. Empty design: zero registers, zero edges → ε pins at target_wns = 0,
// schedule is empty.
TEST(SkewLPSolver, EmptyDesign)
{
  SkewLPSolver solver(0, DefaultOptions());
  auto r = solver.solve();
  EXPECT_EQ(r.status, SkewLPSolver::Status::kOptimal);
  EXPECT_EQ(r.t.size(), 0u);
  EXPECT_NEAR(r.wns, 0.0f, kEpsTol);
}

// 2. Single flop pinned, no edges: t = {0}.
TEST(SkewLPSolver, SingleFlopPinned)
{
  SkewLPSolver solver(1, DefaultOptions());
  solver.pinRegister(0);
  auto r = solver.solve();
  EXPECT_EQ(r.status, SkewLPSolver::Status::kOptimal);
  ASSERT_EQ(r.t.size(), 1u);
  EXPECT_NEAR(r.t[0], 0.0f, kTol);
}

// 3. Two-flop chain with positive setup slack on the only path. Initial
// closure already met → with target_wns = 0 the minimum-perturbation answer
// is t = (0, 0).
TEST(SkewLPSolver, ChainSetupMetNoPerturbation)
{
  SkewLPSolver solver(2, DefaultOptions());
  solver.addSetupEdge({.i = 0, .j = 1, .slack = 1e-9f});
  solver.addHoldEdge({.i = 0, .j = 1, .slack = 500e-12f});
  auto r = solver.solve();
  ASSERT_EQ(r.status, SkewLPSolver::Status::kOptimal);
  ASSERT_EQ(r.t.size(), 2u);
  EXPECT_NEAR(r.t[0], 0.0f, kEpsTol);
  EXPECT_NEAR(r.t[1], 0.0f, kEpsTol);
  EXPECT_NEAR(r.wns, 0.0f, kEpsTol);  // ε* clamped at target_wns = 0
}

// 4. Setup-violated path that is closable by skew. Δ_max = 200 ps; slack =
// −100 ps. ε is capped at target_wns = 0, so closure requires
// t_1 − t_0 ≥ 100 ps. Minimum perturbation: Σ|t| = 100 ps.
TEST(SkewLPSolver, ChainSetupViolatedClosesAtMinimumPerturbation)
{
  SkewLPSolver solver(2, DefaultOptions());
  solver.addSetupEdge({.i = 0, .j = 1, .slack = -100e-12f});
  solver.addHoldEdge({.i = 0, .j = 1, .slack = 500e-12f});
  auto r = solver.solve();
  ASSERT_EQ(r.status, SkewLPSolver::Status::kOptimal);
  ASSERT_EQ(r.t.size(), 2u);
  EXPECT_GE(r.t[1] - r.t[0], 100e-12f - kEpsTol);
  EXPECT_NEAR(std::fabs(r.t[0]) + std::fabs(r.t[1]), 100e-12f, kEpsTol);
  EXPECT_NEAR(r.wns, 0.0f, kEpsTol);  // closed to target
}

// 5. Hold guardband applies only when the edge already has at least that
// much initial headroom (preserve semantics). Hold slack 200 ps, margin
// 50 ps → constraint is t_0 - t_1 ≥ −150 ps. A setup edge of slack 0
// allows any t_0 - t_1 ≤ 0, so the LP can shift by up to 150 ps without
// breaching the guardband. With max_skew of 200 ps it should set
// t_0 - t_1 = −150 ps exactly (LP picks the tightest allowed by hold and
// uses the corresponding setup-side improvement).
TEST(SkewLPSolver, HoldGuardbandBoundsShift)
{
  auto opts = DefaultOptions();
  opts.hold_margin = 50e-12f;
  opts.target_wns = 200e-12f;  // ask for positive ε so LP wants to shift
  SkewLPSolver solver(2, opts);
  solver.addSetupEdge({.i = 0, .j = 1, .slack = 0.0f});
  solver.addHoldEdge({.i = 0, .j = 1, .slack = 200e-12f});
  auto r = solver.solve();
  ASSERT_FALSE(r.t.empty());
  // Hold floor: t_0 - t_1 ≥ -150 ps. LP wants to maximize ε ⇒ minimize
  // (t_0 - t_1) until the hold floor binds, achieving ε = 150 ps. Phase 2
  // then keeps that and minimizes Σ|t|.
  EXPECT_NEAR(r.t[1] - r.t[0], 150e-12f, kEpsTol);
  EXPECT_NEAR(r.wns, 150e-12f, kEpsTol);
}

// 6. Hold-tight edge prevents setup closure: setup needs t_0 - t_1 ≤
// −150 ps but the hold edge with positive slack prevents any negative
// shift past −100 ps. ε* < 0 ⇒ kSetupInfeasible.
TEST(SkewLPSolver, HoldPreventsSetupClosure)
{
  auto opts = DefaultOptions();
  opts.max_skew = 200e-12f;
  opts.hold_margin = 0.0f;
  SkewLPSolver solver(2, opts);
  solver.addSetupEdge({.i = 0, .j = 1, .slack = -150e-12f});
  solver.addHoldEdge(
      {.i = 0, .j = 1, .slack = 100e-12f});  // floor: t_0 - t_1 ≥ -100ps
  auto r = solver.solve();
  EXPECT_EQ(r.status, SkewLPSolver::Status::kSetupInfeasible);
  ASSERT_EQ(r.t.size(), 2u);
  EXPECT_NEAR(r.t[0] - r.t[1], -100e-12f, kEpsTol);  // hold floor binds
  EXPECT_NEAR(r.wns, -50e-12f, kEpsTol);             // -150 - (-100) = -50
}

// 7. Preserve semantics: a hold edge with negative initial slack (existing
// hold violation) does not make the LP infeasible. The constraint becomes
// t_0 - t_1 ≥ 0 (don't worsen), so t = 0 is feasible. The pre-existing
// violation is left for repair_hold to handle, not useful skew.
TEST(SkewLPSolver, ExistingHoldViolationDoesNotBlockLp)
{
  auto opts = DefaultOptions();
  opts.hold_margin = 50e-12f;
  SkewLPSolver solver(2, opts);
  solver.addSetupEdge({.i = 0, .j = 1, .slack = 1e-9f});  // setup already met
  solver.addHoldEdge(
      {.i = 0, .j = 1, .slack = -200e-12f});  // existing hold violation
  auto r = solver.solve();
  ASSERT_EQ(r.status, SkewLPSolver::Status::kOptimal);
  ASSERT_EQ(r.t.size(), 2u);
  EXPECT_NEAR(r.t[0], 0.0f, kEpsTol);
  EXPECT_NEAR(r.t[1], 0.0f, kEpsTol);
}

// 7. Setup infeasible (even with skew): slack = -1 ns and Δ_max = 100 ps
// gives ε* = -1 ns + 200 ps = -800 ps < 0. We still emit a best-effort
// schedule (t_1 - t_0 = +200 ps to maximize closure).
TEST(SkewLPSolver, SetupInfeasibleBestEffortSchedule)
{
  auto opts = DefaultOptions();
  opts.max_skew = 100e-12f;
  SkewLPSolver solver(2, opts);
  solver.addSetupEdge({.i = 0, .j = 1, .slack = -1e-9f});
  solver.addHoldEdge({.i = 0, .j = 1, .slack = 1e-9f});
  auto r = solver.solve();
  EXPECT_EQ(r.status, SkewLPSolver::Status::kSetupInfeasible);
  ASSERT_EQ(r.t.size(), 2u);
  EXPECT_NEAR(r.t[1] - r.t[0], 200e-12f, kEpsTol);
  EXPECT_NEAR(r.wns, -800e-12f, kEpsTol);
}

// 8. Two competing setup paths: 0→1 wants t_1 > t_0; 1→0 (or 2→0) wants
// t_0 > t_2. Three flops in a chain 0→1→2 with both edges tight at
// slack=0. Closure at ε=0 requires t_0 ≤ t_1 ≤ t_2, achievable at
// t_0 = -Δ, t_1 = 0, t_2 = +Δ but only Δ = 0 minimizes perturbation
// when both edges are exactly met (no extra room demanded).
TEST(SkewLPSolver, ChainEvenSlackZeroPerturbation)
{
  SkewLPSolver solver(3, DefaultOptions());
  solver.addSetupEdge({.i = 0, .j = 1, .slack = 0.0f});
  solver.addSetupEdge({.i = 1, .j = 2, .slack = 0.0f});
  solver.addHoldEdge({.i = 0, .j = 1, .slack = 500e-12f});
  solver.addHoldEdge({.i = 1, .j = 2, .slack = 500e-12f});
  auto r = solver.solve();
  ASSERT_EQ(r.status, SkewLPSolver::Status::kOptimal);
  ASSERT_EQ(r.t.size(), 3u);
  EXPECT_NEAR(r.t[0], 0.0f, kEpsTol);
  EXPECT_NEAR(r.t[1], 0.0f, kEpsTol);
  EXPECT_NEAR(r.t[2], 0.0f, kEpsTol);
}

// 9. Asymmetric chain 0→1 (slack −50 ps) then 1→2 (slack +100 ps). The
// optimum is to shift t_1 backwards: t_1 = −50 ps balances the chain.
// t_0 = 0, t_2 = 0 (they're already fine pointing into / out of t_1).
// Σ|t| = 50 ps minimum perturbation.
TEST(SkewLPSolver, AsymmetricChainShiftsOnlyMiddleFlop)
{
  SkewLPSolver solver(3, DefaultOptions());
  solver.addSetupEdge({.i = 0, .j = 1, .slack = -50e-12f});
  solver.addSetupEdge({.i = 1, .j = 2, .slack = 100e-12f});
  solver.addHoldEdge({.i = 0, .j = 1, .slack = 500e-12f});
  solver.addHoldEdge({.i = 1, .j = 2, .slack = 500e-12f});
  auto r = solver.solve();
  ASSERT_EQ(r.status, SkewLPSolver::Status::kOptimal);
  ASSERT_EQ(r.t.size(), 3u);
  // Closure requires t_0 - t_1 ≤ -50 ps  ⇔  t_1 - t_0 ≥ 50 ps.
  EXPECT_GE(r.t[1] - r.t[0], 50e-12f - kEpsTol);
  // 1→2 has +100 ps headroom so no shift demanded.
  EXPECT_NEAR(std::fabs(r.t[0]) + std::fabs(r.t[1]) + std::fabs(r.t[2]),
              50e-12f,
              kEpsTol);
}

// 10. Pinned register cannot move: 0 is pinned, 0→1 has slack = -100 ps,
// so the only way to close is to advance t_1 by 100 ps (i.e., t_1 = +100 ps
// — wait, slack relation t_0 - t_1 + ε ≤ slack ⇒ with t_0 = 0, ε = 0:
// -t_1 ≤ -100 ps ⇒ t_1 ≥ 100 ps).
TEST(SkewLPSolver, PinnedLaunchForcesShiftOnCapture)
{
  SkewLPSolver solver(2, DefaultOptions());
  solver.pinRegister(0);
  solver.addSetupEdge({.i = 0, .j = 1, .slack = -100e-12f});
  solver.addHoldEdge({.i = 0, .j = 1, .slack = 500e-12f});
  auto r = solver.solve();
  ASSERT_EQ(r.status, SkewLPSolver::Status::kOptimal);
  ASSERT_EQ(r.t.size(), 2u);
  EXPECT_NEAR(r.t[0], 0.0f, kTol);
  EXPECT_NEAR(r.t[1], 100e-12f, kEpsTol);
}

// 11. Both endpoints pinned: closure infeasible if slack < 0 since the LP
// cannot move either side. Phase 1 will yield ε* = slack (negative).
TEST(SkewLPSolver, BothPinnedSetupInfeasible)
{
  SkewLPSolver solver(2, DefaultOptions());
  solver.pinRegister(0);
  solver.pinRegister(1);
  solver.addSetupEdge({.i = 0, .j = 1, .slack = -100e-12f});
  solver.addHoldEdge({.i = 0, .j = 1, .slack = 500e-12f});
  auto r = solver.solve();
  EXPECT_EQ(r.status, SkewLPSolver::Status::kSetupInfeasible);
  ASSERT_EQ(r.t.size(), 2u);
  EXPECT_NEAR(r.t[0], 0.0f, kTol);
  EXPECT_NEAR(r.t[1], 0.0f, kTol);
  EXPECT_NEAR(r.wns, -100e-12f, kEpsTol);
}

// 12. target_wns above what skew can deliver: with Δ_max = 50 ps and one
// setup edge of slack 0, the max-achievable ε* = 100 ps. Asking for
// target_wns = 200 ps reports kSetupInfeasible with ε* = 100 ps, and the
// schedule extracts the full available skew (Σ|t| = 100 ps).
TEST(SkewLPSolver, TargetAboveAchievable)
{
  auto opts = DefaultOptions();
  opts.max_skew = 50e-12f;
  opts.target_wns = 200e-12f;
  SkewLPSolver solver(2, opts);
  solver.addSetupEdge({.i = 0, .j = 1, .slack = 0.0f});
  solver.addHoldEdge({.i = 0, .j = 1, .slack = 500e-12f});
  auto r = solver.solve();
  EXPECT_EQ(r.status, SkewLPSolver::Status::kSetupInfeasible);
  ASSERT_EQ(r.t.size(), 2u);
  EXPECT_NEAR(r.wns, 100e-12f, kEpsTol);
  EXPECT_NEAR(r.t[1] - r.t[0], 100e-12f, kEpsTol);
}

// 13. Reproduces the near-closure regression that broke the greedy heuristic:
// many parallel paths with positive slack, a single tight-hold edge. The
// greedy code drove hold WNS to 0; the LP must respect the guardband.
TEST(SkewLPSolver, NearClosureDoesNotConsumeHoldMargin)
{
  auto opts = DefaultOptions();
  opts.hold_margin = 50e-12f;
  SkewLPSolver solver(10, opts);
  // Setup edges with comfortable margin between all adjacent pairs.
  for (int i = 0; i < 9; ++i) {
    solver.addSetupEdge({.i = i, .j = i + 1, .slack = 200e-12f});
    solver.addHoldEdge({.i = i, .j = i + 1, .slack = 500e-12f});
  }
  // One tight hold path: 0→9 with hold slack ≈ guardband (13 ps headroom).
  solver.addHoldEdge({.i = 0, .j = 9, .slack = 63e-12f});
  auto r = solver.solve();
  ASSERT_EQ(r.status, SkewLPSolver::Status::kOptimal);
  ASSERT_EQ(r.t.size(), 10u);
  // No edge should shift (everything is met by far more than the guardband).
  float total_perturb = 0.0f;
  for (float ti : r.t) {
    total_perturb += std::fabs(ti);
  }
  EXPECT_LT(total_perturb, kEpsTol)
      << "Solver should not perturb when initial state is already met.";
  // Hold constraint on 0→9 remains satisfied: t_0 - t_9 + 63 ps ≥ 50 ps.
  EXPECT_GE(r.t[0] - r.t[9] + 63e-12f, 50e-12f - kTol);
}

}  // namespace
}  // namespace rsz
