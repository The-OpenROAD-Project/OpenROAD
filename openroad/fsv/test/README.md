# fsv tests

Run them all with:

```shell
bazelisk test //src/fsv/test/...
```

| Kind | Tests | What they check |
| ----- | ----- | ----- |
| Unit | `TestCubeGreens`, `TestLayeredCubeGreens`, `TestSquareGreens`, `TestDielectric`, `TestSpatialIndex`, `TestItf` | The Green's-function tables, the cube query, the nearest-conductor index and the ITF reader, against exact results. |
| Solver | `TestSolver` | Analytic and self-consistency checks: a cube over ground, reciprocity, permittivity scaling, identical results for any thread count, panel convergence, and conformal invariants. |
| Golden | `TestGolden` | fsv against reference solutions computed by other solvers; see below. |
| Tcl | `field_solver_*` | The Tcl commands, including a cross section against its 2D reference and nets of a routed design (`field_solver_gcd`, see below); each compares its log with the `.ok` file of the same name. |
| Docs | `fsv_man_tcl_check`, `fsv_readme_msgs_check` | The module README against the command definitions and messages. |

## Golden tests

`TestGolden` compares fsv with capacitances computed by independent
solvers. Those solvers do not run in the build or in CI: they are slow and
need tools that are not build dependencies. Their results are stored in
`golden/`, and the test only runs fsv and compares.

Each comparison has three parts in `golden/`:

- **Structure files** (`<name>.txt`): the geometry in µm. `background
  <eps>` and `layer <top> <eps>` give the planar stack bottom-up from the
  ground plane at z = 0; `box <net> x0 y0 z0 x1 y1 z1` adds a conductor box;
  `conformal <name> <bottom> <top> <SW_T> <TW_T> <eps>` wraps every box
  whose z extent is [bottom, top] (lines for one extent nest outward). The
  conformal rules are in the module README and in `fv2d_golden.py`.
- **A reference file**, one per solver, with one line per Maxwell matrix
  entry: `<structure> <master> <net> <capacitance>`. The diagonal entry is
  the total; the others, `ground` included, are minus the couplings.
- **The generator script** that wrote the reference file. Its docstring has
  the command, the solver settings and how accurate the values are.

| Test | Structures | Reference file | Generator | Comparison |
| ----- | ----- | ----- | ----- | ----- |
| `TwoD.MatchesFiniteVolume` | `etch_stop_pair`, `three_wires`, `conformal_pair`, `nested_conformal`, `merged_conformal`, `sky130_met1_pair` | `cross_section.txt` (fF/µm) | `fv2d_golden.py`: 2D finite volumes, extrapolated in grid spacing (numpy, scipy) | Per unit length, in fsv's 2D mode (cross-section scenes). Totals within 0.3%, couplings of at least 5% of the total within 0.5%, each plus 3σ. |
| `LongLine.MatchesFiniteVolume` | `etch_stop_pair`, `three_wires`, `conformal_pair`, `sky130_met1_pair` | `cross_section.txt` (fF/µm) | `fv2d_golden.py` | The 3D solver per unit length: the test extrudes the cross sections to wires 10 and 30 µm long, solves both in 3D, and compares (C(30) − C(10)) / 20, which cancels the end effects. The same tolerances. |
| `ThreeD.MatchesFiniteVolume` | `crossing`, `conformal_line_ends`, `conformal_crossing` | `box_3d.txt` (fF) | `fv3d_golden.py`: 3D finite volumes, extrapolated in grid spacing (numpy, scipy, pyamg) | The 3D structure as given: crossings and line ends, which cross sections cannot show. Totals within 0.5%, couplings of at least 5% within 1%, each plus 3σ. |

## Routed design: sky130hd gcd

`field_solver_gcd` reads nets of gcd routed for sky130hd by
OpenROAD-flow-scripts, trimmed to the conductors near three nets:
`_042_` (a short met1 net), `_119_/_067_` (jogs and 19 vias up to met3)
and `req_msg[10]` (an input ending at a block pin on the die's edge). It
solves them with a 4 µm halo and a fixed walk count.

`field_solver_gcd.def` comes from the flow's `6_final.def`, trimmed by
`gcd/trim_def.py` to a region 9 µm around each of the three nets' routing:
the cells touching it, the block pins in it, every net with routing or a
pin there, and of each net the routing statements that come within it. It
covers windows of up to a 6 µm halo and a 1 µm margin: nets read from it
give the same rows as in the full design, up to the order of the nets.
Routing gcd with another OpenROAD version changes the design, so
regenerate `field_solver_gcd.ok` with it. To regenerate the design:

```shell
cd flow
make DESIGN_CONFIG=./designs/sky130hd/gcd/config.mk
python3 ../tools/OpenROAD/src/fsv/test/gcd/trim_def.py \
  results/sky130hd/gcd/base/6_final.def \
  ../tools/OpenROAD/test/sky130hd/sky130_fd_sc_hd_merged.lef \
  ../tools/OpenROAD/src/fsv/test/field_solver_gcd.def 9 \
  _042_ _119_/_067_ "req_msg[10]"
```

Measured on the full design (64 threads, tolerances of 1% on the total and
5% on each coupling of at least 1% of it), the window hardly matters: against a halo of
6h (h the net's height), totals at 2h moved by at most 0.003% and couplings
by at most 0.3% of the total (0.07% at 3h), while the window's leakage was
up to 2.7% at 2h and 1% at 3h. A net took 0.6–2.8 s to solve and 2–60 ms to
read; 16–24% of start points fell inside the surfaces, where vias and pins
meet wires.

| Net | Pins | Halo 2h / 3h / 4h: leakage | Total change at 3h | Seconds |
| ----- | ----- | ----- | ----- | ----- |
| `_042_` | 2 | 0.99 / 0.64 / 0.34% | < 0.001% | 0.6–1.7 |
| `_118_` | 2 | 2.73 / 0.95 / 0.50% | < 0.001% | 2.0 |
| `_119_/_002_` | 2 | 1.12 / 0.56 / 0.19% | < 0.001% | 1.4–1.6 |
| `_119_/_037_` | 4 | 0.56 / 0.26 / 0.16% | < 0.001% | 2.3–2.8 |
| `net155` | 9 | 0.17 / 0.10 / 0.07% | < 0.001% | 1.7 |
| `net188` | 17 | 0 (the window covers the die) | 0 | 1.0 |

## Benchmark

`BenchSuite` times the solver at its default tolerances (standard errors of
1% of the total, on the total and on every coupling) on three groups, solving every net of
each case: `3d` (the 3D golden structures, and `etch_stop_pair` and
`conformal_pair` extruded to 10 µm lines), `2d` (the six golden cross
sections) and `nets` (the three nets of `field_solver_gcd.def` with a 4 µm
halo, or any design's nets with `--lef`, `--def`, `--itf`, `--nets`,
`--halo`). Each solve reports its seconds, walks, hops per walk, the
relative standard error of one walk (`sigma1`, σ / total × √walks: the walk
count scales with its square), the window's leakage,
the scene's box count and its smallest box dimension. A short solve of
every net first builds the tables it needs, so the times are warm.
`BenchSuite_smoke`, a test, runs the cross sections on two threads.

```shell
bazelisk run //src/fsv/test:BenchSuite -- --threads 64 [--groups 3d,2d,nets] [--csv FILE]
```

`bench_split.py` groups a perf profile into the stages of a walk:

```shell
cd bazel-bin/src/fsv/test/BenchSuite.runfiles/_main
perf record -o perf.data ../../BenchSuite --threads 64 --groups nets
perf report -i perf.data --no-children --sort symbol --stdio \
  | python3 <workspace>/src/fsv/test/bench_split.py
```

### Baseline

64 threads on an AMD EPYC 9B45 (32 cores, 2 threads each), 2026-10-24.
The warm-up (tables and a short solve of each net) took 2.7 s more.

| Group | Solves | Seconds per solve (geometric mean, range) | Hops per walk | `sigma1` | Rejected |
| ----- | ----- | ----- | ----- | ----- | ----- |
| `3d` | 10 | 0.082 (0.013–0.349) | 31–151 | 2.8–7.4 | 0 |
| `2d` | 14 | 0.020 (0.005–0.068) | 20–77 | 1.6–4.8 | 0 |
| `nets` | 3 | 0.70 (0.37–1.59) | 49–87 | 5.7–7.3 | 24–39% |

- **Targets.** 3D patterns are under 1 s and the nets within 1–5 s, but
  a conformal cross section takes 68 ms (`nested_conformal`) against
  50 ms.
- **Variance.** `sigma1` is 1.6–3.9 on most cases, planar or conformal,
  but 6.9–7.4 for the conformal line ends and the upper wire of the
  conformal crossing, and 5.7–7.3 for the routed nets, which need 3–5
  times the walks of the others: the first hop near line ends and the
  target's own shells is the largest cost.
- **Rejected start points.** 24–39% of the walks of routed nets, where
  vias and pins meet wires.
- **Capture distance.** Routed windows hold 0.01 µm boxes against
  0.14–0.2 µm in the patterns, which shrinks the capture distance 14–20
  times, but their hops per walk are in line with the patterns'.

Where the time goes, from `bench_split.py`:

| Stage | `3d` | `2d` | `nets` |
| ----- | ----- | ----- | ----- |
| Dielectric planes (the cube's interface query) | 45% | 47% | 23% |
| Shell candidates (the shell index) | 0% | 0% | 16% |
| Nearest conductor | 9% | 11% | 42% |
| Table sampling | 25% | 24% | 10% |
| Walk loop | 12% | 10% | 5% |
| Random numbers | 6% | 6% | 3% |

### Comparing builds

At 64 threads, repeated runs of one build differ by up to 8%, more than
most changes to the cost of a hop. Compare two builds single-threaded, side
by side, each pinned to its own physical core, and check their results
against each other from the CSVs:

```shell
taskset -c 4 ./BenchSuite.base --threads 1 --csv base.csv &
taskset -c 6 ./BenchSuite.new --threads 1 --csv new.csv &
wait
```

Results are deterministic, so a change meant to leave them unchanged shows
identical CSVs apart from the `seconds` column.

### Sampling and stopping

Three changes after the baseline, 2026-10-25, on the same machine:

- Start points are drawn on the exposed parts of the Gaussian surface,
  computed once per solve, so no draw is rejected.
- A start piece is drawn in proportion to its area times the mean of 1 / h,
  h the first cube's half-size, and its walks are weighted by its area over
  that probability. A walk's weight scales as 1 / h, and small first cubes
  (near shell edges, and on boxes with small offsets) carried most of the
  variance of routed nets.
- Each round is sized to the walks the tolerances still need, and couplings
  are held to a fraction of the total (`tol_coupling`, the same as
  `tol_total` by default) instead of 5% of each coupling of at least 1% of
  it.

| Group | Solves | Seconds per solve (geometric mean, range) | Hops per walk | `sigma1` |
| ----- | ----- | ----- | ----- | ----- |
| `3d` | 10 | 0.053–0.059 (0.010–0.246) | 31–158 | 2.8–6.9 |
| `2d` | 14 | 0.014 (0.004–0.053) | 20–81 | 1.6–4.5 |
| `nets` | 3 | 0.24 (0.19–0.28) | 100–120 | 4.4–4.5 |

`nested_conformal` takes 46–53 ms against the 50 ms target. Solves now stop
at 0.87–0.99% of the total against 1%, where whole rounds used to overshoot
to 0.65–0.85%. Part of the nets' gain is the looser hold on their small
couplings, which the old definition solved to 5% of their own value.

### Surface placement

Around a box with conformal shells the Gaussian surface can sit clear of the
shells' faces or inside the innermost shell, where a first cube meets no
parallel shell face but is smaller. Which costs fewer hops depends on the
structure: on an offset sweep the inside surface was 4 times cheaper on
`conformal_line_ends` and the upper wire of `conformal_crossing`, and 1.5
times dearer on its lower wire. Where the first cubes on the clear surface
shrink (an area-weighted offset / h above 1.1), a solve runs a trial round
of 8192 walks on each surface, on streams of its own that it then discards,
and keeps the inside surface if it costs at most 0.8 of the other. Only the
shells wrapping a box move it (not those of the target's other boxes, such
as the wire a via sits in), and only when its inside offset is at least 0.05
of the usual one. A solve allowed fewer than
131072 walks (`max_walks`) skips the trial. The trial compares the walks each surface needs, held within
`min_walks` and `max_walks`, times its hops per walk. `CapRow::inside_surface` and
`trial_walks`, and the benchmark's CSV, report the choice.
2026-10-25, 64 threads:

| Group | Solves | Seconds per solve (geometric mean, range) | Hops per walk | `sigma1` |
| ----- | ----- | ----- | ----- | ----- |
| `3d` | 10 | 0.037–0.040 (0.010–0.068) | 31–98 | 2.8–6.9 |
| `2d` | 14 | 0.013 (0.005–0.033) | 20–55 | 1.6–6.4 |
| `nets` | 3 | 0.20 (0.15–0.25) | 46–106 | 4.5–5.7 |

`nested_conformal` takes 34–35 ms. Inside surfaces raise `sigma1` (more
walks) but shorten the walks more. Structures whose trial keeps the clear
surface pay for the trial, 5–15% single-threaded.

### Adding or changing a structure

1. Add or edit `golden/<name>.txt`.
2. Regenerate the reference file of its test with the generator; the
   command is at the top of each script. Rerun it on every structure the
   file holds, since the script writes the whole file.
3. Add the structure to the test's list in `cpp/TestGolden.cpp`.
4. Commit the structure, the reference file and the test change together.

A changed generator needs the same regeneration. Expect small changes in
the existing values; the script's header reports how accurate they are.

### Seeing a structure

`fv2d_golden.py --plot DIR --h H <structures>` draws each cross section and
the finite-volume grid at spacing H into `DIR/<name>.png`.
