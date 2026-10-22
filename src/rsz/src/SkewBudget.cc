// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "SkewBudget.hh"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#include "ortools/linear_solver/linear_solver.h"

namespace rsz {

using operations_research::MPConstraint;
using operations_research::MPObjective;
using operations_research::MPSolver;
using operations_research::MPVariable;

// All timing values come in seconds (1e-9 to 1e-12 range). GLOP's simplex
// is numerically unhappy at that scale and returns MPSOLVER_ABNORMAL on
// otherwise trivial LPs. We scale the whole LP into "picoseconds" (×1e12)
// so coefficients and RHS values land near unity, then unscale on output.
constexpr double kTimeScale = 1e12;

SkewLPSolver::SkewLPSolver(int num_registers, const Options& opts)
    : n_(num_registers), opts_(opts), pinned_(num_registers, false)
{
  assert(num_registers >= 0);
}

void SkewLPSolver::addSetupEdge(const SkewSetupEdge& edge)
{
  assert(edge.i >= 0 && edge.i < n_);
  assert(edge.j >= 0 && edge.j < n_);
  setup_edges_.push_back(edge);
}

void SkewLPSolver::addHoldEdge(const SkewHoldEdge& edge)
{
  assert(edge.i >= 0 && edge.i < n_);
  assert(edge.j >= 0 && edge.j < n_);
  hold_edges_.push_back(edge);
}

void SkewLPSolver::pinRegister(int i)
{
  assert(i >= 0 && i < n_);
  pinned_[i] = true;
}

namespace {

// Build the t / u / (ε) / (v_setup) variables and the abs / setup / hold
// rows on `solver` with all time-valued quantities scaled by kTimeScale.
//
// `include_eps`: phase 1 only — ε is a variable in setup rows.
// `eps_fixed_scaled`: phase 2/3 — ε is fixed and folded into setup RHS.
// `include_tns`: phase 2/3 — create one v_e ≥ 0 per setup edge measuring
//   that edge's violation past 0 (v_e ≥ max(0, −slack_new_e)). Used as
//   the TNS objective in phase 2 and as a pinned upper bound in phase 3.
struct LpHandles
{
  std::vector<MPVariable*> t;
  std::vector<MPVariable*> u;
  MPVariable* eps = nullptr;         // null if include_eps == false
  std::vector<MPVariable*> v_setup;  // empty if include_tns == false
};

LpHandles buildScaledLp(MPSolver& solver,
                        int n,
                        const std::vector<bool>& pinned,
                        const SkewLPSolver::Options& opts,
                        const std::vector<SkewSetupEdge>& setup_edges,
                        const std::vector<SkewHoldEdge>& hold_edges,
                        bool include_eps,
                        double eps_fixed_scaled,
                        bool include_tns)
{
  const double inf = MPSolver::infinity();
  const double max_skew_scaled = opts.max_skew * kTimeScale;
  const double target_wns_scaled = opts.target_wns * kTimeScale;
  const double hold_margin_scaled = opts.hold_margin * kTimeScale;

  LpHandles h;
  h.t.assign(n, nullptr);
  h.u.assign(n, nullptr);
  for (int i = 0; i < n; ++i) {
    const double t_lo = (pinned[i] || opts.one_sided) ? 0.0 : -max_skew_scaled;
    const double t_hi = pinned[i] ? 0.0 : max_skew_scaled;
    h.t[i] = solver.MakeNumVar(t_lo, t_hi, "t_" + std::to_string(i));
    h.u[i] = solver.MakeNumVar(0.0, max_skew_scaled, "u_" + std::to_string(i));
    MPConstraint* c_lo = solver.MakeRowConstraint(0.0, inf);
    c_lo->SetCoefficient(h.t[i], 1.0);
    c_lo->SetCoefficient(h.u[i], 1.0);
    MPConstraint* c_hi = solver.MakeRowConstraint(0.0, inf);
    c_hi->SetCoefficient(h.t[i], -1.0);
    c_hi->SetCoefficient(h.u[i], 1.0);
  }

  if (include_eps) {
    // ε lower bound: −1 second (in scaled units) is a soft floor far
    // below any realistic worst-case path slack on a clock-period-scale
    // design. A finite bound (rather than -infinity) keeps GLOP's
    // simplex numerically well-behaved at scale.
    h.eps = solver.MakeNumVar(-1.0 * kTimeScale, target_wns_scaled, "eps");
  }

  if (include_tns) {
    h.v_setup.assign(setup_edges.size(), nullptr);
  }

  for (size_t k = 0; k < setup_edges.size(); ++k) {
    const SkewSetupEdge& e = setup_edges[k];
    const double slack_scaled = e.slack * kTimeScale;
    if (e.frozen) {
      // Unhelpable edge (see SkewSetupEdge::frozen): don't worsen it,
      // don't let it drive ε or the TNS objective.
      MPConstraint* c = solver.MakeRowConstraint(-inf, 0.0);
      c->SetCoefficient(h.t[e.i], 1.0);
      c->SetCoefficient(h.t[e.j], -1.0);
      continue;
    }
    if (include_eps) {
      // t_i - t_j + ε ≤ slack.
      MPConstraint* c = solver.MakeRowConstraint(-inf, slack_scaled);
      c->SetCoefficient(h.t[e.i], 1.0);
      c->SetCoefficient(h.t[e.j], -1.0);
      c->SetCoefficient(h.eps, 1.0);
    } else {
      // t_i - t_j ≤ slack - ε_fixed.
      MPConstraint* c
          = solver.MakeRowConstraint(-inf, slack_scaled - eps_fixed_scaled);
      c->SetCoefficient(h.t[e.i], 1.0);
      c->SetCoefficient(h.t[e.j], -1.0);
    }
    if (include_tns) {
      // slack_new = slack + (t_j - t_i). v_e ≥ max(0, -slack_new):
      //   v_e ≥ 0   (variable bound)
      //   v_e + t_j - t_i ≥ -slack  ⇔  v_e ≥ t_i - t_j - slack
      // Per-edge upper bound: 2·max_skew accounts for the worst-case t
      // contribution, plus -slack to bound the row's structural minimum.
      // A finite (rather than infinite) bound keeps GLOP's simplex
      // numerically well-behaved at scale.
      const double v_ub = std::max(0.0, -slack_scaled) + 2.0 * max_skew_scaled;
      h.v_setup[k] = solver.MakeNumVar(0.0, v_ub, "v_" + std::to_string(k));
      MPConstraint* c = solver.MakeRowConstraint(-slack_scaled, inf);
      c->SetCoefficient(h.v_setup[k], 1.0);
      c->SetCoefficient(h.t[e.j], 1.0);
      c->SetCoefficient(h.t[e.i], -1.0);
    }
  }
  for (const SkewHoldEdge& e : hold_edges) {
    const double slack_scaled = e.slack * kTimeScale;
    // Preserve hold rather than try to *fix* it: enforce
    //   slack_new ≥ min(slack_initial, hold_margin)
    // ⇔ t_i - t_j ≥ min(hold_margin - slack, 0).
    // Always feasible at t = 0, so existing hold violations never render
    // the LP infeasible. hold_margin acts as a guardband on edges with at
    // least that much initial headroom.
    const double bound = std::min(hold_margin_scaled - slack_scaled, 0.0);
    MPConstraint* c = solver.MakeRowConstraint(bound, inf);
    c->SetCoefficient(h.t[e.i], 1.0);
    c->SetCoefficient(h.t[e.j], -1.0);
  }
  return h;
}

}  // namespace

SkewLPSolver::Result SkewLPSolver::solve()
{
  Result result;

  // Two lexicographic phases (WNS, then TNS+perturbation). A fresh GLOP
  // solver per phase: re-using a solved model with a new objective
  // causes ABNORMAL returns.

  // Phase 1: max ε. Reports the worst achievable per-edge slack.
  double eps_scaled = 0.0;
  {
    MPSolver solver("skew_budget_p1", MPSolver::GLOP_LINEAR_PROGRAMMING);
    LpHandles h = buildScaledLp(solver,
                                n_,
                                pinned_,
                                opts_,
                                setup_edges_,
                                hold_edges_,
                                /*include_eps=*/true,
                                /*eps_fixed_scaled=*/0.0,
                                /*include_tns=*/false);

    MPObjective* obj = solver.MutableObjective();
    obj->SetCoefficient(h.eps, 1.0);
    obj->SetMaximization();
    MPSolver::ResultStatus status = solver.Solve();

    if (status == MPSolver::INFEASIBLE) {
      result.status = Status::kHoldInfeasible;
      return result;
    }
    if (status != MPSolver::OPTIMAL && status != MPSolver::FEASIBLE) {
      result.status = Status::kSolverFailure;
      return result;
    }
    eps_scaled = h.eps->solution_value();
    result.wns = static_cast<float>(eps_scaled / kTimeScale);
  }

  result.status = (result.wns >= opts_.target_wns - opts_.wns_tolerance)
                      ? Status::kOptimal
                      : Status::kSetupInfeasible;

  const double eps_fixed_scaled = eps_scaled - opts_.wns_tolerance * kTimeScale;

  // Phase 2: ε pinned; minimize Σ v_e (setup TNS past 0) with a small
  // regularization on Σ u_i so that, among TNS-optimal schedules, the LP
  // prefers smaller perturbation. The combined objective is
  //
  //   min Σ v_e + (perturbation_weight / max(1, N)) · Σ u_i
  //
  // The per-register normalization keeps the regularizer weight stable
  // across designs; for n registers the worst-case Σ u_i is N · max_skew,
  // so the regularizer's total contribution is ~ perturbation_weight ·
  // max_skew at saturation — well below a single ps of TNS improvement at
  // the default weight (1e-3).
  //
  // Splitting this into separate TNS-min and perturbation-min phases was
  // tried and caused GLOP MPSOLVER_ABNORMAL on real designs due to the
  // tight Σ v_e ≤ tns_pin row over 100k+ variables. The combined objective
  // avoids that and halves the solve count.
  {
    MPSolver solver("skew_budget_p2", MPSolver::GLOP_LINEAR_PROGRAMMING);
    LpHandles h = buildScaledLp(solver,
                                n_,
                                pinned_,
                                opts_,
                                setup_edges_,
                                hold_edges_,
                                /*include_eps=*/false,
                                eps_fixed_scaled,
                                /*include_tns=*/true);

    MPObjective* obj = solver.MutableObjective();
    for (MPVariable* v : h.v_setup) {
      if (v != nullptr) {  // null for frozen edges
        obj->SetCoefficient(v, 1.0);
      }
    }
    // Per-register normalization: the regularizer's worst-case
    // contribution is (perturbation_weight / N) · (N · max_skew) =
    // perturbation_weight · max_skew, independent of design size. With
    // the default weight 1e-3 and max_skew ~200 ps, that ceiling is
    // ~0.2 ps — well below a single ps of TNS improvement. Without the
    // /N term the ceiling scales linearly with N and can swing the LP
    // by several ns of TNS on 10 k+ register designs.
    const double u_coeff
        = opts_.perturbation_weight / static_cast<double>(std::max(1, n_));
    for (int i = 0; i < n_; ++i) {
      obj->SetCoefficient(h.u[i], u_coeff);
    }
    obj->SetMinimization();
    MPSolver::ResultStatus status = solver.Solve();

    if (status != MPSolver::OPTIMAL && status != MPSolver::FEASIBLE) {
      result.status = Status::kSolverFailure;
      return result;
    }
    // Σ v_e: scaled TNS magnitude. Subtract out the regularizer's
    // contribution to recover Σ v_e ≈ -TNS in scaled units.
    double sum_v_scaled = 0.0;
    for (MPVariable* v : h.v_setup) {
      if (v != nullptr) {  // null for frozen edges
        sum_v_scaled += v->solution_value();
      }
    }
    result.tns = static_cast<float>(-sum_v_scaled / kTimeScale);

    result.t.resize(n_);
    for (int i = 0; i < n_; ++i) {
      result.t[i] = static_cast<float>(h.t[i]->solution_value() / kTimeScale);
    }
  }
  return result;
}

}  // namespace rsz
