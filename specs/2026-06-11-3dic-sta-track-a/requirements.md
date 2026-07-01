# Requirements — Track A: cross-chiplet STA + duplicated masters (now-slice)

Feature scope, decisions, and context for the Track A now-slice. See the
constitution: `specs/mission.md`, `specs/tech-stack.md`, `specs/roadmap.md`
(Track A), `specs/requirements.md` (stakeholder asks R1–R11).

Branch: `3dic-sta-track-a`.

## Goal

Bring 3DIC cross-chiplet STA up on current HEAD and fix the per-bump
duplicated-master vertex-id collapse — everything in Track A that lands
WITHOUT Osama's `dbUnfoldedInst`. Track A' (flat interior timing of duplicated
chiplets) is explicitly OUT and parked behind `dbUnfoldedInst`.

## State of HEAD (verified)

- Merged: dbObject unfolded model (PR #10588, `dbUnfoldedChip{Inst,RegionInst,
  BumpInst,Conn,Net}`) + dbInst↔chip-bump association (PR #10590).
- **No STA adapter exists**: `dbSta::postRead3Dbx` is an empty stub
  (`// TODO: we are not ready to do timing on chiplets yet`); `dbNetwork.cc`
  has no 3DIC code; no `3dic_*.tcl` tests.
- The v1 adapter (PR #10417) is an **unmerged reference branch** (`3dic-sta-v1`).
  This feature ports its still-applicable mechanisms onto HEAD.

## In scope (A1–A6)

- **A1 Adapter bring-up.** `postRead3Dbx` wiring; `has3DicChip()` mode gate;
  `Pin*`/`Net*`/`Instance*` encoding for bump / chip-net / chip-inst; the
  accessors STA calls (`instance`/`net`/`term`/`port`/`direction`/`vertexId`/
  `setVertexId`); chip-aware iterators (`DbInstanceChildIterator`,
  `DbInstancePinIterator`, `DbNetPinIterator`); boundary bridges
  (`term()` forward, `pin(Term*)` reverse); STA-3000 diagnostics. Ported from
  #10417, re-based on the merged unfolded model.
- **A2 Bump-level per-path identity.** Key bump `Pin*` + vertex id on
  `dbUnfoldedChipBumpInst` (already per-unfold-path on HEAD). Fixes the
  "vertex ids not attached to the unfolded model" collapse.
- **A3 Flat interior of UNIQUE masters.** Single-instance chiplet interiors
  keyed via normal `dbITerm`/`kDbIterm`; full flat `ff → … → ff`.
- **A4 Drop the old side-map.** Move bump vertexId off
  `chip_bump_vertex_ids_` onto `dbUnfoldedChipBumpInst`; re-attach on every
  `constructUnfoldedModel()` rebuild.
- **A5 Shared-master interior guard.** Do NOT descend a duplicated master's
  shared interior (alias → collision/crash). Keep shared-master blocks out of
  the descend set (mirror #10417's `block_to_chip_inst_` filter); duplicated
  chiplet = opaque box, bump pins only, with a loud `STA-3xxx` warning.
- **A6 Regression.** Port #10417's `3dic_cross.tcl`; add the duplicated-master
  assertions. Register in CMake + Bazel.

## Unfolded model: verified correct + build-timing note (2026-06-12)

Initially suspected the unfolded model under-populated (Tcl
`llength [$db getUnfoldedChipNets]` reported 1). **False alarm** — that dbSet
is not SWIG-wrapped, so `llength` on the opaque handle lies. Verified from C++
(temp diag in postRead3Dbx): **nets=4, bumps=6, bumps-on-nets=6** — exactly
the expected counts. Osama confirmed.

**Real finding (kept):** the unfolded model is **not auto-built when the
read_3dbx callback fires** (C++ counts were 0 until an explicit
`db->constructUnfoldedModel()`). So the adapter must call
`constructUnfoldedModel()` in `postRead3Dbx` before STA consumes the unfolded
objects. This is now done. Ties to requirements R3 (rebuild contract).

**Consequence:** M2 (key chip-bump `Pin*` on `dbUnfoldedChipBumpInst`) is
NOT blocked — the data is correct and usable from C++. Only the Tcl-level
inspection is unavailable until the dbSet is wrapped.

## Out of scope (deferred)

- **Track A' — flat interior timing of duplicated chiplets.** Needs
  `dbUnfoldedInst` (Osama, in-progress; he owns the dbSta migration). See
  memory `3dic-dbunfoldedinst-trigger`.
- **ETM black-boxing** (Track E3 / TODO 3) — would let duplicated `.lib`
  chiplets time fully now, but deferred to keep scope tight. The `.lib` is
  already loaded into STA (`3dblox.cpp:436`); wiring it into
  `makeTopCellForChip` is the later work.
- **net2 RC parasitics** (Track D, Arthur's `dbChip*` objects). Track A times
  bonds as zero-delay.
- Chiplet hierarchy (Track B), hardening (Track E).

## Key decisions

1. **Build on the current model.** Per Osama: key bumps on
   `dbUnfoldedChipBumpInst` now; he migrates dbSta if `dbUnfoldedBump →
   dbUnfoldedInst` lands. No throwaway composite-`Pin*` encoding.
2. **Composite-`Pin*` rejected** for interior identity — hits the 32-bit
   `ObjectId` ceiling as a correctness failure (TODO 7) and is throwaway.
   Interior duplicated-master identity waits for `dbUnfoldedInst`.
3. **Duplicated masters are opaque, not wrong.** A5 guard + warning. Missing
   interior paths is a documented, loud limitation — matches #10417.
4. **Zero-delay bonds.** RC is Track D; Track A asserts zero-delay.
5. **Fixture: port #10417's `3dic_cross.tcl`** (fake LEF/DEF/`.bmap`/3dbv)
   rather than authoring fresh.

## Risks / watch-items

- **Silent accessor → segfault.** Every new `Pin*` kind needs `setVertexId`,
  `vertexId`, `direction`, `port` branches (a null produces a non-obvious
  crash deep in graph build). See `3DIC.md`.
- **Pointer-tag alignment (VERIFIED ISSUE).** `_dbUnfoldedChipBumpInst` is
  `_dbObject`(8, 4-aligned) + 3×`dbId`(4) = **20 bytes** — same shape as the
  Stage-4 `_dbChipBumpInst` bug. Tagging it (A2) needs a 4-byte pad →
  `sizeof == 24`. We pad the UNFOLDED object (we tag it), not the raw
  `_dbChipBumpInst` v1 padded. See plan.md 1.3a.
- **Rebuild staleness.** Unfolded tables are rebuilt-on-read, not persisted;
  re-attach vertex ids after each `constructUnfoldedModel()` (R3).
- **2D regression.** `has3DicChip()` must gate first and not perturb flat /
  Verilog-hier paths.
