# 3DIC STA Notes

Living developer-facing doc for the 3DIC STA enablement work landing in
`src/dbSta` and `src/odb`. Captures the mental model + invariants that are
not obvious from reading the code alone. Update incrementally as new
stages land.

**v1 scope.** This PR lands cross-chiplet STA for **single-level**
hierarchies (one top `dbChip` with direct `dbChipInst` children, no
nested chiplets) under a **zero-delay** bond model. Two known gaps —
multi-hierarchical 3DIC support and `dbChipConn` RC parasitics —
are out of scope here and documented as concrete follow-up work in
[`3DIC_TODO.md`](3DIC_TODO.md). The single-level assumption is
load-bearing in `dbNetwork::parent(Instance*)`,
`DbInstanceChildIterator`, and `dbNetwork::isLeaf(Instance*)`.

## Mental model: how a chiplet bump becomes an STA Pin

OpenSTA reasons in terms of `Network` abstractions: Instance, Pin, Net,
Port, Cell. The timing engine doesn't care what the underlying database
object is; it only needs the relationships (`instance(pin)`, `net(pin)`,
`port(pin)`, `term(pin)`, etc.) to walk the design.

In a 2D / single-chip flow the mapping is:

| STA concept       | OpenDB object                             |
|-------------------|-------------------------------------------|
| Top instance      | the chip's `dbBlock` (`Sta` synthesizes a top `Cell`) |
| Leaf instance     | `dbInst`                                  |
| Hierarchical inst | `dbModInst` (Verilog `-hier` flow)        |
| Leaf-cell port    | `dbMTerm`                                 |
| Pin on leaf inst  | `dbITerm`                                 |
| Top-level port    | `dbBTerm`                                 |
| Hier-pin handoff  | `dbModBTerm`/`dbModITerm`                 |
| Net               | `dbNet` / `dbModNet`                      |

For 3DIC (3DBlox), the same master/instance pattern repeats one level up:

| 2D world          | 3DIC world             |
|-------------------|------------------------|
| `dbMaster`        | `dbChip`               |
| `dbInst`          | `dbChipInst`           |
| `dbMTerm`         | `dbChipBump`           |
| `dbITerm`         | `dbChipBumpInst`       |
| `dbNet`           | `dbChipNet`            |

So a **bump on a placed chiplet** (`dbChipBumpInst`) plays the same role
a **terminal on a placed standard cell** (`dbITerm`) plays — it is "one
place where a net touches an instance." That is exactly STA's definition
of a `Pin`. The dbSta adapter reuses STA's existing `Pin*` abstraction
without inventing a parallel `dbChipITerm` / `dbChipMTerm` schema.

The crucial bridge for cross-chiplet timing is `Network::term(Pin*)`. For
a chip-bump-inst Pin, `term()` returns the *master bump's `dbBTerm`*,
which is an ordinary boundary port on the chiplet's internal `dbBlock`.
STA's `visitConnectedPins` then walks Pin → Term → Net into the chiplet's
own netlist and continues the trace. No new traversal code is required —
the existing hierarchical Network machinery already handles it.

## Pin pointer-tag scheme (and its alignment requirement)

`dbNetwork` distinguishes the four kinds of `Pin*` it produces by reusing
the lower 3 bits of the pointer as a type tag:

```cpp
enum class PinPointerTags : std::uintptr_t {
  kNone = 0U,
  kDbIterm = 1U,
  kDbBterm = 2U,
  kDbModIterm = 3U,
  kDbChipBumpInst = 4U,   // Stage 2+
};
```

Encoding is `(char*)raw_ptr + tag`; decoding strips the low 3 bits. The
scheme depends on **every base pointer being 8-byte aligned** so the tag
bits don't collide with real address bits.

Heap allocations from `malloc` / `operator new` return 16-byte-aligned
addresses on x86_64 Linux. Inside an OpenDB `dbTable<T>` page, objects
are packed with stride `sizeof(T)`. If `sizeof(T)` is not a multiple of
8, every other object lands on a 4-aligned address and the encoding
silently produces `kNone` for it.

`_dbObject` is 8 bytes. `_dbChipBumpInst` had three `dbId<T>` fields
(4 bytes each), giving `sizeof = 20`. Half the objects in a `dbTable`
page ended up 4-aligned. The Stage-4 symptom was a sporadic
`ORD-2018 "Pin is not ITerm or BTerm or modITerm."` for whichever bump
happened to fall on the bad stride.

**Rule of thumb for any new dbObject that is the target of a pointer
tag**: ensure `sizeof(T) % 8 == 0`. Either size the class accordingly
when adding fields, or add an explicit 4-byte padding field. Field-by-
field serialization (`operator<<` / `operator>>`) is unaffected; only
the in-memory stride matters.

## How chip-aware behavior is gated

A single predicate, `dbNetwork::has3DicChip()`, returns `true` when the
top is a hierarchical `dbChip` (no own `dbBlock`, has child
`dbChipInst`s). Iterators and accessors check this **first**, before
falling through to the legacy hierarchical (Verilog `dbModInst`) or flat
paths. The three modes are independent:

```
has3DicChip()          ->  chip-hier iteration (Stage 3+)
hasHierarchy()         ->  Verilog module hier (existing)
neither                ->  flat single-block (existing)
```

These do not auto-toggle each other. `read_3dbx` does not set
`hasHierarchy`; Verilog hier read does not set `has3DicChip`.

## Iterator hardening

`DbInstanceChildIterator` and `DbInstancePinIterator` carry several
`dbSet<...>::iterator` members for the various iteration modes. A
default-constructed `dbSet` iterator has **undefined `!=` behavior** —
comparing two default-constructed iterators may not return true, leading
to a deref of an invalid object.

Chip-aware branches in these iterators use explicit `chip_walk_` /
`chip_inst_` bool flags and **short-circuit** `hasNext` / `next` before
touching the unrelated iterators. Any future iterator that adds a new
mode must do the same — do not rely on default-constructed iterators
behaving as "empty."

## dbChipNet ↔ STA Net encoding

A `dbChipNet*` is encoded as `Net*` via plain `reinterpret_cast` (no
pointer tag). Decoded at runtime via `dbObject::getObjectType() ==
dbChipNetObj`. Same scheme that distinguishes `dbNet` vs `dbModNet`.

Reasons tag bits are unused for `dbChipNet`:
- No alignment hazard like Stage 4's `dbChipBumpInst` (we never add a
  small offset to the pointer).
- A single `Net*` type-tag is enough for callers; the existing two-out-
  param `staToDb(Net*, dbNet*&, dbModNet*&)` was extended with a silent
  `dbChipNetObj` case so legacy callers see "neither" and skip.

When extending: if you ever add an offset-style tag to `dbChipNet*`,
audit `_dbChipNet` size for the 8-byte stride rule (the same one that
bit `_dbChipBumpInst`).

## Bump-inst ↔ chip-net reverse lookup (lazy)

`net(Pin*)` for a chip-bump-inst pin needs the owning `dbChipNet`.
odb stores the forward relationship (`dbChipNet::getBumpInst(i, path)`);
the reverse lookup is built once in `dbNetwork::bump_to_chip_net_`
on first call via `ensureBumpToChipNetCache()`. Pattern:

```
if (bump_to_chip_net_.empty() && top_chip_ has chip-nets) {
  walk getChipNets(); walk getBumpInst(i); fill map.
}
```

Reset in `clear()` because `top_chip_` resets to nullptr and the dbObject
table may be torn down.

**Limitation:** if chip-nets are mutated after the first cache hit (e.g.
`addBumpInst` called later), the cache is stale and the new mapping
won't be picked up. In normal flows the chip-net table is built during
`read_3dbx` and immutable afterward, so this is acceptable. Stage 6+ may
add `dbBlockCallBackObj`-style notifications if mutation becomes a
supported flow.

The `dbChipNet::getBumpInst(i, path)` API gives a hierarchical path
(`std::vector<dbChipInst*>`) along with the bump-inst. The cache and the
`DbNetPinIterator` currently drop the path because v1 is single-level
hierarchy. Nested chip hierarchies (post-v1) will need to keep paths to
disambiguate identically-named bumps under different chip-inst trees.

## STA VertexId storage for chip-bump pins (side-map)

OpenSTA's `Graph::makePinVertices` calls `network_->setVertexId(pin, vid)`
so it can later resolve a `Pin*` back to its timing-graph `Vertex` via
`network_->vertexId(pin)`. For regular pins the id is stored on the odb
object itself:

- `_dbITerm` has `sta_vertex_id_` (see `dbITerm.h`).
- `_dbBTerm` has the same.

`_dbChipBumpInst` has **no such field**. Adding one would be an odb
schema change. Instead, `dbNetwork` keeps a side-map:

```cpp
std::map<odb::dbChipBumpInst*, VertexId> chip_bump_vertex_ids_;
```

`setVertexId(chip_bump_pin, id)` writes there; `vertexId(chip_bump_pin)`
reads. Reset in `clear()` since rebuild may invalidate old ids.

**Why this matters**: if either accessor is silent for a new pin type,
the consequence is non-obvious and downstream. `setVertexId` no-ops →
`vertexId` returns `object_id_null = 0` → `Graph::vertex(0)` returns
null → `bfs.enqueue(null)` in `ClkNetwork::findClkPins` segfaults far
from the actual bug. Pattern lesson: every new `Pin*` tag the network
yields must have both `setVertexId` and `vertexId` branches even if the
storage is just a `std::map`.

## How a cross-chiplet timing path is built

This is the full chain — useful when debugging why a path stops short.

```
                  chipA  (uniquely-owned master flop_chip_a)
   .---------------------------------------------.
   |  ff.CK <- clk                               |
   |    |                                        |
   |   ff.Q -> n1 -> inv.A   inv.ZN -> n2 ->     |
   |                                buf.A        |
   |                                buf.Z -> q ----+
   .---------------------------------------------.|
                                                  |  bump_q (chip-bump-inst)
                                                  |  direction OUTPUT
                                                  v
                              bridge (dbChipNet on top dbChip)
                                                  ^
                                                  |  bump_d (chip-bump-inst)
                                                  |  direction INPUT
   .----------------------------------------------+
   |  d <- buf.A   buf.Z -> n1 -> inv.A           |
   |                              inv.ZN -> n2 -> |
   |                                       ff.D   |
   |                                              |
   |  ff.CK <- clk                                |
   .----------------------------------------------.
                  chipB  (uniquely-owned master flop_chip_b)
```

Five distinct layers of accessor work make this traceable:

1. **Pin enumeration.** `DbInstancePinIterator(chip_inst)` yields `dbChipBumpInst` pins via the chip-inst's region-insts × bump-insts. `topInstance` childIterator flattens: each chip-inst's master block's dbInsts are also yielded as leaves so STA's `Graph::makePinVertices` runs for `ff/CK`, `inv/A`, `buf/Z`, etc.
2. **Net enumeration.** `DbInstanceNetIterator(top_instance)` yields top-level `dbChipNet`s. `DbNetPinIterator(chip_net)` walks `chip_net->getBumpInst(i, path)` and yields each bump-inst Pin.
3. **Forward bridge.** `term(chip_bump_pin)` returns the master bump's `dbBTerm` (chiplet inner BTerm) so `Network::visitConnectedPins(Pin*)` descends Pin → Term → inner-net → inner-ITerm-pins.
4. **Reverse bridge.** `pin(Term* = inner_bterm)` looks up `bterm->getChipBump()`, finds the bump-inst on the chip-inst owning the chiplet block (via `block_to_chip_inst_`), and returns it as a Pin. This lets `Network::visitConnectedPins(Net*)`'s termIterator walk ascend from a chiplet inner net up to the chip-net.
5. **Instance/parent attribution.** `instance(inner_iterm_pin)` returns the inner dbInst; `parent(inner_dbInst)` returns the chip-inst owning the block. `instance(inner_bterm_pin)` returns the chip-inst (NOT `top_instance_`) so STA's `isTopLevelPort` correctly classifies it as internal — without this, `Sdc::pinCaps` calls `port(pin)` on `top_cell_` and crashes when no matching Port exists.

## block_to_chip_inst_ and the shared-master limitation

`std::map<odb::dbBlock*, odb::dbChipInst*> block_to_chip_inst_` is built in
`setTopChip`. It only contains chiplet master blocks referenced by **exactly
one** chip-inst:

```
chip-inst A (master=flop_chip_a)  ->  flop_chip_a block: unique -> in map
chip-inst B (master=flop_chip_b)  ->  flop_chip_b block: unique -> in map
chip-inst C (master=SoC)          \
chip-inst D (master=SoC)          /  SoC block: refcount=2     -> NOT in map
```

Chip-insts whose master is in the map become "descendable":

- `isLeaf(chip_inst)` returns true (chip-bump pins get Vertices), but the
  `topInstance` childIterator flat-walk adds the master block's inner
  dbInsts to the DFS frontier so they also get Vertices.
- `parent(inner_dbInst)` returns the owning chip-inst.

Chip-insts whose master is NOT in the map (shared) stay as leaves with no
descent — STA sees them as opaque boxes with bump pins only. The current
implementation cannot timing-analyze the bodies of two chip-insts that
share a master `dbChip` because their inner dbInst pointers alias.
Supporting shared masters requires a virtual `(chip_inst, dbInst)`
instance encoding, deferred post-v1.

## Closing the flop-to-flop loop — Stage 7 wiring (LANDED)

A chip-bump and its chiplet-side inner BTerm form a single hierarchical
boundary. STA's `isDriver(pin) = (isLeaf && output) || (isTopInstance &&
input)` requires the pin to be a driver to participate in wire-edge
formation. With a load-only direction on either side of the boundary,
`Graph::makeWireEdgesFromPin` skips the bump and no cross-chiplet edge
forms in one direction.

**Fix (two parts):**

1. `direction(chip_bump_pin) = PortDirection::bidirect()`. Bidirect is
   both driver and load, so `makeWireEdgesFromPin` runs on every chip
   bump regardless of the underlying BTerm's IoType. As a side effect,
   `Graph::makePinVertices` allocates dual vertices (load + driver) for
   each chip-bump.
2. `dbNetwork::visitConnectedPins(Net*)` chip-net branch: after
   `visitor(bump_pin)`, recurse via `term(bump_pin) → net(term) →
   visitConnectedPins(inner_net, ...)` so STA's **fat-net** wire-edge
   model sees the chiplet's leaf loads through the boundary. Without
   this, the visitor stops at the bump and the path terminates at
   `chipB/d` (the inner BTerm).

The fat-net model collapses the boundary: a wire edge goes directly from
`chipA/buf/Z` (inner driver) to `chipB/buf/A` (inner load across the
chip-net), skipping bump pins entirely. So an inner-LibertyCell-less
chip-bump never needs its own timing arc to carry the *data* path — the
edges form across it.

After Stage 7 the test reports a constrained `chipA/ff → chipB/ff`
setup check with real Liberty delays.

## Anchoring create_clock on chip-bump pins (LANDED)

The natural anchor form

```tcl
create_clock -name clk -period 1.0 [get_pins -of_objects [get_nets clk_top]]
```

now produces a constrained `Path Group: clk` setup check identical to
the inner-CK form. Two fixes were required on top of Stage 7:

**Fix A — Chip-bump pins report BIDIRECT; no synthesized timing arc.**
`dbNetwork::makeTopCellForChip` builds one synthetic **`ConcreteCell`**
per chip master (via the public `makeCell`/`makePort`), with a `Port`
per chip-bump bterm. It has no `LibertyCell` binding, so
`libertyPort(chip_bump_pin)` is null and `dbNetwork::direction(pin)`
falls through to its `PortDirection::bidirect()` branch.

Because the pin is BIDIRECT, `Graph::makePinVertices` allocates **two**
vertices for it — a load and a driver — and `create_clock` seeds the
clock arrival on **both** (`Search::findClockVertices` /
`seedClkArrivals` insert both). The driver vertex then fans out into the
chiplet body through the fat-net wire-edge model (see Stage 7), so the
clock reaches each chiplet's internal CK. **No instance/timing arc is
synthesized** — the load and driver vertices do not need to be joined by
an edge, since both are seeded directly.

(Historical note: an earlier revision synthesized a `LibertyCell` with a
zero-delay combinational self-arc per bump via OpenSTA's private
`LibertyBuilder`. That was dropped — the private header is not part of
OpenSTA's public include API, and the arc turned out to be unnecessary
for propagation. See the wire-load section for its QoR side effect.)

**Fix B — Per-block discriminator in `getDbNwkObjectId`.** Each chiplet
`dbBlock` numbers its iterms/bterms/insts/nets from 1. Without
disambiguation, "clk" net (db_id=1) in chipA collides with "clk" net
(db_id=1) in chipB — `NetSet::contains()` (and `PinSet::contains()`)
sort by `id()` and treat them as equal. `visitConnectedPins`'s
`visited_nets` then dedupes them, so the second chiplet's inner clk
net is silently skipped during chip-net descent, and wire edges from
the clk_top bump driver only reach chipA-side loads.

`dbNetwork::setTopChip` allocates a 1..N discriminator per chiplet
block in `block_disc_`. `blockDiscBits(obj, typ)` stamps the disc into
the upper 4 bits of the encoded `ObjectId` for iterm/bterm/inst/net
when `block_disc_` is non-empty. `id(Net*)` now routes through the
tagged encoder whenever `has3DicChip()` is true (the legacy path
bypassed the encoder in non-hierarchy mode and returned raw
`dnet->getId()`).

Without both fixes the symptom is the same: `all_registers -clock_pins`
still finds both ff/CK pins via `ClkNetwork`'s static BFS, but
`report_clock_skew` reports "No launch/capture paths found" and
constrained `report_checks` reports "No paths found" because the
dynamic Search BFS can't reach the second chiplet's CK pin.

## Wire-load model — fanout includes BIDIRECT chip-bump load vertices

STA does **not** build a per-segment drvr → load chain. `Graph::makeWireEdgesFromPin(drvr_pin)` runs `visitConnectedPins(drvr_pin)` to aggregate **every** drvr and **every** load reachable across the fat net (chipnet + term-descended inner nets), then emits pairwise `drvr × load` wire edges via `makeWireEdge`. Implications for 3DIC:

1. **Cross-chiplet data edge has no intermediate bump-pin hop.** The wire edge `chipA/buf/Z → chipB/buf/A` is created in one shot when `chipA/q_bump` (the BIDIRECT driver iterated from chipA's chip_inst pin walk) is processed. The visitor descends into chipA's inner q dbNet (yielding `chipA/buf/Z`, `chipA.q_bterm`) and chipB's inner d dbNet (yielding `chipB/buf/A`, `chipB.d_bterm`). `FindNetDrvrLoads` then classifies — `chipA/buf/Z` joins `drvrs`, `chipB/buf/A` joins `loads` — and the pairwise loop emits the cross-chip wire edge.

2. **Every BIDIRECT chip-bump appears in BOTH drvrs and loads.** `isDriver` and `isLoad` both return true for `direction == bidirect`. So `chipA/q_bump` and `chipB/d_bump` show up as loads in the aggregated set too, and `chipA/buf/Z` gets extra outgoing wire edges to their **load-side vertices** in addition to the real load `chipB/buf/A`. These edges are harmless for path search — a bump load vertex has no outgoing arc (it is a dead-end load), and `SearchPred` (forward search) only emits paths via the `bidir_drvr_vertex`. No spurious paths form.

3. **Wire-delay calc sees fanout count.** STA's default wire-load lookup uses fanout count (or summed pin caps; chip-bump port cap defaults to 0). Every distinct load on a fat net contributes to the count. If two loads dedup on identity (`PinSet`/`NetSet` sort by `id()` — see the per-block discriminator section above), the fanout under-counts and wire delay drops by one tier.

The `block_disc_` fix is what keeps the three cross-block loads on the
`chipA/buf/Z` fat net distinct — `chipB/buf/A` (the far CMOS input) plus
the two physical microbumps `chipA/q_bump` and `chipB/d_bump`. Fanout = 3,
giving the constrained `slack 0.83`. Verify with
`report_checks -fields {fanout}`: the `chipA/buf/Z` row shows `Fanout 3`.

An earlier revision reported `slack 0.82` (fanout 4). That extra load was
an artifact of the now-removed synthesized self-arc (Fix A history): its
`load → bidir_drvr` edge let a bump's load vertex **re-enter** the chipnet
during `visitConnectedPins`, so the same bond was counted twice. A bond is
a single physical load, so `0.83` / fanout 3 is the physically-correct
value; `0.82` was a one-tier over-count.

## term(Pin*) history (Stage 4–6.5)

`Network::visitConnectedPins(Pin*, visitor)` (sta base, **not virtual**)
calls `net(pin)` first then `term(pin)` and walks `net(term)` to descend.
For a chip-bump pin, `term()` returns the chiplet's inner `dbBTerm`
so STA can trace into the chiplet's internal net.

This was the *cross-boundary bridge* added in Stage 4 but had to be
temporarily disabled in Stage 6 because returning the inner BTerm
caused STA to descend into chiplet bodies whose inner pins had no
Vertex yet — `bfs.enqueue(null)` segfaulted in `findClkPins`.

Stage 6.5 closed the gap and restored `term()`:

1. `topInstance` childIterator flat-walks each uniquely-owned chiplet
   master block's inner dbInsts. Those become leaves so
   `Graph::makePinVertices` allocates Vertices for `ff/CK`, `ff/Q`,
   `inv/A`, `buf/Z`, etc.
2. `parent(inner_dbInst)` → owning chip-inst (via `block_to_chip_inst_`),
   so `pathName` composes `chipA/ff/Q`.
3. `instance(inner_bterm_pin)` → owning chip-inst (NOT `top_instance_`),
   so `isTopLevelPort` correctly returns false and downstream STA does
   not try `port(pin)` on `top_cell_`.
4. `pin(Term* = inner_bterm)` reverse bridge — looks up the bterm's
   chip-bump, walks to the bump-inst on the chip-inst owning this
   block, returns it as a Pin so termIterator walks ascend.
5. `term(chip_bump_pin)` restored.

After Stage 6.5 `report_checks -unconstrained` produces a real
cross-chiplet path:

```
chipA/ff/CK -> ff/Q -> inv/ZN -> buf/Z -> q -> bridge -> chipB/d
```

with Liberty delays from each cell.

## Per-master chiplet Cell

For each unique `dbChip` referenced by some `dbChipInst`, dbNetwork
synthesizes a `Cell` via `makeCell` and a `Port` per master `dbChipBump`
that has a backing `dbBTerm`. This is needed because:

1. `ConcreteNetwork::libertyCell(Cell*)` does not null-check; returning
   `nullptr` from `cell(chip_inst)` would crash inside STA's property
   accessors. Having a non-null Cell with `libertyCell() == nullptr` is
   the safe, expected state.
2. `name(Pin*)` → `pathName(Pin*)` → `portName(pin)` → `name(port(pin))`
   requires `port(pin)` to return a real `Port*` to produce a name.

The map `chip_master_cells_` is reset in `dbNetwork::clear()` since
`ConcreteNetwork::clear()` destroys the libraries that own those Cells.

## Diagnostics

`dbSta::postRead3Dbx` emits two messages: an INFO banner confirming the
STA integration ran and a WARN when nested chiplet hierarchies are
detected (v1 supports flat 3DIC only).

| Msg | Level | Source | When |
|---|---|---|---|
| STA-3000 | INFO  | `dbSta::postRead3Dbx` | Always. `3DIC STA active: <N> chiplets, <M> top-level nets, <K> 3D bond regions, <B> bump pads.` Counts taken at end of `postRead3Dbx`. Tcl-created chip-nets that show up after `read_3dbx` will not be in this count. |
| STA-3001 | WARN  | `dbSta::postRead3Dbx` | A top-level `dbChipInst` references a hierarchical chiplet master (`dbChip::ChipType::HIER`). v1 only wires `dbStaCbk` on direct chiplet `dbBlock`s and only keys identity by raw `dbChipBumpInst*`; nested chiplets need UnfoldedModel-driven callback wiring and per-unfold-path identity. See `3DIC_TODO.md` TODO 1. |

Two structural-integrity checks — orphan chip-nets and unbound chip-bump
ports — are deferred to a follow-up PR. They are not violations during the
**blackbox stage** (chiplets placed structurally before inner blocks /
bterms are bound), so they cannot run unconditionally during `read_3dbx`.
See `3DIC_TODO.md` TODO 5.

Tcl helper `report_3dic_summary` (in `dbSta.tcl`) prints the same
counts plus per-chiplet-instance reference names — useful as a
post-read sanity check or to dump the structural state mid-flow.

## Fixture authoring notes

Non-obvious gotchas when adding new 3DIC tests:

1. **`.bmap` 5th column binds bump → chiplet port.**
   Format per line: `<bump_inst_name> <BUMP_macro> <x> <y> <bterm_name> <signal>`.
   `bterm_name = "-"` leaves the bump unbound — STA cannot cross that
   boundary. Always set the 5th column to a real chiplet `dbBTerm` name
   unless the test specifically exercises unmapped bumps.

2. **`BUMP` macro center offset constrains `.bmap` XY range.**
   `ThreeDBlox::createBump` (`src/odb/src/3dblox/3dblox.cpp:599-602`)
   computes the bump `dbInst` origin as
   `bmap.x * dbu_per_micron - bbox.xCenter() + chip.offset.x`. With
   `fake_bumps.lef`'s `BUMP` macro at `SIZE 29 BY 29`, `bbox.xCenter() =
   29000 DBU`. So `.bmap` XY must be ≥ ~14.5µm to keep the origin
   non-negative, and the chiplet region must be large enough to contain
   the resulting origin — otherwise `Checker::checkBumpPhysicalAlignment`
   fires ODB-0463.

3. **`Connection:` block grounds chiplets.**
   `Checker::checkLogicalConnectivity` requires every chiplet to be
   transitively reachable from a "ground" node (PCB / package side).
   Declare one virtual one-sided `Connection:` with `bot: ~` (or `top:
   ~`) for the bottom-most chiplet:
   ```yaml
   Connection:
     to_pkg:
       top: chipA.regions.front_reg
       bot: ~
   ```
   Without this, ODB-0206 (no ground group) + ODB-0151 (floating chip
   sets) fire. The 3DBlox YAML parser rejects a Connection that
   omits either `top` or `bot` outright — use `~` for the null side.

4. **Physical bump alignment vs logical net assignment.**
   When two chiplets stack with `orient: MZ` and share a bump XY,
   `Checker::checkBumpPhysicalAlignment` requires both bumps at that XY
   to belong to the same top-level `dbChipNet`. Mismatch → ODB-0208.
   Verify by reading the top Verilog and the per-chiplet `.bmap` files
   together: e.g. `chipA.q → bridge` must share its `.bmap` XY with
   `chipB.d → bridge`, not with `chipB.q → out_top`.

5. **Distinct `dbBlock` per chiplet definition is recommended.**
   Two `dbChipInst`s of the same `dbChip` master share the master's
   `dbBlock`. dbSta filters such shared-master blocks out of
   `block_to_chip_inst_` (see "block_to_chip_inst_ and the
   shared-master limitation"), so inner `dbInst`s of a shared master
   become invisible to `leafInstanceIterator`. For path tests, give
   each chiplet its own `ChipletDef:` with a distinct `.def`.

## File map

| Purpose                              | File                                       |
|--------------------------------------|--------------------------------------------|
| Network adapter                      | `src/dbSta/src/dbNetwork.cc`               |
| Adapter declarations                 | `src/dbSta/include/db_sta/dbNetwork.hh`    |
| `postRead3Dbx` entry point           | `src/dbSta/src/dbSta.cc`                   |
| odb 3DBlox schema                    | `src/odb/include/odb/db.h` (`dbChip*`)     |
| odb 3DBlox impls                     | `src/odb/src/db/dbChip*.cpp`               |
| Unfolded model (flat geometric view) | `src/odb/include/odb/unfoldedModel.h`      |
| Tcl regression                       | `src/dbSta/test/3dic_get_cells.tcl`        |

## Stage tracking

Stage-by-stage TODOs, decisions, and known unknowns live in the
plans-side document. The repo doc captures only the durable invariants
that future contributors need to understand the code.
