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

Designs are tested smallest-first. Each takes a few minutes for the smallest
designs:

| # | Platform  | Design   | Notes                    |
|---|-----------|----------|--------------------------|
| 1 | asap7     | gcd      | Baseline, must pass      |
| 2 | asap7     | mock-cpu |                          |
| 3 | asap7     | uart     |                          |
| 4 | nangate45 | gcd      | Different platform       |
| 5 | sky130hd  | gcd      | Different platform       |
| 6 | sky130hs  | gcd      | Different platform       |
| 7 | asap7     | riscv32i | Medium complexity        |
| 8 | asap7     | aes      |                          |
| 9 | nangate45 | aes      | Cross-platform check     |
|10 | nangate45 | jpeg     | Largest in test set      |

## Running the Tests

### Automated (all designs)

```bash
cd /path/to/OpenROAD-flow-scripts/flow
./util/test_write_gds.sh
```

### Single design

```bash
# Run only design index 0 (asap7/gcd)
./util/test_write_gds.sh 0
```

### Manual (step by step)

#### 1. Run the ORFS flow to produce the KLayout GDS

```bash
cd /path/to/OpenROAD-flow-scripts/flow
make DESIGN_CONFIG=designs/asap7/gcd/config.mk
```

This produces `results/asap7/gcd/base/6_final.gds` (via KLayout).

#### 2. Run `write_gds` via OpenROAD

Load the finished ODB and call `write_gds` with the same inputs KLayout
received (layer map, macro GDS files, seal GDS):

```bash
export RESULTS_DIR=results/asap7/gcd/base
export OBJECTS_DIR=objects/asap7/gcd/base

openroad -exit util/write_gds_test.tcl
```

This produces `results/asap7/gcd/base/6_final_openroad.gds`.

The TCL script (`util/write_gds_test.tcl`) reads these environment variables:

| Variable         | Source                | Description                      |
|------------------|-----------------------|----------------------------------|
| `RESULTS_DIR`    | ORFS make variable    | Where ODB and GDS files live     |
| `OBJECTS_DIR`    | ORFS make variable    | Where `klayout.lyt` lives        |
| `GDSOAS_FILES`   | Platform `config.mk`  | Standard cell GDS files to merge |
| `WRAPPED_GDSOAS` | ORFS make variable    | Wrapped macro GDS files          |
| `SEAL_GDSOAS`    | Platform `config.mk`  | Seal ring GDS (if any)           |
| `GDS_ALLOW_EMPTY`| Platform `config.mk`  | Regex for allowed empty cells    |

#### 3. Compare the two GDS files

```bash
klayout -b \
    -rd gds1=results/asap7/gcd/base/6_final.gds \
    -rd gds2=results/asap7/gcd/base/6_final_openroad.gds \
    -rd report=results/asap7/gcd/base/gds_compare_report.txt \
    -r util/gds_dump_compare.py
```

The comparison script loads both GDS files in KLayout and compares:
- Cell counts and names
- Per-cell instance (SREF) counts and transforms
- Per-cell shape counts by GDS layer/datatype
- Detailed shape geometry (sorted for deterministic comparison)

Exit code: 0 = match, 1 = differences found, 2 = error.

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

| File                        | Purpose                                      |
|-----------------------------|----------------------------------------------|
| `flow/util/test_write_gds.sh`    | Main orchestrator — runs all designs   |
| `flow/util/write_gds_test.tcl`   | OpenROAD TCL — calls `write_gds`       |
| `flow/util/gds_dump_compare.py`  | KLayout Python — compares two GDS files|
