---
title: Delta .odb Files
date: 2026/03/13
---

# Delta .odb Files

## Overview

Delta files (`.delta`) store only the differences between two serialized
OpenDB states using the bsdiff algorithm. This reduces artifact sizes
when breaking ORFS stages into substages, where consecutive steps make
small, localized changes (e.g. placement adjustments, timing repair).

ODB serialization is deterministic: the same database state always
produces the same bytes. Delta files exploit this by computing a
byte-level binary diff between a base `.odb` and the current state.

## Tcl Commands

### write_db

```tcl
# Write a full .odb (unchanged behavior)
write_db design.odb

# Write a delta against an explicit base
write_db -base original.odb design.delta

# Write a delta against the remembered base (from last read_db)
write_db design.delta
```

When the output filename ends in `.delta`, `write_db` serializes the
current database, computes a binary diff against the base, and
writes only the differences. The `-base` flag specifies the base
file explicitly; without it, the base remembered from the last `read_db`
is used.

### read_db

```tcl
# Read a full .odb (unchanged behavior)
read_db design.odb

# Read a base .odb and apply delta files in order
read_db step1.delta step2.delta base.odb
```

When multiple filenames are provided, `read_db` auto-detects the base
(`.odb`) and delta (`.delta`) files by extension. The base is loaded
first, then deltas are applied left-to-right. The final state is
remembered as the base for subsequent `write_db .delta` calls.

## File Format

Delta files use the bsdiff algorithm (suffix-array binary diff) with
a self-describing binary header. No external dependencies.

```
Header (32 bytes):
  MAGIC:        4 bytes   "ODBD" (0x4F444244)
  VERSION:      uint32    2
  BASE_SIZE:    uint64    size of base .odb in bytes
  NEW_SIZE:     uint64    size of current .odb in bytes
  CTRL_LEN:     uint32    size of control section in bytes
  DIFF_LEN:     uint32    size of diff section in bytes

Control section (CTRL_LEN bytes, 24-byte tuples):
  DIFF_COUNT:   int64     bytes to copy from old+diff
  EXTRA_COUNT:  int64     bytes to copy verbatim from extra
  OLD_ADJUST:   int64     seek offset in old file

Diff section (DIFF_LEN bytes):
  Byte-level differences to add to old file data

Extra section (remaining bytes):
  New data not present in old file
```

All integers are little-endian. To reconstruct: process control tuples
sequentially — for each tuple, copy DIFF_COUNT bytes from old (adding
diff bytes), then copy EXTRA_COUNT bytes from extra section, then
adjust the old file position by OLD_ADJUST.

## Benchmark Results

Measured on MockArray 4x4 (ASAP7). All sizes are gzip-9 compressed
(what Bazel uses for cache artifacts).

### Stage-Level Transitions

| Transition | Full.gz | Delta.gz | Savings |
|---|---|---|---|
| synth -> floorplan | 450 KB | 241 KB | 47% |
| floorplan -> place | 614 KB | 233 KB | 62% |
| place -> cts | 616 KB | 11 KB | 99% |
| cts -> grt | 895 KB | 345 KB | 62% |
| grt -> route | 1,433 KB | 694 KB | 52% |
| route -> final | 1,494 KB | 91 KB | 94% |

### Floorplan Sub-Steps

| Transition | Full.gz | Delta.gz | Savings |
|---|---|---|---|
| synth -> 2_1_floorplan | 235 KB | 18 KB | 93% |
| 2_1_fp -> 2_2_fp_macro | 235 KB | 2 KB | 99% |
| 2_2_macro -> 2_3_tapcell | 289 KB | 61 KB | 79% |
| 2_3_tapcell -> 2_4_pdn | 450 KB | 165 KB | 64% |

### Place Sub-Steps

| Transition | Full.gz | Delta.gz | Savings |
|---|---|---|---|
| fp -> 3_1_gp_skip_io | 451 KB | 3 KB | 100% |
| 3_1_gp -> 3_2_iop | 485 KB | 47 KB | 91% |
| 3_2_iop -> 3_3_gp | 620 KB | 187 KB | 70% |
| 3_3_gp -> 3_4_resized | 620 KB | 3 KB | 100% |
| 3_4_resized -> 3_5_dp | 614 KB | 33 KB | 95% |

The bsdiff algorithm uses suffix-array matching to find common
subsequences even after data shifts. When ODB adds new objects
(e.g. clock buffers after CTS), data shifts in the serialized stream.
bsdiff handles this efficiently — place->cts drops from 616 KB to 11 KB.

Sub-steps within a stage show dramatic savings: macro placement (99%),
global placement skip-IO (100%), and resize (100%) produce near-zero
deltas since they modify only instance locations or a few nets.

### ORFS Recommendations

Deltas are beneficial for nearly every stage transition:

| Stage | Format | Rationale |
|---|---|---|
| synth | full .odb | New base |
| floorplan sub-steps | .delta | 64-99% savings |
| place sub-steps | .delta | 70-100% savings |
| cts | .delta | 99% savings |
| grt | .delta | 62% savings |
| route | full .odb | New base (still 52% savings possible) |
| final | .delta | 94% savings |

## Implementation

The delta logic is self-contained in `src/utl/include/utl/OdbDelta.h`
and `src/utl/src/OdbDelta.cpp` (~430 lines). Uses the bsdiff algorithm
(Colin Percival, BSD-2-Clause) with prefix-doubling suffix array
construction. No changes to the ODB binary format. No new external
dependencies.

## Reproducing Benchmarks

The benchmark measures delta sizes between consecutive ORFS stages
and sub-steps using MockArray 4x4 (ASAP7).

### Prerequisites

Build OpenROAD with delta support (from `tools/OpenROAD/`):

```bash
bazelisk build //:openroad
```

### Get stage-level .odb files

Build all stage targets (these produce .odb files in bazel-bin):

```bash
bazelisk build //test/orfs/mock-array:MockArray_4x4_base_floorplan
bazelisk build //test/orfs/mock-array:MockArray_4x4_base_place
bazelisk build //test/orfs/mock-array:MockArray_4x4_base_cts
bazelisk build //test/orfs/mock-array:MockArray_4x4_base_grt
bazelisk build //test/orfs/mock-array:MockArray_4x4_base_route
bazelisk build //test/orfs/mock-array:MockArray_4x4_base_final
```

Stage .odb files appear at:
`bazel-bin/test/orfs/mock-array/results/asap7/MockArray/4x4_base/{1_synth,2_floorplan,...}.odb`

### Get sub-step .odb files

Use `_deps` targets to install Make-compatible environments, then
run sub-steps:

```bash
# Install floorplan deps (creates tmp/ with Make environment)
bazelisk run //test/orfs/mock-array:MockArray_4x4_base_floorplan_deps

# Run floorplan sub-steps (produces 2_1 through 2_4 .odb files)
tmp/test/orfs/mock-array/MockArray_4x4_base_floorplan_deps/make do-floorplan

# Install place deps
bazelisk run //test/orfs/mock-array:MockArray_4x4_base_place_deps

# Run place sub-steps (produces 3_1 through 3_5 .odb files)
tmp/test/orfs/mock-array/MockArray_4x4_base_place_deps/make do-place
```

Sub-step .odb files appear at:
`tmp/test/orfs/mock-array/MockArray_4x4_base_{floorplan,place}_deps/_main/test/orfs/mock-array/results/asap7/MockArray/4x4_base/`

### Run stage-level benchmark

```bash
bash test/benchmark_delta.sh bazel-bin/openroad \
  bazel-bin/test/orfs/mock-array/results/asap7/MockArray/4x4_base/
```

### Compute delta for a specific transition

```bash
# Example: compute delta between floorplan and place
bazel-bin/openroad -no_init -no_splash -exit <<EOF
read_db path/to/3_place.odb
write_db -base path/to/2_floorplan.odb /tmp/fp_to_place.delta
EOF

# Compare compressed sizes
gzip -9 -c path/to/3_place.odb > /tmp/full.gz
gzip -9 -c /tmp/fp_to_place.delta > /tmp/delta.gz
ls -la /tmp/full.gz /tmp/delta.gz
```

## Further Work

Ideas for improving delta effectiveness, roughly ordered by expected
impact. All trade compute time for space savings.

### Speed optimizations

- **SA-IS or divsufsort suffix array**: Replace the O(n log²n)
  prefix-doubling SA with an O(n) algorithm. divsufsort is the
  standard choice (used by production bsdiff). Expected 5-10x speedup
  on large designs with no change to delta quality.
- **Parallel suffix array construction**: SA construction is the
  bottleneck. Parallel SA algorithms (pSAIS, parallel divsufsort)
  could use multiple cores.
- **Memory-mapped I/O**: For large designs, mmap the base file instead
  of reading into a vector. Reduces peak memory by avoiding a copy.

### Space optimizations

- **Built-in compression**: Compress the delta file with zstd before
  writing. Currently Bazel applies gzip-9 on top, but bsdiff's
  diff/extra sections compress well with zstd (the original bsdiff
  uses bzip2 internally). This could eliminate the need for external
  compression and improve ratios.
- **ODB-aware section diffing**: Instead of treating the `.odb` as
  an opaque byte stream, split it at ODB section boundaries (instance
  table, net table, wire table, etc.) and diff each section
  independently. Sections that didn't change produce zero-cost deltas.
  Sections that grew (e.g. wires after routing) get better suffix
  matches within their own data type. Requires coupling to ODB
  serialization format.
- **Alternative delta algorithms**: xdelta3 (VCDIFF) uses a
  different matching strategy that may compress better for certain
  workloads, especially when the new file is much larger than the old
  (e.g. after routing adds wire data). Worth benchmarking against
  bsdiff.
- **Optimal base selection**: Currently each delta uses the previous
  stage as base. For some transitions, using an earlier base (e.g.
  synth as base for all floorplan sub-steps) might produce smaller
  deltas if the intermediate steps add and then remove temporary data.

### Operational improvements

- **Delta coalescing**: Merge a chain of deltas into a single delta
  without reconstructing the full `.odb`. Useful when ORFS accumulates
  many small sub-step deltas but downstream only needs the final state.
- **Streaming delta application**: Apply deltas without holding the
  full base in memory. Useful for very large designs where peak memory
  is a concern.
- **Larger design benchmarks**: The MockArray 4x4 (ASAP7) is a small
  design (~3 MB .odb). Benchmarks on larger designs (e.g. 50+ MB .odb)
  would validate that the approach scales and that compression ratios
  hold.
- **Integrity checking**: Add a checksum (e.g. CRC32 or xxHash) of
  the base and result to the delta header to detect base mismatches
  more reliably than size comparison alone.

## Limitations

- Delta files require the exact base `.odb` that was used to create
  them. Using a different base produces a corrupted database (detected
  via base size mismatch).
