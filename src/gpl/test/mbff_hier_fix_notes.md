# MBFF Hierarchical Clustering Fix

## Problem

`cluster_flops` (gpl module) destroyed module hierarchy when applied to a
hierarchical netlist (`link_design -hier` or any design containing
`dbModule` instances).

Manifestations:

1. **Empty submodules.** Original flip-flops were destroyed and the new tray
   instance was created on the top `dbBlock`. After clustering, the
   sub-`dbModule`s that originally contained the flops were left empty.
2. **`dbModNet` connectivity dropped.** The rewiring loop in
   `MBFF::ModifyPinConnections` only captured the flat `dbNet` from each
   original `dbITerm`. The companion `dbModNet` (hierarchical net) was
   silently dropped on disconnect, leaving the resulting odb with broken
   hierarchical connectivity. Downstream queries against the hierarchical
   network crashed (segfault inside `dbITerm::connect(dbModNet*)` callers
   that assume both views are present).
3. **Cross-module clustering.** Flops from different parent modules could
   be merged into a single tray. There is no valid home `dbModule` for
   such a tray.

## Root Cause

Three independent gaps in `src/gpl/src/mbff.cpp`:

| Gap | Location | Effect |
|-----|----------|--------|
| Tray created on top block ignoring parent module | `ModifyPinConnections` (~L796) | Empty submodules, tray at wrong scope |
| Cluster partition keyed only by `Mask` | `SeparateFlops` (~L2375) | Flops from different modules merged |
| Rewiring captured only `dbNet`, not `dbModNet` | `ModifyPinConnections` (~L883) | Hierarchical connectivity dropped |

## Fix

### 1. Partition flops by `(parent dbModule*, Mask)`

`SeparateFlops` now buckets candidate flops by both their parent module
and the existing `Mask`. Flops that share a clock net but live in
different modules are placed in disjoint buckets and never enter the
same ILP run.

```cpp
std::map<std::pair<dbModule*, Mask>, std::vector<Flop>> flops_by_mod_mask;
for (const int idx : indices) {
  const Mask vec_mask = GetArrayMask(insts_[idx], false);
  flops_by_mod_mask[{insts_[idx]->getModule(), vec_mask}].push_back(
      flops_[idx]);
}
```

`Mask` itself is unchanged so the library-cell tables keyed by `Mask`
(`best_master_`, `pin_mappings_`, `slot_to_tray_x_`, etc.) are
unaffected.

### 2. Create tray inside the cluster's parent module

Use the `dbInst::create(block, master, name, physical_only,
parent_module)` overload (the same idiom used by
`cts/src/TritonCTS.cpp`) and pass the parent module of any flop in the
cluster (all share one, by construction of the partition above):

```cpp
dbModule* parent = insts_[flops[i].idx]->getModule();
auto new_tray = dbInst::create(block_,
                               best_master_[array_mask][bit_idx],
                               new_name.c_str(),
                               /*physical_only=*/false,
                               parent);
```

### 3. Preserve `dbModNet` during pin rewiring

Capture both `dbNet*` and `dbModNet*` from each original `dbITerm`
before disconnecting, and reconnect the corresponding tray pin via the
dual-arg `dbITerm::connect(dbNet*, dbModNet*)`. A small helper handles
the flat-only case (`mod_net == nullptr`), because
`dbITerm::connect(dbNet*, dbModNet*)` dereferences the modnet
unconditionally.

```cpp
auto connect_pair
    = [&](dbITerm* tray_iterm, dbNet* n, odb::dbModNet* mn) {
        if (mn) {
          tray_iterm->connect(n, mn);
        } else if (n) {
          tray_iterm->connect(n);
        }
      };
```

The clock-pin path keeps the same pattern: capture `clk_net` and
`clk_mod_net`, connect both at the end.

## Behavior Summary

| Scenario | Before fix | After fix |
|----------|-----------|-----------|
| Flat design (`link_design`) | Works | Identical results, no regression |
| Hierarchical design, single module | Tray at top, submodule empty | Tray placed in correct submodule |
| Hierarchical design, flops span modules | Cross-module tray, submodules empty, `dbModNet` dropped | Per-module trays, hierarchy intact |
| Hierarchical design with `link_design -hier` | Silent `dbModNet` corruption, downstream crash | Both flat and hier connectivity preserved |

Cross-module clustering is no longer attempted. This is a small QoR
trade for hierarchy correctness; a future `-flatten_hierarchy` flag
could opt back into the old behavior if desired.

## Test

`src/gpl/test/mbff_hier.{v,tcl,ok}` regression registered in both
`CMakeLists.txt` and `BUILD`.

- `mbff_hier.v` — top + `sub_a` + `sub_b`, two flops each.
- `mbff_hier.tcl` — `link_design -hier`, places flops, runs
  `cluster_flops`, prints per-module instance counts before/after.
- `mbff_hier.ok` — golden output.

Pre-fix output:

```
AFTER cluster_flops:
mbff_hier: 1 inst(s): _tray_size4_7
sub_a:     0 inst(s):
sub_b:     0 inst(s):
```

Post-fix golden:

```
AFTER cluster_flops:
mbff_hier: 0 inst(s):
sub_a:     1 inst(s): _tray_size2_X
sub_b:     1 inst(s): _tray_size2_Y
```

Existing flat regression `mbff_orig_name` continues to pass with
identical output.

## Files Changed

```
src/gpl/src/mbff.cpp                  fix
src/gpl/test/mbff_hier.v              new
src/gpl/test/mbff_hier.tcl            new
src/gpl/test/mbff_hier.ok             new
src/gpl/test/CMakeLists.txt           register
src/gpl/test/BUILD                    register
```
