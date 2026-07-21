# Validation — Track A: cross-chiplet STA + duplicated masters (now-slice)

How to know Track A is done and mergeable. All four bars must pass.

## Bar 1 — Unique-master flat cross-chiplet path

- A single-instance two-chiplet design: `report_checks` produces a real
  **constrained** `chipA/ff → chipB/ff` setup path traversing the boundary
  (`…/buf/Z → bump → chip-net → bump → …/buf/A`) with **Liberty cell delays**
  on each cell.
- `create_clock` anchored on a chip-bump pin yields a constrained
  `Path Group: clk` (not "No paths found").
- Bonds are **zero-delay** (no RC) — assert the slack matches the
  cell+intra-chiplet-wire-only expectation (RC is Track D).
- Command check: `report_checks -fields {fanout}` shows the expected fanout on
  the cross-chiplet driver net.

## Bar 2 — Duplicated-master: hard-error (unsupported)

Decision revised (PR #10664 review, Osama): duplicated masters are **rejected
with a hard error**, not timed as an opaque box. A partial/opaque graph is a
correctness trap because downstream code assumes a chiplet block is placed
once; refusing the design is safer than silently dropping interior paths. The
opaque-box + non-collapse model is superseded.

- A design instantiating one chiplet master **twice**:
  - `read_3dbx` aborts with **`STA-3004`** ("same chiplet master placed more
    than once … must be instantiated exactly once") — assert via `catch` +
    the golden `[ERROR STA-3004]` line.
  - No timing graph is built for such a design.
- A **hierarchical (nested) chiplet master** likewise aborts with **`STA-3001`**
  ("does not support hierarchical (nested) chiplet masters yet").
- Real duplicated-master support (flat interior timing) is deferred to Track A'
  (`dbUnfoldedInst`); nested support to Track B.

## Bar 3 — No 2D / non-3DIC regression

- All existing `src/dbSta` tests pass unchanged.
- A flat (non-3D) design and a Verilog-`-hier` design still time correctly —
  `has3DicChip()` gating does not perturb them.
- No new warnings/errors on non-3D flows.

## Bar 4 — Build + format hygiene

- Both new fixtures registered in **CMake** AND **Bazel** (verify the Bazel
  target builds — the most common miss).
- `clang-format -i` applied to all touched C++ — **never** `src/sta/*` or
  `*.i`.
- Signed-off commit (`git commit -s`); submodule ref updated if the parent
  repo tracks it.

## How to run

- Unit/integration: `3dic_cross.tcl` (unique-master cross-chiplet path +
  flat-descent cell/pin iteration) and `3dic_get_cells.tcl` (duplicated-master
  `STA-3004` hard-error) under `src/dbSta/test/`, each with a golden `.ok`.
- Regression sweep: the dbSta test suite (CMake `ctest` and the Bazel test
  target).

## Explicitly NOT validated here (later tracks)

- Flat interior timing of duplicated chiplets (Track A' — `dbUnfoldedInst`).
- net2 RC delay vs analytic value (Track D).
- ETM-bound chiplet delays (Track E3).
- Chiplet-of-chiplet hierarchy paths (Track B).
