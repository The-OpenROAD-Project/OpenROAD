# Field Solver (fsv)

A floating-random-walk (FRW) electrostatic field solver for interconnect
capacitance. It is intended to drive extraction model generation and to
validate extracted capacitance on layout.

The solver is mesh-free. Each walk starts on a Gaussian surface around the
master net. It then hops through the largest cube that touches no conductor,
landing according to the cube's surface Green's function, until it reaches a
conductor. The average walk weight per conductor gives one row of the Maxwell
capacitance matrix, together with a standard error.

## Status

It provides:

- Manhattan conductors above a grounded plane at z = 0. The domain is open
  laterally and above, with no artificial boundary.
- A uniform dielectric, or the planar dielectrics of a process stack read
  from an ITF file (`fsv/Itf.h`, `fsv/Stack.h`). Conformal dielectrics are
  read but not solved yet; the Tcl commands ignore them with a warning.
- Exact cube Green's-function tables from Fourier series, for a uniform cube
  and for a cube split by one planar interface (`LayeredCubeGreens`), sampled
  with alias tables. A walk sizes each cube to hold at most one interface and
  snaps it to a tabulated position.
- First hops importance-sampled by the flux kernel, with interpolation
  between tabulated interface positions so that their weights stay bounded.
  A control variate exploits the zero-mean weights.
- Deterministic results for any thread count, using a counter-based Philox
  RNG and fixed batches reduced in order.
- Tcl commands that read a stack, define conductors and solve.

Later phases add conformal dielectrics, a 2D mode, odb input, and Python
commands.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

Coordinates are in µm and capacitances in fF. The commands keep one
dielectric (a uniform permittivity, 1.0 until set, or an ITF stack) and one
set of conductors, which each solve uses. A net is a set of boxes. A master
net's Gaussian surface encloses the bounding box of its boxes, so no other
conductor may touch that bounding box yet.

### Read ITF

The `read_itf` command reads a process stack from an ITF file and makes it
the dielectric. The ground plane is the top of the lowest dielectric, and the
background permittivity fills everything above the stack. Conformal
dielectrics are ignored with a warning until they are supported. The
conductors defined so far are kept.

```tcl
read_itf
    filename
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `filename` | Path of the ITF file. The type is `string`. |

### Set Field Solver Permittivity

The `set_field_solver_permittivity` command makes the dielectric a uniform
relative permittivity, replacing any ITF stack.

```tcl
set_field_solver_permittivity
    permittivity
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `permittivity` | Relative permittivity. The type is `float`, greater than 0. |

### Add Field Solver Box

The `add_field_solver_box` command adds a conductor box to a net, creating
the net on first use. Give the box either by its corners, or by a layer of
the ITF stack and a rectangle, in which case the layer gives the z extent. A
via layer spans from the top of its lower conductor to the bottom of its
upper conductor. A via from `SUBSTRATE` lands on the ground plane, which
would ground its net, so it is rejected.

```tcl
add_field_solver_box
    -net net
    [-box {x0 y0 z0 x1 y1 z1}]
    [-layer layer]
    [-rect {x0 y0 x1 y1}]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-net` | Net name. The type is `string`. Required. |
| `-box` | Corners of the box in µm, above the ground plane at z = 0. The type is a list of 6 `float`s. Excludes `-layer` and `-rect`. |
| `-layer` | Conductor or via layer name of the ITF stack. The type is `string`. Requires `-rect` and a stack from `read_itf`. |
| `-rect` | Corners of the box in the x-y plane in µm. The type is a list of 4 `float`s. Requires `-layer`. |

### Clear Field Solver

The `clear_field_solver` command removes every net and box. The dielectric
is kept.

```tcl
clear_field_solver
```

### Solve Capacitance

The `solve_capacitance` command solves one row of the capacitance matrix
for each master net, reports it, and returns a Tcl dict keyed by master net.
Each value is a dict with these keys:

- `total`, `total_sigma`: the total capacitance and its standard error.
- `ground`, `ground_sigma`: the coupling to the ground plane.
- `couplings`, `coupling_sigmas`: dicts from each other net to its coupling
  capacitance and standard error. Couplings are positive.
- `walks`, `hops`: the walks taken and their total hops.
- `seconds`: the solve's wall-clock time.

The solve stops when the standard error of the total, and of each coupling
of at least `-coupling_threshold` of the total, falls below its tolerance.
It runs on the threads set by `set_thread_count`; the results do not depend
on the thread count.

```tcl
solve_capacitance
    [-coupling_threshold fraction]
    [-max_walks walks]
    [-min_walks walks]
    [-nets nets]
    [-panels panels]
    [-seed seed]
    [-tol_coupling tolerance]
    [-tol_total tolerance]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-coupling_threshold` | Couplings smaller than this fraction of the total do not need to meet `-tol_coupling`. The type is `float` in the range `[0, 1]`. The default is `0.01`. |
| `-max_walks` | Walks after which the solve stops regardless of the tolerances. The type is `int`, at least 1. The default is `100000000`. |
| `-min_walks` | Walks before the tolerances are first checked. The type is `int`. The default is `16384`. |
| `-nets` | Master nets to solve. The type is a list of net names. The default is every net. |
| `-panels` | Panels per cube-face edge in the Green's-function tables. The type is `int`, a positive multiple of 16. The default is `32`. |
| `-seed` | Random seed. The type is `int`, from 0 to 2^63 - 1. The default is `1`. |
| `-tol_coupling` | Target standard error of each coupling, relative to the coupling. The type is `float`, greater than 0. The default is `0.05`. |
| `-tol_total` | Target standard error of the total, relative to the total. The type is `float`, greater than 0. The default is `0.01`. |

## Example scripts

Two wires on met1 of the SkyWater 130 stack, crossed by one on met2:

```text
read_itf sky130.itf
add_field_solver_box -net a -layer met1 -rect {0 0 10 0.14}
add_field_solver_box -net b -layer met1 -rect {0 0.28 10 0.42}
add_field_solver_box -net c -layer met2 -rect {4.5 -3 4.64 3.42}
set result [solve_capacitance -nets a -tol_total 0.005]
puts [dict get $result a couplings b]
```

See also `./test/field_solver_uniform.tcl` and `./test/field_solver_itf.tcl`.

## C++ API

```cpp
fsv::Geometry geometry;                    // nets and boxes in µm, above z = 0
const int a = geometry.addNet("a");
geometry.addBox(a, {{0, 0, 0.5}, {4, 0.1, 0.6}});
// ... more nets and boxes ...

fsv::Solver solver({.threads = 32});       // shared tables + worker pool
const fsv::Scene scene(geometry, 3.9);     // uniform eps_r, or a Stack
fsv::SolveOptions tolerances;              // 1% total, 5% coupling by default
const fsv::CapRow row = solver.solve(scene, a, tolerances);
// row.total(), row.coupling(net), row.ground, with standard errors.
```

Reuse one `Solver` for the whole run and one `Scene` per geometry.
`fsv::FieldSolver` holds the state behind the Tcl commands.

## Regression tests

There are a set of regression tests in `./test`, registered in the tool's
Bazel `test/BUILD` file.

```shell
bazelisk test //src/fsv/test/...
bazelisk run //src/fsv/test:BenchCube   # accuracy and thread scaling
bazelisk run //src/fsv/test:BenchGrid   # throughput on a two-layer wire grid
```

`TestGolden` compares fsv with reference values for the structures in
`test/golden`: cross sections against a 2D finite-volume solution
(`fv2d_golden.py`, numpy and scipy), and a 3D structure against FasterCap
(`fastercap_golden.py`), which is not a build dependency. The scripts
regenerate the references.

## Limitations

- Conformal dielectrics are not solved.
- No other conductor may touch a master net's bounding box.
- Geometry is given box by box; reading it from the design database comes
  later.
