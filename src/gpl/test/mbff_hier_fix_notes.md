# MBFF Hierarchical Clustering — Implementation Notes

## Problem

`cluster_flops` (gpl module, `src/gpl/src/mbff.cpp`) clusters single-bit
flip-flops into multi-bit tray cells. Before these changes, the
clustering pass made several assumptions that held only for flat
netlists. Applied to a hierarchical netlist (`link_design -hier`, or
any odb with non-trivial `dbModule` structure), it produced any of:

1. Empty `dbModule`s whose original flops had been destroyed without
   their tray replacements being placed inside.
2. Trays whose `dbModNet`/`dbNet` associations were inconsistent,
   causing downstream consumers (`estimate_parasitics`,
   `repair_design`'s buffer insertion, etc.) to crash inside the odb
   hier traversal or in flute-based Steiner tree construction.
3. Single trays straddling pins from flops that lived in different
   parent modules — no valid home module for the tray.

For a tray cell whose liberty is expressed via a `statetable` (rather
than the simpler `ff` block — typical for multi-bit cells with sync
or async reset), STA reports the reset port as both an entry in
`Sequential::clear()` and `Sequential::data()`. MBFF's
classification predicates didn't handle this case symmetrically and
ended up wiring data pins onto the reset signal.

## Hierarchy view of the bug

Before, with `link_design -hier` and a small example design where
two leaf modules each host four flops:

```
       (top)                                (top)
         |                                    |
       u_mid                                u_mid
       /   \                                /   \
    leaf_a leaf_b      -->  cluster_   leaf_a leaf_b   <-- empty
    [4 ff] [4 ff]            flops     (no inst)(no inst)
                              creates
       +---------------------+--+
       |                        |
       v                        v
        tray (size 8)  in top   <-- wrong scope
        |
        connectivity: flat dbNet kept, dbModNet dropped
```

After the changes:

```
       (top)
         |
       u_mid
       /   \
    leaf_a leaf_b
      |      |
    tray    tray       <-- each tray lives in its leaf's dbModule
    (sz 4)  (sz 4)         flat + hier net both preserved
```

## Root causes

| # | Where                                            | What                                                                                                                                                                          |
|---|--------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | `MBFF::SeparateFlops`                            | Cluster partition keyed only by clock-net + `Mask`. Flops with different parent modules could fall into the same bucket.                                                      |
| 2 | `MBFF::ModifyPinConnections` (tray creation)     | `dbInst::create(block, master, name)` placed the new tray on the top `dbBlock`, ignoring the surviving parent `dbModule`.                                                     |
| 3 | `MBFF::ModifyPinConnections` (pin rewiring)      | Captured only `dbNet*` from each original iterm. The companion `dbModNet*` was dropped on `iterm->disconnect()` and never re-bound to the tray iterm.                         |
| 4 | `MBFF::IsDPin`                                   | Used `getLibertyCell()` (which silently substitutes the lib cell with its test cell). Then compared sequential FuncExpr ports against a `lib_port` looked up against the **original** cell. The pointer mismatch caused `seq.clear()->hasPort(lib_port)` to miss, so `IsDPin` returned `true` for ports that are actually reset/preset on statetable cells. |
| 5 | `dbITerm::connect(dbNet*)` semantics             | Single-arg ODB connect only updates the flat side. When a shared tray pin is re-bound across N original flops, a previously-attached modnet stays — producing inconsistent (`flat`, `mod`) pairs that crash downstream walkers.                              |

## Implementation

All changes live in `src/gpl/src/mbff.cpp`. The header was not
touched.

### Fix 1. Partition by `(parent dbModule*, Mask)`

`SeparateFlops` now buckets candidate flops by both their parent
module and the existing `Mask`. Flops in different modules can never
enter the same ILP run, so each cluster has exactly one parent
module by construction.

```cpp
std::map<std::pair<dbModule*, Mask>, std::vector<Flop>> flops_by_mod_mask;
for (const int idx : indices) {
  const Mask vec_mask = GetArrayMask(insts_[idx], false);
  flops_by_mod_mask[{insts_[idx]->getModule(), vec_mask}]
      .push_back(flops_[idx]);
}
```

`Mask` itself is unchanged — library-cell tables keyed by `Mask`
(`best_master_`, `pin_mappings_`, `slot_to_tray_x_`, ...) remain
correct.

### Fix 2. Create tray inside the cluster's parent module

Use the `dbInst::create(block, master, name, physical_only,
parent_module)` overload (the same idiom as `cts/src/TritonCTS.cpp`)
and pass the parent module of any flop in the cluster — by
construction of Fix 1, they all share one.

```cpp
dbModule* parent = insts_[flops[i].idx]->getModule();
auto new_tray = dbInst::create(block_,
                               best_master_[array_mask][bit_idx],
                               new_name.c_str(),
                               /*physical_only=*/false,
                               parent);
```

### Fix 3. Preserve `dbModNet` during pin rewiring

Capture both `dbNet*` and `dbModNet*` from each original iterm before
disconnecting, then rebind the matching tray iterm to both via the
dual `dbITerm::connect(dbNet*, dbModNet*)` overload.

A free helper `reconnectIterm` (file-local) covers the three real
arities so callers don't have to:

```cpp
void reconnectIterm(dbITerm* tray_iterm, dbNet* net, odb::dbModNet* mod_net)
{
  if (net && mod_net) {
    tray_iterm->connect(net, mod_net);
  } else if (net) {
    tray_iterm->disconnectDbModNet();   // see Fix 5
    tray_iterm->connect(net);
  } else if (mod_net) {
    tray_iterm->connect(mod_net);
  }
}
```

`dbITerm::connect(dbNet*, dbModNet*)` cannot be called with a null
modnet — its implementation dereferences the modnet pointer
unconditionally — hence the explicit dispatch.

The same helper is used for the deferred clock pin rebind at the end
of `ModifyPinConnections`.

### Fix 4. IsDPin must scan the right cell's sequentials

`MBFF::IsDPin` previously called `getLibertyCell(cell)`, which
substitutes the lib cell with its test cell when one exists. The
sequentials it then walked belong to the **test** cell, whose
`Sequential::clear()` FuncExpr port set holds **test cell**
LibertyPort pointers. The `lib_port` used in the `hasPort()` check,
however, came from `network_->libertyPort(pin)` — pointing into the
**original** cell. Different pointer ⇒ `hasPort()` returned false ⇒
`IsDPin` answered `true` for ports that are actually reset/preset on
statetable cells.

Pictorially, before the fix:

```
  cell ---getLibertyCell()---> test_cell.sequentials()
                                          |
                                          v
                                seq.clear()->ports() = { test_cell.RD_port, ... }
                                          ^
                                          |
                  hasPort( lib_port from original cell )    -> false
                                          ^
                                          |
                                          POINTER MISMATCH
```

The fix uses the raw lib cell and scans both the regular and test
cell sequentials with the appropriate port lookup. The check is
identical to the existing logic in `IsClearPin`/`IsPresetPin`, so
all three predicates now share a single helper:

```cpp
bool portInSequentialFunc(const sta::LibertyCell* lib_cell,
                          const sta::LibertyPort* lib_port,
                          sta::FuncExpr* (sta::Sequential::*get)() const);
```

- `IsClearPin`  := `portInSequentialFunc(.., &Sequential::clear)`
- `IsPresetPin` := `portInSequentialFunc(.., &Sequential::preset)`
- `IsDPin`      := `false` if either of the above is true, then the
                   usual `INPUT && !exclude(clock/supply/scan)` check.

### Fix 5. Defensive modnet clear in `reconnectIterm`

Why `disconnectDbModNet()` before single-arg `connect(net)`?

Shared tray pins (clear, scan, supply) are reconnected once per
original flop in the cluster. When a previous iteration has already
attached a modnet to the shared tray pin, a subsequent iteration
that passes only a flat net would call the single-arg `connect()`
overload — which by ODB design **only** clears the flat side and
leaves any prior modnet attached. The result is an inconsistent
`(flat, mod)` pair on the tray pin.

```
iter k-1: tray.SE -> connect(net_a, mod_a)
                        flat=net_a, mod=mod_a   (OK)

iter k:   tray.SE -> connect(net_b)             (mod_net captured was null)
                        disconnectDbNet()          (only flat cleared by ODB)
                        flat=net_b, mod=mod_a   (INCONSISTENT)
```

Explicit `disconnectDbModNet()` before the single-arg connect
restores the invariant: after any `reconnectIterm()` call the iterm
holds a coherent pair.

## Behavior Summary

| Scenario                                                  | Before fix                                                        | After fix                                                   |
|-----------------------------------------------------------|-------------------------------------------------------------------|-------------------------------------------------------------|
| Flat design (`link_design`)                               | Works                                                             | Identical results, no regression                            |
| Hierarchical design, flops all in one submodule           | Tray at top, submodule empty                                      | Tray placed in correct submodule                            |
| Hierarchical design, candidate flops span modules         | Cross-module tray, submodules empty, modnet corruption            | Per-module trays, hierarchy intact                          |
| `link_design -hier`, statetable tray cells (sync/async R) | `estimate_parasitics` / `repair_design` crash in odb hier walkers | Both flat and hier connectivity preserved, downstream clean |

Cross-module clustering is no longer attempted. This is a small QoR
trade for hierarchy correctness; a future `-flatten_hierarchy` flag
could opt back into the old behavior if desired.

## Regression Test

`src/gpl/test/mbff_hier.{v,tcl,ok}`, registered in both
`CMakeLists.txt` and `BUILD`.

The Verilog defines a 3-level hierarchy (`top → mid → leaf`) with
two `leaf` instances under one `mid`, four flops per leaf:

```
mbff_hier
   └── u_mid
        ├── l0  (leaf)
        │     ├── ff0..ff3
        ├── l1  (leaf)
        │     ├── ff0..ff3
```

The driver script `mbff_hier.tcl`:

1. Reads LEF/lib for asap7 single-bit + 2-bit + 4-bit tray cells.
2. `link_design -hier mbff_hier`.
3. Spreads the 8 flops on a 4×2 grid and places them.
4. Creates a clock.
5. Runs `cluster_flops`.
6. Runs `set_wire_rc` + `estimate_parasitics -placement`.
7. Prints `ESTIMATE_PARASITICS_OK`.

Expected golden output (`mbff_hier.ok`):

```
...
Sizes used
  4-bit: 2
...
ESTIMATE_PARASITICS_OK
```

Two 4-bit trays are formed (one per leaf), and `estimate_parasitics`
completes cleanly — both proving that:

- Trays are placed inside their respective `leaf` modules (not at
  the top), and
- `dbModNet` topology is consistent enough that the post-cluster
  Steiner-tree build doesn't trip the crash signature seen in cva6.

Existing flat regression `mbff_orig_name` continues to pass with
identical output.

## Files Changed

```
src/gpl/src/mbff.cpp                  fix (5 changes + 1 shared helper)
src/gpl/test/mbff_hier.v              regression (new hier design)
src/gpl/test/mbff_hier.tcl            regression driver
src/gpl/test/mbff_hier.ok             regression golden
src/gpl/test/CMakeLists.txt           register mbff_hier
src/gpl/test/BUILD                    register mbff_hier
```
