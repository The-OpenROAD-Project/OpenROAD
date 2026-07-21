# Roadmap — 3DIC STA

High-level implementation order in **very small, individually verifiable
phases**. Start at duplicated-masters (smallest fix unblocking the biggest
prior-impl gap), then hierarchy, then top-level active IO, then RC. Each phase
should land as its own branch/PR with its own regression test, registered in
CMake **and** Bazel.

Foundation (MERGED): PR #10588 (dbObject unfolded model) + PR #10590
(dbInst↔chip-bump). The STA adapter itself is NOT in HEAD — `postRead3Dbx` is
an empty stub; PR #10417 is an unmerged reference branch. So Track A starts by
*bringing up* the adapter, not refactoring in-tree code. See `tech-stack.md`.

---

## Track A — Cross-chiplet STA + duplicated masters

Split at the line `dbUnfoldedInst` gates. **Track A lands now** on merged code
(10588 + 10590); **Track A' is deferred** to Osama's `dbUnfoldedInst` (he owns
the dbSta migration). Decision context: flat descent into chiplet interiors is
a hard requirement (ETM black-boxing is not always available), so interior
duplicated-master timing genuinely needs per-path interior identity =
`dbUnfoldedInst`. The composite-`Pin*` alternative was rejected (hits the
32-bit `ObjectId` ceiling as a correctness failure, TODO 7; and is throwaway).
ETM and flat-descend coexist **per chiplet** (black-box where a `.lib` exists,
descend where not).

### Track A (lands now — no `dbUnfoldedInst`)

**A1. Bring up the adapter on HEAD (port from PR #10417).**
The adapter is not in HEAD (`postRead3Dbx` is a stub; no 3DIC code in
`dbNetwork.cc`; no `3dic_*.tcl`). Port the still-applicable #10417 mechanisms
(mode gating, chip-aware iterators, `term()`/`pin()` boundary bridges, fat-net,
diagnostics) onto current HEAD. Recreate a minimal single-level cross-chiplet
fixture (`3dic_cross.tcl`). Register in CMake + Bazel.

**A2. Bump-level per-path identity.**
Key the bump `Pin*` and its vertex id on `dbUnfoldedChipBumpInst` (already
per-unfold-path on HEAD). This is the literal "vertex ids not attached to the
unfolded model" fix — duplicated masters get **distinct bump vertices**, no
boundary collapse. Per Osama: base on the current model; he migrates to
`dbUnfoldedInst` if/when the swap lands.

**A3. Flat interior of UNIQUE masters.**
For a chiplet instantiated once, its interior `dbITerm`s are unique → key them
as normal `kDbIterm`, store vertex id on the iterm. Full flat
`ff → … → ff` cross-chiplet paths. (Exactly #10417's proven scope.)

**A4. Drop the old side-map.**
`vertexId`/`setVertexId` for bumps read/write `dbUnfoldedChipBumpInst`; delete
the `chip_bump_vertex_ids_` flat side-map. Re-attach ids on every
`constructUnfoldedModel()` rebuild (R3).

**A5. Duplicated-master HARD-ERROR (revised — PR #10664 review).**
Do NOT descend a *duplicated* master's shared interior — walking the shared
block twice aliases its iterms → collision/crash. **Decision revised (Osama):**
instead of timing a duplicated chiplet as an opaque box, **reject the design
with a hard error `STA-3004`.** Rationale: much downstream code assumes a
chiplet block is placed once, so a partial/opaque graph is a correctness trap;
refusing is safer than silently dropping interior paths. Nested (HIER) masters
likewise hard-error (`STA-3001`) rather than warn. Real duplicated-master
support is Track A' (`dbUnfoldedInst`); nested is Track B.

**A6. Regression.**
(i) unique-master cross-chiplet flat `ff → ff` path + flat-descent cell/pin
iteration (`3dic_cross.tcl`); (ii) duplicated-master design asserting the A5
`STA-3004` hard-error (`3dic_get_cells.tcl`). Register in CMake + Bazel.

### Track A' (deferred — gated on `dbUnfoldedInst`)

**A'1. Vertex-id field on `dbUnfoldedInst`.**
Add `sta_vertex_id_` (mirror `_dbITerm`) to the per-`(path, interior dbInst)`
object. Audit `sizeof % 8 == 0` if it is a pointer-tag target.

**A'2. Key interior `Pin*`/`Instance*` to `dbUnfoldedInst`.**
Swap interior keying from raw `dbITerm`/`dbInst` to the path-qualified
`dbUnfoldedInst`; lift the A5 guard so duplicated interiors descend. Osama owns
this dbSta migration.

**A'3. Duplicated-interior regression.**
Same master twice; assert two **independent flat interior** setup paths
(`X/ff → … → Y/ff` distinct from each placement). Remove the A5 warning for
descendable duplicated masters.

---

## Track B — Chiplet hierarchy (chiplet-of-chiplets)

**B1. Path-aware `Instance*` for chip-insts.**
Encode `dbUnfoldedChipInst*` as `Instance*` (path-tagged) instead of raw
`dbChipInst*`. Update `instance(chip_bump_pin)` to return the path-qualified
instance.

**B2. Recursive child/pin iterators.**
`DbInstanceChildIterator` recurses into HIER chip-insts; `DbInstancePinIterator`
yields the path-qualified unfolded-instance pins at every level.
`DbNetPinIterator` yields unfolded-instance-keyed pins (bump = `dbInst`/iterm).

**B3. Path-aware parent + block lookup.**
`parent(Instance*)` walks one step up the unfold path. Replace
`block_to_chip_inst_` with an unfold-path-aware lookup
(`dbUnfoldedChipInst` → block).

**B4. Remove the HIER-master warning + nested regression.**
Wire `dbStaCbk` on leaf blocks reached through HIER chiplets (dedupe by
`dbBlock*`). Drop STA-3001 HIER warning. New fixture: `top → mid×2 → leaf`;
assert paths across all unfolded leaves; assert warning absent.

---

## Track C — Active top-level interconnect (boundary.png)

NB: IO driver/receiver are **interior chiplet leaf cells** (corrected), so v1's
existing chiplet-interior descent + bump bridge already time them. This track
is mostly about net2 being *active* (real driver/load + RC), not new top-level
cells. Much of it composes with Track D.

**C1. Confirm interior IO cells time across the bump boundary.**
Verify the existing descent forms `buf1 → IO_driver1 → [bumpA] → net2 →
[bumpB] → IO_receiver1 → buf2` with Liberty arcs on the IO cells. Likely works
with no new code beyond Track A/B — this is a validation step.

**C2. Real direction on net2; revisit the BIDIRECT-bump hack.**
net2 now has a real driver (IO_driver1 out) and real load (IO_receiver1 in)
one bridge hop inside each chiplet. Check whether v1's BIDIRECT-bump
workaround is still needed on net2 or can be dropped where real direction
exists.

**C3. Active-interconnect regression.**
Fixture matching boundary.png (IO cells inside chiplets). Assert constrained
`ff1 → ff2` path includes IO driver + receiver cell delays (and net2 RC once
Track D lands).

---

## Track D — Parasitics on top interconnect (net2 RC)

**D1. Interchip parasitic objects (odb — Arthur, in progress).**
New `dbChipRSeg`/`dbChipCapNode`/`dbChipCCSeg`, owned by the hierarchical
`dbChip`, reached via `dbChipNet` (decided w/ Matt; see `requirements.md` R6).
`dbChipConn` stays the geometric bond descriptor. STA work depends on Arthur's
object API landing; track his branch.

**D2. New STA parasitic translator for the `dbChip*` family.**
Walk `dbChipNet::getRSegs/getCapNodes/getCCSegs` → OpenSTA
`makeResistor`/`makeCapNode`/`makeCouplingCap`, per corner
(`ParasiticAnalysisPt*`). Mirrors the existing `dbNet` walk. A cross-chip net
spans two object families (leaf-block `dbRSeg`/`dbCapNode` + hier-chip
`dbChip*`) stitched at the bump/`dbBTerm` boundary by SPEF node-name. Coupling
caps may be grounded/lumped initially.

**D3. RC regression + reporting.**
Compare slack vs zero-delay baseline; delta matches analytic RC delay. Extend
`report_3dic_summary` with per-conn R/C. New diagnostics STA-3004/3005.

---

## Track E — Hardening (as needed, pull forward if blocking)

- **E1.** STA-gated structural-integrity checks (orphan chip-nets, unbound
  bumps) — filter unbound bumps out of pin enumeration to kill null-deref
  crash (`3DIC_TODO.md` TODO 5).
- **E2.** Callback-driven `bump_to_chip_net_` invalidation (TODO 4).
- **E3.** ETM-bound chiplets — real vendor `.lib` when present (TODO 3).
- **E4.** Internally-hierarchical chiplets under flat top (TODO 6).
- **E5.** ObjectId 20-bit-per-block ceiling — global id allocator (TODO 7).
- **E6.** `write_verilog` on the 3DIC model (found in the 2026-07-03 ASAP7 field
  test). Output is currently invalid: interior leaves are written as direct
  children of the synth top under chiplet-local names → duplicate instance
  identifiers across dies; and `makeTopCellForChip` emits a portless top. Needs
  path-qualified/unique interior names (shares Track A′ identity) + top-port
  synthesis. `VerilogWriter` itself is faithful — fix is in dbSta.
- **E7.** Chip-net pin enumeration for `report_checks -through`. `DbNetPinIterator`
  yields only cell-less bidirect bump/BTerm bridge pins (not on the arc), so
  `-through` an f2f net finds nothing while `-through` the interior receiver iterm
  works. Surface the bridged interior iterms (or give the bump pin the on-arc
  vertex). dbSta-only; no `src/sta/` change. (2026-07-03 field test.)

---

## Ordering rationale

- **Track A lands now**; **A' waits on Osama's `dbUnfoldedInst`** (interior
  duplicated-master descent). A delivers real value alone: flat cross-chiplet
  timing for unique masters + non-collapsing bumps for duplicated ones.
- A before B: path-qualified chip-inst identity (A/B1) is the prerequisite for
  hierarchy. B's *unique-master* hierarchy can build on A; B's duplicated
  nested interiors share A''s `dbUnfoldedInst` dependency.
- C/D after A: active-IO + RC compose on a correct graph; doing them first
  would bake assumptions that break under duplicated masters.
- E pulled forward only when a crash/blocker (e.g. unbound-bump segfault)
  obstructs an earlier track.
