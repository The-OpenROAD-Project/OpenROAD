# Field Solver (fsv)

A floating-random-walk (FRW) electrostatic field solver for interconnect
capacitance. It is intended to drive extraction model generation and to
validate extracted capacitance on layout.

The solver is mesh-free. Each walk starts on a Gaussian surface around the
target net, the net whose row is solved. It then hops through the largest cube that touches no conductor,
landing according to the cube's surface Green's function, until it reaches a
conductor. The average walk weight per conductor gives one row of the Maxwell
capacitance matrix, together with a standard error.

## Status

It provides:

- Manhattan conductors above a grounded plane at z = 0. The domain is open
  laterally and above, with no artificial boundary.
- A uniform dielectric, or the dielectrics of a process stack read from an
  ITF file (`fsv/Itf.h`, `fsv/Stack.h`): planar layers, and conformal
  dielectrics around the conductors of each layer.
- Exact cube Green's-function tables from Fourier series, for a uniform cube
  and for a cube split by one interface plane (`LayeredCubeGreens`), sampled
  with alias tables. A walk sizes each cube to hold at most one interface
  plane, of any orientation, and snaps it to a tabulated position
  (`Dielectric`). Within the capture distance of an edge where two
  interfaces meet, cubes keep a minimum size.
- First hops importance-sampled by the flux kernel, with interpolation
  between tabulated interface positions so that their weights stay bounded.
  A control variate exploits the zero-mean weights.
- Deterministic results for any thread count, using a counter-based Philox
  RNG and fixed batches reduced in order.
- Cross sections (`Geometry2D` rectangles in the x-z plane, solved through
  `Scene2D`; `add_field_solver_rect`): a 2D walk on squares, with
  the square counterparts of the cube tables (`SquareGreens`,
  `LayeredSquareGreens`), giving capacitance per unit length. Wires run
  along y, infinitely long.
- Nets read from the design with their neighbours within a halo
  (`fsv/NetWindow.h`; `add_field_solver_nets`), with an estimate of how
  much the conductors left outside could matter.
- Tcl commands that read a stack, define conductors and solve.

A later phase adds Python commands.

## Conformal dielectrics

A box whose z extent equals a conductor layer's gets that layer's conformal
dielectrics. Conformal dielectric k of a box is the box grown by the sum of
the first k SW_T on all four sides, line ends included, and by the sum of
the first k TW_T on top; its bottom stays at the box's bottom. Conductors
cut into conformal dielectrics, and conformal dielectrics replace the planar
layers they cross. The boxes of one net on one layer grow their shells from
their union, so a wire drawn as several boxes gets the shells of the merged
wire. Shells of the same conformal dielectric merge where wires are close;
between different nets or layers, shells of different ones must not
overlap, and a scene where they do is rejected.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

Coordinates are in µm and capacitances in fF. The commands keep one
dielectric (a uniform permittivity, 1.0 until set, or an ITF stack) and one
set of conductors, which each solve uses. A net is a set of boxes, which
may overlap. A target net's Gaussian surface is the boundary of its boxes,
each grown by its own offset (at most half its gap to other conductors), so
nets may wrap around each other; no other conductor may touch a target's
box. Around conformal dielectrics a short trial picks whether the surface
sits clear of their faces or inside the innermost one.

### Read ITF

The `read_itf` command reads a process stack from an ITF file and makes it
the dielectric. The ground plane is the top of the lowest dielectric, and the
background permittivity fills everything above the stack. Conformal
dielectrics wrap the boxes on their conductor layers (see Conformal
dielectrics above). The conductors defined so far are kept.

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

### Add Field Solver Rect

The `add_field_solver_rect` command adds a conductor rectangle of a cross
section to a net, creating the net on first use. A cross section lies in
the x-z plane; its wires run along y, infinitely long, and its results are
per unit length, in fF/µm. Conformal dielectrics grow on the x sides and on
top. Rectangles of different nets must not overlap. Give the rectangle
either by its corners, or by a layer of the ITF stack and an x span, in
which case the layer gives the z extent.

The field solver holds either boxes or rectangles: the first shape added
after `clear_field_solver` decides which, and adding the other kind is an
error.

```tcl
add_field_solver_rect
    -net net
    [-rect {x0 z0 x1 z1}]
    [-layer layer]
    [-x {x0 x1}]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-net` | Net name. The type is `string`. Required. |
| `-rect` | Corners of the rectangle in the x-z plane in µm, above the ground plane at z = 0. The type is a list of 4 `float`s. Excludes `-layer` and `-x`. |
| `-layer` | Conductor or via layer name of the ITF stack. The type is `string`. Requires `-x` and a stack from `read_itf`. |
| `-x` | The rectangle's x span in µm. The type is a list of 2 `float`s. Requires `-layer`. |

### Add Field Solver Nets

The `add_field_solver_nets` command replaces the conductors with nets of the
loaded design and their neighbours, and makes those nets the default targets
of `solve_capacitance`. A net's shapes are its routed wires and vias,
special wires, and the pins of its instance and block terminals, on routing
and cut layers; the ITF stack gives each layer's z extent.

Neighbours are the shapes of other nets that reach within the halo of the
nets' bounding box, clipped a margin beyond it, and the cells' obstructions
and unconnected pins, which have no net and are grounded. An obstruction is
often a routing blockage over a whole macro, its pins included, so each is
trimmed clear of every net's shapes on its layer by the layer's minimum
spacing (its width when the LEF gives none), as close as real metal could
come; leftover pieces narrower than the layer's width are dropped. The defaults scale
with the stack: the halo is three times the height of the nets' highest
shape, and the margin half of it. Overlapping shapes of different nets,
non-Manhattan special wires, and instances or block pins that are not
placed are errors.

Each row reports the window's leakage: the weight of walks that reached
outside the halo, where conductors are missing, and came back to a net.
It is a conservative estimate of how much the conductors left out could
change the total and the couplings (20 to 70 times the change on parallel
wires), and it falls about as the square of the halo. Sides of the halo
past the die area leak nothing.

Every net and cell of the design is visited, so reading takes time in
proportion to the design. Target nets are never clipped: a net that spans
the die, such as a clock or power net, brings most of the design into the
window.

```tcl
add_field_solver_nets
    -nets nets
    [-halo halo]
    [-margin margin]
    [-layer_map layer_map]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-nets` | Target nets. The type is a list of net names. Required. |
| `-halo` | Distance in µm from the nets' bounding box within which neighbours are included. The type is `float`, at least 0. The default is three times the nets' height. |
| `-margin` | Distance in µm beyond the halo at which neighbour shapes are clipped. The type is `float`, at least 0. The default is half the nets' height. |
| `-layer_map` | Design layers that map to differently named ITF layers, as pairs of names: design layer, then ITF layer. The type is a list of names. The default maps layers by name. |

### Clear Field Solver

The `clear_field_solver` command removes every net, box and rectangle. The
dielectric is kept.

```tcl
clear_field_solver
```

### Solve Capacitance

The `solve_capacitance` command solves one row of the capacitance matrix
for each target net, reports it, and returns a Tcl dict keyed by target net.
Each value is a dict with these keys:

- `total`, `total_sigma`: the total capacitance and its standard error.
- `ground`, `ground_sigma`: the coupling to the ground plane.
- `couplings`, `coupling_sigmas`: dicts from each other net to its coupling
  capacitance and standard error. Couplings are positive.
- `walks`, `hops`: the walks taken and their total hops.
- `seconds`: the solve's wall-clock time.
- `cross_section`: 1 when the row is per unit length, for rectangles from
  `add_field_solver_rect`.
- `window_leakage`: for nets from `add_field_solver_nets`, the window's
  leakage in fF (0 otherwise).

The solve stops when the standard errors of the total and of every coupling,
the ground's included, fall below their tolerances, both relative to the
total. A coupling's relative error can therefore be large when the coupling
is a small part of the total.
It runs on the threads set by `set_thread_count`; the results do not depend
on the thread count.

```tcl
solve_capacitance
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
| `-max_walks` | Walks after which the solve stops regardless of the tolerances. The type is `int`, at least 1. The default is `100000000`. |
| `-min_walks` | Walks before the tolerances are first checked. The type is `int`. The default is `16384`. |
| `-nets` | Target nets to solve. The type is a list of net names. The default is the nets of `add_field_solver_nets`, else every net. |
| `-panels` | Panels per cube-face edge in the Green's-function tables. The type is `int`, a positive multiple of 16. The default is `32`. |
| `-seed` | Random seed. The type is `int`, from 0 to 2^63 - 1. The default is `1`. |
| `-tol_coupling` | Target standard error of each coupling, relative to the total. The type is `float`, greater than 0. The default is `-tol_total`. |
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

The same met1 pair as a cross section, per unit length:

```text
clear_field_solver
add_field_solver_rect -net a -layer met1 -x {0 0.14}
add_field_solver_rect -net b -layer met1 -x {0.28 0.42}
set result [solve_capacitance -nets a -tol_total 0.005]
```

Two nets of a loaded design, with their neighbours:

```text
read_itf sky130.itf
add_field_solver_nets -nets {net1 net2}
set result [solve_capacitance]
```

See also `./test/field_solver_uniform.tcl`, `./test/field_solver_itf.tcl`,
`./test/field_solver_cross_section.tcl` and `./test/field_solver_nets.tcl`.

## Python

`Design.getFieldSolver()` returns the session the Tcl commands use, with
the same names in camel case. Boxes and rectangles take their coordinates
as numbers, `addNets` takes a net or a list of nets, `halo`, `margin` and
a `layer_map` dict, and `solve` takes the options of `solve_capacitance` as
keywords and returns the rows as dicts keyed by net, with the keys of the
Tcl results. With no nets, `solve` solves the default targets. An option
left at `None` keeps its default; one out of range raises `ValueError`.

```python
from openroad import Design, Tech

tech = Tech()
tech.readLef("sky130hd.tlef")
tech.readLef("sky130_fd_sc_hd_merged.lef")
design = Design(tech)
design.readDef("design.def")
solver = design.getFieldSolver()
solver.readItf("sky130.itf")
solver.addNets(["net1", "net2"], halo=10)
rows = solver.solve(tol_total=0.005)
print(rows["net1"]["couplings"]["net2"])
```

The other methods are `setPermittivity`, `addBox(net, x0, y0, z0, x1, y1,
z1)`, `addLayerRect(net, layer, x0, y0, x1, y1)`, `addRect(net, x0, z0, x1,
z1)`, `addLayerSpan(net, layer, x0, x1)`, `clearGeometry`, `netNames`
(without the grounded net of obstructions) and `defaultTargets`. See
`./test/field_solver_python.py`.

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
`test/golden`:

- cross sections with planar and conformal dielectrics against a 2D
  finite-volume solution (`fv2d_golden.py`, numpy and scipy);
- a crossing, and line ends and a crossing with conformal dielectrics,
  against a 3D finite-volume solution (`fv3d_golden.py`, numpy, scipy and
  pyamg).

None of these tools is a build dependency; the scripts regenerate the
references. `test/README.md` lists which test uses which structures and
reference files, and how to add or regenerate them.

## Limitations

- Only conformal dielectrics with THICKNESS = 0 (shells around the wires,
  nothing deposited between them) and no BW_T are read.
- A net from the design is read as rectangles; non-Manhattan shapes are not
  supported.
