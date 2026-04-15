# Multi-Height Cell Placement Test

Regression test for [#9932](https://github.com/The-OpenROAD-Project/OpenROAD/issues/9932):
DPL-0400 topological sort cycle in the shift legalizer when multi-height
cells coexist with single-height cells during `improve_placement`.

## What are multi-height cells?

Standard cells normally occupy a single row (1x height). Multi-height cells
span 2, 3, 4 or more rows. Examples include double-height flip-flops, wide
I/O drivers, and SRAM macros that span dozens of rows. These cells appear in
multiple row segments simultaneously, which makes placement legalization more
complex.

## What this test covers

| Use case | Cell |
|----------|------|
| Single-height (baseline) | `SINGLE_BUF` (s1, s2) |
| Double-height (2 rows) | `DOUBLE_BUF` (d1, d2) |
| Quad-height (4 rows) | `QUAD_BUF` (q1) |
| Mixed heights in one design | All 5 cells together |
| Movable multi-height placement | All cells are `+ PLACED` (not FIXED) |

All cells are **movable** (`+ PLACED` in DEF, not `+ FIXED`), which is
critical: the DPL-0400 bug only affects movable multi-height cells.

## Synthetic PDK

Multi-height PDKs are typically under NDA. This test uses a self-contained
synthetic PDK defined in `multi_height.lef`:

- Row height: 1.4 um, site width: 0.19 um (Nangate45 geometry)
- Three sites: `CoreSite` (1x), `DoubleSite` (2x), `QuadSite` (4x)
- Three cell types: `SINGLE_BUF`, `DOUBLE_BUF`, `QUAD_BUF`

## Running the test

```bash
bazelisk test //test/orfs/multi-height:multi_height_test
```

With streamed output for debugging:

```bash
bazelisk test //test/orfs/multi-height:multi_height_test --test_output=streamed
```

## Adding new test scenarios

1. Add new cells to `multi_height.lef` (e.g., a `TRIPLE_BUF` with
   SIZE 0.76 BY 4.2 and a `TripleSite` of SIZE 0.19 BY 4.2).
2. Add corresponding rows and component instances to `multi_height.def`.
3. The test automatically covers any new cells through `improve_placement`.

## Files

| File | Purpose |
|------|---------|
| `multi_height.lef` | Synthetic PDK: technology layers, sites, cell definitions |
| `multi_height.def` | Design: rows, placed cells, pins, nets |
| `multi_height.tcl` | OpenROAD script: read LEF/DEF, run `improve_placement` |
| `test_multi_height.sh` | Shell driver: run OpenROAD, check for DPL-0400 errors |
| `BUILD.bazel` | Bazel test definition |
