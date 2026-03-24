# Per-Command OpenROAD Binaries

Standalone binaries for individual OpenROAD commands. Each links only
the modules it needs — fast rebuild, direct `gdb`, reproducible bug
reports.

## Quick Start

```bash
# Build (seconds — links dpl + odb + utl, not all 37 modules)
bazelisk build //test/orfs/openroad:detailed_placement

# Run
bazelisk run //test/orfs/openroad:detailed_placement -- \
  --read_db placed.odb --write_db legalized.odb -max_displacement 10

# Or with LEF/DEF
bazelisk run //test/orfs/openroad:detailed_placement -- \
  --read_lef Nangate45.lef --read_def placed.def --write_def legalized.def
```

## Debugging a Crash

```bash
# 1. Build (fast — only dpl module)
bazelisk build //test/orfs/openroad:detailed_placement

# 2. Debug DIRECTLY — no TCL, no framework, just C++ main()
gdb --args bazel-bin/test/orfs/openroad/detailed_placement \
  --read_db crash.odb -max_displacement 10

# 3. Or valgrind / ASAN — same binary, same args
valgrind bazel-bin/test/orfs/openroad/detailed_placement \
  --read_db crash.odb

# 4. Edit src/dpl/src/Place.cpp, rebuild (seconds), re-run
```

## Filing a Bug Report

```
Steps to reproduce:
1. Download the attached crash.odb
2. Run:
   bazelisk run //test/orfs/openroad:detailed_placement -- \
     --read_db crash.odb -max_displacement 10
3. Observe segfault in Opendp::detailedPlacement at Place.cpp:142
```

No environment variables. No TCL script. Just files and one command.

## Generic CLI (all commands)

`openroad_cmd` defines the CLI for ALL OpenROAD commands. It routes to
the native `cc_binary` when one exists, and falls back to the monolithic
`openroad` binary + generated TCL otherwise.

```bash
# detailed_placement → native binary (fast, debuggable)
bazelisk run //test/orfs/openroad:openroad_cmd -- \
  detailed_placement --read_db in.odb --write_db out.odb -max_displacement 10

# global_route → monolithic fallback (no native binary yet)
bazelisk run //test/orfs/openroad:openroad_cmd -- \
  global_route --read_db in.odb --write_db out.odb -congestion_iterations 50
```

The CLI interface is the same regardless of backend. TCL flags pass
through verbatim — `-max_displacement 10` in TCL is `-max_displacement 10`
on the command line.

## ORFS Integration

The bash wrapper can replace a TCL stage script in the ORFS Makefile:

```makefile
# Before: framework-inverted TCL, 8+ env vars
$(OPENROAD_CMD) $(SCRIPTS_DIR)/detail_place.tcl

# After: direct CLI, explicit args, no env vars
openroad-detailed-placement \
  --read_db $(RESULTS_DIR)/3_4_place_resized.odb \
  --write_db $(RESULTS_DIR)/3_5_place_dp.odb
```

## Available Binaries

| Binary | Module deps | Source files | Reduction | Test result |
|--------|------------|-------------|-----------|-------------|
| `detailed_placement` | dpl + odb + utl | 1,095 | 3.5x fewer | 294 cells placed, 0 failures (gcd Nangate45) |
| `check_placement` | dpl + odb + utl | 1,095 | 3.5x fewer | Detects violations + passes on legalized |
| `filler_placement` | dpl + odb + utl | 1,095 | 3.5x fewer | 2,092 fillers placed (gcd Nangate45) |
| `optimize_mirroring` | dpl + odb + utl | 1,095 | 3.5x fewer | 134 mirrored, -1% HPWL (gcd Nangate45) |
| **`detailed_route`** | **drt + dst + stt + odb + utl** | **1,165** | **3.2x fewer** | **20s, 3,623 vias, exit 0 (gcd ASAP7)** |
| `check_antennas` | ant + odb + utl | 1,061 | 3.6x fewer | 0 violations (gcd ASAP7 routed) |
| `extract_parasitics` | rcx + odb + utl | 1,102 | 3.4x fewer | Runs on routed ODB with -lef_rc |
| `tapcell` | tap + odb + utl | 1,059 | 3.6x fewer | 114 endcaps (gcd Nangate45) |
| *monolithic `openroad`* | *all 37 modules* | *3,788* | *baseline* | |

Each standalone binary compiles **3-4x fewer source files** than the
monolithic `openroad`. The ~1,100 file baseline is `odb` + `sta` (shared
core). The delta between `detailed_placement` (1,095) and
`detailed_route` (1,165) is only 70 files — the actual `drt` + `dst` +
`stt` module code. The monolithic binary adds **2,600+ files** from the
other 34 modules you don't need.

## Debug Builds

```bash
# Debug build of just detailed_route — compiles 1,165 files
bazelisk build -c dbg //test/orfs/openroad:detailed_route

# vs. debug build of monolithic openroad — compiles 3,788 files
bazelisk build -c dbg //:openroad
```

After the initial build, incremental debug rebuilds (edit one `.cpp`)
take ~10 seconds for a standalone binary. The monolithic binary relinks
all 37 modules.

## How It Works

The `cc_binary` calls the C++ API directly:

```
main() → odb::dbDatabase::read() → dpl::Opendp::detailedPlacement() → write()
```

No `Tcl_Interp`. No `InitOpenRoad()`. No SWIG. No framework.

## Not Yet Standalone

**PDN** (`pdngen`): requires a multi-command configuration sequence
(`define_pdn_grid`, `add_pdn_stripe`, `add_pdn_ring`, `add_pdn_connect`)
before running. Does not map cleanly to a single CLI invocation — would
need a config file or TCL passthrough.

**grt, rsz, gpl, cts**: depend on `sta`/`dbSta` (static timing analysis),
which significantly increases the dependency chain and compilation units.
