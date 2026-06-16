# syn IR

The syn intermediate representation is a flat, technology-independent netlist used by OpenROAD's synthesis submodule. Its design is inspired by [prjunnamed](https://github.com/prjunnamed/prjunnamed)'s IR, translated to C++ and adapted for OpenROAD use case.

## Core Types

### Net

A `Net` represents a single-bit signal. Three constants are available:

- `Net::zero()` -- logic 0
- `Net::one()` -- logic 1
- `Net::undef()` -- X (undefined)

All other Nets are produced as instance outputs by `Graph::add`.

One additional out-of-band value exists for transient use. It does not refer to a real instance output:

- `Net::sentinel()` -- an algorithmic dead/unset/end-of-list marker for pass-internal use. For example, `normalize()` initializes its old-to-new net map to `sentinel()` everywhere and overwrites entries as instances are emitted; the cut-enumeration pass uses `sentinel()` to terminate fixed-size cut arrays; frontends fill a `Bundle` with sentinels initially and replace them as connections are made, any leftover sentinel runs are then swept into explicit `Dangling` instances.

### Bundle

A `Bundle` is an owned multi-bit signal bundle. It supports random access (`v[i]` returns the i-th `Net`), `len()`, `empty()`, `slice()`, `concat()`, and extend operations. `Bundle` is always one-dimensional.

Factory functions: `Bundle::zero(w)`, `Bundle::ones(w)`, `Bundle::undef(w)`, `Bundle::sentinel(w)`, `Bundle::fromVec(nets)`. The brace-initializer constructor `Bundle{a, b, c}` packs **MSB-first** — `a` is the high bit, `c` is the low bit — opposite to the LSB-first convention of `fromVec`. `Bundle::fromConst(const Const&)` and `toConst()` bridge to the `Const` representation used for constant-folded values.

### BundleView

A `BundleView` is a lightweight, non-owning reference to a multi-bit signal. It can represent either a single `Net` or a slice of a `Bundle`. Both `Net` and `const Bundle&` convert to `BundleView` implicitly. Supports `len()`, `operator[]`, and `slice()`.

`BundleView` is the return type of operation accessors (see below), allowing callers to read operands uniformly regardless of width.

### ControlNet

A `ControlNet` packages a `Net` with a one-bit polarity, used by `Dff` for clock, clear, reset, and enable signals so that active-low controls can be expressed without materializing an explicit inverter in the graph. A positive (active-high) control fires when the net is 1; a negative (active-low) control fires when the net is 0.

A `ControlNet` can also be constant: an always-active control (the net is tied so the polarity-adjusted value is permanently 1) or an always-inactive one (permanently 0). `Dff` uses these for unused control inputs — `enable` defaults to always-active and the other controls default to always-inactive.

## Graph

The `Graph` class is the top-level netlist container.

### Adding instances

```cpp
Graph g;
Bundle a = g.add<Input>("a", 8);
Bundle b = g.add<Input>("b", 8);
Bundle sum = g.add<Adc>(a, b, Net::zero());
g.add<Output>("sum", sum);
```

`add<T>(args...)` allocates the instance, determines its output width, and returns a `Bundle` representing the output bits.

### Resolving nets

```cpp
auto [inst, offset] = g.resolve(net);
```

`resolve(Net)` returns a pointer to the owning `Instance` and the bit offset within that instance's output. This is the primary way to navigate from a net back to the instance that drives it.

### Reading an instance's outputs

```cpp
BundleView outs = g.output(inst);
```

`output(Instance*)` returns a non-owning `BundleView` over the instance's output bits, so callers can iterate (`for (Net bit : outs)`) or index (`outs[i]`) without copying. It is the natural counterpart to `resolve(net)`.

## Instance Hierarchy

All instances derive from `Instance`. `entryType()` identifies the concrete type and `outputWidth()` returns the output width. Three further classification methods drive how passes — most importantly `normalize()` — treat each instance:

- `hasEffects()` — true for instances with observable side effects: `Output`, `Other`, `Target` when its cell is a macro, and `Name` when not tentative. These act as roots for liveness and topological emission in `normalize()`.
- `hasState()` — true for sequential instances: `Dff`, `Other`, and `Target` when its cell has sequentials. `normalize()` emits them immediately on first encounter and defers DFS of their inputs, so feedback paths do not trigger spurious `LoopBreaker` insertion.
- `isSliceable()` — true for the bitwise instances whose output bits can be emitted independently: `Buffer`, `Not`, `And`, `Or`, `Andnot`, `Xor`, `Mux`, `LoopBreaker`, and `Dangling`. `normalize()` exploits this to drop dead bits and their fanin.

Concrete types are queried and accessed via templated helpers on `Instance*`: `is<T>()` returns a bool, `try_as<T>()` returns a casted pointer or `nullptr`, and `as<T>()` asserts the type and returns the casted pointer. These match against base classes — `is<Buffer>()` is true for both `BufferFine` and `BufferWide`.

### Tie-offs

Always single-bit. No inputs.

- `TieHigh` -- constant 1
- `TieLow` -- constant 0
- `TieX` -- constant X (undefined)

### Operations

All operations provide accessors returning `BundleView` (or `Net` for single-bit control signals such as `Mux::sel()` and `Adc::cin()`, or `ControlNet` for `Dff`'s clock/clear/reset/enable).

| Operation | Accessors | Output width |
|-----------|-----------|--------------|
| Buffer    | `a()`     | input width |
| Not       | `a()`     | input width |
| And       | `a()`, `b()` | input width |
| Or        | `a()`, `b()` | input width |
| Andnot    | `a()`, `b()` | input width |
| Xor       | `a()`, `b()` | input width |
| Mux       | `sel()` (Net), `a()`, `b()` | data width |
| Adc       | `a()`, `b()`, `cin()` (Net) | input width + 1 |
| Eq, ULt, SLt | `a()`, `b()` | 1 |
| Shl, UShr, SShr, XShr | `a()`, `b()`, stride | first input width |
| Mul, UDiv, UMod, SDivTrunc, SDivFloor, SModTrunc, SModFloor | `a()`, `b()` | first input width |
| Dff       | `data()`, `clock()`, `clear()`, `reset()`, `enable()` (ControlNets, see [ControlNet](#controlnet)), `initValue()`, `resetValue()`, `clearValue()` | data width |
| LoopBreaker | `a()` | input width |

#### LoopBreaker

`LoopBreaker` is semantically the identity function — its output equals its input — but it carries the meaning "this edge was a combinational back-edge". Frontends do not emit it directly; `Graph::normalize()` inserts a `LoopBreaker` whenever its topological DFS would otherwise revisit a net already on the stack, severing the cycle at a single, identifiable point. Topological-order checks (`checkNormalization()`) then treat `LoopBreaker` — alongside `Input`, `hasEffects()` instances, and stateful instances — as a place where inputs are allowed to follow outputs in the table.

### Non-operations

- `Input` -- design input port (name + width).
- `Dangling` -- a source for intentionally unconnected bits (width). Has no inputs; just produces that many output bits with no driver. Frontends use it to mark signals the source design left floating so they are not silently tied off, and optimization passes treat it as a boundary alongside `Input`.
- `Output` -- design output port (name + Bundle).
- `Name` -- named signal. See [Names and tentative names](#names-and-tentative-names) below.
- `Target` -- target-specific instance.
- `Other` -- generic instance.

#### Names and tentative names

A `Name` carries the source-level identifier for a signal. It holds the name string, a `Bundle` of value bits, a `[from, to)` bit range (so a name can describe a slice of a wider declared signal), an `is_vector` flag (set when the source declaration was a vector), and a `tentative` flag.

Names come in two flavors, distinguished by the `tentative` flag:

- **Non-tentative names** are observable: `hasEffects()` returns true, so `normalize()` treats them as roots in both phases. The signal values they reference are preserved exactly: no bits are dropped, no inverters are revived, and the Name is not split. (Net IDs in the value Bundle still go through the standard remap, including buffer forwarding.)
- **Tentative names** are best-effort labels (e.g. for debugging or human-readable dumps) that the optimizer is free to drop or rewire. `hasEffects()` returns false, so they are filtered out *before* either phase runs. `Graph::makeNamesTentative()` flips every existing `Name` into this mode in one shot.

During `normalize()` tentative names are pulled out of the graph and reattached after the table is rebuilt:

- If a referenced bit is dead, `normalize()` walks back through `Buffer` and `Not` instances to find a live source. A pure buffer chain is collapsed to its driver; if the bit is reachable only through an odd number of inversions, a single inverter is revived to keep the polarity correct.
- After remap, each tentative name is split on dead bits: every contiguous run of still-live bits becomes one new tentative `Name` instance whose `[from, to)` range is shifted to cover just that surviving slice.

---

## In-Memory Representation

### Net

A `Net` is a plain 32-bit index (`uint32_t`). The three constants are backed by actual tie-off instances pre-allocated at indices 0, 1, 2 in the graph's table. All other indices refer to instance outputs.

### Bundle

`Bundle` uses a discriminated union with four representations:

1. **Empty** -- zero-width signal.
2. **Single** -- one `Net`, stored inline (no allocation).
3. **Consecutive** -- a base `Net` plus a width. Bits are at consecutive indices (`base`, `base+1`, ..., `base+width-1`). This is the common case for multi-bit instance outputs.
4. **Generic** -- a `std::vector<Net>` for arbitrary bit collections (e.g. concatenation of non-adjacent slices).

`fromVec` automatically selects the consecutive representation when applicable.

### BundleView

`BundleView` stores a `Net` (for the single-bit case), a `const Bundle*` (null for the single-bit case, non-null for the Bundle case), plus `offset_` and `len_` fields for slicing into the referenced Bundle.

### NetTableEntry

The base type for all entries in the paged table. Contains a single `uint32_t` header with the entry type packed in the upper 8 bits and the object index (within a page) in the lower 7 bits.

### NetTable

A paged object table. Entries are allocated in blocks of 128 slots, where each slot is 16 bytes (`kSlotSize`, defined as `sizeof(PlaceholderEntry)`).

Addressing uses `NetTableId` (a `uint32_t`): the upper bits select the block and the lower 7 bits select the slot within the block.

Every slot is in one of three states: empty (`kVoid` — e.g. a slot vacated by `removeInstance` or padded in by `padTableTo`), an inline instance, or a `PlaceholderEntry` referencing a heap-allocated instance. Entries never span multiple slots.

### Allocation model

Instances are stored in one of two ways:

**Inline instances** fit in a single 16-byte slot and have a single-bit output. The slot's `NetTableId` is the output `Net`. These include:
- Tie-offs: `TieHigh`, `TieLow`, `TieX` (8 bytes)
- Unary Fine: `BufferFine`, `NotFine` (12 bytes)
- Binary Fine: `AndFine`, `OrFine`, `AndnotFine`, `XorFine` (16 bytes)

**Heap-allocated instances** are allocated with `operator new` and referenced through `PlaceholderEntry` slots in the table. Each output bit occupies one slot as a `PlaceholderEntry` containing an `Instance*` pointer and a bit offset. These include all Wide types, `MuxFine` and `AdcFine` (24 bytes each, exceeds slot size), comparisons, shifts, arithmetic, `Dff`, `LoopBreaker`, and all non-operation types (`Input`, `Dangling`, `Output`, `Name`, `Target`, `Other`).

`Graph::add<T>` chooses the path based on `plan()` (or `sizeof(T)` when no `plan()` exists): if the concrete size is `<= kSlotSize`, construct inline; otherwise heap-allocate.

### PlaceholderEntry

A `PlaceholderEntry` stores:
- `offset_` (`uint32_t`) -- the bit offset within the instance's output.
- `instance_` (`Instance*`) -- pointer to the heap-allocated instance.

The pointer is placed last in the struct to avoid alignment padding, giving exactly 16 bytes total (4 header + 4 offset + 8 pointer).

Heap-allocated instances store a `baseIndex_` field (the `NetTableId` of their first output slot) so they can be relocated when `normalize()` rebuilds the table.

### Output addressing

For inline instances, the slot's `NetTableId` is the output `Net`.

For heap-allocated instances, each output bit has a `PlaceholderEntry` slot. `Graph::resolve(Net)` checks the entry type: if it is a placeholder, it returns `{ph.instance(), ph.offset()}`; otherwise the slot holds an inline instance and the offset is 0.

### Normalization

`Graph::normalize()` rebuilds the table in topological order, performing per-bit dead code elimination and instance splitting along the way. The result satisfies the invariants checked by `checkNormalization()`: every combinational input is defined before its use, and any combinational cycle is broken by an explicit `LoopBreaker` instance.

1. **Liveness analysis (per-bit dead code elimination).** Roots are instances where `hasEffects()` is true (`Output`, `Other`, `Target` when the cell is a macro, `Name` when not tentative) plus `Input`. Tentative `Name` instances are pulled out of the graph and saved aside. Liveness propagates backward bit-by-bit: for each live net, `Instance::visitSlice(bit_offset, ...)` is called, which on bitwise ops (`Buffer`, `Not`, `And`, `Or`, `Andnot`, `Xor`, `Mux`, `LoopBreaker`) only walks the input bit(s) feeding that output bit. Non-bitwise ops fall back to walking all inputs.

2. **Tentative-name fixup.** For each tentative `Name` whose value bit is dead, walk back through buffers and inverters to find a live source. If found, rewrite the bit (or revive a single inverter and use its output) so the name remains valid. Liveness is re-propagated from any newly-revived nets.

3. **Topological emission.** A fresh `NetTable` is allocated and seeded with the three tie-offs at slots 0/1/2. Effect roots and `Input` instances are emitted first, then a DFS walks their input nets:
   - `Buffer` / `LoopBreaker` outputs are forwarded: the net map points the output net at the (already-emitted) fanin instead of emitting a copy.
   - Stateful instances (`Dff`, sequential `Target`, `Other`) are emitted immediately when reached; DFS of their inputs is deferred until all roots have been visited, so feedback paths do not trigger spurious `LoopBreaker` insertion.
   - Sliceable bitwise instances are emitted bit-by-bit; the emitted slice greedily expands to neighboring output bits whose inputs are already visited. Partially-live wide ops thus become narrower instances via `emitSlice`, dropping dead bits and their fanin.
   - For all other instances, all inputs are recursively DFS'd before the instance is emitted.
   - A combinational cycle is broken by inserting a `LoopBreaker` on the back-edge.

4. **Net remap.** The DFS builds a `net_map` (old id → new id). After emission, `baseIndex` on every heap instance is repointed at its new offset-0 slot, and `visitMut(remap)` rewrites every input net reference on every kept instance.

5. **Re-emit tentative Names.** Each saved tentative `Name`'s value bits are remapped through `net_map`. Contiguous runs of still-live bits become a new heap-allocated `Name` with `from`/`to` adjusted to cover the surviving slice; dead bits split the run.

6. **Swap and free.** The new table replaces the old one, heap instances that did not survive are destroyed, and `checkNormalization()` verifies the result.

### Fine/Wide dispatch internals

The base class (`Buffer`, `Not`, `And`, etc.) provides two static methods used by `Graph::add`:

- `plan(args...)` -- returns `sizeof(FineSubclass)` or `sizeof(WideSubclass)` based on input widths.
- `construct(mem, args...)` -- placement-news the correct subclass into the allocated memory.

Types without Fine/Wide variants need neither method. `Graph::add` detects their presence with `requires` and falls back to `sizeof(T)` and direct placement-new.
