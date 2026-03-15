# Timing Report Feature — Resume Notes

## Commits (on detached HEAD from 7cf7b720fd)

```
cb10281a42 Add timing report HTML/MD generation with Bazel integration
fe269a3de1 Add Timing Python API for GUI-free timing reports
```

To cherry-pick onto a branch:
```bash
git cherry-pick fe269a3de1 cb10281a42
```

## What's Done

### Commit 1: C++ API (fe269a3)
- **Timing.h**: 3 new structs (`ClockInfo`, `TimingArcInfo`, `TimingPathInfo`) + 5 new methods
- **Timing.cc**: Implementations using same STA API as Qt GUI's `STAGuiInterface`
- **OpenRoad-py.i**: SWIG templates for new types (`std::pair`, nested struct vectors)
- **test/timing_report_api.py**: Unit test (needs `.ok` golden file generated)
- **test/BUILD**: Added to `PYTHON_TESTS` list

### Commit 2: Bazel + HTML/MD generation (cb10281)
- **etc/timing_report.py**: Python script generating `1_timing.html` + `1_timing.md`
- **etc/BUILD**: Exports `timing_report.py`
- **test/orfs/timing.bzl**: `orfs_timing()` and `orfs_timing_stages()` macros
- **test/orfs/gcd/BUILD**: Added `orfs_timing_stages()` call for all 6 stages
- **MODULE.bazel**: Reverted to no patches (approach changed to local macros)

## What's In Progress

### Bazel build test (`bazelisk build //test/orfs/gcd:gcd_synth_timing`)

**Status**: Build was running when session ended. The build chain:
1. `gcd_synth` (ORFS synthesis) — likely cached from prior runs
2. `gcd_synth_timing_gen` (genrule) — runs `openroad -python etc/timing_report.py`
3. `gcd_synth_timing` (sh_binary) — launcher that opens HTML

**Known issues to debug if build fails**:
- The genrule uses `$(location //:openroad)` — verify the openroad binary has Python support compiled in
- The genrule moves `1_timing.html` to `gcd_synth_timing.html` — verify paths in sandbox
- The `timing_report.py` calls `design.readDb()` and `design.evalTclString("read_sdc ...")` — these are existing API, should work
- The new Timing methods (`getWorstSlack`, etc.) need the C++ to be compiled — verify the openroad binary includes the new code (it should if built from this tree)

### Potential compilation issues in Timing.cc
- Uses `sta::PathExpanded`, `sta::PathEnd`, `sta::PathGroup` — verify includes resolve
- Uses `sta->worstSlack(getMinMax(minmax))` — returns `Slack` = `float`
- Uses `sta->endpoints()` — returns `VertexSet&`
- `path_end->pathDelay()` can return `nullptr` — handled with ternary
- `path_end->pathGroup()->name()` returns `const std::string&`

## What's Not Done

### mock-array timing targets
- Need to add `orfs_timing_stages()` to `test/orfs/mock-array/BUILD`
- Element: `orfs_timing_stages(name = "Element", variant = "4x4_base", ...)`
- MockArray: `orfs_timing_stages(name = "MockArray", variant = "4x4_base", ...)`

### flow/Makefile `synth_time` target
- `flow/scripts/synth_timing_report.py` exists (copy of `etc/timing_report.py`)
- `flow/Makefile` change is ready but NOT committed (it's in ORFS tree, not OpenROAD)
- Target: `make DESIGN_CONFIG=./designs/nangate45/gcd/config.mk synth_time`

### Golden file for unit test
- Run: `cd tools/OpenROAD && openroad -python test/timing_report_api.py > test/timing_report_api.ok`
- Or via Bazel regression test framework

### bazel-orfs integration (future, after demo is proven)
- Originally planned to patch bazel-orfs to add `timing=True` to `orfs_flow()`
- Abandoned patching because pinned bazel-orfs (commit 858b4fdb) has monolithic 2299-line `openroad.bzl` — completely different structure from local `/home/oyvind/bazel-orfs/`
- Current approach: `orfs_timing_stages()` macro called directly in BUILD files
- Future: when bazel-orfs is updated, add `timing` parameter to `orfs_flow()` natively

## File Inventory

### In OpenROAD tree (committed)
```
include/ord/Timing.h          — 3 structs + 5 methods added
src/Timing.cc                 — ~170 lines implementation
src/OpenRoad-py.i             — SWIG templates
test/timing_report_api.py     — unit test
test/BUILD                    — test list updated
etc/timing_report.py          — HTML+MD generator script
etc/BUILD                     — exports timing_report.py
test/orfs/timing.bzl          — orfs_timing() + orfs_timing_stages()
test/orfs/gcd/BUILD           — timing targets for gcd
```

### In ORFS tree (NOT committed, separate concern)
```
flow/scripts/synth_timing_report.py  — copy of etc/timing_report.py
flow/Makefile                        — synth_time target (not yet added)
```

## How to Test

```bash
# Unit test (needs openroad with Python)
cd tools/OpenROAD
openroad -python test/timing_report_api.py

# Bazel build
cd tools/OpenROAD
bazelisk build //test/orfs/gcd:gcd_synth_timing

# Bazel run (opens browser)
bazelisk run //test/orfs/gcd:gcd_synth_timing

# ORFS Makefile (from flow/ dir)
make DESIGN_CONFIG=./designs/nangate45/gcd/config.mk synth
make DESIGN_CONFIG=./designs/nangate45/gcd/config.mk synth_time
```
