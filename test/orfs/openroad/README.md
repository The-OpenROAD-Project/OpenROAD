# Per-Command OpenROAD Binaries

Standalone binaries for individual OpenROAD commands. Each links only
the modules it needs — fast rebuild, direct `gdb`, reproducible bug
reports.

## Quick Start

```bash
# Build (seconds — links dpl + odb + utl, not all 37 modules)
bazelisk build //test/orfs/openroad:openroad-detailed_placement

# Run
bazelisk run //test/orfs/openroad:openroad-detailed_placement -- \
  --read_db placed.odb --write_db legalized.odb -max_displacement 10

# Or with LEF/DEF
bazelisk run //test/orfs/openroad:openroad-detailed_placement -- \
  --read_lef Nangate45.lef --read_def placed.def --write_def legalized.def
```

## Debugging a Crash

```bash
# 1. Build (fast — only dpl module)
bazelisk build //test/orfs/openroad:openroad-detailed_placement

# 2. Debug DIRECTLY — no TCL, no framework, just C++ main()
gdb --args bazel-bin/test/orfs/openroad/openroad-detailed_placement \
  --read_db crash.odb -max_displacement 10

# 3. Or valgrind / ASAN — same binary, same args
valgrind bazel-bin/test/orfs/openroad/openroad-detailed_placement \
  --read_db crash.odb

# 4. Edit src/dpl/src/Place.cpp, rebuild (seconds), re-run
```

## Filing a Bug Report

```
Steps to reproduce:
1. Download the attached crash.odb
2. Run:
   bazelisk run //test/orfs/openroad:openroad-detailed_placement -- \
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

Binaries follow the `openroad-{command}` naming convention (like
`git-{cmd}` — principle of least astonishment).

| Binary | Module deps | Source files | Reduction | Test result |
|--------|------------|-------------|-----------|-------------|
| `openroad-detailed_placement` | dpl + odb + utl | 1,095 | 3.5x fewer | 294 cells, bit-for-bit identical to monolithic |
| `openroad-check_placement` | dpl + odb + utl | 1,095 | 3.5x fewer | Detects violations + passes on legalized |
| `openroad-filler_placement` | dpl + odb + utl | 1,095 | 3.5x fewer | 2,092 fillers placed (gcd Nangate45) |
| `openroad-optimize_mirroring` | dpl + odb + utl | 1,095 | 3.5x fewer | 134 mirrored, bit-for-bit identical to monolithic |
| **`openroad-detailed_route`** | **drt + dst + stt** | **1,165** | **3.2x fewer** | **20s, 3,623 vias (gcd ASAP7)** |
| `openroad-check_antennas` | ant + odb + utl | 1,061 | 3.6x fewer | 0 violations (gcd ASAP7 routed) |
| `openroad-extract_parasitics` | rcx + odb + utl | 1,102 | 3.4x fewer | Runs on routed ODB with -lef_rc |
| `openroad-tapcell` | tap + odb + utl | 1,059 | 3.6x fewer | 114 endcaps (gcd Nangate45) |
| **`openroad-global_route`** | **grt + ant + dpl + stt + dbSta** | **1,160** | **3.3x fewer** | **563 nets, 10,767 um (gcd Nangate45)** |
| `openroad-repair_design` | rsz + est + grt + dpl + dbSta | 1,184 | 3.2x fewer | Builds, needs liberty for STA |
| `openroad-global_placement` | gpl + rsz + grt + dbSta | 1,755 | 2.2x fewer | Placement converged (gcd Nangate45) |
| `openroad-clock_tree_synthesis` | cts + rsz + est + stt + dbSta | 1,193 | 3.2x fewer | Builds, needs liberty for STA |
| `openroad-init_floorplan` | ifp + dbSta | 1,066 | 3.6x fewer | Supports -utilization and -die_area modes |
| `openroad-analyze_power_grid` | psm + est + dpl + grt + dbSta | 1,211 | 3.1x fewer | Supports -net, -voltage_file, -enable_em |
| `openroad-density_fill` | fin + odb + utl | 1,132 | 3.3x fewer | Takes -rules JSON from PDK |
| `openroad-place_pins` | ppl + dbSta | 1,137 | 3.3x fewer | Pins assigned (gcd Nangate45) |
| `openroad-macro_placement` | mpl + par + dbSta | 1,632 | 2.3x fewer | Needs design with macros |
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
bazelisk build -c dbg //test/orfs/openroad:openroad-detailed_route

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

## Known Workarounds

These are hacks local to `test/orfs/openroad/` — no upstream modules
were modified. Each should be fixed properly in the respective module.

**`_exit(0)` in sta-dependent binaries** (global_route, repair_design,
global_placement, clock_tree_synthesis): `sta::ReportTcl::~ReportTcl()`
calls `Tcl_UnstackChannel()` on a nullptr `Tcl_Interp`, causing a
segfault during cleanup. Using `_exit(0)` skips destructors. The routing/
placement itself completes correctly and output is flushed before exit.
Fix: make `ReportTcl` destructor null-safe w.r.t. `Tcl_Interp`.

**No-op `AbstractGraphicsFactory` in detailed_route**: `TritonRoute::
initDesign()` calls `graphics_factory_->reset()` unconditionally. Without
a graphics factory, it segfaults. The standalone binary provides a no-op
implementation. Fix: null-check `graphics_factory_` in `initGraphics()`.

**`dbSta(nullptr, db, &logger)`**: works because `initVars` already
checks `if (tcl_interp)` before calling `setTclInterp()`. This is not a
hack — it's supported by the existing code, just undocumented.

## Not Yet Standalone

**PDN** (`pdngen`): requires a multi-command configuration sequence
(`define_pdn_grid`, `add_pdn_stripe`, `add_pdn_ring`, `add_pdn_connect`)
before running. Does not map cleanly to a single CLI invocation — would
need a config file or TCL passthrough.

**grt, rsz, gpl, cts**: depend on `sta`/`dbSta` (static timing analysis),
which significantly increases the dependency chain and compilation units.
