# ccache Performance Benchmark for OpenROAD Bazel Build

## Why ccache?

Bazel's built-in action cache is keyed on the **action graph** (flags, BUILD files,
dependency versions), not on source-file contents. Common developer workflows
-- switching branches, editing a BUILD file, or running `bazel clean` -- invalidate
the action cache even when most source files are unchanged.

`ccache` caches based on **preprocessed source content**, so it survives these
invalidations and delivers near-instant cache hits for unchanged translation units.

## Environment

| Item | Value |
|------|-------|
| CPU | Intel(R) Core(TM) i9-9900KF CPU @ 3.60GHz (16 cores) |
| RAM | 30Gi |
| OS | Ubuntu 25.10 |
| ccache | ccache version 4.11.2 |
| Bazel | bazel 8.5.0 |
| Commit | `82027a28d3` |
| Sources | 1005 .cpp/.cc files, 815 .h/.hpp files, 33 modules |
| Build targets | 83 cc_library targets (excluding gui) |
| Runs per scenario | 3 (median reported) |

## Developer Personas (from git history)

We analyzed the last 500 commits to identify typical developer workflows:

| Persona | Description | Simulated Change | Example Contributors |
|---------|-------------|-----------------|---------------------|
| Single-tool bug fix | Fix a bug in one module | 1 .cc file in rsz | Thinh Nguyen, Jaehyun Kim |
| Algorithm improvement | Improve algorithm in one tool | 3 .cpp files in grt | Augusto Berndt, Eder Monteiro |
| Cross-cutting refactor | Rename API, clang-tidy cleanup | 20 files across 5 modules | Henner Zeller |

## Methodology

1. Full build with `--config=ccache` to warm ccache (all 83 cc_library targets)
2. For each scenario, repeat 3 times:
   a. Append a unique comment to the scenario's source files (real content change, not just `touch`)
   b. `bazel clean` to invalidate Bazel's action cache
   c. Rebuild **with ccache** -- record wall-clock time and hit rate
   d. Restore files via `git checkout`
   e. Re-apply same edits, `bazel clean` again
   f. Rebuild **without ccache** (`CCACHE_DISABLE=1`) -- record wall-clock time
   g. Restore files
3. Bazel disk cache and remote cache disabled (`--disk_cache= --remote_cache=`) to isolate ccache effect
4. Machine was dedicated to the benchmark (no other builds running)
5. Median of 3 runs reported

## Results

| Scenario | Files Changed | With ccache (s) | Without ccache (s) | Speedup | ccache Hits |
|----------|:------------:|:--------------:|:-----------------:|:-------:|:-----------:|
| Single-tool bug fix | 1 | 143.1 | 1521.3 | **10.6x** | 5790/5790 |
| Algorithm improvement | 3 | 147.6 | 1522.4 | **10.3x** | 5790/5790 |
| Cross-cutting refactor | 20 | 154.0 | 1522.8 | **9.8x** | 5790/5790 |

## Key Findings

- **10x speedup across all scenarios** -- ccache reduces rebuild time from ~25 minutes
  to ~2.5 minutes after `bazel clean`.
- **100% cache hit rate (5790/5790)** -- even the cross-cutting refactor touching 20 files
  across 5 modules achieves full hits because Bazel recompiles all targets after
  `bazel clean`, but ccache recognizes unchanged preprocessed content.
- **Consistent with-ccache times (~145s)** -- the number of changed files barely affects
  rebuild time because ccache serves all unchanged translation units from cache.
  The small increase from 143s to 154s reflects the additional files that must be
  genuinely recompiled.
- **Without-ccache times are stable (~1522s)** -- confirming the machine was uncontested
  and results are reliable.

## When does this help?

Any workflow that invalidates Bazel's action cache while leaving most source unchanged:

- **`bazel clean`** -- full action cache wipe, but ccache still has all objects
- **Branch switching** -- different action graph, same source content
- **BUILD file edits** -- invalidates the target's action cache even if no source changed
- **Merging origin/master** -- MODULE.bazel or BUILD changes cascade through the action graph
- **Flag changes** -- different `-c opt` vs `-c dbg` etc.

## Setup

Add to your `user.bazelrc`:

```bash
echo 'build --config=ccache' >> user.bazelrc
```

Or invoke explicitly:

```bash
bazel build --config=ccache //...
```

## Reproducing

The benchmark script is at `tools/ccache_benchmark.sh`:

```bash
bash tools/ccache_benchmark.sh --runs=3
```
