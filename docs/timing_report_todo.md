# Timing Report — Remaining Work

## Must fix

- **C++ getTimingPaths() / getClockInfo() abort** after getWorstSlack()
  due to STA search state reuse.  Likely fix: call
  `sta->searchPreamble()` before `findPathEnds()` in Timing.cc.
  See docs/timing_api_bugs.md for details.

- **Hold analysis** (Min slack): skipped to avoid contaminating STA
  state.  Once the search state bug is fixed, enable
  `getWorstSlack(Min)` and `getTotalNegativeSlack(Min)`.

- **Unit test golden file**: `test/timing_report_api.py` needs a
  `.ok` golden file.  Run via `openroad -python` (CMake build) or
  fix the C++ bug so it works in Bazel too.

## Should fix

- **Liberty file selection**: the Starlark rule filters `PdkInfo.libs`
  to NLDM FF by string matching (`"NLDM" in path and "_FF_" in path`).
  This should use the platform's actual `LIB_FILES` from `config.mk`
  to match what the ORFS flow uses.

- **Clock waveform and sources**: the Tcl fallback for clock info
  doesn't extract waveform edges or source pin names.  Fix the C++
  `getClockInfo()` or extend the Tcl extraction.

- **`from` pin in arc detail**: the JSON from `report_checks` doesn't
  include the previous pin in each arc.  The `from` field is always
  empty.  Could be computed from consecutive arc entries.

- **Multi-corner**: only tested with single-corner post-synth.
  Later stages (CTS, route) may have multi-corner setups.

## Nice to have

- **ORFS Makefile integration**: `flow/Makefile` target `synth_time`
  using `openroad -python` (works without Bazel).  Needs the script
  copied to `flow/scripts/` in the ORFS tree.

- **mock-array timing targets**: added to BUILD but not tested.
  Run `bazelisk build //test/orfs/mock-array:Element_4x4_base_synth_timing`
  to verify.

- **Arc waterfall chart**: the HTML template has a placeholder for
  per-arc delay bar visualization.  Currently no arcs render visually
  because the arc detail panel display logic needs the `from` pin.

- **Markdown report**: currently minimal (~3KB).  Could include
  ASCII histogram, top failing paths table, summary stats.
