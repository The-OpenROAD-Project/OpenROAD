# Parasitic Extraction (pex)

pex is a 2.5D RC extractor under development. Its capacitance model is a
set of tables that the module builds itself by solving small wire patterns
with the field solver (`src/fsv`) on an ITF process stack, so no external
reference extractor is needed. The same solver checks the model on routed
nets.

## Status

The first step is in: the patterns built from a stack by layer name
(`pex/Patterns.h`), the model tables with their lookups (`pex/Model.h`),
the table builder (`pex/ModelBuilder.h`), the model file
(`pex/ModelJson.h`), a reference evaluator that applies the model to a
field-solver geometry by brute force (`test/cpp/Reference.h`), and the
modeling experiments below. There are no commands yet.

## Model

A wire is cut into pieces wherever its surroundings change. A piece's
context is its layer and width, the spacing to the nearest same-layer
neighbor on each side (or none past the table's last spacing), and the
nearest covering layer below (the substrate when none) and above (none when
open). Four kinds of table describe a layer:

- **Lateral.** One table per `(layer, below, above)` context holds, per
  unit length, the capacitance to the ground plane, to each neighbor and
  to each covering plane, over a grid of widths and of the spacing on each
  side; a spacing index past the grid means no neighbor. Lookups are
  linear in the width and in the logarithms of the spacings, a coupling
  interpolating geometrically along its own side. Each point is one cross
  section solved in 2D: the wire between its neighbors, with a plane on
  each covering layer. The contexts reach a set depth each way; past it
  only a few far layers at spaced distances get contexts, and a cover
  between two of them takes both, weighted in the logarithm of its
  distance (`pex/CoverMap.h`; e12 measures what that costs), so the table
  count grows with the depth and the far factors, not with the square of
  the layer count.
- **Crossing.** A covering layer is rarely a plane: it is wires crossing
  the piece. One table per `(layer, cover)` pair holds the share of the
  plane's effect that the crossings keep over the uncovered gap between two
  of them, for the total and for the coupling to the crossings, against
  the gap. It comes from periodic arrays of crossing wires at several
  widths and coverages, solved in 3D at two lengths so the ends cancel. A
  piece under crossings blends the open and the covered lateral values by
  that share. The amount kept, gap × share, is linear between the
  tabulated gaps, and its slope is the one-sided share a piece beside a
  single crossing takes; it is held within [0, 1], so a noisy table never
  takes capacitance away.
- **Diagonal.** A parallel wire on a nearby layer that lies beside the
  piece rather than over it (a power rail one layer down, say) couples
  strongly and neither table above sees it. One table per `(layer, other)`
  pair holds the coupling to such a wire per unit length, what it adds to
  the total and what it takes from the coupling to the piece's own
  neighbor on that side, over the parallel wire's width, its edge-to-edge
  gap and the spacings to the piece's neighbors on that side and on the
  far side, from cross sections solved in 2D.
- **End.** One table per context holds what a line end adds to the wire's
  capacitance, over the widths, from a short wire solved in 3D less its
  per-unit-length value times its length.

A point and its mirror share one solve, and each solve is seeded by its
position, so a table does not depend on the thread count. A table the
model already holds is not built again, so a build resumes from a saved
model. The model file is JSON with a format version.

## Experiments

The experiments measure, against the field solver, what each
simplification of the model costs. Every row gives a quantity's true value,
its standard error, the value the simplification gives, and the relative
error. They run on one conductor layer of a stack:

```
bazelisk build //src/pex/test:Experiments
bazel-bin/src/pex/test/Experiments --threads 64 --tol 0.003 \
    --itf src/fsv/test/sky130.itf --layer met2 --csv met2.csv
```

| Group | Measures | Simplification tested | Decides |
| --- | --- | --- | --- |
| e1 additivity | a wire whose neighbor and cover change along it | the pieces' per-unit-length values plus the end terms | whether transitions need a correction |
| e2 crossing | a wire under or over an array of crossing wires at a coverage fraction, alone, with neighbors, and with a plane on the other side | plane × coverage, and the blend by the crossing table's share | the crossing model |
| e3 ends | short wires of several lengths; two line ends facing across a gap | per-unit-length × length | the end term and its dimensions |
| e4 lateral | one to three neighbors per side; the nearest neighbor's width | only the nearest neighbor, at minimum width | the lateral cutoff and the table's neighbor width |
| e5 vertical | planes on the three nearest layers each way, present or absent in every combination | only the nearest present plane on each side | the context depth of the tables |
| e6 asymmetric | different spacings on the two sides | the ground averaged over the two symmetric cases, each coupling from its own | whether the table keeps both spacings |
| e7 interpolation | off-grid widths and spacings, spacings past the grid, a width past the grid | the interpolated table | the table grid and the interpolation |
| e8 vias | a via at a wire's end and in its middle, with and without the cut | the wires without the via | the via term |
| e9 nets | nets of a routed design, each solved within a halo | the model applied to the same boxes by the reference evaluator | the attainable accuracy |
| e11 diagonal | a parallel wire on a nearby layer beside the wire, alone and with neighbors | the wire without it | the diagonal table |
| e12 far covers | a wire under or over a plane on each layer the builder would not tabulate | the tabulated covers it resolves to, and the open context a lookup would fall back to without them | the far cover factors |

e9 reads LEF and DEF (by default the gcd window of the fsv tests) or an
ODB file, builds the model for every routing layer of the design the stack
has, saves it to `--model` after each layer, and reads it back on later
runs, finishing a build that stopped:

```
bazel-bin/src/pex/test/Experiments --groups e9 --threads 64 --tol 0.003 \
    --odb 6_final.odb --model sky130.json --sample 50 --halo 10
```

`--smoke` runs a short version of every group, which `Experiments_smoke`
does as a test. The lateral tables for e6, e7 and e9 take most of the
time; e2 and e3 solve in 3D and take minutes at 0.3%.

## Regression tests

```
bazelisk test //src/pex/test/...
```
