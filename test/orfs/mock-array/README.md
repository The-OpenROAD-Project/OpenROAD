# mock-array test case

A parameterized 2D array of identical processing elements (`Element`), each
containing four multipliers with registered crossbar routing. The design
exercises OpenROAD's handling of macro-dominated floorplans, hierarchical
synthesis, and repeated-macro placement.

## Why mock-array exists

mock-array is deliberately simple RTL that nonetheless stresses corners of the
physical design flow that production SoCs hit: large counts of identical macros,
tight macro-to-macro spacing, hierarchical power reporting, and equivalence
checking across synthesis and P&R. Because the design is self-contained and
fast to build, it runs in CI on every pull request and catches regressions
that would otherwise escape unit tests.

## Bugs exposed by mock-array

### Macro placer (MPL/RTLMP)

mock-array's regular grid of 16-64 identical macros plus negligible standard
cell logic is an extreme but valid topology. It has driven multiple MPL fixes:

- **Boundary push corner case** -- the placer's "pusher" shoved macros toward
  core edges, destroying the array. Fixed by detecting a single centralized
  macro array and skipping the push (`d1dd49a964`).
- **Exchange swap probability** -- simulated annealing swaps are no-ops when
  macros are identical. Fix: scale exchange probability by the fraction of
  identical macros (`52c6ce2128`, issue #3875).
- **SA perturbation count** -- the perturbation budget was too low for 64-macro
  clusters, preventing convergence (`f7cbe0a86d`).
- **Fine shaping with a single std cell cluster** -- the shaper didn't shrink
  the negligible std cell cluster, leaving it large enough to displace macros
  (`2ae90677cd`).
- **SA centralization revert** -- reverting centralization on cost increase
  prevented convergence for the single-array-single-std-cell pattern
  (`38bf6edce2`).
- **Pin access blockages** -- making blockages hard constraints consumed
  enough area to make mock-array infeasible; fix: shrink pin access area
  when needed (PR #8438, issue #8366).

### Clock tree synthesis

- CTS produces large clock skew in macro-dominated designs, causing massive
  hold violations (issues #8255, #8516, #4989). mock-array requires a
  `-30 ps` hold slack margin workaround.

### Hierarchical flow

mock-array is the primary test vehicle for `read_db -hier` and hierarchical
power reporting. It has exposed:

- Crash in `vt_swap` after `read_db -hier` (issue #8141).
- Missing `reg2reg` path group with hierarchical DB (issue #8166).
- No TAPCELL insertion in hierarchical flow (issue #8254).
- Net name mismatch between netlist and SPEF (issues #8297, #8551).
- Incremental vs full STA timing mismatch post-CTS (issue #8240).

### Other tools

- `read_vcd` segfault in STA on mock-array power test (issue #8857).
- `write_macro_placement` didn't Tcl-escape special characters in macro
  names (`475eb39584`).
- Verilator reported 17k UNDRIVEN warnings on hierarchical output
  (issue #8108).
- Equivalence checking (eqy) failures from `write_verilog` output and
  FILLER/TAPCELL cell interactions (issues #8869, #8905, #9035; fix
  PR #8966).

## What mock-array does NOT test

The design is intentionally simple. These areas need other test cases:

- **Irregular hierarchies** -- mock-array is a perfectly regular NxN grid.
  Designs with mixed macro sizes, deep nesting, or irregular connectivity
  exercise different MPL code paths (e.g. segfaults on fazyrv #7616,
  darksocv #9251).
- **Multiple clock domains** -- single-clock design; CTS and STA bugs
  involving clock domain crossings are invisible.
- **Routing congestion** -- macro-dominated with light standard cell
  routing. DRT bugs under heavy congestion (e.g. megaboom #6066) are
  not tested.
- **Technology portability** -- runs on ASAP7 only. PDK-specific bugs
  (GF180, IHP130, SKY130) are missed.
- **Large scale** -- 4x4 and 8x8 configurations are small. Bugs that
  manifest only at thousands of macros or millions of cells are missed.
- **Analog/mixed-signal** -- no ring macros, 90-degree rotation, or
  complex placement blockage interactions.
- **Detailed placement** -- macro-dominated; standard cell DPL bugs
  (e.g. `improve_placement` #9862) would not be caught.
- **IO pads / flipchip** -- simple pin placement; pad-related PDN,
  RDL routing, and bump assignment bugs are not tested.

## Design structure

```
MockArray (parameterized WIDTH x HEIGHT)
  +-- Element[r][c]              (concrete wrapper, synthesized as macro)
        +-- ElementInner         (parameterized DATA_WIDTH, COLS)
              +-- Multiplier x4  (crossbar: left/up/right/down)
              |     +-- multiplier (Amaranth gate-level blackbox)
              +-- LSB chain      (shift register with periodic register breaks)
```

Each `Element` registers all four directional inputs, feeds them through a
crossbar of four multipliers, and registers the outputs. A LSB chain shifts
bits left-to-right across columns with a register break every `COLS/2`
positions.

## Configurations

| Config | Rows | Cols | Elements | Multipliers |
|--------|------|------|----------|-------------|
| 4x4    | 4    | 4    | 16       | 64          |
| 8x8    | 8    | 8    | 64       | 256         |

## Build variants

- **`base`** -- hierarchical: `Element` is synthesized and placed as a
  hard macro, then instantiated inside `MockArray`. Tests hierarchical
  synthesis, abstract generation, macro placement, and hierarchical power
  reporting.
- **`flat`** -- flat synthesis of the entire design. Tests standard flat
  flow and Verilator simulation for VCD-based power analysis.

## Synthesis flow

Hierarchical synthesis uses three separate Yosys runs stitched together:

1. `Element` RTL synthesized with **slang** frontend (multiplier blackboxed).
2. `multiplier.v` (Amaranth-generated) synthesized with **native Yosys**
   frontend (slang doesn't support the behavioral Verilog style).
3. Netlists concatenated into a combined Element netlist.
4. `MockArray` RTL synthesized with slang (both Element and multiplier
   blackboxed), then combined with the Element and multiplier netlists.

## Running tests

```bash
# Quick: analysis only (no build), checks for Bazel conflicts
bazelisk build --nobuild //test/orfs/mock-array:all

# Fast: interface comparison (SV vs Chisel reference)
bazelisk test //test/orfs/mock-array:compare_interfaces_4x4
bazelisk test //test/orfs/mock-array:compare_interfaces_8x8

# Medium: synthesis only
bazelisk build //test/orfs/mock-array:MockArray_4x4_slang_synth
bazelisk build //test/orfs/mock-array:Element_4x4_slang_synth

# Full: place-and-route (takes minutes)
bazelisk test //test/orfs/mock-array:MockArray_4x4_base_test
bazelisk test //test/orfs/mock-array:MockArray_4x4_flat_test

# Power and equivalence (slowest)
bazelisk test //test/orfs/mock-array:eqy_tests
```
