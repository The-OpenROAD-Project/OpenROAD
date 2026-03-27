# Bazel Test Performance Improvement Plan

> **Staleness warning**: This document is a snapshot of the state of affairs
> at commit `8f7f531beb` (2026-03-27). OpenROAD evolves quickly — test counts,
> file counts, and BUILD patterns will have changed since this was written.
> Before acting on the numbers or recommendations below, re-run the analysis
> commands to refresh the data. This document is intended as a starting point
> for tools like Claude to speed up their survey, not as a source of truth.
> This cache is also useful for users who don't have, need, or want Claude —
> it is the Claude equivalent of a log.

## TL;DR — Recommendations for Contributors

1. **Run only what you need.** `bazelisk test src/grt/...` is far faster than
   `bazelisk test //src/...`. A full run touches 1,682 tests.

2. **Trust the remote cache.** Most tests will be cache hits. A typical
   incremental run re-executes only ~60 tests; the other ~1,300 are served
   from cache in under a second each.

3. **Watch for these slow tests on the critical path:**
   - `rmp/aes_genetic` (239s), `rsz/repair_fanout6` (151s),
     `rmp/gcd_genetic` (134s), `cgt/ibex_sky130hd` (114s)
   - If your change doesn't touch rmp/rsz/cgt, these won't re-run.

4. **Compilation is the real bottleneck on a clean build.** The slowest files
   are `drt/FlexDR.cpp` (213s), several `dpl/*.cxx` files (~200s each), and
   SWIG wrappers (~198s each). Minimize header changes to avoid recompilation
   cascades.

5. **Avoid broad `glob(["**/*"])` in test BUILD files.** Each test gets its
   own symlink tree — broad globs multiply file count × test count. Prefer
   explicit file lists or per-PDK filegroups. See the detailed analysis below.

6. **Known failing tests** (as of 2026-03-27):
   - `rmp/aes_genetic` — times out at 300s (was 239s, flaky)
   - `ppl/annealing_mirrored_hang` — times out at 300s
   - `ant/ant_man_tcl_check` — documentation count mismatch
   - `bazel/verify_orfs_tags` — env var issue, only works with `bazel run`

## Purpose

The intention is to do a periodic pass using this document with Claude to
improve testing time and minimize ineffective tests (duplicates, overly long
running tests, tests with poor signal-to-noise ratio). This document caches
the analysis so Claude can skip the initial survey and jump straight to
curation.

## Context

`bazelisk test ...` runs 1,682 tests across 33 modules. A full uncached build
takes ~13 minutes wall time with `--jobs=200`. Symlink tree creation for test
runfiles accounts for ~171s (22%) of that wall time.

## Profile Results (2026-03-27, commit 8f7f531beb)

### Wall time breakdown

A `bazelisk test //src/...` run with `--jobs=200` completed in **781s** wall time.
The parallelism factor was 195x (152,281s total CPU time across all actions).

| Category | CPU time | % of CPU | Notes |
|----------|----------|----------|-------|
| Compilation | 148,050s | 97.2% | 2,574 actions; dominates CPU |
| Linking | 1,484s | 1.0% | 27 link actions |
| SWIG | 1,409s | 0.9% | 50 actions |
| Test execution | 595s | 0.4% | 60 tests actually ran (rest cached) |
| Proto generation | 486s | 0.3% | or-tools protos |
| Symlink trees | 98s | 0.1% | 223 runfiles trees |
| Genrules | 84s | 0.1% | messages_txt etc. |

### Key insight: symlinks dominate wall time, not CPU time

Symlink tree creation took **171s of wall time (22%)** despite only 98s of CPU.
This is because symlink creation is I/O-bound and partially serialized. On a
warm-cache run (1,309 of 1,369 tests cached), symlinks become the primary
bottleneck since compilation is skipped.

### Symlink creation time by module

| Module | Trees | Total (s) | Avg (s) | Max (s) |
|--------|------:|----------:|--------:|--------:|
| rsz | 55 | 19.3 | 0.35 | 1.41 |
| pdn | 26 | 19.1 | 0.74 | 1.82 |
| dbSta | 12 | 11.7 | 0.97 | 2.72 |
| dpl | 13 | 8.1 | 0.63 | 1.29 |
| odb | 23 | 7.2 | 0.31 | 1.25 |
| gpl | 19 | 6.3 | 0.33 | 0.47 |
| tap | 9 | 5.1 | 0.57 | 1.09 |
| ppl | 12 | 4.3 | 0.36 | 0.66 |
| cgt | 3 | 3.8 | 1.27 | 1.54 |
| grt | 9 | 3.0 | 0.33 | 0.46 |

Note: not every test creates its own tree on every run — Bazel reuses
unchanged runfiles trees. In this run only 223 of ~1,400 trees were created.

### Slowest compilations

| Seconds | File |
|--------:|------|
| 213 | src/drt/src/dr/FlexDR.cpp |
| 206 | src/dpl/src/optimization/detailed_mis.cxx |
| 206 | src/dpl/src/optimization/detailed.cxx |
| 206 | src/dpl/src/Place.cpp |
| 199 | src/mpl/swig.cc |
| 198 | src/pdn/swig.cc |
| 198 | src/odb/test/cpp/scan/TestScanChain.cpp |

Compilation of dpl files is expensive (167s avg) because of heavy template
use and Boost includes. SWIG-generated files are also slow (~198s each).

### Compile time by module (top 10)

| Module | Files | Total CPU (s) | Avg (s) |
|--------|------:|--------------:|--------:|
| external | 1,148 | 50,983 | 44.4 |
| odb | 344 | 28,636 | 83.2 |
| scip | 376 | 20,229 | 53.8 |
| drt | 80 | 8,276 | 103.5 |
| dpl | 37 | 6,173 | 166.8 |
| sta | 171 | 6,135 | 35.9 |
| api | 54 | 4,093 | 75.8 |
| grt | 18 | 2,110 | 117.2 |
| rcx | 21 | 2,098 | 99.9 |
| gui | 9 | 1,423 | 158.1 |

## Slowest Tests

### Top 20 individual tests by wall time

| Seconds | Test |
|--------:|------|
| 239 | src/rmp/test/aes_genetic-tcl |
| 151 | src/rsz/test/repair_fanout6-tcl |
| 134 | src/rmp/test/gcd_genetic-tcl |
| 114 | src/cgt/test/ibex_sky130hd-tcl |
| 105 | src/rsz/test/repair_design3-tcl |
| 91 | src/rsz/test/replace_arith_modules2-tcl |
| 76 | src/rsz/test/repair_design3_verbose-tcl |
| 73 | src/rsz/test/replace_arith_modules3-tcl |
| 59 | test/downstream_rules_verilator_not_dep_test |
| 56 | test/downstream_rules_shell_not_dep_test |
| 56 | src/rmp/test/aes_annealing-tcl |
| 55 | test/downstream_rules_pkg_not_dep_test |
| 53 | src/rmp/test/gcd_annealing1-tcl |
| 41 | src/cgt/test/aes_nangate45-tcl |
| 37 | src/rmp/test/gcd_annealing2-tcl |
| 33 | src/gpl/test/simple10-tcl |
| 28 | src/rsz/test/replace_arith_modules1-tcl |
| 28 | src/psm/test/zerosoc_pads-tcl |
| 26 | src/grt/test/pin_access2-tcl |
| 25 | src/psm/test/aes_test_multiple_bterms-tcl |

### Total time by module

| Module | Tests | Total (s) | Avg (s) |
|--------|------:|----------:|--------:|
| rsz | 202 | 754 | 3.7 |
| rmp | 27 | 580 | 21.5 |
| psm | 41 | 232 | 5.7 |
| grt | 102 | 208 | 2.0 |
| test | 58 | 186 | 3.2 |
| cgt | 3 | 159 | 53.0 |
| cts | 44 | 133 | 3.0 |
| mpl | 39 | 116 | 3.0 |
| gpl | 62 | 92 | 1.5 |
| pad | 57 | 75 | 1.3 |
| dpl | 84 | 51 | 0.6 |
| ppl | 128 | 26 | 0.2 |
| rcx | 11 | 25 | 2.3 |
| pdn | 135 | 19 | 0.1 |
| odb | 122 | 19 | 0.2 |
| dbSta | 64 | 19 | 0.3 |
| ifp | 44 | 16 | 0.4 |
| dft | 17 | 10 | 0.6 |

### Observations

- **rmp** has the worst avg time (21.5s) — genetic/annealing tests are inherently slow
- **cgt** has only 3 tests but averages 53s each (full chip gate tests)
- **rsz** dominates total time (754s) due to sheer test count (202)
- **pdn** has 135 tests averaging 0.1s — test setup dominates actual work
- **ppl** has 128 tests averaging 0.2s — same pattern, setup-dominated

## Failing Tests (2026-03-27)

| Test | Issue |
|------|-------|
| `//src/ant/test:ant_man_tcl_check-py` | "Command counts do not match" — documentation check |
| `//src/ppl/test:annealing_mirrored_hang-tcl` | **Timed out** at 300s — possible infinite loop or very slow convergence |
| `//bazel:verify_orfs_tags` | `BUILD_WORKSPACE_DIRECTORY` unbound — only works with `bazel run`, not `bazel test` |

The `annealing_mirrored_hang` timeout is notable — this test may need a
smaller input or the algorithm may have a convergence issue.

## Analyzing Build & Test Performance

### Profiling a test run

```bash
# Profile the full test suite
bazelisk test //src/... --profile=/tmp/bazel_profile.json.gz 2>&1 | tee /tmp/bazel_test.log

# Open the profile in a browser (Chrome, or https://ui.perfetto.dev)
# Look for: which phase dominates, individual test times, symlink creation overhead
```

### Extracting test durations from cached results

Bazel stores test results as XML in `bazel-testlogs/`. Extract durations with:

```bash
# Top 40 slowest tests
find $(bazelisk info bazel-testlogs) -name "test.xml" \
  -exec sh -c 'name=$(echo "$1" | sed "s|.*/testlogs/||; s|/test\.xml||"); \
  time=$(grep -oP '\''time="\K[0-9.]+'\'' "$1" | head -1); \
  echo "$time $name"' _ {} \; | sort -rn | head -40

# Total time per module
find $(bazelisk info bazel-testlogs) -name "test.xml" \
  -exec sh -c 'name=$(echo "$1" | sed "s|.*/testlogs/||; s|/test\.xml||"); \
  time=$(grep -oP '\''time="\K[0-9.]+'\'' "$1" | head -1); \
  module=$(echo "$name" | sed "s|src/\([^/]*\)/.*|\1|; s|test/.*|test|"); \
  echo "$module $time"' _ {} \; \
  | awk '{sum[$1]+=$2; count[$1]++} END {for(m in sum) printf "%s | %d | %.0f | %.1f\n", m, count[m], sum[m], sum[m]/count[m]}' \
  | sort -t'|' -k3 -rn
```

### Counting symlinks per module

```bash
# Estimate symlinks: tests × (own files + regression_resources files)
for module in rsz pdn ppl grt dpl gpl odb dbSta ifp pad tap cts psm mpl; do
  tests=$(bazelisk query "kind(test, //src/$module/...)" 2>/dev/null | wc -l)
  own_files=$(find "src/$module/test/" -type f 2>/dev/null | wc -l)
  uses_rr=$(grep -q 'regression_resources' "src/$module/test/BUILD" && echo 334 || echo 0)
  echo "$module | $tests | $own_files | $uses_rr | $((tests * (own_files + uses_rr)))"
done
```

### Monitoring running tests

Test output goes to:
- **`bazel-testlogs/<target>/test.log`** — main output (available after completion)
- **`bazel-testlogs/<target>/test.xml`** — JUnit XML with timing
- **`bazel-testlogs/<target>/test.outputs/`** — undeclared outputs (`TEST_UNDECLARED_OUTPUTS_DIR`)

During execution, the test framework (`test/bazel_test.sh`) creates:
- `$TEST_UNDECLARED_OUTPUTS_DIR/results/<test_name>-<ext>.log`

To stream output live:

```bash
bazelisk test //src/grt/test:pin_access2-tcl --test_output=streamed
```

To keep sandbox files for debugging failures:

```bash
bazelisk test //src/rsz/test:repair_design3-tcl --sandbox_debug
```

To check which tests are currently running:

```bash
# Watch bazel-testlogs for new test.xml files appearing
watch -n2 'find bazel-testlogs -name "test.log" -newer /tmp/test_start_marker | wc -l'
```

## Problem Analysis

### Symlink sources (per test)

Each regression test gets symlinks from three sources:

1. **Module-local `test_resources`** — files in `src/<module>/test/`
2. **`//test:regression_resources`** — 334 files from `test/` (PDK data, .lib, .lef, .v files)
3. **OpenROAD binary transitive runfiles** — the binary + its deps

### Top offenders by estimated symlinks

| Module | Tests | Own files | +regression_resources | Total symlinks |
|--------|------:|----------:|----------------------:|---------------:|
| rsz    |   201 |       670 |                     0 |        134,670 |
| pdn    |   163 |       453 |                   334 |        128,281 |
| ppl    |   152 |       408 |                   334 |        112,784 |
| dpl    |   112 |       328 |                   334 |         74,144 |
| odb    |   121 |       307 |                   334 |         77,561 |
| grt    |   117 |       346 |                     0 |         40,482 |
| dbSta  |    64 |       219 |                   334 |         35,392 |
| **Total** | **1,497** | | | **~823K** |

### Resource pattern categories

- **`regression_resources` only** (22 modules): ant, cgt, cts, dbSta, dft, dpl, drt, fin, ifp, mpl, odb, pad, par, pdn, psm, ram, rcx, rmp, tap, upf, utl
- **Own `glob(["**/*"])` only** (8 modules): est, exa, gpl, grt, rsz, stt, web, gui
- **BOTH** (1 module): ppl
- **Explicit files, no regression_resources** (2 modules): cut, dst

### Missing Bazel test coverage

| Module | .ok files | Bazel tests | Gap |
|--------|----------:|------------:|-----|
| sta    |        34 |           0 | Full — no BUILD file at all |
| ram    |         1 |           0 | 1 test marked `MANUAL_FOR_BAZEL` |

## Improvement Plan (ordered by impact/churn ratio)

### DONE: Trim `@tcl_lang//:tcl_core` (820 files eliminated per test)

**Files**: `MODULE.bazel`, `bazel/patches/tcl_lang_trim_core.patch`

The `tcl_core` filegroup used `glob(["library/**"])` matching 831 files.
A patch excludes tzdata (596), msgs (127), encoding (82), and other
unused Tcl library subdirs (http, tcltest, opt, msgcat, dde, platform, reg).

**Measured result**: 831 → 11 tcl_lang symlinks per test.

Note: removing openroad's transitive runfiles entirely from tests does NOT work.
The sandbox only mounts the test's declared runfiles, so `InitRunFiles.cpp`
cannot resolve `tcl_lang/library/` via `/proc/self/exe` inside the sandbox.

### DONE: Trim `//test:regression_resources` (54 files eliminated per test)

**File**: `test/BUILD`

Excluded `orfs/**` and `downstream/**` from the glob — these directories
are never used by regression tests.

**Measured result**: 334 → 280 regression_resources files per test.

### Combined result

Per-test runfiles (pdn/core_grid_dual_followpins):
- 11: `tcl_lang+/library/` (was 831)
- 280: `test/` regression_resources (was 334)
- ~131: module-local data, openroad binary, scripts, tclreadline, openmp
- **422 total** (was 1,242 — **66% reduction**)

Total across ~1,400 tests: ~1.74M → ~591K symlinks (saved ~1.15M).

### TODO: Further reduce `//test:regression_resources`

### TODO: Refine module-local `glob(["**/*"])` patterns

9 modules use `glob(["**/*"])` on their own test directories.
Top targets: rsz (670 files × 201 tests), ppl (408 × 152), grt (346 × 117).

### TODO: Fix failing tests

- **`annealing_mirrored_hang`**: Timed out at 300s
- **`ant_man_tcl_check`**: Documentation count mismatch
- **`verify_orfs_tags`**: `BUILD_WORKSPACE_DIRECTORY` unbound in test

### TODO: Add sta Bazel tests

34 `.ok` files in `src/sta/test/` with 0 Bazel tests (coverage, not performance).

## Verification

After each phase, measure:

```bash
# Time the full test suite
time bazelisk test //src/... --profile=/tmp/after_phase_N.json.gz

# Count symlinks in output base
find $(bazelisk info output_base)/execroot -type l 2>/dev/null | wc -l

# Compare profiles
# Open both .json.gz files in perfetto.dev and compare total wall time
```

## Key Files

- `test/BUILD` — `regression_resources` glob and per-PDK filegroups (lines 89-148)
- `test/regression.bzl` — `regression_test` macro, runfiles construction (lines 51-64, 295-297)
- `test/bazel_test.sh` — test entry point, sets up `RESULTS_DIR` under `TEST_UNDECLARED_OUTPUTS_DIR`
- `test/regression_test.sh` — actual test runner, creates `<test_name>-<ext>.log`
- `test/helpers.tcl` — Tcl test utilities, uses `TEST_TMPDIR` and `RESULTS_DIR`
- `test/helpers.py` — Python test utilities, uses `TEST_TMPDIR` and `TEST_SRCDIR`
- `.bazelrc` — Bazel configuration (126 lines)
- `src/rsz/test/BUILD` — largest module by symlink count
- `src/pdn/test/BUILD` — second largest
- `src/ppl/test/BUILD` — only module using BOTH patterns
- `src/grt/test/BUILD` — comments acknowledge "overly broad glob" (line 109)
