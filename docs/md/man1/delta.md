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
current database, computes a block-level diff against the base, and
writes only the changed blocks. The `-base` flag specifies the base
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

![Delta benchmark chart](delta_benchmark.png)

| Transition | Full.gz | Delta.gz | Savings |
|---|---|---|---|
| synth -> floorplan | 450 KB | 194 KB | 57% |
| floorplan -> place | 614 KB | 209 KB | 66% |
| place -> cts | 616 KB | 4 KB | 99% |
| cts -> grt | 895 KB | 281 KB | 69% |
| grt -> route | 1,434 KB | 583 KB | 59% |
| route -> final | 1,494 KB | 9 KB | 99% |

The bsdiff algorithm uses suffix-array matching to find common
subsequences even after data shifts. When ODB adds new objects
(e.g. clock buffers after CTS), data shifts in the serialized stream.
bsdiff handles this efficiently — place->cts drops from 616 KB to 4 KB.

### ORFS Recommendations

Deltas are beneficial for nearly every stage transition:

| Stage | Format | Rationale |
|---|---|---|
| synth | full .odb | New base |
| floorplan | .delta | 57% savings |
| place | .delta | 66% savings |
| place substages | .delta | Minimal changes, near-zero delta |
| cts | .delta | 99% savings |
| grt | .delta | 69% savings |
| route | full .odb | New base (still 59% savings possible) |
| final | .delta | 99% savings |

## Implementation

The delta logic is self-contained in `src/utl/include/utl/OdbDelta.h`
and `src/utl/src/OdbDelta.cpp` (~430 lines). Uses the bsdiff algorithm
(Colin Percival, BSD-2-Clause) with prefix-doubling suffix array
construction. No changes to the ODB binary format. No new external
dependencies.

## Limitations

- Delta files require the exact base `.odb` that was used to create
  them. Using a different base produces a corrupted database (detected
  via base size mismatch).
