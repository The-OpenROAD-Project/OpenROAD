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

This is phase 1 of the implementation plan: a homogeneous 3D solver with a
C++ API only. It provides:

- Manhattan conductors above a grounded plane at z = 0. The domain is open
  laterally and above, with no artificial boundary.
- A single uniform dielectric (`SolveOptions::eps_r`).
- Exact cube Green's-function tables from the Fourier series, sampled with
  alias tables.
- First hops importance-sampled by the gradient of the Green's function. A
  control variate exploits the zero-mean weights.
- Deterministic results for any thread count, using a counter-based Philox
  RNG and fixed batches reduced in order.

- A process-stack reader for ITF files (`fsv/Itf.h`): planar dielectrics,
  conductors, nested conformal dielectrics and vias, with the ground plane at the
  top of the lowest dielectric and the background dielectric above. It
  covers the keywords the SkyWater 130 ITF uses, and reports any other
  keyword by name. The solver does not use the stack yet.

Later phases add the following:

- Solving with planar and conformal dielectrics from the stack.
- A 2D mode.
- odb input.
- Tcl and Python commands.

## Usage

```cpp
fsv::Geometry geometry;                    // nets and boxes in µm, above z = 0
const int a = geometry.addNet("a");
geometry.addBox(a, {{0, 0, 0.5}, {4, 0.1, 0.6}});
// ... more nets and boxes ...

fsv::Solver solver({.threads = 32});       // shared tables + worker pool
const fsv::Scene scene(geometry, 3.9);     // spatial index, eps_r
fsv::SolveOptions tolerances;              // 1% total, 5% coupling by default
const fsv::CapRow row = solver.solve(scene, a, tolerances);
// row.total(), row.coupling(net), row.ground, with standard errors.
```

Reuse one `Solver` for the whole run and one `Scene` per geometry.

## Tests

```
bazelisk test //src/fsv/test/...
bazelisk run //src/fsv/test:BenchCube   # accuracy and thread scaling
bazelisk run //src/fsv/test:BenchGrid   # throughput on a two-layer wire grid
```
