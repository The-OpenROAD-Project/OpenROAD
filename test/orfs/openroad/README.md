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

## How It Works

The `cc_binary` calls the C++ API directly:

```
main() → odb::dbDatabase::read() → dpl::Opendp::detailedPlacement() → write()
```

No `Tcl_Interp`. No `InitOpenRoad()`. No SWIG. No framework.
Links: `//src/dpl` + `//src/odb` + `//src/utl` (3 targets, not 37).
