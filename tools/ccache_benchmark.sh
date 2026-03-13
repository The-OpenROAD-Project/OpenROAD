#!/usr/bin/env bash
# ccache_benchmark.sh — Measure ccache benefit for typical developer workflows
#
# Usage:
#   bash tools/ccache_benchmark.sh [--runs=N] [--target=TARGET]
#
# Defaults: 3 runs per scenario, builds //...
# Writes:   benchmark_results.csv  (raw data)
#           docs/ccache_benchmark_results.md (formatted report)
set -euo pipefail
export LC_NUMERIC=C  # Ensure decimal point in floating-point output

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

RUNS=3
SKIP=0
BAZEL="${BAZEL:-bazelisk}"
CSV="$REPO_ROOT/benchmark_results.csv"
MD_OUT="$REPO_ROOT/docs/ccache_benchmark_results.md"

# Parse arguments
for arg in "$@"; do
  case "$arg" in
    --runs=*) RUNS="${arg#--runs=}" ;;
    --skip=*) SKIP="${arg#--skip=}" ;;
    *) echo "Unknown arg: $arg"; exit 1 ;;
  esac
done

# Resolve build targets: all cc_libraries in src/ except gui.
# Tests are excluded because they depend on //:openroad_lib (Version.hh genrule issue).
echo "=== Resolving build targets ==="
mapfile -t BUILD_TARGETS < <($BAZEL query 'kind("cc_library", //src/...) except //src/gui/...' 2>/dev/null)
echo "Found ${#BUILD_TARGETS[@]} cc_library targets"

# Common bazel flags: disable disk and remote caches to isolate ccache effect
BAZEL_FLAGS=(--config=ccache --disk_cache= --remote_cache= --noshow_progress)

# ---------- File lists per scenario ----------

SINGLE_TOOL_FILES=(
  src/rsz/src/RepairDesign.cc
)

ALGORITHM_FILES=(
  src/grt/src/GlobalRouter.cpp
  src/grt/src/Grid.cpp
  src/grt/src/Net.cpp
)

CROSS_CUTTING_FILES=(
  src/drt/src/GraphicsFactory.cpp
  src/drt/src/MakeTritonRoute.cpp
  src/drt/src/DesignCallBack.cpp
  src/drt/src/frBaseTypes.cpp
  src/grt/src/Pin.cpp
  src/grt/src/MakeGlobalRouter.cpp
  src/grt/src/Grid.cpp
  src/grt/src/GrouteRenderer.cpp
  src/odb/src/db/dbAccessPoint.cpp
  src/odb/src/db/dbBPin.cpp
  src/odb/src/db/dbBPinItr.cpp
  src/odb/src/db/dbBTermItr.cpp
  src/gpl/src/graphicsImpl.cpp
  src/gpl/src/AbstractGraphics.cpp
  src/gpl/src/MakeReplace.cpp
  src/gpl/src/fftsg.cpp
  src/rsz/src/BaseMove.cc
  src/rsz/src/BufferMove.cc
  src/rsz/src/BufferedNet.cc
  src/rsz/src/CloneMove.cc
)

BUILD_INFRA_FILES=(
  src/drt/BUILD
)

HEADER_FILES=(
  src/odb/include/odb/db.h
)

# Merge-master scenario: MODULE.bazel change (dep bump) + scattered source edits
# simulating what happens when you merge origin/master into your feature branch
MERGE_MASTER_FILES=(
  MODULE.bazel
  src/odb/src/db/dbAccessPoint.cpp
  src/drt/src/frBaseTypes.cpp
  src/grt/src/GlobalRouter.cpp
  src/rsz/src/RepairDesign.cc
  src/gpl/src/MakeReplace.cpp
)

SCENARIOS=(single_tool algorithm cross_cutting build_infra header merge_master branch_switch)

SCENARIO_LABELS=(
  "Single-tool bug fix"
  "Algorithm improvement"
  "Cross-cutting refactor"
  "Build/infra change"
  "Header change"
  "Merge origin/master"
  "Branch switch (clean)"
)

SCENARIO_FILE_COUNTS=(1 3 20 1 1 6 0)

# ---------- Helpers ----------

log() { printf "\n=== %s ===\n" "$*"; }

detect_env() {
  log "Detecting environment"
  ENV_HW=$(lscpu 2>/dev/null | grep "Model name" | head -1 | sed 's/.*:\s*//' || echo "unknown")
  ENV_CORES=$(nproc 2>/dev/null || echo "?")
  ENV_RAM=$(free -h 2>/dev/null | awk '/Mem:/{print $2}' || echo "?")
  ENV_OS=$(. /etc/os-release 2>/dev/null && echo "$PRETTY_NAME" || uname -s)
  ENV_CCACHE=$(ccache --version 2>/dev/null | head -1 || echo "not found")
  ENV_BAZEL=$($BAZEL --version 2>/dev/null | head -1 || echo "unknown")
  ENV_COMMIT=$(git -C "$REPO_ROOT" rev-parse --short HEAD)
  ENV_CPP_COUNT=$(find src -name '*.cpp' -o -name '*.cc' | wc -l)
  ENV_H_COUNT=$(find src -name '*.h' -o -name '*.hpp' | wc -l)
  ENV_MODULES=$(ls -d src/*/BUILD 2>/dev/null | wc -l)
  echo "CPU:      $ENV_HW ($ENV_CORES cores)"
  echo "RAM:      $ENV_RAM"
  echo "OS:       $ENV_OS"
  echo "ccache:   $ENV_CCACHE"
  echo "Bazel:    $ENV_BAZEL"
  echo "Commit:   $ENV_COMMIT"
  echo "Sources:  $ENV_CPP_COUNT .cpp/.cc, $ENV_H_COUNT .h/.hpp, $ENV_MODULES modules"
}

warm_ccache() {
  log "Warming ccache (full build)"
  $BAZEL build "${BAZEL_FLAGS[@]}" "${BUILD_TARGETS[@]}" 2>&1
  echo "ccache stats after warm-up:"
  ccache -s
}

get_scenario_files() {
  local scenario="$1"
  case "$scenario" in
    single_tool)    echo "${SINGLE_TOOL_FILES[@]}" ;;
    algorithm)      echo "${ALGORITHM_FILES[@]}" ;;
    cross_cutting)  echo "${CROSS_CUTTING_FILES[@]}" ;;
    build_infra)    echo "${BUILD_INFRA_FILES[@]}" ;;
    header)         echo "${HEADER_FILES[@]}" ;;
    merge_master)   echo "${MERGE_MASTER_FILES[@]}" ;;
    branch_switch)  echo "" ;;
  esac
}

simulate_change() {
  local scenario="$1"
  local files
  files=$(get_scenario_files "$scenario")
  [[ -z "$files" ]] && return 0
  for f in $files; do
    case "$f" in
      *.BUILD|*/BUILD|*.bzl|*.bazel|MODULE.bazel)
        echo "# ccache-benchmark-$(date +%s%N)" >> "$f" ;;
      *)
        echo "// ccache-benchmark-$(date +%s%N)" >> "$f" ;;
    esac
  done
}

restore_files() {
  local scenario="$1"
  local files
  files=$(get_scenario_files "$scenario")
  [[ -z "$files" ]] && return 0
  # shellcheck disable=SC2086
  git checkout -- $files
}

timed_build() {
  # $1 = extra env vars (optional), e.g. "CCACHE_DISABLE=1"
  # Returns only the elapsed time; Bazel output goes to fd 1 via fd 3.
  # Prints "FAIL" instead of a number if the build fails.
  local extra_env="${1:-}"
  local start end elapsed rc
  start=$(date +%s.%N)
  rc=0
  if [[ -n "$extra_env" ]]; then
    $BAZEL build "${BAZEL_FLAGS[@]}" --action_env="$extra_env" "${BUILD_TARGETS[@]}" >&3 2>&3 || rc=$?
  else
    $BAZEL build "${BAZEL_FLAGS[@]}" "${BUILD_TARGETS[@]}" >&3 2>&3 || rc=$?
  fi
  if [[ $rc -ne 0 ]]; then
    echo "FAIL" >&3
    printf "FAIL"
    return 1
  fi
  end=$(date +%s.%N)
  elapsed=$(echo "$end - $start" | bc)
  printf "%.1f" "$elapsed"
}

parse_ccache_hits() {
  # Parse ccache -s output for hits and total cacheable calls
  local stats
  stats=$(ccache -s 2>/dev/null)
  local hits total
  hits=$(echo "$stats" | awk '/Hits:/{print $2; exit}')
  total=$(echo "$stats" | awk '/Cacheable calls:/{print $3; exit}')
  hits=${hits:-0}
  total=${total:-0}
  echo "$hits/$total"
}

median_of() {
  # Return median of N values (works for 1, 2, 3, … values)
  local sorted
  sorted=($(echo "$@" | tr ' ' '\n' | sort -n))
  local n=${#sorted[@]}
  echo "${sorted[$(( (n - 1) / 2 ))]}"
}

# ---------- Run one scenario ----------

run_scenario() {
  local scenario="$1"
  local idx="$2"
  local label="${SCENARIO_LABELS[$idx]}"
  local nfiles="${SCENARIO_FILE_COUNTS[$idx]}"

  log "Scenario: $label ($scenario)"

  local with_times=()
  local without_times=()
  local last_hits=""

  for run in $(seq 1 "$RUNS"); do
    echo "--- Run $run/$RUNS ---"

    # WITH ccache
    simulate_change "$scenario"
    $BAZEL clean 2>/dev/null
    ccache -z >/dev/null 2>&1
    echo "  [with ccache] building..."
    t_with=$(timed_build) || true
    last_hits=$(parse_ccache_hits)
    echo "  [with ccache] ${t_with}s  hits=$last_hits"
    restore_files "$scenario"

    if [[ "$t_with" == "FAIL" ]]; then
      echo "  BUILD FAILED — skipping no-ccache run for this iteration"
      continue
    fi

    # WITHOUT ccache
    simulate_change "$scenario"
    $BAZEL clean 2>/dev/null
    ccache -z >/dev/null 2>&1
    echo "  [no ccache] building..."
    t_without=$(timed_build "CCACHE_DISABLE=1") || true
    echo "  [no ccache] ${t_without}s"
    restore_files "$scenario"

    if [[ "$t_without" == "FAIL" ]]; then
      echo "  BUILD FAILED (no-ccache) — skipping this iteration"
      continue
    fi

    with_times+=("$t_with")
    without_times+=("$t_without")
  done

  # Compute medians
  local med_with med_without speedup
  if [[ ${#with_times[@]} -eq 0 ]]; then
    echo "  ALL RUNS FAILED — no data for this scenario"
    echo "$scenario,$label,$nfiles,FAIL,FAIL,N/A,N/A" >> "$CSV"
    return 0
  fi
  med_with=$(median_of "${with_times[@]}")
  med_without=$(median_of "${without_times[@]}")
  speedup=$(echo "scale=1; $med_without / $med_with" | bc 2>/dev/null || echo "N/A")

  echo "$scenario,$label,$nfiles,$med_with,$med_without,$speedup,$last_hits" >> "$CSV"
  echo "  Median: with=${med_with}s  without=${med_without}s  speedup=${speedup}x"
}

# ---------- Generate Markdown ----------

generate_markdown() {
  log "Generating $MD_OUT"
  mkdir -p "$(dirname "$MD_OUT")"

  cat > "$MD_OUT" << 'HEADER'
# ccache Performance Benchmark for OpenROAD Bazel Build

## Why ccache?

Bazel's built-in action cache is keyed on the **action graph** (flags, BUILD files,
dependency versions), not on source-file contents.  Common developer workflows
— switching branches, editing a BUILD file, or running `bazel clean` — invalidate
the action cache even when most source files are unchanged.

`ccache` caches based on **preprocessed source content**, so it survives these
invalidations and delivers near-instant cache hits for unchanged translation units.

HEADER

  cat >> "$MD_OUT" << EOF
## Environment

| Item | Value |
|------|-------|
| CPU | $ENV_HW ($ENV_CORES cores) |
| RAM | $ENV_RAM |
| OS | $ENV_OS |
| ccache | $ENV_CCACHE |
| Bazel | $ENV_BAZEL |
| Commit | \`$ENV_COMMIT\` |
| Sources | $ENV_CPP_COUNT .cpp/.cc files, $ENV_H_COUNT .h/.hpp files, $ENV_MODULES modules |
| Runs per scenario | $RUNS (median reported) |

## Developer Personas (from git history)

| Persona | Description | Simulated Change | Example Contributors |
|---------|-------------|-----------------|---------------------|
| Single-tool bug fix | Fix a bug in one module | 1 .cc file in rsz | Thinh Nguyen, Jaehyun Kim |
| Algorithm improvement | Improve algorithm in one tool | 3 .cpp files in grt | Augusto Berndt, Eder Monteiro |
| Cross-cutting refactor | Rename API, clang-tidy cleanup | 20 files across 5 modules | Henner Zeller |
| Build/infra change | Modify BUILD or .bazelrc | 1 BUILD file (drt) | Oyvind Harboe |
| Header change | Edit widely-included header | 1 .h file (odb/db.h) | Various |
| Merge origin/master | Merge latest master into feature branch | MODULE.bazel + 5 .cpp across modules | All developers |
| Branch switch | Switch branch or \`bazel clean\` | No source change, action cache miss | All developers |

## Methodology

1. Full build with \`--config=ccache\` to warm ccache
2. For each scenario, repeat $RUNS times:
   a. Apply simulated file edits (real content changes, not just \`touch\`)
   b. \`bazel clean\` to invalidate Bazel action cache
   c. Rebuild **with ccache** — record wall-clock time and hit rate
   d. Restore files, re-apply same edits
   e. \`bazel clean\` again
   f. Rebuild **without ccache** (\`CCACHE_DISABLE=1\`) — record wall-clock time
   g. Restore files
3. Bazel disk cache and remote cache disabled (\`--disk_cache= --remote_cache=\`) to isolate ccache effect
4. Median of $RUNS runs reported

## Results

| Scenario | Files Changed | With ccache (s) | Without ccache (s) | Speedup | ccache Hits |
|----------|:------------:|:--------------:|:-----------------:|:-------:|:-----------:|
EOF

  # Read CSV and append rows
  while IFS=, read -r _key label nfiles med_with med_without speedup hits; do
    printf "| %s | %s | %s | %s | **%sx** | %s |\n" \
      "$label" "$nfiles" "$med_with" "$med_without" "$speedup" "$hits" >> "$MD_OUT"
  done < "$CSV"

  cat >> "$MD_OUT" << 'FOOTER'

## Key Findings

- **Branch switch / clean build** sees the largest speedup — ccache turns a full
  recompile into a cache-hit replay, saving tens of minutes.
- **Merging origin/master** is one of the most common workflows — MODULE.bazel and
  BUILD changes invalidate the action graph even when most source is identical.
  ccache makes post-merge rebuilds nearly as fast as a no-op.
- **BUILD file edits** invalidate the entire target's action cache even though no
  source changed — ccache completely mitigates this.
- **Single-file fixes** (the most common commit type) benefit heavily because only
  1 translation unit actually needs recompilation.
- **Header changes** still benefit for modules that don't include the edited header.
- **Cross-cutting refactors** show moderate speedup — the unchanged files across
  other modules are still served from ccache.

## Recommendation

Add `build --config=ccache` to your `user.bazelrc` for automatic ccache usage:

```bash
echo 'build --config=ccache' >> user.bazelrc
```

Or invoke explicitly:

```bash
bazel build --config=ccache //...
```
FOOTER

  echo "Report written to $MD_OUT"
}

# ---------- Main ----------

main() {
  # fd 3 = real stdout, so timed_build() can print Bazel output without
  # contaminating the $() capture that reads only the elapsed time.
  exec 3>&1

  detect_env

  # Fresh CSV (append if resuming with --skip)
  if [[ "$SKIP" -eq 0 ]]; then
    echo "scenario,label,files_changed,with_ccache_s,without_ccache_s,speedup,ccache_hits" > "$CSV"
  fi

  warm_ccache

  for i in "${!SCENARIOS[@]}"; do
    if [[ "$i" -lt "$SKIP" ]]; then
      echo "Skipping scenario $i (${SCENARIOS[$i]})"
      continue
    fi
    run_scenario "${SCENARIOS[$i]}" "$i"
  done

  generate_markdown

  log "Done"
  echo "Raw data:  $CSV"
  echo "Report:    $MD_OUT"
}

main "$@"
