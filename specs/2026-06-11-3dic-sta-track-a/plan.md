# Plan — Track A: cross-chiplet STA + duplicated masters (now-slice)

Numbered task groups. Build on current HEAD (10588 + 10590 merged). Port from
the unmerged PR #10417 reference where applicable. Re-key identity onto the
merged unfolded model (`dbUnfoldedChipBumpInst`), NOT raw `dbChipBumpInst*`.

Reference for mechanism detail: `3DIC.md`, `3DIC_TODO.md`, and the
`3dic-sta-v1` branch.

---

## 1. Adapter scaffolding + mode gate (A1)

1.1 Add `has3DicChip()` to `dbNetwork` — true when top is a hierarchical
    `dbChip` (no own `dbBlock`, has child chip-insts). Checked FIRST, before
    `hasHierarchy()` / flat paths; must not perturb 2D.
1.2 Implement `dbSta::postRead3Dbx(dbChip*)` (currently a stub): set the top
    chip on `dbNetwork`, ensure the unfolded model is built
    (`constructUnfoldedModel()`), install `dbStaCbk` on chiplet blocks, emit
    STA-3000 INFO summary.
1.3 Add the `Pin*` pointer-tag for chip bumps (`kDbChipBumpInst=4`, targets
    `dbUnfoldedChipBumpInst*`) + `Net*` encoding for `dbChipNet`
    (reinterpret_cast + `getObjectType()` check).
1.3a **VERIFIED ALIGNMENT FIX (required).** `_dbUnfoldedChipBumpInst` =
    `_dbObject`(8, 4-aligned) + 3×`dbId`(4) = **20 bytes** → the Stage-4
    bug: half the objects in a `dbTable` page are 4-aligned, tag decodes to
    `kNone`, sporadic `ORD-2018`. Add a 4-byte pad field (or reorder) so
    `sizeof == 24`. Schema edit:
    `src/odb/src/codeGenerator/schema/.../dbUnfoldedChipBumpInst.json` +
    regenerate, OR hand-add to `_dbUnfoldedChipBumpInst`. NB: v1 padded the
    RAW `_dbChipBumpInst`; we tag the UNFOLDED object, so pad THAT one.
    (Raw `_dbChipBumpInst` no longer needs the pad unless tagged elsewhere.)

## 2. Core accessors for the chip-bump pin kind (A1)

2.1 `instance(pin)`, `net(pin)`, `term(pin)`, `port(pin)`, `direction(pin)` —
    add a branch for the chip-bump pin. (BIDIRECT direction for bumps, per
    #10417, so wire edges form across the boundary.)
2.2 `vertexId(pin)` / `setVertexId(pin, id)` branches for the chip-bump pin.
    **Both** must exist (a silent one → null vertex → segfault in graph build).
2.3 Per-master synthesized stub `Cell`/`Port` (`makeTopCellForChip`) so
    `cell(chip_inst)` / `port(pin)` return non-null.

## 3. Iterators + boundary bridges (A1)

3.1 `DbInstanceChildIterator`: top chip → chip-insts; flat-walk each
    uniquely-owned chiplet block's inner `dbInst`s as leaves (so
    `makePinVertices` runs on `ff/CK`, `buf/Z`, …).
3.2 `DbInstancePinIterator(chip_inst)`: yield bump pins via region-insts ×
    bumps. `DbNetPinIterator(chip_net)`: walk `getConnectedBumps()`.
3.3 Boundary bridges: `term(bump_pin)` → inner `dbBTerm` (forward);
    `pin(Term* = inner_bterm)` → bump (reverse). Wire `visitConnectedPins`
    chip-net branch to descend through the boundary (fat-net model).
3.4 Attribution: `instance(inner_iterm)` → inner dbInst; `parent(inner_dbInst)`
    → owning chip-inst; `instance(inner_bterm_pin)` → chip-inst (NOT top) so
    `isTopLevelPort` is correct. Per-block `block_disc_` ObjectId
    discriminator so identically-numbered objects across chiplet blocks don't
    collide in `PinSet`/`NetSet`.

**Milestone M1:** `report_checks` yields a real unique-master cross-chiplet
`chipA/ff → chipB/ff` path with Liberty delays (zero-delay bonds).

## 4. Bump-level per-path identity (A2 + A4)

4.1 Key the bump `Pin*` on `dbUnfoldedChipBumpInst` (per-unfold-path), not raw
    `dbChipBumpInst*`.
4.2 Store/read bump vertex id on/keyed-by `dbUnfoldedChipBumpInst`; delete the
    `chip_bump_vertex_ids_` flat side-map.
4.3 Re-attach ids after every `constructUnfoldedModel()` rebuild (tables are
    rebuilt-on-read, not persisted — R3).

## 5. Duplicated-master hard-error (A5, revised PR #10664)

5.1 In `setTopChip`, count chip-inst placements per master block
    (`block_refs`). A block placed by >1 chip-inst is a duplicated master:
    **hard-error `STA-3004`** ("must be instantiated exactly once") rather than
    dropping it to an opaque box. Downstream code assumes single placement.
5.2 A hierarchical (HIER) master (chip-inst whose master has no own dbBlock)
    likewise **hard-errors `STA-3001`** in `setTopChip` — refuse to build a
    partial graph. (Supersedes the earlier STA-3001 *warning*.)

**Milestone M2:** duplicated-master design aborts with `STA-3004`; nested
(HIER) design aborts with `STA-3001`; unique-master design times cleanly.

## 6. Tests + build registration (A6)

6.1 `3dic_cross.tcl` (unique-master cross-chiplet path) also asserts the
    A3 flat-descent cell set (`get_cells *` → chip-insts + interior ff/inv/
    buf/bumps) and per-chip-inst bump-pin counts.
6.2 `3dic_get_cells.tcl`: duplicated-master fixture (`example.3dbx`, one master
    placed twice) asserts `read_3dbx` aborts with `STA-3004` (revised A5 —
    hard-error, not opaque box).
6.3 Confirm no 2D / Verilog-hier regression (existing dbSta tests green).
6.4 Register both fixtures in **CMake AND Bazel**. `clang-format -i` touched
    C++ (never `src/sta/*` or `*.i`).

## 7. Wrap-up

7.1 Update `specs/roadmap.md` Track A checkboxes; note M1/M2 reached.
7.2 Signed-off commit (`git commit -s`). Submodule ref bump if needed.

---

## Sequencing notes

- Groups 1–3 are the bring-up (M1) — independent of duplicated-master work.
- Groups 4–5 add duplicated-master correctness at the bump/boundary (M2).
- Group 4 (key on `dbUnfoldedChipBumpInst`) should be done as the bumps are
  first wired (group 2/3), not bolted on later — avoids a raw-pointer interim.
- A' (interior duplicated descent) is NOT in this plan; gated on
  `dbUnfoldedInst` (memory `3dic-dbunfoldedinst-trigger`).
