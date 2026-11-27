# Parasitic Extraction (pex)

pex is a 2.5D RC extractor under development. Its capacitance model is a
set of tables that the module builds itself by solving small wire patterns
with the field solver (`openroad/fsv`) on an ITF process stack, so no external
reference extractor is needed. The same solver checks the model on routed
nets.

## Status

pex extracts a routed block into ODB parasitics and writes SPEF. A net's
resistor network comes from its routed wire, with resistance from the
ITF stack; its capacitance comes from the model, evaluated on the
block's boxes around each wire. The model is built by the module itself,
and the tables' accuracy against the field solver is measured in the
experiments below and, on a design's nets, with `compare_pex_fsv`. One
corner is extracted; several corners, performance, and the remaining
accuracy work come next.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

A model is built once per ITF stack and read with that stack before
extraction:

```tcl
build_pex_model -itf sky130.itf -out sky130.json
read_pex_model -itf sky130.itf sky130.json
extract_pex
write_pex_spef gcd.spef
```

Threads come from `set_thread_count`; the result does not depend on the
number of threads.

### Build Pex Model

The `build_pex_model` command builds the capacitance tables of a model
by solving small wire patterns on the ITF stack with the field solver,
and saves the model after each layer. A model already in the file that
was built from the same stack with the same settings is finished rather
than rebuilt, so a build that stopped resumes. A full model of sky130's
six routing layers takes about three hours at 64 threads.

```tcl
build_pex_model
    -itf itf
    -out model
    [-layers layers]
    [-tol tolerance]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-itf` | The ITF file of the process stack. |
| `-out` | The model file to write (JSON). |
| `-layers` | The conductor layers to tabulate, by ITF name. The default is the routing layers of the technology LEF that the stack has. |
| `-tol` | The relative tolerance of each field-solver solve. The default value is `0.005`, and the allowed values are floats greater than 0. |

### Read Pex Model

The `read_pex_model` command reads a model and the ITF stack it was built
from, which gives the layer heights and the resistance data. A model
records its stack, and one built from another stack is refused.

```tcl
read_pex_model
    -itf itf
    model
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-itf` | The ITF file the model was built from. |
| `model` | The model file. |

### Extract Pex

The `extract_pex` command extracts the signal nets of the design that
have routed wires into ODB, replacing any parasitics there. Each net gets
a node at each terminal, at each junction a branch leaves from, and
where its wire's paths start or end; its wire shapes become resistors in
series between nodes. Ground capacitance is split between a resistor's
two nodes by where it lies along it, and a coupling joins the nearer
node of each net. Supply nets, obstructions and fill are neighbors only:
coupling to them goes to ground. The cells' instance pins are neighbors
only as well, since the cells' timing models count their capacitance: a
net's routing ends at its pins without counting them, and what couples
to another net's pins goes to ground. A net pair's coupling is kept only
when its total reaches `-coupling_threshold` or `-coupling_ratio` of either
net's total capacitance; otherwise it goes to ground on both nets. Either
test alone keeps a pair, so setting either option to 0 keeps every
coupling.
With `-coupling_to_ground` every pair's coupling goes to ground.

```tcl
extract_pex
    [-max_res resistance]
    [-coupling_threshold capacitance]
    [-coupling_ratio ratio]
    [-coupling_to_ground]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-max_res` | Wire shapes join in series into one resistor up to this resistance, in ohms; 0 makes each shape its own resistor. The default value is `50`, and the allowed values are floats of at least 0. |
| `-coupling_threshold` | The total coupling of a net pair, in fF, at or above which it is kept; 0 keeps every pair. The default value is `0.1`, and the allowed values are floats of at least 0. |
| `-coupling_ratio` | The fraction of either net's total capacitance at or above which a net pair's coupling is kept; 0 keeps every pair. The default value is `0.03`, and the allowed values are floats from 0 to 1. |
| `-coupling_to_ground` | Send every net pair's coupling to ground on both nets, whatever the thresholds: the coupling is still computed and each net keeps its total, but the parasitics have no coupling capacitors. |

### Compare Pex Fsv

The `compare_pex_fsv` command checks extraction against the field solver:
it solves each net's window (the net and the shapes within `-halo` of
it) with fsv and reports the extracted total against fsv's, and how many
of its couplings to other signal nets above 5% of its total are within
20%, the accuracy target's measures. The net's instance pins stay in the
window as a conductor of their own, trimmed clear of its routing by the
layer's spacing, and its total leaves out their coupling to the routing;
the other nets' pins are grounded, as the extraction has them. Run it
after `extract_pex`; the field solver takes the model's ITF stack.

```tcl
compare_pex_fsv
    [-nets nets]
    [-sample count]
    [-halo halo]
    [-tol tolerance]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-nets` | The extracted nets to compare. Give either `-nets` or `-sample`. |
| `-sample` | Compare this many extracted nets: every (N/count)-th of the N extracted nets in the block's net order, or all of them when there are no more than count. The allowed values are integers greater than 0. |
| `-halo` | The window's reach around a net, in µm. The default value is `10`, and the allowed values are floats of at least 0. |
| `-tol` | The field solver's relative tolerance on a net's total, which its couplings also take as a fraction of the total. The default value is `0.005`, and the allowed values are floats greater than 0. |

### Write Pex Spef

The `write_pex_spef` command writes the parasitics in ODB as SPEF, in fF
and ohms. Pins are named instance:pin, ports by name, and internal nodes
net:junction.

```tcl
write_pex_spef
    [-nets nets]
    filename
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-nets` | The nets to write. The default is every extracted net. |
| `filename` | The SPEF file to write. |

## Python

`Design.getPex()` returns the session the Tcl commands use, with the same
names in camel case: `buildModel(itf, out, layers=(), tol=0.005)`,
`readModel(itf, model)`, `extract(max_res=50, coupling_threshold=0.1,
coupling_ratio=0.03, coupling_to_ground=False)`, `compareFsv(nets=(), sample=0, halo=10,
tol=0.005)` and `writeSpef(path, nets=())`, where `layers` and `nets` take
a name or a list of names. An option out of range raises
`ValueError`.

```python
from openroad import Design, Tech

tech = Tech()
tech.readLef("sky130hd.tlef")
tech.readLef("sky130_fd_sc_hd_merged.lef")
design = Design(tech)
design.readDef("design.def")
extractor = design.getPex()
extractor.readModel("sky130.itf", "sky130.json")
extractor.extract()
extractor.writeSpef("design.spef")
```

See `./test/pex_python.py`.

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
  distance (`src/CoverMap.h`; e12 measures what that costs), so the table
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
  between the two layers; a parallel wire farther out adds nothing.
- **End.** One table per context holds what a line end adds to the wire's
  capacitance, over the width, the spacing to a neighbor on each side
  running past the end and the gap to a wire facing the end, split into
  parts to the planes, the neighbors and the facing wire, from a short
  wire solved in 3D less its per-unit-length value times its length. A
  neighbor that ends where the wire does takes the end of one running
  past: the field solver puts that end 15–18% lower, but the end with no
  neighbor is higher still (`e20`). The end measured on a 2 µm wire is
  settled with a cover or a neighbor near; alone with no cover either
  side it still grows with the length, by 15–25% from 2 to 10 µm.
  A wire running past a neighbor's end couples to that end as the end
  does to it, the same part of the neighbor's end, so both nets see the
  same coupling. The wire takes it as the neighbor running on past its
  end, by the length over which the wire's coupling to it per unit
  length makes up that part: 0.1 to 0.7 µm in sky130, shorter between
  covers. The wire's pieces along that length take the lateral table
  with the neighbor, so its total, covers and ground change as they do
  beside the neighbor, which is what the field solver shows (`e21`). The
  neighbor's end is looked up with the neighbor on its other side, which
  takes about half of it, and as wide as the shapes of its net joining
  it there: the other wire of a bend. Another end nearby along the wire
  keeps 1 − exp(−d / 2 length) of the length at a distance d, the law of
  the crossing tables' kept share: a shape facing the neighbor's end
  across a gap, so a narrow gap in a neighbor track all but closes; and
  the wire's own end, so two wires ending together take none of it from
  each other, what the wire's end loses going to ground. Wires side by
  side that do not overlap along their length do not couple, though the
  field solver finds up to 0.07 fF between two that end where the other
  starts.

A point and its mirror share one solve, and each solve is seeded by its
position, so a table does not depend on the thread count. A table the
model already holds is not built again, so a build resumes from a saved
model. The model file is JSON with a format version and fingerprints of
the stack and of the build settings: a build resumes only a model with
the same settings, and extraction refuses a model built from another
stack.

## Experiments

The experiments measure, against the field solver, what each
simplification of the model costs. Every row gives a quantity's true value,
its standard error, the value the simplification gives, and the relative
error. They run on one conductor layer of a stack:

```
bazelisk build //openroad/pex/test:Experiments
bazel-bin/openroad/pex/test/Experiments --threads 64 --tol 0.003 \
    --itf openroad/fsv/test/sky130.itf --layer met2 --csv met2.csv
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
| e9 nets | nets of a routed design, each solved within a halo | the model applied to the same boxes by the evaluator | the attainable accuracy |
| e11 diagonal | a parallel wire on a nearby layer beside the wire, alone and with neighbors | the wire without it | the diagonal table |
| e12 far covers | a wire under or over a plane on each layer the builder would not tabulate | the tabulated covers it resolves to, and the open context a lookup would fall back to without them | the far cover factors |
| e13 probed covers | the same, for the covers chosen from probes, at widths and spacings the probes did not use | the chosen covers it resolves to | the cover tolerance and the probe variants |
| e14 attribution | e9's nets in nested windows: alone, own layers, next layers, everything | the model in each window | which interaction causes a net's error |
| e15 junctions | bends, T junctions, layer changes, landing pads, crossings reaching past one side or both, facing ends, in open, dense and tight surroundings | the evaluator | the evaluator's handling of each structure |
| e16 one-sided ends | a line end with a neighbor on one side only | the end table | the end table's neighbor columns |
| e17 dense context | a wire among rails, a strap, crossings and neighbors, built up part by part; rails at several coverages | the evaluator | how the tables combine in dense layout |
| e18 power grid | e9's nets with the power nets alone, merged, split by layer, and with groups of signal wires added back | the evaluator | where the coupling to the power grid goes wrong |
| e19 lateral reach | neighbors at spacings in multiples of the height to the nearest plane | the wire alone | how far the lateral table reaches |
| e20 end length | wires from 0.5 to 40 µm, alone, between neighbors running past the ends or ending with the wire, and with one neighbor | per-unit-length × length | the length the end is measured at, and the neighbors' end states |
| e21 neighbor ends | a wire beside a neighbor whose end runs past the wire's end, ends with it, short of it, or before it starts; a short neighbor; a second one facing the end across a gap; a neighbor turning away; an end with another neighbor on the wire's other side, a wider wire, or a farther neighbor behind | the evaluator | the reach of a neighbor's end along the wire, and what ends near each other keep of it |

e9 reads LEF and DEF (by default the gcd window of the fsv tests) or an
ODB file, builds the model for every routing layer of the design the stack
has, saves it to `--model` after each layer, and reads it back on later
runs, finishing a build that stopped:

```
bazel-bin/openroad/pex/test/Experiments --groups e9 --threads 64 --tol 0.003 \
    --odb 6_final.odb --model sky130.json --sample 50 --halo 10
```

`--smoke` runs a short version of every group, which `Experiments_smoke`
does as a test. The lateral tables for e6, e7 and e9 take most of the
time; e2 and e3 solve in 3D and take minutes at 0.3%.

`--coarse` builds the model on small grids (two widths, three spacings,
one cover layer each side). The regressions' model,
`test/sky130_coarse.json` (100 KB), takes two minutes at 64 threads:

```
bazel-bin/openroad/pex/test/Experiments --groups model --coarse --tol 0.03 \
    --itf openroad/fsv/test/sky130.itf --layers li1,met1,met2,met3,met4,met5 \
    --model openroad/pex/test/sky130_coarse.json --threads 64
```

## Accuracy

The target, on 50 sampled nets each of sky130hd gcd and aes against the
field solver: the total within 5% for 90% of the nets, and 90% of the
couplings to other signal nets above 5% of the net's total within 20%.
Coupling to the supplies counts as ground, since they do not switch.

The model is built from `sky130.itf` for li1 through met5 (46,787
solves, about 3 hours at 64 threads, 2.3 MB). First the model alone:
each sampled net is solved by the field solver with every shape within
10 µm of it, and the model is applied to the same shapes (the `e9`
experiment group below):

| model alone, 200 nets | gcd | aes |
| --- | --- | --- |
| totals within 5% | 81.5% | 88.5% |
| total error, median | +1.9% | +1.5% |
| total error, 90th percentile | 6.6% | 5.5% |
| signal couplings above 5%, within 20% | 91% | 92% |

The instance pins are context here as in extraction (see
`compare_pex_fsv`). Counting them in their nets instead puts the
median total at −1.6% (gcd) and −2.5% (aes) and 72% of totals within
5%: the model takes too little from the irregular li1 shapes inside the
cells, which the cells' timing models count anyway.

Building the same model again moves the share of totals within 5% by up
to 6 points, so 50 nets do not resolve smaller changes.

Extraction itself, checked with `compare_pex_fsv -sample 200 -halo 10
-tol 0.003` on the routed designs with the same model:

| extraction, 200 nets | gcd | aes |
| --- | --- | --- |
| totals within 5% | 89.5% | 90.5% |
| total error, median | +2.0% | +1.4% |
| total error, 90th percentile | 5.1% | 4.8% |
| signal couplings above 5%, within 20% | 92% | 91% |
| extraction, 1 thread | 5.3 s | 338 s |
| extraction, 16 threads | 0.39 s | 23 s |

These are the same measures, but taken from the extracted block rather
than from each net's 10 µm window: crossings and neighbors are not cut
off at the window's edge, and couplings are averaged between the two
nets and grounded below the thresholds. rcx takes about 2 s for aes on
one thread, so extraction must become far faster; most of its time is
in the crossing lookups, through string-keyed tables and logarithms per
piece.

Resistance agrees with rcx on gcd: over the 485 nets above 1 Ω, pex's
total resistance per net is 0.995 of rcx's at the median (0.989 to
1.001 from the 10th to the 90th percentile). pex takes each wire's
silicon width from `sky130.itf`'s resistive etch, which narrows every
layer but li1.

What remains, largest first:

- **Wires of the layer not beside the wire.** A side with no neighbor
  over a piece counts as open, and a neighbor hides what lies behind
  it. Two wires on adjacent tracks that end where the other starts
  couple by up to 0.07 fF, and a second neighbor behind the first takes
  0.13 fF from an 8 µm wire with no covers, 0.02 fF between covers
  (`e21`); neither is modeled. In the model-alone check the signal
  couplings sum to 3.4% of a net's total too little at the median, and
  ground and the supplies to 5.5% too much, part of it the straps
  below.
- **Narrow straps.** A power strap two or three layers up gets 30–50%
  too much (the `e18` experiment with the power nets split by layer):
  the cover tables are solved with full planes, and a strap narrower
  than a few heights above the wire takes much less than a plane. Over the wire it is to be
  looked up in the diagonal table at overlapping gaps.
- **Shielding of parallel wires.** A strap beside the wire with rails
  between them is looked up as if the rails were absent, up to 28% high
  (the `e17` experiment at low rail coverage). The diagonal coupling is
  to be scaled by the cover's share, as for straps over rails.
- **Crossing reach by side.** A crossing reaching past one side of the
  wire only is split evenly between the sides; with a neighbor on one
  side the coupling to the crossing is off by −25% to +46% (the `e15`
  experiment).
- **Opposite plane in the diagonal tables.** The plane on the wire's
  other side ends past the parallel wire, so it is wider in each sample
  solve than in the base solve without that wire; the plane's own edge
  effect goes into the deltas, `plane_delta` included. The plane is to
  have one extent across a table, which needs the diagonal tables
  rebuilt.
- **Supply coupling.** The coupling to the supplies is 11–16% high
  summed over the nets, mostly through covers (see narrow straps); the
  ground itself is within 10%, and the substrate takes 3–5 times what
  the field solver gives it, at the expense of the shapes over it.

## Regression tests

```
bazelisk test //openroad/pex/test/...
```

The C++ tests check the model's tables, builder and file, the evaluator,
the index against a brute-force scan, the resistor network on hand-made
wires, extraction (the index's candidates against every box, identical
ODB at any thread count, and every part of the capacitance in ODB) and
the SPEF writer. `pex_extract` runs the commands on the gcd excerpt of the
fsv tests with the small model, `test/sky130_coarse.json`, and reads the
SPEF back with OpenSTA; `pex_python` does the same from Python, and
`pex_errors` checks the commands' errors.
