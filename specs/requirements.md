# Requirements — what STA needs from Osama (3DBlox + unfolded model)

Asks on the odb/3DBlox side (Osama owns `dbChip*` schema + the PR #10588
dbObject unfolded model) that 3DIC STA depends on. Grouped by blocking vs
nice-to-have. References: limitations analysis of PR #10588, `3DIC_TODO.md`,
`mission.md`, `roadmap.md`.

---

## BLOCKING — STA cannot land the track without these

### R1+R2 (MERGED). Path-qualified `dbInst` unfold identity + vertex-id home
**Context shift (Osama, in-progress):** the bump objects (`dbChipBump` /
`dbChipBumpInst` / `dbUnfoldedChipBumpInst`) are being **eliminated** — a bump
is just a `dbInst`. So the two formerly-separate identity problems collapse
into one:
- (old R1) per-bump vertex identity, and
- (old R2) interior leaf-cell unfold identity,

are now **the same problem**: path-qualified `dbInst` identity under
unfolding. Two placements of one chiplet master still alias the same inner
`dbInst*` (the block is shared), so timing collapses for bumps AND interior
cells alike.
- **Need: a shared `dbUnfoldedInst`** (path-qualified instance) STA can tag as
  `Instance*`, with its pins surfaced as `Pin*`, carrying a
  `sta_vertex_id_`-class field (mirroring `_dbITerm`/`_dbBTerm`). One object
  serves bumps and interior cells.
- **Constraint:** if STA tags `dbUnfoldedInst*` (or its pin) as a low-3-bit
  `Pin*`/`Instance*`, total `sizeof(_dbUnfoldedInst) % 8 == 0` (the
  `_dbChipBumpInst` sizeof-20 alignment bug). Add padding if needed.
- **Decisions for Osama:** (a) does `dbUnfoldedInst` land as part of the
  elimination work, or separately? (b) owner — odb-side object (preferred, so
  codegen/serialization is consistent) vs an STA-side `(dbUnfoldedChipInst,
  dbInst)` → record map. (c) vertex-id field on it, odb- or dbSta-owned.
- Without this, duplicated-master timing collapses — the headline goal.
- **Scope grown (odb architect, 2026-07-22):** `dbUnfoldedInst` alone is insufficient
  — dbSta keys timing on iterm/bterm/net, not the inst (vertex ids live on
  iterms). Duplicated-interior support pulls the whole family:
  **`dbUnfoldedITerm`, `dbUnfoldedBTerm`, `dbUnfoldedNet`**, with the
  vertex-id field on the unfolded ITERM (mirror `_dbITerm`). This also makes
  R3(a) (rebuild hook) a hard dependency — vertex-id homes on rebuilt-on-read
  objects need invalidation signaling. NB: the flat bump-pin redesign
  (roadmap) survives the family unchanged — bump identity migrates
  raw→unfolded iterm with the same swap as every interior iterm.

### R3. Stable identity / rebuild contract for unfolded objects — PARTIAL
**Resolved (verified in code):** model is rebuilt on read (`operator>>` →
`constructUnfoldedModel()`), NOT deserialized. `dbUnfoldedBuilder` clears all
unfolded tables and rebuilds from scratch each call → OIDs/pointers are **NOT
stable across rebuilds**. STA contract settled: **treat every rebuild as full
invalidation; re-attach vertex ids after each `constructUnfoldedModel()`.**
**Still open with Osama:**
- (a) a **rebuild-notification hook** so STA knows a rebuild occurred (today no
  signal; STA would have to re-run unconditionally).
  - **Concrete instance found (2026-07-03 ASAP7 field test, fixed):**
    `dbDatabase::triggerPostRead3Dbx` (`dbDatabase.cpp:1210`) rebuilds the
    unfolded model *after* notifying `postRead3Dbx` observers, freeing the
    objects STA indexed in the callback → use-after-free on the first chip-net
    query. Fix = build **before** notifying (pure reorder). STA's net-count
    staleness check cannot see this same-count rebuild — the general hook still
    wanted. See `2026-07-03-asap7-field-test.md` F1.
- (b) persist-vs-rebuild decision (lift no-serial?) — affects save/restore
  after `make_graph`, which currently loses any id on these objects.
- Decision needed: persist the unfolded tables to `.odb` (lift `no-serial`)
  vs keep rebuild-only + STA rebuilds graph ids each time. Save/restore of a
  design after `make_graph` currently loses any id stored on these objects.

### R4. Hierarchical net enumeration semantics — RESOLVED (in code)
Verified in `dbUnfoldedBuilder`: `buildUnfoldedChip` recurses into HIER masters
(pushing `path`); `unfoldNets` runs at top AND per HIER master with its
`parent_path`; each bump resolves path-qualified via
`findUnfoldedChip(concatPath(parent_path, rel_path))` where `rel_path` comes
from `net->getBumpInst(i, rel_path)`. So a chiplet placed twice unfolds its
nets twice with the correct per-placement bumps; cross-level nets resolve via
`concatPath`. maliberty's concern is implemented.
- **Caveat:** built on `dbChipBumpInst`/`dbUnfoldedChipBumpInst` (being
  eliminated) — object types shift, but the concatPath path-resolution
  semantic is the model. Lock with a Track B regression.

---

## IMPORTANT — needed for top-level active-IO + RC (Tracks C/D)

### R5a. Bump endpoint contract — RESOLVED (2026-07-22; implemented)
Per the odb architect: `dbChipBump` is a **purely logical construct** — the
chiplet's external interface merging bterm + iterm into one endpoint. The
loose bindings (optional bterm; inst-but-not-iterm reference) are tightened by
two STA-side network-creation checks (implemented in `setTopChip`, scoped to
bumps **connected to a chip-net**; parse stays permissive for blackbox-stage):
- (a) connected bump must have a bound bterm → `STA-3005`.
- (b) connected bump's inst has exactly **one iterm** → `STA-3006`. This is
  the type-check of the **passive-bump contract**: bumps are passive INOUT
  pads (verified: all 4225 ASAP7 bumps single INOUT PAD pin); active elements
  are interior IO leaf cells with directed pins (TSV_mod etc.). An "active
  bump" is modeled as interior IO cell + passive bump, never by relaxing (b).
- **Spare bumps** (no port/net — 3887/4225 on ASAP7) are legal, exempt, and
  filtered out of pin enumeration.

### R5. Driver/load (direction) at the bump boundary
`dbUnfoldedChipNet::getConnectedBumps()` returns a flat bump vector with no
driver/load semantics. With IO driver/receiver as **interior** chiplet cells
(corrected), net2's real driver/load are the IO-cell iterms one bump-bridge
hop inside each chiplet — direction comes from their Liberty, derived through
the bound `dbBTerm` IoType.
- API present (both directions): `dbChipBump::getBTerm()/setBTerm()` and
  `dbBTerm::getChipBump()`. Remaining ask is runtime/flow, not API: confirm the
  binding is reliably populated post-unfold so STA recovers real direction
  across the boundary (and can potentially drop the v1 BIDIRECT-bump hack on
  net2 — Track C2).

### R6. Interchip parasitics as ODB objects, owned by the hierarchical `dbChip`
For net2 / bond RC (Track D), STA needs the extracted RC network for a
cross-chip net.
- **Correction (Arthur):** parasitics in ODB are **objects, not fields**. A
  net's RC is `dbRSeg` + `dbCapNode`, owned by `dbBlock`, reached via
  `dbNet`. Do NOT add scalar `resistance_`/`capacitance_` to `dbChipConn` —
  that mis-models it and forces a lossy lumped translator. `dbChipConn`
  remains the physical/geometric bond descriptor (thickness, region pair); it
  may inform the extracted value but does not store the RC network.
- **Ownership problem:** the top hierarchical `dbChip` has **no `dbBlock`**,
  so there is no block table to own RC objects for a `dbChipNet`. Interchip
  parasitics should be **owned by the hierarchical `dbChip`, accessed through
  `dbChipNet`** — mirroring `dbBlock → dbNet → dbRSeg/dbCapNode` one level up.
- **RESOLVED (Arthur + Matt, impl started):** create **new** objects
  `dbChipRSeg` / `dbChipCapNode` / `dbChipCCSeg`, owned by the hierarchical
  `dbChip`, backpointing to `dbChipNet`, accessed via `dbChipNet` (mirroring
  `dbBlock → dbNet → dbRSeg/dbCapNode/dbCCSeg`). NOT reuse — `dbCapNode` is
  welded to `dbNet` (`dbId<_dbNet>` backpointer) and every accessor resolves
  against the owning block (`dbNet::getCapNodes()` → `block->cap_node_itr_`);
  reuse would force a generic/variant owner + rewriting all APIs.
- **STA implication of the new objects:** STA needs a **new parasitic
  translator** for the `dbChip*` family (`dbChipNet::getRSegs/getCapNodes/
  getCCSegs` → OpenSTA `makeResistor`/`makeCapNode`/`makeCouplingCap`). Same
  per-segment shape as the `dbNet` walk; different accessors → new but
  familiar code. A cross-chip net's RC spans **two object families** —
  `dbRSeg`/`dbCapNode` (leaf blocks) + `dbChip*` (hier chip) — so the walker
  crosses object-type families at the boundary, stitched by SPEF node-name.
- **Coupling caps (`dbChipCCSeg`) exist in the model.** STA may initially
  ground/lump them (no SI/crosstalk), but the data carries coupling — don't
  assume grounded-only.
- **Stitched-ownership model (Arthur, `arthur_k_approach.png`).** A cross-chip
  net is ONE electrical RC chain spanning multiple owners. For the passive
  RDL-interposer case:
  ```
  Block A net1  → bump1 → RDL net2 → bump2 → Block B net3
  (block-owned)  (hier   (RDL dbBlock,  (hier   (block-owned)
                  dbChip) ChipType::RDL) dbChip)
  ```
  - Bump/bond parasitics owned by the **hierarchical `dbChip`** (block-less).
  - RDL parasitics owned by the **RDL `dbBlock`** (`ChipType::RDL` — the RDL
    chip HAS a block, unlike the top hier chip).
  - Block-internal net RC owned by each chiplet's own block.
  - **Continuity rule:** at each ownership boundary the junction `dbCapNode`
    is the SAME node ("same node in the output SPEFs"). STA must treat it as a
    single parasitic node so the net is not split at the boundary.
- **What STA needs, regardless of object choice:**
  1. Walk the full electrical net across all owners (block → hier-chip bump →
     RDL block → …) as one parasitic network.
  2. Shared-junction-node identity across owners — **RESOLVED (Arthur):**
     `dbCapNode`s are restricted to their block/chip, so the boundary is NOT a
     shared object. Stitch is by **node-name consistency** in SPEF (leaf
     SPEF port name == inter-chip SPEF bump node name). This works for STA
     because that boundary name resolves to a Pin (the chiplet `dbBTerm` /
     bump) — the same point `dbNetwork` already bridges via `term()`/`pin()`.
     - **Condition:** valid only if STA consumes inter-chip RC via **SPEF
       read**. If STA ever reads in-memory `dbRSeg`/`dbCapNode` directly, the
       name match is gone → would need the boundary name recoverable from the
       db objects, or an explicit ODB junction-mapping. Keep SPEF-read as the
       source of truth for inter-chip parasitics to avoid this.
     - **Duplicated-master caveat:** identical leaf SPEFs reuse the same
       internal port names across placements; inter-chip SPEF bump nodes must
       be instance-qualified so each placement matches the right bump. Ties to
       the per-unfold-path identity work (R1/R2, Track A).
  3. **Cap-node → pin binding** at real endpoints. NB: two distinct
     interconnect cases — (a) passive RDL/bump stack
     (`arthur_k_approach.png`), endpoints are block iterms via bumps;
     (b) active IO (`boundary.png`), endpoints are the IO driver output iterm
     / receiver input iterm — which are **interior chiplet cells reached one
     bump-bridge hop in**, so the net2 cap nodes bind to the bump/`dbBTerm`
     boundary nodes and the IO-cell iterms sit inside the chiplet SPEF.
  4. Per-corner support (one RC set per `ParasiticAnalysisPt*`).
- **Pre-route (EST):** `estimate_parasitics` will model the cross-chip net
  more simply pre-routing; detailed RC topology is post-route. STA handles
  both detailed and reduced. (Arthur hasn't scoped the EST model yet.)

### R7. IO-cell location — RESOLVED (mostly moot)
**Corrected:** IO driver/receiver live **inside** the chiplets as ordinary
interior leaf cells, NOT at top level. So there is **no top-level IO-cell
representation to define** and **no `net1b`/`net3b` boundary-handoff problem** —
the boundary is the existing bump ↔ `dbBTerm` bridge, and `net1b`/`net3b` are
ordinary chiplet-interior nets. Remaining ask:
- Confirmation that Arthur's OpenRCX extraction targets `net2` (the active
  cross-chip net between the two chiplets' boundary bumps) and emits a SPEF
  the STA parasitic path can consume, stitched per R6.

---

## NICE-TO-HAVE / hardening (later tracks)

### R8. Mutation callbacks on chip-net / bump-inst
For cache invalidation (`bump_to_chip_net_`), `dbBlockCallBackObj`-style hooks
that fire on `dbChipNet` / bump mutation (create/destroy/addBumpInst/
removeBumpInst) (`3DIC_TODO.md` TODO 4). NB: bump-inst object type shifts with
the elimination (bump → `dbInst`); hook on whatever survives.

### R9. Unbound-bump query — DONE (2026-07-22)
Query exists: `dbChipBump::getBTerm() == nullptr`. STA-side work landed:
connected bumps validated (STA-3005/3006, see R5a); unbound spares filtered
out of `DbInstancePinIterator` and `makeTopCellForChip`. The `make_graph`
null-deref class is closed. Orphan-net case remains diagnostic-only (open,
minor).

### R10. `dbMarker` source-type support for chip objects — STILL MISSING
Verified: `src/odb/src/db/dbMarker.cpp` has no `dbChipNetObj` / `dbChipObj`
case (errors ODB-0290). Needed before GUI markers on orphan chip-nets /
unbound bumps (TODO 5 out-of-scope note). Low priority.

### R11. ETM hookup — RESOLVED (already loaded into STA)
Verified: `read_3dbx` already **loads** each `ChipletDef: external.liberty_file`
into STA — `src/odb/src/3dblox/3dblox.cpp:436` calls
`sta_->readLiberty(liberty_file, cmdScene, MinMaxAll::all(), true)` (parsed in
`dbvParser.cpp:172`). So the vendor ETM `.lib` is reachable AND loaded; no
Osama ask. Remaining is **STA-side** (Track E3 / TODO 3): consult the loaded
`LibertyCell` in `makeTopCellForChip` instead of synthesizing a stub, and
suppress the BIDIRECT fallback when an ETM is present.

### R12. 3dbv path-glob robustness — MINOR (found 2026-07-03)
`baseParser.cpp matchesPattern` supports only a **single** `*` (prefix+suffix; a
second `*` is literal). A two-wildcard liberty/LEF entry in a `.3dbv` matches
**zero** files with **no warning** — silently loaded no std-cell liberty in the
ASAP7 field test, surfacing only much later as `report_checks` "No paths found".
- Ask (odb, minor): warn when a resolved glob matches 0 files; optionally
  support multiple `*`. Workaround: single-`*` patterns per file group.

---

## Open decisions to confirm with Osama (summary)

Only the genuinely-open items remain below; resolved/STA-side reqs are marked
inline in their sections.

| # | Status | Decision still needed | Owner |
|---|--------|-----------------------|-------|
| R1+R2 | **OPEN (blocker)** | Unfolded FAMILY (`dbUnfoldedInst` + `dbUnfoldedITerm`/`BTerm`/`Net`), vertex id on unfolded ITERM — landed with bump-elimination? owner? | Osama |
| R3 | **OPEN (hardened)** | Rebuild-notification hook — now a hard dep of the unfolded family (vertex ids on rebuilt objects); persist-vs-rebuild (save/restore) | Osama |
| R5 | open (flow) | bump→`dbBTerm` binding reliably populated post-unfold? NB: #10077 read order makes net-less bmap rows silently unbindable — defin-side rebind is a possible ask (see A9) | Osama/flow |
| R5a | RESOLVED (implemented) | connected-bump checks STA-3005/3006; passive-bump contract; spares exempt + filtered | — |
| R6 | RESOLVED | new `dbChipRSeg`/`dbChipCapNode`/`dbChipCCSeg` (Arthur+Matt, impl started); STA writes a new translator | Arthur |
| R7 | RESOLVED | IO cells interior; remaining: confirm RCX targets net2 → SPEF | Arthur |
| R4 · R9 · R11 | RESOLVED/DONE | net enumeration done; unbound-bump handling implemented (R9 DONE); ETM `.lib` already loaded — remaining ETM work is STA-side | — |
| R8 · R10 | nice-to-have | callbacks (TODO 4); `dbMarker` chip cases (TODO 5) | Osama |
| R12 | nice-to-have | 3dbv glob: warn on 0-match / support multi-`*` (field test) | Osama |
