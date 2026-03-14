# Testing `write_gds` — Native DEF-to-GDS Export

## Overview

The `write_gds` TCL command provides native DEF-to-GDS conversion in OpenROAD,
replacing the KLayout `def2stream.py` dependency. This document describes the
integration test procedure that validates `write_gds` output against KLayout's
output on real ORFS designs.

## Prerequisites

- OpenROAD built and installed from this branch
- KLayout installed (for GDS comparison and the existing flow)
- ORFS checkout at `../flow` relative to the OpenROAD source

## Build

```bash
cd /path/to/OpenROAD-flow-scripts/tools/OpenROAD
bazelisk run //:install
```

Bazel builds are idempotent — output file timestamps are fixed, so use content
hashes (not dates) to verify a fresh build.

## Test Designs

Designs are tested smallest-first. The ORFS flow must complete once to produce
the KLayout GDS and ODB files. After that, only `write_gds` is re-run when
OpenROAD is updated.

| # | Platform  | Design   | Notes                    |
|---|-----------|----------|--------------------------|
| 0 | asap7     | gcd      | Baseline, must pass      |
| 1 | asap7     | mock-cpu |                          |
| 2 | asap7     | uart     |                          |
| 3 | nangate45 | gcd      | Different platform       |
| 4 | sky130hd  | gcd      | Different platform       |
| 5 | sky130hs  | gcd      | Different platform       |
| 6 | asap7     | riscv32i | Medium complexity        |
| 7 | asap7     | aes      |                          |
| 8 | nangate45 | aes      | Cross-platform check     |
| 9 | nangate45 | jpeg     | Largest in test set      |

## Running the Tests

### All designs

```bash
cd /path/to/OpenROAD-flow-scripts/flow
./util/run_all_gds_tests.sh
```

### Single design by index

```bash
./util/test_write_gds.sh 0   # asap7/gcd
```

### Compare only (no flow rebuild)

```bash
./util/run_gds_compare.sh --design asap7/gcd
./util/run_gds_compare.sh --index 0
```

## Expected Differences

The flattened comparison normalizes both GDS files to physical coordinates
(microns), eliminating structural differences (VIA cells vs expanded
boundaries, different cell hierarchy). Remaining acceptable differences:

| Category | Example | Cause |
|----------|---------|-------|
| Rounding | `0.8155` vs `0.815` | Different DBU resolution (4000 vs 1000 DBU/µm) |
| Extra pin labels | VDD/VSS on layer 60/2 | OpenROAD writes all pin labels |
| PR boundary | Layer 235/0 missing | KLayout writes die area boundary |

### Results Summary (13 tested designs, 6 platforms)

| Design | Total Diffs | Layout Match |
|--------|-------------|-------------|
| asap7/gcd | 5 | All routing/via/cell layers match |
| asap7/mock-cpu | 5 | All routing/via/cell layers match |
| asap7/uart | 5 | All routing/via/cell layers match |
| asap7/aes | 5 | All routing/via/cell layers match |
| nangate45/gcd | 1 | Perfect match (only PR boundary) |
| nangate45/aes | 2 | Pin labels + PR boundary |
| nangate45/jpeg | 2 | Pin labels + PR boundary (1.2M+ shapes) |
| nangate45/dynamic_node | 2 | Pin labels + PR boundary |
| sky130hd/gcd | 2 | Pin labels + PR boundary |
| sky130hd/chameleon | N/A | write_gds succeeds (377MB, too large for flattened comparison) |
| sky130hs/gcd | 30 | Layer mapping mismatch (sky130hs GDS uses different layer numbers) |
| ihp-sg13g2/gcd | 7 | Pin shapes, labels, PR boundary |
| ihp-sg13g2/spi | 7 | Pin shapes, labels, PR boundary |

- `asap7/riscv32i` skipped: yosys-abc crashes during synthesis (unrelated to write_gds)
- `gf180/uart-blocks` skipped: yosys synthesis fails (PDK issue, unrelated to write_gds)

## Output

- `gds_compare_results.csv` — summary table of all designs
- `results/<platform>/<design>/base/gds_compare_report.txt` — per-design diff
- `logs/<platform>/<design>/base/write_gds_test.log` — OpenROAD output
- `logs/<platform>/<design>/base/gds_compare.log` — KLayout comparison output

## Handling Failures

- If the ORFS flow fails for a design (KLayout is not always robust), the
  test script logs the failure and **continues to the next design**.
- The first design (`asap7/gcd`) is confirmed to complete. If the script
  fails on that design, the script itself is buggy.

## Test Scripts

| File                              | Purpose                                |
|-----------------------------------|----------------------------------------|
| `flow/util/test_write_gds.sh`    | Main orchestrator — runs flow + test   |
| `flow/util/run_all_gds_tests.sh` | Runs all 10 designs sequentially       |
| `flow/util/run_gds_compare.sh`   | Compare two GDS files (no flow)        |
| `flow/util/write_gds_test.tcl`   | OpenROAD TCL — calls `write_gds`       |
| `flow/util/gds_dump_compare.py`  | KLayout Python — flattened comparison  |
| `flow/util/check_def_units.sh`   | Check DEF units in an ODB file         |

## Unit Tests

```bash
bazelisk test //src/odb/test/cpp:TestGDSOrientations
bazelisk test //src/odb/test/cpp:TestGDSMergeCells
bazelisk test //src/odb/test/cpp:TestGDSMergeSeal
bazelisk test //src/odb/test/cpp:TestGDSDuplicateCells
bazelisk test //src/odb/test/cpp:TestGDSPruneUnreferenced
bazelisk test //src/odb/test/cpp:TestGDSLayerMap
bazelisk test //src/odb/test/cpp:TestGDSValidate
bazelisk test //src/odb/test/cpp:TestGDSWriteRoundtrip
```

## Bugs Found and Fixed

| Bug | Symptoms | Fix |
|-----|----------|-----|
| MY/MX orientation swap | Mirrored cells rotated 180° | Swap flipX angle for MY (180°) and MX (0°) |
| DBU scaling in mergeCells | Merged std cell shapes 4x too large | Scale coordinates by source/target DBU ratio |
| Orphan cell inclusion | 174 unused std cells in output | Added pruneUnreferencedCells() after merge |
| MYR90/MXR90 swap | (pre-existing) Rotated mirror orientations wrong | Fixed in earlier commit |
| GDS UNITS not set | DBU mismatch between tools | Set lib UNITS from block DEF units |
| Odd-length GDS strings | KLayout warns "Odd record length" | Pad STRING records to even byte length |
| sky130 layer map | met2-met5 missing from sky130hd output | Fall back to "drawing" and lowercase purposes in lookup |
| EDI comma-separated types | All routing missing on ihp-sg13g2 | Split comma-separated EDI purpose types (e.g., "NET,SPNET,PIN") |
| Empty .lyt layer map | No layer mapping for ihp-sg13g2 | Fall back to `<map-file>` EDI reference when inline map is empty |
