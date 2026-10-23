# fsv tests

Run them all with:

```shell
bazelisk test //src/fsv/test/...
```

| Kind | Tests | What they check |
| ----- | ----- | ----- |
| Unit | `TestCubeGreens`, `TestLayeredCubeGreens`, `TestDielectric`, `TestSpatialIndex`, `TestItf` | The Green's-function tables, the cube query, the nearest-conductor index and the ITF reader, against exact results. |
| Solver | `TestSolver` | Analytic and self-consistency checks: a cube over ground, reciprocity, permittivity scaling, identical results for any thread count, panel convergence, and conformal invariants. |
| Golden | `TestGolden` | fsv against reference solutions computed by other solvers; see below. |
| Tcl | `field_solver_*` | The Tcl commands; each compares its log with the `.ok` file of the same name. |
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
| `CrossSection.MatchesFiniteVolume` | `etch_stop_pair`, `three_wires`, `conformal_pair`, `nested_conformal`, `merged_conformal`, `sky130_met1_pair` | `cross_section.txt` (fF/µm) | `fv2d_golden.py`: 2D finite volumes, extrapolated in grid spacing (numpy, scipy) | Per unit length. The structures are cross sections; the test extrudes them to wires 10 and 30 µm long, solves both in 3D, and compares (C(30) − C(10)) / 20, which cancels the end effects. Totals within 0.3%, couplings of at least 5% of the total within 0.5%, each plus 3σ. |
| `ThreeD.MatchesFiniteVolume` | `crossing`, `conformal_line_ends`, `conformal_crossing` | `box_3d.txt` (fF) | `fv3d_golden.py`: 3D finite volumes, extrapolated in grid spacing (numpy, scipy, pyamg) | The 3D structure as given: crossings and line ends, which cross sections cannot show. Totals within 0.5%, couplings of at least 5% within 1%, each plus 3σ. |

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
