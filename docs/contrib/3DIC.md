# 3DIC STA Notes

Living developer-facing doc for the 3DIC STA enablement work landing in
`src/dbSta` and `src/odb`. Captures the mental model + invariants that are
not obvious from reading the code alone. Update incrementally as new
stages land.

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

## Hierarchical boundary pins need dual vertices (Stage 7 work)

A chip-bump and its chiplet-side inner BTerm form a single hierarchical
boundary. Each side has dual role:

| Pin | Outside view (chip-net side) | Inside view (chiplet net side) |
|---|---|---|
| `chipB.bump_d` (INPUT) | load of `bridge` | driver of `d`-bterm |
| `chipB.d-bterm` (INPUT IoType) | driven by `bump_d` | driver of `chipB` internal `d` net |

STA's `isDriver(pin) = (isLeaf && output) || (isTopInstance && input)`
classifies based on one direction value. With `direction = INPUT` and
`instance = chip-inst` (leaf), both pins are loads — no
`makeWireEdgesFromPin` runs from either side, so no edges from `bump_d` to
`d-bterm` or from `d-bterm` to `ff.D` get created. The launch-side works
(driver `chipA.bump_q` reaches both bridge loads and chipA's q-net
loads via `term()` descent), but capture-side stops at `chipB/d`.

The correct STA model is **two vertices per hierarchical pin** — a
load-side vertex and a driver-side vertex — connected by an internal arc.
STA already supports this via `direction(pin)->isBidirect() == true`:
`Graph::makePinVertices` allocates `vertex` and `bidir_drvr_vertex`
both. Stage 7 will override `dbNetwork::direction(chip_bump_pin)` to
return BIDIRECT and verify the resulting edge structure closes the
flop-to-flop loop.

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
