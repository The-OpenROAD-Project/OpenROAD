# OpenROAD Testing Strategy

This document defines **where** new tests should live and **how** to handle the
dependencies that make C++ unit testing of OpenROAD hard. It complements
`testing.md`, which covers the mechanics of writing and registering a test.

## Goal

Move the bulk of *functional correctness* coverage into minimally-scoped C++
unit tests, while keeping a thin layer of Tcl/Python tests that prove the
bindings work and a small curated set of full-flow tests that prove the stages
compose. Core logic should be tested with semantic assertions, not golden-file
diffs.

> **Policy.** The end state is to **migrate off** the bulk of the Tcl/Python
> golden tests, not keep them indefinitely. But that migration is a large,
> incremental effort delivered as a long series of small steps -- never a single
> big-bang rewrite. Two things happen in parallel:
>
> 1. **New tests** default to C++ unit tests now (this strategy's pyramid).
> 2. **Existing golden tests** are retired in batches as their coverage is
>    re-expressed in C++ -- the C++ test and the removal of the Tcl/Python test
>    it replaces land in the *same* change. Prioritized by pain (flaky/slow/
>    frequently-broken modules and code you are already touching), with each
>    batch a reviewable change of its own.
>
> A golden test is only deleted once equivalent or better coverage exists in
> C++; don't drop coverage to hit a ratio. Until then it stays.

## Why move off golden-file tests

The repo today is ~1,674 Tcl + ~265 Python regression tests vs. ~55 C++ unit
tests. Most functionality is verified by reading LEF/DEF, running a command, and
`diff`-ing a golden `.ok`/`.defok`/`.vok` file. That model has structural
weaknesses:

1. **It tests serialization, not logic.** A passing diff means bytes matched, not
   that the result is *correct*. When a buffer count changes you learn something
   differed, not whether the new number is right.
2. **It couples to unrelated behavior.** `diff_file` breaks on any upstream
   formatting/ordering change, and it only reports the first difference (so
   golden regen must be wholesale).
3. **No fault isolation.** A flow test exercises the readers + STA + the unit
   under test at once; a failure could be anywhere.
4. **It's slow.** Each test spawns a full `openroad` process and re-parses the
   Nangate45/Sky130 libraries.

C++ unit tests built on the existing fixtures fix all four: semantic assertions,
no file coupling, single-unit scope, sub-millisecond setup.

## The test pyramid

Aim every **new** test at the lowest tier that can express it.

### Tier 1 -- C++ unit tests (default; target the majority of new tests)

One function/class, inputs built programmatically, assertions on observable
state (`EXPECT_EQ(net->getITermCount(), 3)`), not on a serialized dump. This is
where algorithmic correctness, edge cases, and regression-bug pins belong.

### Tier 2 -- Binding smoke tests (thin: ~1 per public command)

Their *only* job is to prove the Tcl/Python binding marshals arguments and
returns without crashing -- not to validate the algorithm. Call the command once
on a trivial design and assert it ran. Prefer `PASSFAIL_TESTS` (exit-code only)
over golden diffs so there is no `.ok` file to maintain.

### Tier 3 -- Full-flow integration tests (keep deliberately few)

A curated handful per module that prove the stages compose end-to-end on a real
design (e.g. gcd, aes). These are the "the flow still works" canaries. You want
dozens across the project, not hundreds -- and this small curated set is the
*permanent* home of golden-file testing. Most of today's golden tests are really
unit-logic tests wearing a flow-test costume: those are the migration target
(demote to Tier 1), not this canary layer. Resist adding new Tier-3 tests when a
Tier-1 test would do.

The migration shape is therefore: **a new feature lands as Tier-1 semantic tests
plus one Tier-2 smoke test, instead of a new Tier-3 golden test.**

## Handling dependencies

This is the crux. Pick the **lightest fixture that exposes the dependency the
unit actually needs.**

| Dependency of the unit under test | Fixture / approach | Cost |
| --- | --- | --- |
| None -- pure algorithm/geometry/graph kernel | No fixture; plain structs + `odb` geom primitives | trivial |
| An `odb` database (cells, nets, placement) | `tst::Fixture` / `odb::SimpleDbFixture` + `makeInst`/`makeBTerm`/`makeNets` | sub-ms |
| STA timing | `tst::IntegratedFixture` (kNangate45 / kSky130hd) | ms (libs loaded once) |
| Genuine cross-tool composition | Keep as a Tier-3 flow test | full process |

The existing fixture stack already supports this:

- **`tst::Fixture`** (`src/tst/include/tst/fixture.h`) -- owns `db_`, `sta_`,
  `logger_`; provides `loadTechLef`/`loadLibaryLef`/`readLiberty` and the netlist
  builders `makeInst`, `makeBTerm`, `makeNets`. The header explicitly states
  these are meant to make C++ setup "competitive with writing a Verilog or DEF
  test case by hand."
- **`odb::SimpleDbFixture`** (`src/odb/test/cpp/helper/helper.h`) -- pre-builds a
  minimal tech/lib/chip/block and `createMaster*` helpers.
- **`tst::IntegratedFixture`** (`src/tst/include/tst/IntegratedFixture.h`) -- wires
  `sta_`, `resizer_`, `dp_`, `grt_`, `ant_`, `stt_`, `ep_` against real libs and
  offers `readVerilogAndSetup`. This is how `rsz` and `dbSta` already test; use
  them as the reference pattern.

### Principles

1. **Depend on data/interfaces, not the whole pipeline.** If a function needs a
   fully placed-and-routed DB just to test one calculation, that is a design
   smell. Refactor the kernel to take the data it needs (a struct, a span, an
   `odb` geometry) so it can be tested with no fixture. *This
   refactor-for-testability is the single highest-leverage move* -- it improves
   the code and makes the test trivial.
2. **Builders over checked-in files.** Every time a test needs "a block with a
   row of 3 placed cells," that should be a fixture method, not a new DEF.
   Growing per-module builder helpers (the `SimpleDbFixture` pattern, extended to
   dpl/grt/cts/...) is the shared library that makes Tier-1 cheap. Invest here.
3. **Inject dependencies.** Prefer constructor/parameter injection so a unit can
   be handed a minimal hand-built DB (or a narrow fake of an *external* tool's
   output) instead of discovering global state.
4. **Do not mock `odb`.** It is the lingua franca, it is cheap to instantiate,
   and faking it is more work than building a tiny real one. Fake/stub only
   expensive *external* tools, and only when the unit needs a narrow slice of
   their output.

## Keeping binding tests minimal but honest

- One smoke test per public Tcl command and per Python command, on a trivial
  design, asserting invocation + basic return marshaling. Convert to
  `PASSFAIL_TESTS` where possible.
- Treat the C++ test as the source of truth for correctness; bindings prove
  plumbing only. Do **not** duplicate algorithm assertions across Tcl + Python.
- A future linter (not yet implemented) could enumerate public commands from the
  `.tcl`/`.i` files and flag any without a smoke test, guaranteeing binding
  coverage without hand-curation.

## Decision tree for a new test

```
Is it pure logic with no DB?            -> Tier 1, no fixture
Does it need an odb DB only?            -> Tier 1, tst::Fixture / SimpleDbFixture
Does it need STA/resizer/router state?  -> Tier 1, tst::IntegratedFixture
Is it only proving a binding marshals?  -> Tier 2, smoke test (PASSFAIL)
Does it genuinely span multiple tools?  -> Tier 3, flow test (golden, used sparingly)
```

## Runbook: retiring a batch of golden tests

**Rule: the C++ test and the removal of the Tcl/Python test(s) it replaces land
in the same change.** Once a behavior is covered by a C++ unit test, the golden
test that previously pinned it is redundant and should be deleted in that same
commit -- not left behind "for safety" and not deferred to a later cleanup. The
one gate is coverage equivalence (step 5): you only delete what the C++ test now
covers.

A "batch" is one such reviewable change: it adds C++ tests for a small, related
group of behaviors and removes the golden tests they supersede. Keep batches
small enough to review in one sitting -- a few related tests, not a whole module
at once.

1. **Pick a target.** Prioritize by pain: flaky/slow/frequently-broken tests,
   tests for code you are already modifying, or a tightly-related cluster (e.g.
   all `buffer_ports*`). Avoid the curated Tier-3 flow canaries -- those stay.
2. **Characterize what each golden test actually verifies.** Read the `.tcl`/`.py`
   and its `.ok`/`.defok`/`.vok`. Write down the *intent* ("inserts a buffer on
   each output port", "rejects nets wider than the layer max"), not the byte
   diff. This intent is what the C++ test will assert.
3. **Choose the fixture** via the decision tree above -- the lightest one that
   exposes the dependency. If the unit needs the whole pipeline to test one
   calculation, refactor the kernel to take its data directly first (a separate,
   prior change), then test it with no/low fixture.
4. **Write the C++ unit test(s)** with semantic assertions on observable state.
   One behavior per `TEST_F`; build inputs with `makeInst`/`makeBTerm`/`makeNets`
   or `createMaster*` rather than checked-in DEFs. Add a builder helper if the
   same construction recurs. Register in BOTH build systems.
5. **Confirm coverage is equal-or-better.** The C++ tests must cover every
   behavior the golden test pinned (use the intent list from step 2 as a
   checklist). A C++ test that asserts *more* (e.g. an error/rejection path the
   golden never reached) is the goal. If some behavior genuinely cannot be
   re-expressed in C++, keep that one golden test and note why -- but still
   remove the rest of the batch.
6. **Delete the superseded Tcl/Python test(s) and their artifacts, and
   de-register from BOTH build systems** -- in this same change. Remove the
   `.tcl`/`.py` and its `.ok`/`.defok`/`.vok`; drop the name from
   `src/<module>/test/CMakeLists.txt` (`or_integration_tests`) **and**
   `src/<module>/test/BUILD` (`regression_test`). Remove checked-in DEF/LEF data
   only if nothing else references it (grep first -- fixtures like Nangate45 and shared `data/` files
   are used by many tests).
7. **Check residual binding coverage.** Removing a *golden* test does not have to
   mean the command loses all Tcl/Python exercise: a command is often also
   touched by a broader binding test (e.g. odb's `test_inst.py`/`test_inst.tcl`).
   If the deleted test was the *only* thing invoking that command from a binding,
   add a thin Tier-2 smoke test (`PASSFAIL`, trivial design) so the entry point
   stays exercised. Note in the commit where binding coverage now lives.
8. **Verify.** Run the module's C++ tests and remaining regressions in both
   builds (`ctest`/`make test` and the Bazel target) to confirm the new tests
   pass and nothing references the removed names.

> The discipline that keeps this safe: deletion is gated on coverage
> equivalence (step 5), and every add/remove touches CMake + Bazel together (a
> half-registered or half-deregistered test breaks Bazel CI while passing local
> `make test`).

## Registration reminder

Every new test -- C++ or Tcl -- must be registered in **both** CMake and Bazel.
Forgetting the Bazel `BUILD` entry passes local `make test` but breaks Bazel CI.
See `testing.md` for the exact macros (`or_integration_tests` / `regression_test`
/ `cc_test` + `gtest_discover_tests`).
