# Mission — 3DIC Static Timing Analysis

## Purpose

Enable correct, scalable cross-chiplet static timing analysis (STA) for 3DIC
(3DBlox) designs in OpenROAD, adding active top-level interconnect timing and
avoiding the structural restrictions of the prior v1 prototype (PR #10417).

**State of HEAD (verified):** the odb structural model (`dbChip*`) and the
dbObject unfolded model are merged, but **no STA adapter exists** —
`dbSta::postRead3Dbx` is an empty stub (`// TODO: we are not ready to do timing
on chiplets yet`) and `dbNetwork.cc` has none of the 3DIC machinery. The v1
adapter (PR #10417) is an **unmerged reference branch** (`3dic-sta-v1`), not
in-tree code. So this effort brings 3DIC STA up on HEAD, using #10417 as a
design reference — not a refactor of in-tree code.

STA is stage 9 of the RosettaStone Pin-3D flow (`3DIC_overview.md`). The
structural skeleton exists in OpenDB/3DBlox (`dbChip`, `dbChipInst`,
`dbChipNet`, `dbChipConn`, + the unfolded model; the `dbChipBump*` objects are
being eliminated — see `tech-stack.md`); this work is the adapter layer that
maps that skeleton onto OpenSTA's `Network` abstraction so the rest of the
timing engine works on 3D designs.

## Headline goals

Two committed goals for this effort:

### 1. Fix duplicated masters + chiplet hierarchy

The single largest prior-impl gap. v1 keyed per-bump `VertexId` in a flat
side-map on the raw `dbChipBumpInst*` pointer
(`std::map<dbChipBumpInst*, VertexId> chip_bump_vertex_ids_`). When the same
chiplet master is instantiated more than once, every placement reuses the SAME
`dbChipBumpInst*`, so all placements collapse to one `VertexId` → graph
topology is wrong → cross-placement paths break. v1 also could not descend
chiplet-of-chiplet nesting (`dbChip` HIER master).

Target state:
- Per-unfold-path timing identity. Repeated chiplet masters time
  independently.
- Multi-level chiplet hierarchy (`top → mid [HIER] → leaf`) is timeable.

### 2. Active top-level interconnect (net2)

The top-level cross-chip net is **not** a passive zero-delay wire. As learned
at the NGMM meeting (`boundary.png`), the path runs through real **I/O
driver** and **I/O receiver** cells. **Correction to an earlier reading of
`boundary.png`:** the IO driver/receiver live **inside** the chiplets — they
are ordinary chiplet-interior leaf cells, NOT floating top-level instances the
figure appears to show.

```
ChipletA: ff1 -> buf1 -> net1a -> IO_driver1 -> [net1b -> bumpA]
top:                                             net2  (cross-chip, active)
ChipletB:                          [bumpB -> net3b] -> IO_receiver1 -> net3a -> buf2 -> ff2
```

Target state:
- IO driver/receiver time as **interior chiplet leaf instances** with Liberty
  arcs — the same machinery that already times `buf`/`ff` inside a chiplet.
  No new top-level cell representation.
- The chiplet boundary stays the existing **bump ↔ `dbBTerm` bridge**.
- `net2` (the cross-chip net between the two bumps) is **active**: its real
  driver is IO_driver1's output and its real load is IO_receiver1's input
  (each one bump-bridge hop inside the chiplet), and it carries extracted RC
  (Arthur's extraction) — not a zero-delay bond.

## What "success" looks like

- `report_checks` produces real constrained `ChipletA/ff1 → ChipletB/ff2`
  setup/hold paths that traverse `buf1 → IO_driver1 → net2 → IO_receiver1 →
  buf2`, with Liberty cell delays AND net2 RC delay.
- The same design instantiating a chiplet master N times reports N
  independent, correct timing paths.
- A nested chiplet (chiplet-within-chiplet) reports paths across all unfolded
  leaf instances.

## Non-goals (this effort)

- Full SI / crosstalk analysis; frequency-dependent bond models; SSTA/OCV
  bond statistics. (Note: coupling-cap objects `dbChipCCSeg` DO exist in the
  inter-chip parasitic model; STA may initially ground/lump them, but the data
  carries coupling.)
- Reinventing the structural 3DBlox schema — reuse OpenDB as-is.
- 2D / non-3DIC timing behavior must remain unchanged.

## Reuse stance

Do not reinvent the wheel. The PR #10417 branch (unmerged) is a working
reference for the adapter ideas — `Pin*` pointer-tag scheme, `term()`/`pin()`
boundary bridges, per-block ObjectId discriminator, fat-net wire model,
`postRead3Dbx` diagnostics — **port** the ones that still apply onto HEAD
rather than re-deriving them. Re-base identity onto the merged dbObject
unfolded model + path-qualified `dbInst` (see `tech-stack.md`), NOT onto the
`dbChipBumpInst*` keys #10417 used. Prior design and known gaps are in
`3DIC.md` / `3DIC_TODO.md`; treat `3DIC_overview.md` as positioning reference
only (may diverge).

## Principles

1. **Minimal, targeted changes.** Isolate the problem; smallest fix that
   addresses it. No broad refactors while diagnosing.
2. **Trace bugs upstream** to the data-creation point, not the serialization
   point.
3. **Fail safe, fail loud.** A new `Pin*` kind must have both `setVertexId`
   and `vertexId` branches, a `direction()` branch, and a `port()` branch — a
   silent accessor produces a non-obvious downstream segfault.
4. **Don't touch `src/sta/` (OpenSTA upstream) without asking.** Prefer fixes
   in `src/dbSta/`, `src/odb/`, `src/rsz/`.
