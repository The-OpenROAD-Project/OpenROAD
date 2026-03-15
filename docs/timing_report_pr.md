# Timing Report HTML Generation — Integration Demo

## Summary

Self-contained HTML timing reports generated from any ORFS stage,
matching the OpenROAD Qt GUI (Charts + Timing Report widgets) as
closely as possible without modifying `src/sta`.

**This is an integration demo.**  Individual features and bug fixes
will be broken out into separate PRs.  This branch will be
force-rebased on top of those changes as they land, serving as a
living status report.

## Usage

```bash
cd tools/OpenROAD

# Build timing report for any ORFS stage
bazelisk build //test/orfs/gcd:gcd_synth_timing
bazelisk build //test/orfs/mock-array:MockArray_4x4_base_synth_timing

# Open in browser
bazelisk run //test/orfs/gcd:gcd_synth_timing
bazelisk run //test/orfs/mock-array:MockArray_4x4_base_synth_timing
```

Adding timing reports to any ORFS design is one line in BUILD:

```python
load("//test/orfs:timing.bzl", "orfs_timing_stages")

orfs_timing_stages(
    name = "gcd",
    stages = ["synth", "floorplan", "place", "cts", "grt", "route"],
    timing_script = "//etc:timing_report",
)
```

## What it produces

A single self-contained HTML file (~300KB for MockArray) with:

- **Endpoint Slack Histogram** — nice-bucket algorithm ported from
  `chartsWidget.cpp`, red/green bars, dropdowns for path group
  and clock filtering
- **Timing Report table** — columns match Qt `TimingPathsModel`:
  Capture Clock, Required, Arrival, Slack, Skew, Logic Delay,
  Logic Depth, Fanout, Start, End (all in ps)
- **Data Path Details** — per-arc Pin, Fanout, ↑/↓, Time, Delay,
  Slew, Load (matching `TimingPathDetailModel`)
- **Unconstrained pin count** below histogram
- **Draggable sashes** between panels

Plus a markdown summary for PR comments:

**PASS** Timing — `MockArray` — `asap7` |
WNS `0.0000 ns` TNS `0.0000 ns` | Endpoints 5172

## Architecture

```
┌──────────────────────────────────────────────────┐
│  timing.bzl  (custom Starlark rule _timing_gen)  │
│  - Accesses PdkInfo.libs for liberty files       │
│  - Accesses OrfsInfo for ODB/SDC                 │
│  - Runs etc/timing_report py_binary              │
└─────────────────────┬────────────────────────────┘
                      │
┌─────────────────────▼────────────────────────────┐
│  etc/timing_report.py                            │
│  - C++ Timing API: WNS, TNS, endpoint slacks    │
│  - Tcl report_checks -format json: path detail   │
│  - Tcl all_clocks: clock info                    │
│  - Generates 1_timing.html + 1_timing.md         │
└──────────────────────────────────────────────────┘
```

The Qt GUI implementation (`chartsWidget.cpp`, `staGui.cpp`,
`timingWidget.cpp`) is the single source of truth.  This script
generates a practical facsimile.

## Known Issues

### STA search state crash (blocker for C++ extension points)

The C++ `Timing::getTimingPaths()`, `getClockInfo()`, and
`getSlackHistogram()` abort when called from the Bazel Python API.
The root cause is that `sta->slack()` / `sta->endpoints()` /
`search->findPathEnds()` leave STA's internal search state in a
condition where subsequent calls crash.  `ensureGraph()` +
`searchPreamble()` don't help.

**Workaround**: timing paths extracted via Tcl `report_checks
-format json` which goes through the proper Tcl command lifecycle.
Histogram bucketing done in JS (ported `snapInterval` algorithm).

**Fix needed**: changes to `src/sta` to properly reset search state
between Python API calls, or expose a `resetSearch()` method.

### Histogram buckets differ slightly from Qt

The JS `snapInterval` algorithm matches `chartsWidget.cpp` but
unconstrained endpoint filtering differs slightly (we count all
infinite-slack endpoints; Qt only counts inputs and connected nets).

### Missing Qt features

See `docs/timing_report_todo.md` for the full list including:
macro clock insertion latency tooltips, clock network delay summary
row, hold analysis, column visibility toggle, per-path-group
histogram filtering via endpoint data.

## Files Changed

| File | Purpose |
|------|---------|
| `include/ord/Timing.h` | 3 structs + 5 methods (SWIG-compatible) |
| `src/Timing.cc` | C++ implementation (~240 lines) |
| `src/OpenRoad-py.i` | SWIG templates for new types |
| `etc/timing_report.py` | HTML+MD generator (~750 lines) |
| `etc/BUILD` | `py_binary` target |
| `test/orfs/timing.bzl` | Custom Starlark rule + macro |
| `test/orfs/gcd/BUILD` | Timing targets for gcd |
| `test/orfs/mock-array/BUILD` | Timing targets for MockArray |
| `docs/timing_api_bugs.md` | STA crash documentation |
| `docs/timing_report_todo.md` | Qt GUI discrepancy list |
