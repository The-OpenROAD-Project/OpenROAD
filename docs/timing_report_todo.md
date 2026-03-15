# Timing Report — Remaining Work

Discrepancies vs the Qt GUI (chartsWidget.cpp, staGui.cpp,
timingWidget.cpp) found by side-by-side comparison.

## Bugs in HTML report

- **Logic Delay column shows arrival instead of logic_delay**
  (`timing_report.py` renders `ps(p.arrival)` in the Logic Delay
  column instead of `ps(p.logic_delay)`).

- **Capture Clock column right-aligned** — Qt has it left-aligned
  (staGui.cpp:116).

- **X-axis label calculation** assumes all bins same width using
  first bin size — should use the actual interval from
  `snapInterval()`.

## Missing Qt features

### Header column tooltips (staGui.cpp:164-191)

- **Skew**: "The difference in arrival times between source and
  destination clock pins of a macro/register, adjusted for CRPR and
  subtracting a clock period.  Setup and hold times account for
  internal clock delays."
- **Logic Delay**: "Path delay from instances (excluding buffers and
  consecutive inverter pairs)"
- **Logic Depth**: "Path instances (excluding buffers and consecutive
  inverter pairs)"

### Macro clock insertion latency tooltip (staGui.cpp:376-407)

On any pin belonging to a macro whose liberty model defines
`max_clock_tree_path` / `min_clock_tree_path`, Qt shows:

    Macro {master} liberty {max_clock_tree_path}
    (internal clock tree delay to sequential elements): {delay}

This is important for hierarchical designs where macro internal
clock tree delay can exceed data path delay.

### Clock summary row in path detail (staGui.cpp:413-428)

Qt path detail shows a first row "clock network delay" with arrival
and computed delay (arrival minus first node).  HTML has no
equivalent.

### Sash between histogram and timing report

Qt uses a QSplitter between the charts panel and the timing report
panel.  HTML has a sash between timing report and path detail but
not between histogram and timing report.

### Fanout/load display when zero (staGui.cpp:444,451)

Qt shows empty string when fanout=0 or load=0.  HTML shows "0" or
"0.000".

### Rise/fall column alignment

Qt center-aligns the ↑/↓ column.  HTML right-aligns it.

### Infinity handling (staGui.cpp:75-81)

Qt displays unconstrained slack as "∞" or "−∞".  HTML will show
the string "Infinity".

### Hold slack histogram

Qt supports toggling between Setup and Hold slack in the histogram.
HTML only generates setup data.

### Column visibility toggle

Qt allows hiding/showing columns via a dropdown menu.  HTML shows
all columns always.

### Bar styling

Qt uses opaque fills with dark borders (`#8b0000` negative,
`#006400` positive).  HTML uses 80% opacity with lighter borders.

### Y-axis snap algorithm

Qt uses a digit-based algorithm (computeMaxYSnap, computeYInterval
in chartsWidget.cpp:964-990).  HTML uses coefficient-based
`snapInterval(maxCount/4)` which produces different tick spacing.

## Backend issues

- **C++ getTimingPaths() / getClockInfo() abort** after
  getWorstSlack() — STA search state reuse.  See
  docs/timing_api_bugs.md.

- **Liberty file selection**: Starlark rule filters PdkInfo.libs by
  string matching `"NLDM" in path and "_FF_" in path`.  Should use
  the platform's actual LIB_FILES.

- **Clock waveform and sources**: Tcl fallback doesn't extract
  waveform edges or source pin names.

- **Unit test golden file**: test/timing_report_api.py needs .ok
  file.

- **Hold analysis**: skipped to avoid STA state contamination.

- **Multi-corner**: untested with later stages.
