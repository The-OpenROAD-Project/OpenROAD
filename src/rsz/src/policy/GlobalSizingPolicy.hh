// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "LRSubproblem.hh"
#include "MoveCommitter.hh"
#include "OptimizationPolicy.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupContext.hh"
#include "rsz/Resizer.hh"
#include "sta/GraphClass.hh"
#include "sta/MinMax.hh"

namespace sta {
class Edge;
class LibertyCell;
class Vertex;
class dbNetwork;
}  // namespace sta

namespace rsz {

// Tunables for the Lagrangian-Relaxation global sizing driver. Internal to the
// policy - not user-facing through Tcl. Live here as a struct so each piece has
// a stable name and so we can plug in env-var overrides later without rewriting
// the policy.
struct LRParams
{
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

// GlobalSizingPolicy: Lagrangian-Relaxation-driven global sizing + Vt
// assignment, packaged as an OptimizationPolicy phase.
//
// Outer loop (in iterate()): allocate λ/μ → seed → project → repeat
// {update → project → Jacobi sweep over leaf instances → pass-level
// accept/reject by WNS regression}.
// Each gate's replacement decision uses LRSubproblem's per-gate cost. Skips the
// OptimizationPolicy generator/candidate pipeline and the target_collector - LR
// is not target-driven.
class GlobalSizingPolicy : public OptimizationPolicy
{
 public:
  GlobalSizingPolicy(Resizer& resizer,
                     MoveCommitter& committer,
                     RepairSetupContext& setup_context,
                     const OptimizerRunConfig& config);
  ~GlobalSizingPolicy() override;

  const char* name() const override { return "GlobalSizingPolicy"; }
  bool start() override;
  void iterate() override;

 private:
  enum class PresizeMode
  {
    kDisabled = 0,
    kMinSizeMaxVt = 1,
    kMaxSizeMinVt = 2,
  };
  using PresizeCellCache
      = std::unordered_map<sta::LibertyCell*, sta::LibertyCell*>;

  // === Setup ================================================================
  // Parse the optional env-var override for the LR starting point.
  PresizeMode readPresizeMode() const;
  // Pick the deterministic presize target within the swappable-equivalent set.
  sta::LibertyCell* selectPresizeCell(
      sta::LibertyCell* current_cell,
      PresizeMode mode,
      PresizeCellCache& presize_cell_cache) const;
  // Apply the selected presize to the live design before LR state is seeded.
  int applyPresize(PresizeMode mode);
  // Discover graph size (edges, endpoints), set dcalc_ap_, size vectors.
  void allocate();
  // Delay-proportional λ seed + WNS-biased μ seed.
  void seedMultipliers(const LRParams& params);
  // Multiplicative λ update via dual-subgradient + re-seed of μ from the
  // current slack picture. Called at the start of each outer iteration
  // after iteration 0.
  void updateMultipliers(const LRParams& params);
  // Reverse-topological projection onto the KKT flow-balance polytope.
  // After projection:
  //   Σλ_in(v) = Σλ_out(v) for internal v
  //   Σλ_in(k) = μ_k for each endpoint k
  void projectFlowBalance(const LRParams& params);
  // Tally of one Jacobi sweep. `moves` is the total cell replacements applied
  // to the journal this sweep (tentative - the pass-acceptance test in
  // iterate() may still roll the whole sweep back).
  struct SweepStats
  {
    int moves = 0;
    int upsizes = 0;
    int downsizes = 0;
  };

  // One Jacobi sweep over all leaf instances, in three phases:
  //   A buildSnapshots()  - main thread: freeze each gate's timing/DRC state
  //   B parallel evaluate - workers: score every snapshot independently
  //   C applyDecisions()  - main thread: apply the winning replacements
  // The per-sweep timing update is done by the caller (iterate()), once,
  // after this returns.
  SweepStats singleSweep(float timing_weight);

  // Phase A pre-pass: Compute the per-vertex depth-normalized downsize budget
  //   budget(v) = max(0, slack(v) - margin) / depth(v)
  // where depth(v) is the gate count on the longest path through v.
  // Distributing by depth guarantees the per-path sum of budgets <= path slack,
  // while using each vertex's own (worst-path) slack keeps every gate within
  // all its paths.
  void computeSlackBudgets();

  // Phase A: Capture the frozen per-gate snapshots for every evaluable leaf
  // instance, in a stable order. Reads live STA and warms the lazy
  // Liberty/dbNetwork caches on the main thread.
  std::vector<LRSubproblem::GateSnapshot> buildSnapshots();

  // Phase C: Apply the accepted replacements in vector order. No timing query
  // may run here - the single batched update happens in iterate() afterwards.
  SweepStats applyDecisions(
      const std::vector<LRSubproblem::GateDecision>& decisions,
      int visited);

  // Auto-scale timing weight so the output-cone timing term is comparable to
  // the leakage term on the median gate of this design. Anchored to the
  // output-cone term only (not the upstream-Cin term).
  float computeAutoTimingWeight(const LRParams& params) const;

  // === Diagnostics ==========================================================
  struct DesignSnap
  {
    double total_leakage = 0.0;
    double total_area = 0.0;
    int instances = 0;
    int with_leakage = 0;
  };
  DesignSnap computeDesignSnap() const;

  // === Graph helpers ========================================================
  bool isDataArc(const sta::Edge* edge) const;
  float edgeMaxArcDelay(sta::Edge* edge) const;

  // === Policy state =========================================================
  LRParams lr_params_;
  sta::dbNetwork* db_network_ = nullptr;

  // Per-edge multipliers, indexed by sta::Edge::id (sparse)
  std::vector<float> lambda_;
  // Per-vertex depth-normalized downsize budget, indexed by sta::Graph vertex
  // id. Rebuilt each sweep by computeSlackBudgets().
  std::vector<float> vertex_budget_;
  // Per-endpoint multipliers, indexed by a dense endpoint index
  std::vector<float> mu_;
  // Dense endpoint bookkeeping
  std::vector<sta::Vertex*> endpoint_vertices_;
  std::unordered_map<const sta::Vertex*, int> endpoint_index_;

  sta::DcalcAPIndex dcalc_ap_ = 0;
  std::unique_ptr<LRSubproblem> subproblem_;  // Per-gate cost evaluator
  const sta::MinMax* policy_max_ = sta::MinMax::max();
};

}  // namespace rsz
