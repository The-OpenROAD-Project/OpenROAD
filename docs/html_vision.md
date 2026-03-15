# Vision: Static HTML as the User Interface

## The Problem

The Qt GUI requires click-and-wait interaction. In the age of CI and
AI-assisted development, a self-contained HTML artifact generated as part
of the build is strictly superior: instant, shareable, diffable, CI-native,
air-gap safe.

## The Vision

**Qt GUI** becomes a **developer tool** — deep insight into internals,
debugging, experimental features. It remains valuable for developers who
need to inspect every detail of the EDA flow. Qt becomes a developer-only
dependency, not a deployment dependency.

**Static HTML** becomes the **user tool** — an interactive report that
replicates all user-facing Qt GUI features. Even if the build takes hours,
the HTML is instantaneous to open. Zero runtime dependencies. Works
offline. One file to download, share, or attach to a PR.

**Python** is a **build-time dependency only** — it extracts data from
OpenROAD's C++ APIs and Tcl commands to generate the HTML. It is not
needed at runtime and is not a deployment concern.

## Architecture

```
C++ (source of truth)          Python (extraction)         HTML (rendering)
─────────────────────          ───────────────────         ─────────────────
Qt GUI code defines what       Python calls C++ API +      Self-contained HTML
values/algorithms/sort         Tcl commands to extract     renders pre-computed
orders are correct.            pre-computed data.           data. Zero deps.
                               No reimplementation.
Qt = developer dependency.     Generates single .html.     Works offline.
Python = build dependency.     No runtime dependency.       No click-and-wait.
```

**Key principle**: OpenSTA Tcl commands (`report_checks`, etc.) and the Qt
GUI C++ code are the single source of truth. Python extension points
extract data via:

1. Existing Tcl commands (`report_checks -format json`, `all_clocks`, etc.)
2. New C++ methods on `ord::Timing` exposed via SWIG
3. ODB Python bindings for geometry data

No algorithm reimplementation in JavaScript or Python.

## Prioritized Features

Ordered by minimum churn / maximum value. Each tier is independently
useful.

### Tier 1: Tcl-only, no C++ changes needed

| # | Feature | Approach |
|---|---------|----------|
| 1 | Fix path table click/filter | Fix JS slack range matching |
| 2 | Show `<No clock>` for unconstrained | Check empty clock in JSON |
| 3 | Sort equal-slack paths by endpoint | JS tiebreaker sort |
| 4 | Sash between histogram and report | CSS/JS draggable divider |
| 5 | Title: platform/design/variant - stage | Read from env vars |
| 6 | Tooltip: `[lo, hi) ps` bracket format | JS string format change |
| 7 | DRC violations panel | `dbMarkerCategory.writeJSON()` via Tcl |
| 8 | Hold analysis tab | `report_checks -path_delay min` |
| 9 | Path group endpoint histograms | `report_checks -group_path_count` per group |

### Tier 2: Small C++ extension points (1-2 methods each)

| # | Feature | C++ method | Mirrors |
|---|---------|------------|---------|
| 10 | Histogram from C++ (exact match) | `Timing::getSlackHistogram()` | `chartsWidget.cpp` `populateBins()` + `utl::Histogram` |
| 11 | Unconstrained pin count (exact) | Inside `getSlackHistogram()` | `removeUnconstrainedPinsAndSetLimits()` |
| 12 | Path group names | `Timing::getPathGroupNames()` | `sta->search()->pathGroups()` |
| 13 | Time/cap unit scale | `Timing::getTimeUnitScale()` | `sta->units()->timeUnit()->scale()` |
| 14 | Fix getClockInfo crash | Add `initSTA()` preamble | `STAGuiInterface::initSTA()` |
| 15 | Fix getTimingPaths crash | Add `initSTA()` preamble | Same |

### Tier 3: Medium C++ extension points

| # | Feature | C++ method | Mirrors |
|---|---------|------------|---------|
| 16 | Clock tree data | `Timing::getClockTrees()` | `STAGuiInterface::getClockTrees()` |
| 17 | Macro clock insertion tooltip | In path detail extraction | `clkTreeDelay()` on liberty |
| 18 | Routing congestion grid | `Timing::getRoutingCongestion()` | `GlobalRoutingDataSource::populateMap()` |
| 19 | Placement density grid | `Timing::getPlacementDensity()` | `PlacementDensityDataSource` |

### Tier 4: Layout geometry export (large scope)

| # | Feature | Approach | Scope |
|---|---------|----------|-------|
| 20 | Layout image (raster) | Export from ODB geometry, render in Canvas | Medium |
| 21 | Instance boundaries | Export `dbInst` bbox + master name as JSON | Medium |
| 22 | Routing shapes per layer | Export via Search RTree queries as JSON | Large |
| 23 | Layer controls (toggle/color) | `dbTech::getLayers()` properties | Medium |
| 24 | Interactive zoom/pan | Canvas-based viewport with spatial index | Large JS |
| 25 | Full vector layout viewer | Implement JSON Painter backend | Large |

### Tier 5: Full parity features

| # | Feature | Approach |
|---|---------|----------|
| 26 | Instance inspector (click to inspect) | Export properties per instance |
| 27 | Net highlighting | Export net connectivity + routing |
| 28 | Hierarchy browser | Export `dbModule` tree |
| 29 | Pin density heatmap | Grid export |
| 30 | IR drop heatmap | PSM data export |
| 31 | Clock tree SVG visualization | From #16 data, SVG rendering |
| 32 | Power/ground net display | Special net geometry export |

## Dependencies

- **Qt**: Developer-only dependency. Not needed for HTML generation.
- **Python**: Build-time only. Extracts data, generates HTML, then done.
- **HTML output**: Zero dependencies. One file, works offline, instant.

No GUI build required at any tier. All geometry data comes from ODB C++
APIs and Tcl commands, not from Qt rendering.

## Usage

```bash
# Timing report (any stage)
bazelisk run //test/orfs/gcd:gcd_synth_timing

# Full HTML with layout viewer (any stage, no GUI needed)
bazelisk run //test/orfs/gcd:gcd_route_html
```

## Current State

Working prototype: `etc/timing_report.py` generates HTML with endpoint
slack histogram, timing path table + detail, draggable sashes, path group
/ clock dropdown filtering, unconstrained pin count.

PR: https://github.com/The-OpenROAD-Project/OpenROAD/pull/9770
