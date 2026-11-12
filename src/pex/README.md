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
modeling experiments below. There are no commands yet; the extractor
itself (block index, segmentation, ODB and SPEF output) comes next, with
the accuracy work below resumed once it runs end to end.

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
  the layer count. By default the builder chooses the covers itself
  instead: it probes each usable layer with a few 2D solves and keeps the
  fewest, always the nearest and the farthest, whose interpolation
  predicts the rest within 2% of the total (`planCovers`; e13 checks the
  choice where the probes did not look and finds it well inside that).
  A cover tolerance of 0 goes back to the depth and far factors.
  Past the grid's last spacing, more spacings run out to a multiple of the
  height to the nearest plane (6, or 10 with a cover on one side only,
  whose open side lets the field spread further): how far a neighbor
  still couples depends on that height, not on the minimum spacing (e19).
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
  takes capacitance away. A crossing that ends near the piece keeps less
  than one running on past it; a reach table, over how far the crossing
  runs past the piece and the piece past the crossing, scales its share,
  each side of the piece weighted equally.
- **Diagonal.** A parallel wire on a nearby layer that lies beside the
  piece rather than over it (a power rail one layer down, say) couples
  strongly and neither table above sees it. One table per `(layer, other)`
  pair holds the coupling to such a wire per unit length, what it adds to
  the total and what it takes from the coupling to the piece's own
  neighbor on that side, over the parallel wire's width, its edge-to-edge
  gap and the spacings to the piece's neighbors on that side and on the
  far side, from cross sections solved in 2D. With a plane on the piece's
  other side, a second table holds the same with that plane, plus what the
  parallel wire takes from the coupling to the plane; the evaluator blends
  the two by the plane's share. The gaps run out to 4 times the distance
  between the two layers.
- **End.** One table per context holds what a line end adds to the wire's
  capacitance, over the width, the spacing to a neighbor on each side
  running past the end and the gap to a wire facing the end, split into
  parts to the planes, the neighbors and the facing wire, from a short
  wire solved in 3D less its per-unit-length value times its length.

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
| e13 probed covers | the same, for the covers chosen from probes, at widths and spacings the probes did not use | the chosen covers it resolves to | the cover tolerance and the probe variants |
| e14 attribution | e9's nets in nested windows: alone, own layers, next layers, everything | the model in each window | which interaction causes a net's error |
| e15 junctions | bends, T junctions, layer changes, landing pads, crossings reaching past one side or both, facing ends, in open, dense and tight surroundings | the reference evaluator | the evaluator's handling of each structure |
| e16 one-sided ends | a line end with a neighbor on one side only | the end table | the end table's neighbor columns |
| e17 dense context | a wire among rails, a strap, crossings and neighbors, built up part by part; rails at several coverages | the reference evaluator | how the tables combine in dense layout |
| e18 power grid | e9's nets with the power nets alone, merged, split by layer, and with groups of signal wires added back | the reference evaluator | where the coupling to the power grid goes wrong |
| e19 lateral reach | neighbors at spacings in multiples of the height to the nearest plane | the wire alone | how far the lateral table reaches |

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

## Accuracy

The target, on 50 sampled nets each of sky130hd gcd and aes against the
field solver: the total within 5% for 90% of the nets, and 90% of the
couplings to other signal nets above 5% of the net's total within 20%.
Coupling to the supplies counts as ground, since they do not switch.

With the model built from `sky130.itf` for li1 through met5 (46,787
solves, about 3 hours at 64 threads, 2.3 MB):

| e9, 50 nets | gcd | aes |
| --- | --- | --- |
| totals within 5% | 68% | 68% |
| total error, 90th percentile | 7.5% | 7.3% |
| ground plus supply coupling, median | +10.1% | +10.2% |
| signal coupling summed per net, median | −1.9% | −0.6% |
| signal couplings above 5%, within 20% | 84% | 81% |

Building the same model again moves the share of totals within 5% by up
to 6 points, so 50 nets do not resolve smaller changes.

What remains, largest first:

- **Narrow straps.** A power strap two or three layers up gets 30–50%
  too much (e18 with the power nets split by layer): the cover tables
  are solved with full planes, and a strap narrower than a few heights
  above the wire takes much less than a plane. Over the wire it is to be
  looked up in the diagonal table at overlapping gaps.
- **Shielding of parallel wires.** A strap beside the wire with rails
  between them is looked up as if the rails were absent, up to 28% high
  (e17 at low rail coverage). The diagonal coupling is to be scaled by
  the cover's share, as for straps over rails.
- **Low signal couplings.** The largest coupling misses are 30–60% low,
  each 5–11% of its net's total; not yet traced.
- **Crossing reach by side.** A crossing reaching past one side of the
  wire only is split evenly between the sides; with a neighbor on one
  side the coupling to the crossing is off by −25% to +46% (e15).
- **Opposite plane in the diagonal tables.** The plane on the wire's
  other side ends past the parallel wire, so it is wider in each sample
  solve than in the base solve without that wire; the plane's own edge
  effect goes into the deltas, `plane_delta` included. The plane is to
  have one extent across a table, which needs the diagonal tables
  rebuilt.
- **Sample size.** 200 nets per design, so that a 2-point change is
  measurable.

## Regression tests

```
bazelisk test //src/pex/test/...
```
