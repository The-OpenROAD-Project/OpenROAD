# Timing Python API — Known Bugs & Concept Demo Status

## What this is

A concept demo for generating self-contained HTML timing reports from
any ORFS stage, runnable via `bazelisk build` / `bazelisk run`.  The
pipeline works end-to-end:

```
bazelisk build //test/orfs/gcd:gcd_synth_timing   # build report
bazelisk run //test/orfs/gcd:gcd_synth_timing      # open in browser
```

## What works

| API | Status | Notes |
|-----|--------|-------|
| `getWorstSlack(Max)` | OK | Returns WNS |
| `getTotalNegativeSlack(Max)` | OK | Returns TNS |
| `getEndpointCount()` | OK | |
| `getEndpointSlackMap(Max)` | OK | All endpoint names + slacks |
| `getClockInfo()` | **BUG** | Aborts after any prior STA call |
| `getTimingPaths()` | **BUG** | Aborts after any prior STA call |
| `getWorstSlack(Min)` / hold | Not tested | Skipped to avoid contaminating state |

## Bug: STA search state reuse causes abort

### Symptom

Calling `getTimingPaths()` or `getClockInfo()` after
`getWorstSlack()` causes a C++ `abort()` (core dump).  The same
methods work fine if called *first*, before any other STA query.

### Root cause (hypothesis)

`getWorstSlack()` calls `sta->worstSlack()` internally, which runs
`Search::findPathEnds()`.  This populates STA's internal search
state.  When `getTimingPaths()` subsequently calls
`Search::findPathEnds()` with different parameters, STA hits an
assert or null pointer in its search infrastructure because the
previous search state was not properly reset.

Similarly, `getClockInfo()` calls `sta->cmdMode()->sdc()->clocks()`
which traverses STA's SDC data structures — these may be in an
inconsistent state after a search pass.

### Likely fix

Before calling `findPathEnds()` in `Timing::getTimingPaths()`, add:

```cpp
sta->ensureGraph();
sta->searchPreamble();
```

Or more precisely, call `sta->search()->deletePathsInvalidated()`
to clear stale search state.  The Qt GUI code in
`STAGuiInterface` likely doesn't hit this because it operates in a
different call sequence (the GUI creates a fresh Timing object per
query, or the Tcl event loop resets state between commands).

### Workaround (current)

The concept demo uses these workarounds:

- **Endpoint data**: `getEndpointSlackMap()` works and provides
  endpoint names + slacks, which is the primary data for the
  histogram and path table.
- **Clock info**: Extracted via Tcl `all_clocks` / `get_property`
  instead of the C++ `getClockInfo()`.
- **Path detail** (arcs, cell delays, slew, load): Not available in
  the concept demo.  The HTML still renders a histogram and path
  table, but without per-arc detail.

## Bug: evalTclString("sta::corners") infinite loop

Calling `design.evalTclString("sta::corners")` from within the
Python API triggers "too many nested evaluations (infinite loop?)".
This is likely because `sta::corners` returns C++ iterator objects
that confuse the nested Tcl evaluator.  Not blocking — we don't
need corner enumeration for the concept demo.

## What's missing for production

1. **Per-arc path detail**: needs the `getTimingPaths()` bug fixed
2. **Hold analysis**: needs `getWorstSlack(Min)` tested in isolation
3. **Multi-corner support**: the demo loads all NLDM FF libs from
   `PdkInfo.libs`; production should use only the libs specified by
   the ORFS flow's `LIB_FILES` variable
4. **Waveform data in ClockInfo**: the Tcl fallback doesn't extract
   clock waveform edges or source pins

## Files

| File | Role |
|------|------|
| `include/ord/Timing.h` | 3 structs + 5 methods (top-level namespace) |
| `src/Timing.cc` | C++ implementation (~170 lines) |
| `src/OpenRoad-py.i` | SWIG templates for new types |
| `etc/timing_report.py` | HTML+MD generator (concept demo) |
| `etc/BUILD` | `py_binary` for timing_report |
| `test/orfs/timing.bzl` | Custom Starlark rule + `orfs_timing_stages()` macro |
| `test/orfs/gcd/BUILD` | Timing targets for gcd |
| `test/orfs/mock-array/BUILD` | Timing targets for Element + MockArray |
| `test/timing_report_api.py` | Unit test (needs golden file) |
