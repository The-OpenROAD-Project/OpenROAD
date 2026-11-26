# Design smells: metrics, thresholds, and triage

Most detection strategies follow Lanza & Marinescu, *Object-Oriented
Metrics in Practice* (2006). Long Method, Long Parameter List, Data
Clumps, and Middle Man come from Fowler's *Refactoring*. Each metric
comes from `extract.py`'s facts. Each threshold is a `detect.py` flag.

"Own" members are those of the method's class, its enclosing classes, its
bases, and (for odb) the `_dbFoo` implementation behind `dbFoo`. "Foreign"
members are all others, except classes matching `--ignore-class` (std,
boost, Qt, the logger, and geometry value types like `odb::Rect`, whose
getters are not data worth moving behavior to).

## Contents

- [Feature Envy](#feature-envy)
- [God Class](#god-class)
- [Data Class](#data-class)
- [Message Chains](#message-chains)
- [Inappropriate Intimacy](#inappropriate-intimacy)
- [Shotgun Surgery](#shotgun-surgery)
- [Middle Man](#middle-man)
- [Long Method and Long Parameter List](#long-method-and-long-parameter-list)
- [Data Clumps](#data-clumps)
- [OpenROAD false positives](#openroad-false-positives)
- [Choosing where code should live](#choosing-where-code-should-live)

## Feature Envy

A method that works mostly with another class's data.

| Metric | Meaning | Flag when |
|---|---|---|
| ATFD | distinct foreign members used | `> --atfd` (5) |
| LAA | own / (own + foreign) distinct members | `< --laa` (1/3) |
| FDP | distinct classes supplying the foreign members | `<= --fdp` (3) |

FDP separates envy from coordination: a method that pulls data from one
or two classes belongs with them, while one that touches six is
orchestrating and is fine where it is.

Methods whose only use of the envied class is reading its public fields
are hidden unless you pass `--include-data`. These are usually code that
reads options structs, which belongs to the Data Class smell (below).

**Refactoring:** Move Method onto the envied class. If only part of the
method is envious, first Extract Method on that part. When the method
needs one value from its current class, pass that value as a parameter
rather than passing `this`.

## God Class

A large class that does too much and is not cohesive.

| Metric | Meaning | Flag when |
|---|---|---|
| WMC | sum of cyclomatic complexity of its methods | `>= --wmc` (47) |
| TCC | fraction of public method pairs sharing an own field | `< --tcc` (1/3) |
| ATFD | distinct foreign members used by all its methods | `> --class-atfd` (5) |

TCC measures cohesion. Low TCC means the methods fall into groups that
touch disjoint fields, and each group is a candidate class.

The table's last column shows the sizes of those groups, as the number
of methods in each. `--clusters` lists each group's fields and methods.
Groups are linked through the fields they share. "Hub" fields (used by
more than a quarter of the methods, like `logger_` or `db_`) are left
out of the linking, because they would join everything. Reading the
sizes:

- One giant group plus crumbs (`110/2`) means there is no clean split.
  Report a crumb only if it is a coherent concept on its own.
- Two or more substantial groups (`40/25/12`) are the Extract Class
  candidates. Name the new class after what its fields represent.

**Refactoring:** Extract Class along a substantial group.

Large engine classes (`Resizer`, `GlobalRouter`, `FlexDR`) are always
flagged. A finding there is only useful if you name a specific cluster
that could become its own class. "This class is big" is not a finding.

## Data Class

A class that exposes data but has little behavior of its own. The
behavior that uses the data lives elsewhere, and it usually shows up as
Feature Envy aimed at this class.

| Metric | Meaning | Flag when |
|---|---|---|
| WOC | non-accessor public methods / all public members | `< --woc` (1/3) |
| NOPA + NOAM | public fields + public accessor methods | `> 2` with WMC < 31, or `> 4` with WMC < 47 |

A method counts as an accessor when its body is one statement that
returns or assigns a single own field. Structs with no public methods
are plain aggregates (options structs, records) and are skipped unless
you pass `--include-aggregates`. In C++ these are intentional and not a
smell.

The `envied by` column lists Feature Envy methods aimed at this class.
Those methods are the behavior to move in. A Data Class with no envious
users is usually fine as it is.

**Refactoring:** Move the envying behavior in (Move Method), then make
fields private and remove setters nobody needs.

## Message Chains

Code like `a->getB()->getC()->getD()` couples the caller to the whole
navigation path. Links through the method's own class and through value
types are dropped before the chain is measured.

Flag when the chain has `>= --chain` (4) links through `>= --chain-classes`
(3) distinct classes. Identical chains are grouped, and the report is
sorted by occurrences × length.

A second table, **Repeated sub-chains**, counts runs of two to four
links wherever they occur, so a short chain repeated many times still
shows up. This is usually the more actionable table. Its columns are
sites and distinct methods. A run that mostly occurs inside a longer
frequent run is reported only as that longer run. `--subchain-count`
sets the minimum number of sites (5).

**Refactoring:** Hide Delegate. Add a shortcut method on the first class
in the chain, or keep a cached member, when the same chain appears at
several sites.

Repeated sub-chains that are *not* findings:

- Navigating a plain data structure's fields, such as
  `Tree::branch -> Branch::x` in FastRoute or CTS. Those are array-of-struct
  layouts, not object navigation.
- odb's journaling calls (`_dbBlock::journal_ -> dbJournal::...`), which
  are generated code.

**OpenROAD idiom:** `db_->getChip()->getBlock()` is the standard way to
reach the block. It is a finding only when it is repeated enough that
the class should hold `block_` or a `getBlock()` helper. Check whether
the module already has such a helper before you propose adding one.
Prefer a helper that looks the block up each time to a cached pointer,
because the block can be replaced between commands.

## Inappropriate Intimacy

Two classes that each use at least `--intimacy` (4) of the other's
members. Nested classes are excluded.

**Refactoring:** If the pair is really one concept, merge it, or move the
members that cross over. If the dependency should only go one way,
replace the back-references with a callback or an interface.

## Shotgun Surgery

A method called from many methods in many other classes, so changing it
means editing all of them.

| Metric | Meaning | Flag when |
|---|---|---|
| CM | distinct calling methods outside the method's class | `> --cm` (7) |
| CC | distinct classes those callers belong to (a free function counts by file) | `> --cc` (5) |

Calls are matched by the callee's USR, so overloads are distinct.
Calls through a base-class virtual count toward the base, not the
override. Getter-sized methods (no branches and at most five lines) are
skipped, and so are classes declared under `include/` (module public
APIs, meant to be called widely) unless you pass `--include-api`.
Callers are only seen within the extracted paths.

The remaining hits are mostly a module's internal data model, such as
drt's `frLayer` and `frViaDef` getters. Those are fine. A finding is a
**protocol** that callers must repeat correctly, such as a
`beginAction`/`endAction` pair or a fixed sequence of three calls.

**Refactoring:** wrap the protocol in one method (or an RAII guard for
begin/end pairs) so callers make one call.

## Middle Man

A class where at least `--mm-ratio` (0.5) of its public methods, and at
least `--mm-min` (3) of them, only forward one call to another class.
A forwarding method has one statement, no branches, and exactly one
call to another class's method. Forwarding to the class's own nested
`Impl` (pimpl) is skipped unless you pass `--include-pimpl`.

Not findings:

- Public facades over a private implementation, such as `odb::defin`
  over `definReader`. They exist to keep the implementation out of the
  public header.
- Adapters that bind an external callback interface (odb `dbBlockCallBackObj`
  subclasses, such as gpl's `nesterovDbCbk`) to the tool's own class.
- Containers and iterators, such as `dbSet` over `dbIterator`.

**Refactoring:** Remove Middle Man by having callers talk to the target
directly, or Inline Class when the middle class adds nothing.

## Long Method and Long Parameter List

Long Method flags methods and free functions over `--max-lines` (100,
the repo's own function-size guideline) or with cyclomatic complexity
over `--max-cyclo` (25). Long Parameter List flags signatures with more
than `--max-params` (6) parameters. Overrides are skipped, because the
base class fixes their signature.

Both lists are long on a large codebase. They are most useful as
support: a long method that is also Feature Envy or the center of a
God Class cluster. **Refactorings:** Extract Method, and for long
parameter lists, Introduce Parameter Object (often an existing options
struct) or Preserve Whole Object.

## Data Clumps

A group of 3 or more parameters, or fields, that recurs in at least
`--clump-count` (4) places. Places are method signatures (overloads and
overrides count once), free-function signatures, and class field lists.
Items match on type and name, or on name alone with `--clump-by name`. A
group that mostly occurs inside a larger frequent group is reported only
as the larger one. The `(in fields)` count shows how many occurrences
are class fields. A group in both fields and parameters often means a
struct is half-introduced.

Typical findings:

- Coordinates: `lx, ly, ux, uy` or `x1, y1, x2, y2` should be
  `odb::Rect`, and `x, y` plus a layer or direction is a grid-point
  type.
- Context objects: `db, sta, logger` passed everywhere. A finding only
  if a context struct would remove real plumbing. Many classes take
  these in a constructor and that is fine.

**Refactoring:** Introduce Parameter Object, preferring an existing type.

## OpenROAD false positives

Before reporting a finding, check whether it is one of these patterns.
Each one reads another object's data on purpose:

- **Serializers and parsers:** `lefout`, `defout`, `definReader`,
  `*Writer`, `*Reader`, `*Parser`, `dbvWriter`, `gdsout`, UPF and SPEF
  writers. Their job is to walk another object. Moving their logic onto
  odb classes would couple the database to file formats. Feature Envy
  excludes these by name by default (`--fe-exclude`). Anything not
  matched by name still needs this check.
- **Inspectors:** `*Descriptor::getProperties` (gui, web). Same reason.
- **Options plumbing:** methods that copy an options struct into
  setters, such as `setParams` or `applySettings`.
- **odb public/impl pairs:** `dbFoo` methods are thin facades over
  `_dbFoo`. The extractor already treats `_dbFoo` as own.
- **Tcl/SWIG glue and `*-py.i`:** excluded at extraction.
- **Generated code:** `odb/src/db/*.cpp` written by the odb code
  generator. Those files contain markers such as `// Generator Code
  Begin`. Findings inside generated regions are only fixable in the
  generator's templates.
- **Algorithms on a graph object:** partitioners and coarseners (par),
  FastRoute (`grt/src/fastroute`), and similar code operate on a central
  data structure by design. Moving them onto it would make that structure
  a God Class. Report these only if a self-contained part of the logic
  belongs to the data structure itself.

## Choosing where code should live

"Move it onto the envied class" is right only when the dependency stays
pointed the right way:

- **Same module:** usually fine to move.
- **Envied class in a lower module** (odb, utl, sta, or another tool's
  public API): do not move tool-specific logic into it. If the code
  really is generic, propose it as a separate change to that module.
  Otherwise keep it where it is, or move it into a helper class inside
  the current module.
- **Envied class in a peer module:** moving the code would create a new
  cross-module dependency. Per `docs/agents/coding.md`, that dependency
  should go through an interface looked up in `utl::ServiceRegistry`,
  not a direct link to the other module's implementation library. Say
  so in the finding.
- **OpenSTA (`openroad/sta`):** never propose changes there. Put fixes
  in `dbSta`, `rsz`, or the caller.
- **Hot loops** (gpl Nesterov, drt maze, grt): a move must not add
  indirection or virtual calls in the inner loop. Flag the performance
  risk and suggest a benchmark rather than asserting the change is
  free.
