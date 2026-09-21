// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace sta {
class ArcDelayCalc;
class Edge;
class Pin;
class Scene;
}  // namespace sta

namespace rsz {

class Resizer;

// LRSubproblem: Evaluates the per-gate Lagrangian subproblem
//
//     minimize_{x ∈ S_i}
//       leakage(x)
//       + Σ_{e ∈ out(i)} λ_e · d_e(x)
//       + Σ_{p ∈ inputs(i)} Σ_{e ∈ arcs_to_drv(p)} λ_e · d_e(U, load_perturbed)
//
// where load_perturbed = load_U - C_in(current x_i, p) + C_in(candidate, p).
// The first sum prices the gate's own internal arcs; the second prices the
// upstream driver U's delay change caused by varying the candidate's input
// capacitance on pin p.
//
// === Threading model ========================================================
// The evaluation is split so a Jacobi sweep can run in parallel:
//   - snapshot(inst)         : MAIN THREAD ONLY. Reads live STA state (slews,
//                              load caps, cap checks), fills the lazy Liberty
//                              caches, and freezes everything needed to score a
//                              gate into a GateSnapshot.
//   - evaluateSnapshot(snap) : WORKER SAFE. Reads only `snap` + read-only
//                              Liberty data and the caller-provided per-thread
//                              ArcDelayCalc. No STA graph reads, no shared
//                              arc_delay_calc_, no cache writes, no mutation.
// Replacements chosen by evaluateSnapshot are applied later, serially, via
// applyReplacement.
class LRSubproblem : public sta::dbStaState
{
 public:
  // Per-input-pin upstream context, captured once per snapshot() call and
  // reused across every candidate cell. Each entry corresponds to one input
  // pin of the instance whose driver belongs to a real upstream standard cell.
  // Pins with no driver (PIs), driverless nets, or whose upstream sum-of-λ is
  // at the floor are filtered out at build time.
  struct UpstreamCtx
  {
    // Input port of the instance under its current cell. Used to look up
    // the same port (by name) on each candidate cell so we can read the
    // candidate's input capacitance for this pin.
    const sta::LibertyPort* orig_in_port = nullptr;
    // Output port of the upstream driver U at this pin's driver. Constant
    // across candidates - only the load it sees changes per candidate.
    sta::LibertyPort* drv_port = nullptr;
    // Current load capacitance at U's driver pin (farads). Includes the current
    // cell's contribution; we subtract C_in(current) and add C_in(candidate) to
    // get the perturbed load each candidate.
    float load_U_cur = 0.0f;
    // Input capacitance on this pin under the instance's CURRENT cell.
    float c_in_cur = 0.0f;
    // Σλ over U's gate-internal data arcs that terminate at the driver pin.
    // These are the arcs whose delay depends on the load U drives.
    float lambda_U_drv = 0.0f;
  };

  // Frozen per-output-pin electrical state for one instance.
  struct OutputCtx
  {
    // Output port under the instance's current cell. Candidate ports are looked
    // up by name on each candidate cell.
    const sta::LibertyPort* port = nullptr;
    float load_cap = 0.0f;    // graph_delay_calc_->loadCap (frozen)
    float lambda_sum = 0.0f;  // Σλ over gate-internal arcs into this pin
    // Elmore-slew DRC inputs (frozen). slew is the STA graph slew at this pin's
    // load vertex; drive_res is the current port's drive resistance.
    // The candidate's output slew is estimated as
    //   slew/(drive_res*load_cap) * cand_drive_res * load_cap.
    float slew = 0.0f;
    float drive_res = 0.0f;
  };

  // Snapshot of one driver pin's max-cap check on a fanin net.
  struct DriverCapCheck
  {
    float cap = 0.0f;        // current load cap at the driver pin
    float max_cap = 0.0f;    // cap limit
    float cap_slack = 0.0f;  // current cap slack
    bool corner_ok = false;  // max_cap > 0 && a corner was returned
  };

  // Per-input-pin context for the input-side max-cap DRC
  // (Resizer::replacementPreservesMaxCap, frozen).
  struct InputMaxCapCtx
  {
    const sta::LibertyPort* in_port = nullptr;  // current cell's input port
    float old_cap = 0.0f;  // input pin cap under the CURRENT cell
    std::vector<DriverCapCheck> drivers;
  };

  // One swappable candidate with its precomputed leakage-equivalent cost.
  struct Candidate
  {
    sta::LibertyCell* cell = nullptr;
    float leakage = 0.0f;  // leakageOrArea(cell), precomputed on main thread
  };

  // Everything evaluateSnapshot needs to score one instance, frozen on the
  // main thread.
  struct GateSnapshot
  {
    sta::Instance* inst = nullptr;
    sta::LibertyCell* cur_cell = nullptr;
    float cur_leakage = 0.0f;
    const sta::Scene* scene = nullptr;
    // Distributed downsize budget for this gate: the min over its output pins
    // of the depth-normalized slack budget  max(0, slack - margin) / depth,
    // frozen on the main thread (computed by the policy's computeSlackBudgets).
    // A downsize may add at most this much delay on any output pin (times a
    // safety factor). Because the per-path sum of these budgets is <= the path
    // slack, simultaneous (Jacobi) downsizes within budget cannot overshoot a
    // path.
    float budget = 0.0f;
    std::vector<OutputCtx> outputs;
    std::vector<UpstreamCtx> upstream;
    std::vector<InputMaxCapCtx> inputs;
    std::vector<Candidate> candidates;  // excludes cur_cell
  };

  // Result of one per-gate evaluation, applied later in serial.
  struct GateDecision
  {
    sta::Instance* inst = nullptr;
    sta::LibertyCell* best_cell = nullptr;  // nullptr -> keep current
    float best_cost = 0.0f;                 // leakage + Σλ·d at best_cell
    float baseline_cost = 0.0f;             // same for current cell
    // True iff best_cell has strictly lower leakage-equivalent cost than the
    // current cell. Used by the outer loop to apply asymmetric acceptance:
    // any cost drop is enough on a downsize, but timing-noise hysteresis
    // still applies to upsizes. False when best_cell == nullptr.
    bool best_is_downsize = false;
  };

  explicit LRSubproblem(Resizer* resizer);
  ~LRSubproblem() override = default;

  void init();

  // MAIN THREAD ONLY. Capture the frozen state needed to evaluate `inst`.
  // Returns false (and leaves `snap` unspecified) when `inst` is don't-touch,
  // has no liberty cell, or has no usable output pin. `lambda` is indexed by
  // sta::Edge::id (sparse, size `lambda_size`). `budget` is the per-vertex
  // depth-normalized downsize budget indexed by sta::Graph vertex id (size
  // `budget_size`); the gate's frozen budget is the min over its output pins.
  bool snapshot(sta::Instance* inst,
                const float* lambda,
                int lambda_size,
                const float* budget,
                int budget_size,
                bool include_clock_network,
                GateSnapshot& snap);

  // WORKER SAFE. Evaluate the subproblem for a prepared snapshot using the
  // caller-provided per-thread ArcDelayCalc. `timing_weight` scales the Σλ·d
  // timing term against the leakage objective. `budget_safety` (<= 1) scales
  // the gate's frozen downsize budget in the feasibility guard.
  GateDecision evaluateSnapshot(const GateSnapshot& snap,
                                float timing_weight,
                                float budget_safety,
                                sta::ArcDelayCalc* arc_delay_calc) const;

  // Leakage-equivalent cost for `cell`. Returns Resizer::cellLeakage when
  // the Liberty exposes leakage; otherwise returns area · area-to-leakage
  // scale (computed once at init() from the current design's distribution
  // of leakage and area on cells that DO have leakage). Mutates a lazy cache;
  // call only on the main thread.
  float leakageOrArea(sta::LibertyCell* cell) const;

  // Apply the LR-chosen replacement at `inst`. Wraps Resizer::replaceCell;
  // returns true on success. Called from GlobalSizingPolicy in serial inside
  // an open pass-level journal.
  bool applyReplacement(sta::Instance* inst, sta::LibertyCell* replacement);

 private:
  bool isDataArc(const sta::Edge* edge) const;
  // Walks leaf instances once to populate area_to_leakage_scale_ and
  // expose any pure-area-only-library degenerate case.
  void computeLeakageScale();

  // Worker-safe cost of running `cell` at the snapshotted instance.
  // `cell_leakage` is the precomputed leakage-equivalent cost of `cell`.
  float evaluateCellCost(const GateSnapshot& snap,
                         sta::LibertyCell* cell,
                         float cell_leakage,
                         float timing_weight,
                         sta::ArcDelayCalc* arc_delay_calc) const;

  // Read the max-rise/fall input capacitance of `port` on `cell` (farads).
  // Returns 0 if the port is missing on the cell. Worker-safe (Liberty read).
  float portInputCap(sta::LibertyCell* cell, const char* port_name) const;

  // Worker-safe DRC filter over a frozen snapshot. Returns true iff installing
  // `replacement` would not introduce any max-cap or max-slew violation -
  // either on the input side (fanin nets due to larger input pin caps) or on
  // each output pin (current load cap against the new cell's cap limit, and
  // estimated output slew against the new cell's drive resistance).
  bool candidateDrcOkSnapshot(const GateSnapshot& snap,
                              sta::LibertyCell* replacement) const;

  // Worker-safe downsize feasibility guard over a frozen snapshot. Returns true
  // iff installing the (lower-leakage) `replacement` adds, on every output pin,
  // no more delay than `safety * snap.budget`. snap.budget is the depth-
  // normalized, distributed slack budget frozen by the policy: because the
  // per-path sum of gate budgets is <= path slack, simultaneous downsizes
  // within budget cannot overshoot, so no per-gate discount is needed. A gate
  // with no budget (<= 0) cannot be downsized.
  bool downsizeFitsSlackBudget(const GateSnapshot& snap,
                               sta::LibertyCell* replacement,
                               float safety,
                               sta::ArcDelayCalc* arc_delay_calc) const;

  Resizer* resizer_ = nullptr;
  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;

  // Computed at init() from this design's (leakage, area) distribution on
  // instances whose current cell exposes Liberty leakage. Used by
  // leakageOrArea() to give area-only cells a leakage-equivalent cost.
  // Zero when no instance exposes leakage (degenerate area-only case).
  float area_to_leakage_scale_ = 0.0f;

  const sta::MinMax* max_ = sta::MinMax::max();
  bool initialized_ = false;
};

}  // namespace rsz
