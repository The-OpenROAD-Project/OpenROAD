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
  read but not solved yet.
- Exact cube Green's-function tables from Fourier series, for a uniform cube
  and for a cube split by one planar interface (`LayeredCubeGreens`), sampled
  with alias tables. A walk sizes each cube to hold at most one interface and
  snaps it to a tabulated position.
- First hops importance-sampled by the flux kernel, with interpolation
  between tabulated interface positions so that their weights stay bounded.
  A control variate exploits the zero-mean weights.
- Deterministic results for any thread count, using a counter-based Philox
  RNG and fixed batches reduced in order.

Later phases add conformal dielectrics, a 2D mode, odb input, and Tcl and
Python commands.

## Usage

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

## Tests

```
bazelisk test //src/fsv/test/...
bazelisk run //src/fsv/test:BenchCube   # accuracy and thread scaling
bazelisk run //src/fsv/test:BenchGrid   # throughput on a two-layer wire grid
```

`TestGolden` compares fsv with reference values for the structures in
`test/golden`: cross sections against a 2D finite-volume solution
(`fv2d_golden.py`, numpy and scipy), and a 3D structure against FasterCap
(`fastercap_golden.py`), which is not a build dependency. The scripts
regenerate the references.
