# Spec Constitution — MBFF Clock-Tree Power Credit

**Feature:** Promote higher MBFF banking ratios by crediting clock-tree power
savings in the `cluster_flops` ILP cost model.

**Status:** IMPLEMENTED — built clean, tests pass, not yet committed.
**Module:** `src/gpl` (`mbff.cpp`, `mbff.h`, `replace.*`)
**Branch:** `mbff_improve_banking`

---

## 0. Work Summary (as built)

### UI change (the experiment knob)

New optional flag on `cluster_flops`:

```tcl
cluster_flops -clock_power_weight <k>    # float, k >= 0, default 0.0
```

- `k = 0` (default): legacy cost model, behavior bit-identical to before.
- `k > 0`: credits per-sink clock-tree power `P_ct = k * single_bit_power`,
  making large trays relatively cheaper → higher banking.
- Negative `k`: rejected with `[ERROR GPL-0115]`.

### How to run the experiment

```tcl
# baseline (no clock-tree credit)
cluster_flops -tray_weight 8.0 -timing_weight 0.0 -max_split_size -1 \
  -clock_power_weight 0.0
# crank the knob to promote banking
cluster_flops -tray_weight 8.0 -timing_weight 0.0 -max_split_size -1 \
  -clock_power_weight 3.0
```

Read the `Sizes used` histogram printed at the end of each run. Customer recipe:
sweep `k` 1.0 → 3.0 until the banking target is met.

### Observed results (test design `mbff_hier.v`, 8 flops, `-tray_weight 8`)

| `-clock_power_weight` | Sizes used | Banking |
|-----------------------|-----------|---------|
| 0.0 (default)         | `2-bit: 4` | 2X     |
| 3.0                   | `4-bit: 2` | 4X     |

Same design and tray weight; the knob alone moves the solution from 2X to 4X.

### Files changed

Code: `replace.tcl`, `replace.i`, `include/gpl/Replace.h`, `replace.cpp`,
`mbff.h`, `mbff.cpp`, `README.md`. Build: `test/CMakeLists.txt`, `test/BUILD`.
Tests (new): `mbff_clock_power_off`, `mbff_clock_power_on`,
`mbff_clock_power_err` (`.tcl` + `.ok`).

### Verification

- Existing `mbff_hier` / `mbff_orig_name` goldens unchanged at default `k=0`
  (ORFS unaffected — G2 gate met).
- 3 new tests pass; clang-format clean; openroad builds.

---

## 1. Problem Statement

A customer reports `cluster_flops` cannot exceed ~2.5X banking ratio, while
commercial tools achieve >4X. Higher banking is wanted because it sharply
reduces CTS routing load and yields more competitive/accurate post-route power.

### Root cause

The MBFF clustering ILP (`MBFF::RunILP`, `mbff.cpp:880-900`) minimizes:

```
min  Σ (flop→slot displacement)
   + beta · Σ (timing-path displacement)
   + alpha · Σ_used_trays  tray_cost[t]
```

with (`mbff.cpp:856-873`, fed by `norm_power_` set in `SetRatios`):

```
tray_cost[N-bit] = tray_total_power / single_bit_power     // = N · norm_power_[N]
tray_cost[1-bit] = 1.0
tray_total_power = leakage + internal_energy · activity     // cell power only
```

A merge of N single-bit flops into one N-bit tray is chosen only when

```
alpha · (N − tray_total/single_bit_power)  >  extra displacement cost.
```

The **only** power benefit currently modeled is *in-cell* clock-pin sharing
(one CK pin's internal energy serves N bits, captured by `getInternalEnergy`).
The dominant real-world benefit of banking — collapsing N clock-tree sinks
(buffers + clock-net wire feeding those pins) down to 1 — is **external to the
cell** and therefore **not counted**. With the savings understated, the ILP
stops banking early (~2.5X).

### Why ORFS does not exhibit this

ORFS libraries already show a large per-bit tray-power drop, so the missing
clock-tree term is not the gating factor; banking proceeds. Customer libraries
show only a modest per-bit drop, so the missing term gates hard. **Implication:
any change to default behavior risks shifting ORFS QoR and golden tests.**
The fix must therefore be opt-in with a default that reproduces today exactly.

---

## 2. Goals / Non-Goals

### Goals
- G1. Let the cost model credit clock-tree power saved by banking, so higher
  banking ratios become achievable on the customer design.
- G2. Default behavior **bit-identical** to today (ORFS unaffected, no golden
  churn).
- G3. Expose control through a `cluster_flops` flag (custom flag promoting
  higher banking).
- G4. Minimal, targeted change — no broad refactor of the ILP.

### Non-Goals (this change)
- N1. "Wasted bits" / partially-filled trays — **deferred** to a follow-up
  (see §7).
- N2. Auto-estimating real clock-tree power from buffer libraries / RC.
- N3. Any change to displacement or timing-path terms of the objective.

---

## 3. Design

### 3.1 Cost model

Model a per-sink clock-tree power `P_ct`. An N-bit tray presents **one** clock
sink regardless of N, so N→1 banking eliminates (N−1)·P_ct of clock-tree power.
Fold this into the per-tray cost, keeping the 1-bit baseline at 1.0:

```
tray_cost[N-bit] = (tray_total_power + P_ct) / (single_bit_power + P_ct)
tray_cost[1-bit] = 1.0
```

- `P_ct = 0`  ⇒  `tray_cost = tray_total/single_bit_power`  ⇒  **identical to
  today** (satisfies G2).
- `P_ct > 0`  ⇒  numerator/denominator both gain one sink's worth; since
  `tray_total > single_bit_power`, the ratio shrinks toward 1, making large
  trays relatively cheaper ⇒ higher banking (satisfies G1).

### 3.2 Knob semantics (decided)

- Relative weight `k` via flag, default `k = 0`.
- `P_ct = k · single_bit_power_`  (unit-free; "clock-tree power per flop =
  k × the flop's own cell power").
- Decision rationale: intuitive, no need to know absolute watts at preroute,
  default 0 = opt-in, ORFS untouched.

### 3.3 Implementation point

Single conceptual change in `MBFF::SetRatios` (`mbff.cpp:~1988-1992`):

```cpp
// today:
//   norm_power_[i] = (tray_total / slot_cnt) / single_bit_power_;
// proposed:
const float p_ct = clock_power_weight_ * single_bit_power_;   // per sink
norm_power_[i]
    = ((tray_total + p_ct) / slot_cnt) / (single_bit_power_ + p_ct);
```

`tray_cost = slot_cnt · norm_power_[i] = (tray_total + p_ct)/(single_bit_power_
+ p_ct)` as specified. 1-bit `tray_cost` stays hard-coded 1.0 (`mbff.cpp:868-869`),
which equals `(single_bit_power_+p_ct)/(single_bit_power_+p_ct)` — consistent.

No change to `RunILP`'s cost loop is required; it already reads `norm_power_`.

### 3.4 Plumbing path

```
cluster_flops -clock_power_weight k        (src/gpl/src/replace.tcl)
  → gpl::replace_run_mbff_cmd(..., k)      (src/gpl/src/replace.i)
  → Replace::runMBFF(..., k)               (src/gpl/src/replace.{h,cpp})
  → MBFF::Run(...) or MBFF ctor            (src/gpl/src/mbff.{h,cpp})
  → store clock_power_weight_ member; used in SetRatios
```

Thread as a new trailing parameter (default `0.0`) so existing call sites and
the SWIG signature change minimally.

---

## 4. Interface Change

New optional flag on `cluster_flops`:

```
cluster_flops
    [-tray_weight tray_weight]          # alpha, default 32.0
    [-timing_weight timing_weight]      # beta,  default 0.1
    [-max_split_size max_split_size]    # default 500
    [-num_paths num_paths]              # default 0
    [-clock_power_weight k]             # NEW: P_ct = k·single_bit_power, default 0.0
```

- Type: float, `k >= 0`. Validate non-negative; error `GPL` code on negative.
- `k = 0` (default): no behavioral change.
- Guidance to customer: start at `k = 1.0`, sweep up (e.g. 1–3) for more banking.

---

## 5. Files Touched

| File | Change |
|------|--------|
| `src/gpl/src/replace.tcl` | add `-clock_power_weight` key, default 0.0, validation, pass to cmd |
| `src/gpl/src/replace.i` | add float param to `replace_run_mbff_cmd` |
| `src/gpl/src/replace.h` | add param to `runMBFF` decl |
| `src/gpl/src/replace.cpp` | add param to `runMBFF`, pass into MBFF |
| `src/gpl/src/mbff.h` | add `clock_power_weight_` member + ctor/Run param |
| `src/gpl/src/mbff.cpp` | store weight; modify `SetRatios` formula; ctor init |
| `src/gpl/test/...` | new regression test (see §6) |
| `src/gpl/doc/` or `docs/` | document the flag |

clang-format C++ files before commit (not `*.i`). Commit with `-s`.

---

## 6. Testing

- **T1 (regression / no-change):** existing MBFF tests (`mbff_hier`,
  `mbff_orig_name`) must pass **unchanged** with default `k=0`. This is the G2
  gate.
- **T2 (banking increases):** new Tcl test on a small synthetic FF cluster:
  run `cluster_flops` with `k=0` then a high `k`; assert the reported "Sizes
  used" histogram shifts toward larger trays (higher banking) at high `k`.
  Register in **both** CMake and Bazel (per project rule).
- **T3 (validation):** negative `k` errors out cleanly.
- Sanity: `SetRatios` debug print (`mbff.cpp:1993`) shows reduced `norm_power`
  for large trays as `k` rises.

Acceptance: customer test reaches >2.5X (target ≥4X) at some finite `k`, with
ORFS golden files untouched at default.

---

## 7. Deferred — Wasted Bits (follow-up)

ILP constraint `tray_used ≤ slots_used` (`mbff.cpp:853`) only requires ≥1 slot
filled, allowing partially-filled trays. Out of scope here. Candidate
follow-ups, in preference order:
1. Post-ILP downsizing pass: swap an underfilled N-bit tray to the smallest
   master covering its filled slots (saves area/power, no sink-count change).
2. Empty-slot penalty `+ gamma·(slot_cnt − slots_used)` in the ILP objective.

Tracked separately to keep this diff focused and reviewable.

---

## 8. Risks

- R1. `single_bit_power_` could be ~0 (missing lib power) ⇒ `P_ct` ~0 and
  ratio degenerate. Mitigation: existing `single_bit_power_ > 0` guard
  (`mbff.cpp:1988`) already wraps the formula; keep it.
- R2. Very large `k` could over-bank and raise displacement/timing cost.
  Acceptable — it is an opt-in tuning knob; document recommended range.
- R3. SWIG signature change must stay in sync across `.i`/`.tcl`/`.cpp`.

---

## 9. Rollout

1. Implement §3–§5 behind default `k=0`.
2. Verify T1 (no ORFS change), then T2/T3.
3. Hand customer the flag + recommended sweep.
4. Open follow-up issue for wasted-bits (§7).
