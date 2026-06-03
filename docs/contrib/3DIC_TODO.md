# 3DIC STA — Post-v1 TODOs

v1 (Stages 0–8, see `3DIC.md`) lands cross-chiplet timing for **single-level**
hierarchies under a **zero-delay** bond model. Two known gaps remain.
This document outlines the concrete refactors needed to close each.

---

## TODO 1 — Multi-hierarchical 3DIC support

### Problem

v1 stores per-chip-bump `VertexId` in a flat side-map keyed by
`dbChipBumpInst*`:

```cpp
// dbNetwork.hh
std::map<odb::dbChipBumpInst*, VertexId> chip_bump_vertex_ids_;
```

The `Pin*` identity for a chip-bump is the tag-encoded `dbChipBumpInst*`.

This works for **single-level** 3DIC (one top `dbChip` with direct
`dbChipInst` children). In **multi-hierarchical** 3DIC (chiplet of
chiplets, e.g. `top → mid [HIER] → leaf`), a single `mid` chiplet
master can be placed multiple times at top. Each placement reuses the
SAME inner `dbChipBumpInst*` pointers (they belong to the `mid`
schema, not to a per-placement copy). The unfolded model produces
distinct physical bumps per unfold path
`(top.mid_inst_X, leaf_inst_Y, bump)`, but they all hash back to one
`dbChipBumpInst*` → side-map collapses them to one `VertexId` →
graph topology wrong → cross-hierarchy paths break.

### Concrete changes

1. **Store `VertexId` on `UnfoldedBump`**, not in `dbNetwork`.
   - File: `src/odb/include/odb/unfoldedModel.h`. Add
     `VertexId sta_vertex_id_{object_id_null};` to `UnfoldedBump`
     (or equivalent field).
   - `UnfoldedBump` already carries `std::vector<dbChipInst*> path`,
     so it uniquely identifies each unfolded instance regardless of
     nesting depth.
   - Delete `chip_bump_vertex_ids_` from `dbNetwork.hh`.

2. **Re-key the `Pin*` for chip-bumps from `dbChipBumpInst*` to
   `UnfoldedBump*`.**
   - File: `src/dbSta/src/dbNetwork.cc`.
   - Change the pointer-tag scheme:
     ```cpp
     // current: kDbChipBumpInst = 4U  (dbChipBumpInst* tagged)
     // new:     kDbChipBumpInst = 4U  (UnfoldedBump* tagged)
     ```
     `UnfoldedBump` already requires 8-byte alignment (contains a
     `Point3D` etc.), so the lower-3-bits encoding works unchanged.
   - Update `staToDbChipBumpInst(Pin*)` to instead return an
     `UnfoldedBump*`. Rename to `staToUnfoldedBump`.
   - Update every emitter of a chip-bump `Pin*` to encode the right
     `UnfoldedBump*` (DbInstancePinIterator, DbNetPinIterator,
     visitConnectedPins chip-net branch, term-descent inverse map
     in `pin(Term*)`).

3. **Update `vertexId(Pin*) / setVertexId(Pin*, id)`** to read/write
   the `sta_vertex_id_` field on the `UnfoldedBump` directly, no
   side-map.

4. **`instance(Pin*)` for chip-bumps must walk the unfold path.**
   - Currently:
     ```cpp
     return dbToSta(bump->getChipRegionInst()->getChipInst());
     ```
     This returns the leaf-level chip_inst. For multi-hier, the
     `Instance*` returned must reflect the FULL path —
     `UnfoldedBump::path` ends in the leaf chip_inst, but the
     `Instance` in STA's hierarchy is the path-qualified instance,
     not the raw `dbChipInst*` master.
   - Implies: chip_inst `Instance*` encoding also needs path-aware
     identity. Two options:
     - (a) Encode `UnfoldedChip*` (already path-tagged in
       UnfoldedModel) as Instance, replacing the current
       `dbChipInst*` tag.
     - (b) Compose a `(dbChipInst*, path_hash)` pair into a single
       64-bit Instance* by allocating shadow records — heavier.
     - (a) is the clean path; mirrors the `Pin*` change.

5. **Iterators must descend hierarchical chip_insts recursively.**
   - `DbInstanceChildIterator` for a top `dbChip`: if a child
     `dbChipInst` has a HIER master (no `dbBlock`, contains nested
     `dbChipInst` children), recurse into its children. v1 short-
     circuits with `staToDbChipInst(instance) != nullptr` returning
     no children; this must change.
   - `DbInstancePinIterator` for a non-leaf chip_inst: yield its
     `UnfoldedBump`s at every level, not just leaf chip-bumps.
   - `pinIterator(Net*)` / `DbNetPinIterator` for a `dbChipNet`:
     resolve each `dbChipBumpInst` via `getBumpInst(i, path)` (already
     path-aware in odb) but yield the `UnfoldedBump*`-keyed Pin
     instead of the raw bump-inst pin.

6. **`block_to_chip_inst_` becomes path-aware.**
   - Current map collapses `dbBlock → dbChipInst*` (filters
     shared-master case). For multi-hier with shared mid-chip masters,
     a single `dbBlock` is reached by multiple unfold paths.
   - Replace with `std::map<UnfoldedChip*, dbChipInst*>` or a path
     lookup via `UnfoldedModel::findUnfoldedChip(path)`.

7. **`block_disc_` discriminator**: still keyed by `dbBlock`. Still
   correct because cross-block id collisions are at the `dbBlock`
   level, not the unfold-path level. No change needed.

8. **Update `dbNetwork::parent(Instance*)`** to walk one step up the
   `UnfoldedChip::path` (currently just returns `top_instance_`
   for any chip_inst).

9. **Hook `dbStaCbk` on inner leaf blocks reachable through HIER
   chiplets.** Today `dbSta::postRead3Dbx` (`src/dbSta/src/dbSta.cc`)
   loops `chip->getChipInsts()` and installs one `dbStaCbk` per
   top-level chip-inst's `dbBlock`. For a HIER chiplet master, the
   top-level chip-inst points at a hierarchical `dbChip` (no own
   `dbBlock`), so no callback is wired; mutations on the leaf blocks
   nested inside are invisible to STA.
   - Walk `db_->getUnfoldedModel()->getChips()`; for each unfolded
     chip whose master has an associated `dbBlock`, install a
     `dbStaCbk` if not already owned. Dedupe by leaf `dbBlock*`.
   - When a leaf master is reached via N unfold paths, only one
     callback is needed (the callback fires on edits to the
     `dbBlock`, not per unfold instance).
   - Until this lands, `postRead3Dbx` emits `STA-3001 WARN` when any
     top-level chip-inst references a HIER master, telling the user
     v1 STA supports flat 3DIC only. Remove that warning when the
     UnfoldedModel-driven callback wiring is in place.

### Test plan

- Extend `dbSta/test/3dic_cross.tcl` with a second test fixture
  (`3dic_cross_hier.tcl`?) that places `mid` chip twice at top, with
  one leaf chiplet inside each `mid`. Verify constrained setup check
  forms across the four leaf instances.
- Verify `report_3dic_summary` count of chiplets matches the number of
  UNFOLDED instances, not just the top-level `dbChipInst` count.
- Confirm `STA-3001` no longer fires once the UnfoldedModel-driven
  callback wiring (item 9) is in place; add a regression that asserts
  the warning is absent on a clean nested fixture.

---

## TODO 2 — Parasitics on dbChipConn (RC bond model)

### Problem

v1 treats every cross-chiplet wire edge as zero-delay. STA sums only
the Liberty cell delays plus the (zero) wire delay → cross-chip
slack reflects ONLY cell logic + intra-chiplet wire load, not the
physical bond delay between chiplets. Real 3DIC bonds (hybrid bond,
TSV, microbump) contribute non-trivial RC.

### Concrete changes

1. **Define a parasitic-bearing field on `dbChipConn`.**
   - File: `src/odb/src/codeGenerator/schema/chip/dbChipConn.json`.
   - `dbChipConn` already has `thickness`. Add per-conn lumped R, C
     (or a reference to a parasitic model). Suggested fields:
     ```
     - name: resistance_, type: double, default: 0.0
     - name: capacitance_, type: double, default: 0.0
     ```
     Or a richer model id pointing to a library of bond profiles.

2. **Bind parasitics in dbSta after read.**
   - File: `src/dbSta/src/dbSta.cc::postRead3Dbx`.
   - After `setTopChip` + `constructUnfoldedModel`, iterate
     `chip->getChipNets()`. For each chip-net, locate the
     `dbChipConn` that bonds the two bump endpoints (region-pair
     match) and compute the lumped RC for that net.
   - Use the existing OpenSTA hook:
     ```cpp
     sta_->parasitics()->makeParasiticNetwork(
         net, /*reduced=*/false, parasitic_ap);
     // attach lumped R + C nodes to the chip-bump pins
     ```
     The `Net*` here is the `dbChipNet`-encoded `Net*`.
   - Per-corner: use `ParasiticAnalysisPt*` from the active scene.

3. **Optional: parasitic reduction.**
   - Real 3DIC will want pi-model or detailed RC tree, not pure lumped.
     OpenSTA's `ParasiticReduce` can collapse a detailed RC tree to a
     pi-model. Wire the dbChipConn parameters into that pipeline.

4. **Reporting hook.**
   - Extend `report_3dic_summary` to list, per dbChipConn, the bound
     parasitic R / C / wire delay (if available).
   - New diagnostic: STA-3004 INFO at end of `postRead3Dbx`:
     `"<K> chip-bond regions carry parasitic R/C."`
   - STA-3005 WARN if a `dbChipConn` has thickness but no R/C model
     bound — flags users that the bond will be modeled as zero delay.

5. **Test plan.**
   - Extend `3dic_cross.tcl` with a corner where the `bridge`
     chip-bond carries a non-zero RC. Compare slack against the
     zero-delay baseline; verify delta matches the analytic
     RC-delay calc.

### Out of scope (defer to a later iteration)

- Multi-conductor / coupling cap modeling (cross-bond crosstalk).
- Frequency-dependent / wideband bond models.
- Bond statistics for OCV / SSTA.

---

## TODO 3 — ETM-bound chiplets (use real Liberty when available)

### Problem

v1 builds a stub **`ConcreteCell`** (no `LibertyCell` binding) per
chiplet master, with a plain `Port` per chip-bump bterm and no timing
arc — cross-chiplet delay is cell logic plus (zero) bond wire delay only.
That works when the chiplet ships only as DEF + bump map. When the
chiplet vendor instead ships an **Extracted Timing Model (ETM)** — a real
`.lib` whose `cell` matches the chiplet name and whose ports match the
chip-bump bterm names — the stub is wrong: it hides the ETM's real
clock-to-q, setup/hold, and internal arcs.

3DBlox already supports this via `external.liberty_file:` under
`ChipletDef:`. dbSta just doesn't consult it.

### Concrete changes

1. **Lookup before synthesis in `makeTopCellForChip`.**
   - File: `src/dbSta/src/dbNetwork.cc`. Inside the per-master loop:
     ```cpp
     LibertyCell* etm = network_->findLibertyCell(master->getName());
     if (etm) {
       chip_master_cells_[master] = reinterpret_cast<Cell*>(etm);
       continue;  // skip stub ConcreteCell synthesis
     }
     // no ETM — fall through to the public makeCell/makePort stub.
     ```
   - The ETM `LibertyCell`'s `LibertyPort`s must match the chip-bump
     bterm names. Validate via a sanity pass:
     ```cpp
     for (each bump bterm) {
       if (!etm->findLibertyPort(bterm->name)) warn(STA-3006, ...);
     }
     ```

2. **Suppress the BIDIRECT direction fallback when ETM is present.**
   - For a stub `ConcreteCell` chip-bump pin, `libertyPort(pin)` is null,
     so `dbNetwork::direction(pin)` returns BIDIRECT (dual vertices, both
     clock-seeded — how v1 propagates a clock through a bump). With an
     ETM the bump pin resolves to a real `LibertyPort`, so `direction`
     returns its INPUT/OUTPUT/INOUT and the ETM's own arcs drive
     clock-edge propagation; no BIDIRECT fallback is taken.

3. **Skip the `chip_master_lib_` stub Library entirely** when
   every chiplet master has an ETM.

4. **Diagnostic.**
   - STA-3000 INFO line: append `, ETM-bound: <K>/<N> chiplets` so the
     user sees how many chiplets are running on real Liberty vs stubs.
   - STA-3006 WARN: ETM cell found but bump bterm name has no matching
     LibertyPort.

### Test plan

- Generate a tiny ETM `.lib` for `flop_chip_a` (one cell, clock/d/q
  ports with realistic delays).
- Extend `3dic_cross.tcl` to read the ETM and verify the slack number
  includes the ETM's internal delay, not zero.

### Why this matters

ETM-based chiplet flows are how production 3DIC designs actually ship.
v1's stub synthesis is fine for early bring-up; downstream RTL-to-GDS
flows that integrate vendor chiplets will need this path.

---

## TODO 4 — Callback-driven cache invalidation for `bump_to_chip_net_`

### Problem

`dbNetwork::refreshBumpToChipNetCache()` currently rebuilds the
bump-inst → chip-net map only when the odb chip-net **count** changes.
That catches `dbChipNet_create` between lookups (the common case in
Tcl test fixtures), but it MISSES in-place rewires:

- `existing_chip_net->addBumpInst(bump, path)` on a chip-net that's
  already in the cache → cache stays stale for the new bump.
- `existing_chip_net->removeBumpInst(bump)` similarly.

A user who builds chip-nets dynamically (without creating new ones)
will silently get null `net(bump_pin)` for the rewired bumps.

### Concrete change

Wire `dbNetwork` to listen on a `dbBlockCallBackObj`-style hook that
fires on `dbChipNet` and `dbChipBumpInst` edits (create / destroy /
addBumpInst / removeBumpInst). On any such signal, invalidate the
cache (e.g. clear and reset the size counter). dbCbk currently emits
no chip-net / bump-inst signals — adding those is the prerequisite.

### Files

- `src/odb/include/odb/dbBlockCallBackObj.h` — add the new hook
  methods.
- `src/odb/src/db/...` (the dbChipNet / dbChipBumpInst impl) — fire
  the new callbacks.
- `src/dbSta/src/dbSta.cc::dbStaCbk` — implement the hooks to
  invalidate `bump_to_chip_net_` and reset
  `bump_to_chip_net_cache_size_` to 0.

---

## TODO 5 — STA-gated structural-integrity checks (orphan chip-nets, unbound chip-bumps)

### Problem

Two structural-integrity conditions that matter for STA correctness:

1. **Orphan `dbChipNet`** — a chip-net with zero attached `dbChipBumpInst`s.
   STA enumeration yields no Pin*s for it; paths through that net never
   form. Today silent.
2. **Unbound chip-bump** — `dbChipBump::getBTerm() == nullptr` (5th column
   of `.bmap` is `-`). `dbNetwork::term(chip_bump_pin)` returns null and
   STA cannot descend through the bump; cross-chiplet paths terminate at
   the boundary. Today silent.

Both are diagnostic-only — runtime null-checks in `dbNetwork::term()`,
`dbNetwork::pin(Term*)`, and `bump_to_chip_net_` already handle the
nullable cases without crashing. The check just surfaces the gap earlier
than the eventual "no paths found" message from `report_checks`.

### Why not run unconditionally on `read_3dbx`

3DIC design flows have a **blackbox stage** in which chiplets are placed
structurally before their inner blocks / bterms are bound:

- `.bmap` col 5 may legitimately be `-` (chiplet body not loaded yet).
- A `dbChipNet` may exist before any bump-inst is attached (Tcl-built
  nets, design under construction, partial reads).

Treating these as violations during blackbox would generate false-positive
WARN spam on every `read_3dbx` and every intermediate save/restore. The
checks only make sense when STA is **about to consume** the design.

A prior version of this PR landed these as `ODB-0405` / `ODB-0406` inside
`odb::Checker::check()`, which `OpenRoad::read3Dbx` invokes unconditionally
after read. Reverted because of the blackbox false-positive concern; this
TODO captures the proper landing.

### Concrete change

Two acceptable shapes; pick one in the follow-up PR.

**(a) Lazy dbSta-side check on first STA consumption.**
- File: `src/dbSta/src/dbSta.cc`. Add `bool sta_checks_done_ = false;` to
  `dbSta`. On the first call into a path-reporting entry point
  (`findRequireds`, `findClkPins`, equivalent), if `db_network_->has3DicChip()`,
  walk `top_chip->getChipNets()` and per-master `getChipBumps()` once and
  warn on orphan nets / unbound bumps. Set the flag so re-entry doesn't
  re-walk.
- Invalidate on `clear()` / `setTopChip()` so a fresh design re-checks.
- Message ids: reuse `STA-3001` / `STA-3002` (currently free; STA-3001 is
  used by the HIER-master warning — pick fresh ids, e.g. `STA-3010` /
  `STA-3011`).

**(b) Explicit Tcl validation command.**
- File: `src/dbSta/src/dbSta.tcl`. Add `validate_3dic_for_sta` that runs
  the same walks, intended for users to call before `report_checks` /
  `report_clock_skew`. Optionally call it automatically from the existing
  `report_3dic_summary` helper.
- Pro: zero risk of false positives — user opts in.
- Con: requires user discipline; missed call → silent "no paths found".

Recommend (a) with the lazy first-use pattern; mirrors the existing
`bump_to_chip_net_` cache lifecycle.

### Test plan

- Extend `dbSta/test/3dic_cross.tcl` (or a new fixture) with an
  intentionally-orphan `dbChipNet` and verify the warning fires when
  `report_checks` runs but not earlier.
- Confirm `dbSta/test/3dic_get_cells.tcl` (blackbox, unbound bumps,
  no STA traversal) remains silent.

### What this TODO replaces

The reverted `Checker::checkOrphanChipNets` / `Checker::checkBumpPortBindings`
methods and their ODB-0405 / ODB-0406 messages. Git history (this PR's
prior commits) is the reference for what the walks themselves looked
like; the logic is reusable, only the gating changes.

### Out of scope

- Markers in the GUI for orphan chip-nets need `dbMarker::getName()` to
  handle `dbChipNetObj` before `addSource(chip_net)` is safe. Currently
  the switch in `src/odb/src/db/dbMarker.cpp:638` lacks a `dbChipNetObj`
  case and errors `ODB-0290` on any later name lookup. Same for the
  `dbChipObj` (chiplet master) source on unbound-bump markers. Either
  extend the switch first, or skip `addSource` and rely on `setComment`.

---

## File / module references

- `src/dbSta/include/db_sta/dbNetwork.hh` — `chip_bump_vertex_ids_`,
  `block_to_chip_inst_`, `block_disc_`.
- `src/dbSta/src/dbNetwork.cc` — pointer-tag scheme, iterator
  short-circuits to refactor for multi-hier; ObjectId discriminator.
- `src/odb/include/odb/unfoldedModel.h` — `UnfoldedBump`,
  `UnfoldedChip`, `UnfoldedModel::findUnfoldedChip`.
- `src/odb/src/codeGenerator/schema/chip/dbChipConn.json` — extend
  for parasitic fields.
- `src/dbSta/src/dbSta.cc::postRead3Dbx` — parasitic binding entry.
- OpenSTA `Parasitics::makeParasiticNetwork` — existing hook.

## v1 single-level constraint, where it lives in code

- `dbNetwork::parent(Instance*)` line ≈1373 comment: "Single-level
  chip hierarchy in v1; deeper nesting is post-v1."
- `DbInstanceChildIterator` line ≈338 comment: "Chip-insts
  themselves are leaves (Stage 4 model)."
- `dbNetwork::isLeaf(Instance*)` line ≈1441: chip_insts treated as
  leaves unconditionally.
