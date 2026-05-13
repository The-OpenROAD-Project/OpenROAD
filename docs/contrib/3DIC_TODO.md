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

### Test plan

- Extend `dbSta/test/3dic_cross.tcl` with a second test fixture
  (`3dic_cross_hier.tcl`?) that places `mid` chip twice at top, with
  one leaf chiplet inside each `mid`. Verify constrained setup check
  forms across the four leaf instances.
- Verify `report_3dic_summary` count of chiplets matches the number of
  UNFOLDED instances, not just the top-level `dbChipInst` count.

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
