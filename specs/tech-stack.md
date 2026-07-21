# Tech Stack — 3DIC STA

## Languages / build

- **C++** (core adapter + odb schema), **Tcl** (commands, regression tests).
- Build: **CMake** and **Bazel** — every new test/target must be registered in
  BOTH. Forgetting Bazel is the most common mistake.
- `clang-format -i` on all touched C++ before commit. **Never** on `src/sta/*`
  or `*.i` files.
- Commits: `git commit -s` (DCO). After submodule edits, update parent repo
  reference via `git submodule update --init --recursive`.

## Engine layering

```
OpenSTA core (src/sta)   — Graph / Search / Sdc / Parasitics / ClkNetwork
        ▲  speaks only Network abstractions: Pin* Net* Instance* Term* Port* Cell*
        │  (upstream — do not modify without asking)
dbNetwork adapter (src/dbSta)  — maps 3DBlox structure onto Network
        ▲
OpenDB / 3DBlox (src/odb)  — dbChip* structural model + unfolded model
```

## Foundation: the dbObject-backed unfolded model (PR #10588 — MERGED 72f47ed)

**Decision: build on the dbObject unfolded model as the canonical
foundation.** It replaced the non-persistent struct `UnfoldedModel` with
persistent `dbObject`-derived classes owned by `dbDatabase`. **Actual
post-merge class names** (verified in `include/odb/db.h`):

| dbObject class             | Role |
|----------------------------|------|
| `dbUnfoldedChipInst`       | one placement of a chip in the unfolded tree |
| `dbUnfoldedChipRegionInst` | unfolded interface region |
| `dbUnfoldedChipBumpInst`   | unfolded bump (per-unfold-path) — **being eliminated, see below** |
| `dbUnfoldedChipConn`       | unfolded bond connection |
| `dbUnfoldedChipNet`        | unfolded cross-chiplet net (`getConnectedBumps()`) |

Built by `dbUnfoldedBuilder` (walks the chip-inst tree); entry point
`dbDatabase::constructUnfoldedModel()`. Geometry computed on demand from source
`dbInst` transforms. Tables are **rebuilt on read** (`operator>>` calls
`constructUnfoldedModel()`), NOT deserialized — so any STA id stored on these
objects must be re-established after each rebuild (R3).

### dbInst ↔ chip-bump association (PR #10590 — MERGED 0bc30cf)

`_dbInst` gained `chip_region_` + `bump_` fields; `dbInst::getChipBump()`,
`dbBTerm::getChipBump()`, `dbChipBump::getInst()`,
`dbChipBump::create(chip_region, inst)`. The chiplet boundary `dbBTerm`
binding is intact (`dbChipBump::getBTerm()/setBTerm()` still present) — the
inst↔bump link is an *additional* association (bump's physical realization /
geometry source), not a replacement for the bterm bridge.

### IN-PROGRESS, HIGH IMPACT: bump classes being eliminated (Osama)

Osama is removing **`dbChipBump` / `dbChipBumpInst` / `dbUnfoldedChipBumpInst`**
entirely: each `dbChipBump` has exactly one `dbInst`, and `dbInst` already
carries the region + bump ids, so the bump objects are redundant. **A bump
becomes just a `dbInst`.** Consequences for STA (drive the re-frame in
`requirements.md` R1/R2 and `roadmap.md` Track A):

- **No dedicated chip-bump `Pin*` tag needed.** A bump's pin is the inst's
  `dbITerm` — already `kDbIterm`. The v1 `kDbChipBumpInst=4` tag scheme
  largely dissolves.
- **R1 and R2 merge.** Bump identity == interior-cell identity == "path-
  qualified `dbInst` under unfolding." Vertex id lives on a shared
  **`dbUnfoldedInst`** (Arthur/Osama's foreshadowed object), NOT on a bump
  object. One mechanism solves duplicated-masters for bumps AND interior cells.
- **`dbUnfoldedChipNet` endpoints** shift from bump-inst to (unfolded)
  `dbInst`/`dbITerm`; `getConnectedBumps()` and consumers change.
- **Status:** in-progress, not landed — current tree still has the bump
  classes. Do not hard-code against `dbChipBumpInst*` identity; design Track A
  around path-qualified `dbInst` identity so it survives the elimination.

## dbNetwork adapter mechanisms (from PR #10417 — UNMERGED, to PORT)

These mechanisms are **not in HEAD** — they live on the `3dic-sta-v1` branch
(PR #10417). `dbSta::postRead3Dbx` on HEAD is an empty stub and `dbNetwork.cc`
has no 3DIC code. Port the still-applicable ones onto HEAD; re-key identity off
`dbChipBumpInst*` (see elimination above).

- **`Pin*` pointer-tag scheme.** Low 3 bits of pointer = type tag
  (`kDbIterm=1, kDbBterm=2, kDbModIterm=3`; v1 added `kDbChipBumpInst=4`).
  Requires every base pointer 8-byte aligned — **`sizeof(T) % 8 == 0` for any
  dbObject targeted by a tag** (the `_dbChipBumpInst` sizeof-20 alignment bug).
  NB: once bumps are `dbInst`s (elimination above), the bump pin is a plain
  `dbITerm` (`kDbIterm`) and tag 4 may be unneeded — but a path-qualified
  `dbUnfoldedInst*` tag is likely needed instead; audit its `sizeof`.
- **`Net*` encoding** for `dbChipNet` via plain `reinterpret_cast` + runtime
  `getObjectType()` check (no tag bits).
- **Boundary bridges:** `term(chip_bump_pin)` → inner `dbBTerm` (forward);
  `pin(Term* = inner_bterm)` → bump-inst (reverse). These let
  `visitConnectedPins` cross the chiplet boundary.
- **Fat-net wire model:** `makeWireEdgesFromPin` aggregates all drvrs × loads
  across the fat net; bumps are plumbing, not delay hops (until RC binds).
- **Per-block ObjectId discriminator** (`block_disc_`): stamps a 1..N tag into
  upper bits of encoded `ObjectId` so identically-numbered objects in
  different chiplet blocks don't collide in `PinSet`/`NetSet`. Note the 20-bit
  per-block id ceiling in 3DIC mode (`3DIC_TODO.md` TODO 7).
- **Mode gating:** `has3DicChip()` checked first; independent of
  `hasHierarchy()` (Verilog module hier) and the flat path.

## Active top-level interconnect (boundary.png)

- **IO driver/receiver live INSIDE the chiplets** — ordinary chiplet-interior
  leaf `dbInst`s with Liberty cells (corrects an earlier reading of
  `boundary.png` that placed them at top level). They time via the same
  interior descent that already handles `buf`/`ff`. No new top-level cell
  representation; the chiplet boundary stays the existing bump ↔ `dbBTerm`
  bridge.
- `net2` is the active cross-chip net between the two bumps: real driver =
  IO_driver1 output, real load = IO_receiver1 input (each one bump-bridge hop
  inside the chiplet). Because real direction now exists on both sides, the
  v1 BIDIRECT-bump hack may be unnecessary on net2 — confirm during Track C.
- `net2` parasitics: extracted RC owned by the hierarchical `dbChip` as a new
  object family **`dbChipRSeg`/`dbChipCapNode`/`dbChipCCSeg`** (Arthur+Matt,
  in progress — see `requirements.md` R6), reached via `dbChipNet`. STA writes
  a new translator (`dbChipNet::getRSegs/getCapNodes/getCCSegs` → OpenSTA
  `makeResistor`/`makeCapNode`/`makeCouplingCap`), mirroring the `dbNet` walk;
  a cross-chip net spans two object families stitched at the boundary by SPEF
  node-name. Per corner (`ParasiticAnalysisPt*`). Extraction by OpenRCX.

## Key files

| Purpose | File |
|---|---|
| Network adapter | `src/dbSta/src/dbNetwork.cc` |
| Adapter declarations | `src/dbSta/include/db_sta/dbNetwork.hh` |
| `postRead3Dbx` entry | `src/dbSta/src/dbSta.cc` |
| Tcl helpers | `src/dbSta/src/dbSta.tcl` |
| odb 3DBlox schema | `src/odb/include/odb/db.h` (`dbChip*`) |
| odb 3DBlox impls | `src/odb/src/db/dbChip*.cpp` |
| Unfolded model | `src/odb/src/db/dbUnfolded*.{h,cpp}`, `dbUnfoldedBuilder.cpp` |
| Schema codegen | `src/odb/src/codeGenerator/schema/chip/*.json` |
| Tcl regression | `src/dbSta/test/3dic_*.tcl` — **on PR #10417 branch, not HEAD; port/recreate** |

## Diagnostics convention

(Target behavior — `postRead3Dbx` is an empty stub on HEAD; this is the
PR #10417 design to re-establish.) `postRead3Dbx` should emit structure +
warnings up front (STA-3000 INFO summary; STA-3001 WARN on unsupported HIER
master in #10417 — orphan-chip-net / unbound-bump checks were *deferred*, not
implemented, see `3DIC_TODO.md` TODO 5) so issues surface immediately rather
than as silent "no paths found" later. Every new gap gets a numbered STA-3xxx
message.

## Testing

- Tcl integration tests under `src/dbSta/test/` with golden output; artificial
  fixtures (fake LEF/DEF/`.bmap`/3dbv). PR #10417's fixtures (`3dic_cross.tcl`,
  `3dic_get_cells.tcl`) are on the unmerged branch — port/recreate them.
- Fixture gotchas live in `3DIC.md` ("Fixture authoring notes"): `.bmap` 5th
  column binds bump→bterm; BUMP macro center constrains `.bmap` XY;
  `Connection:` block grounds chiplets; distinct `dbBlock` per chiplet for path
  tests.
- Register every test in CMake AND Bazel.
