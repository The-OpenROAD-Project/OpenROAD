#!/bin/bash
set -eu
cd "$(dirname "$0")/../.."

LOGDIR=$(mktemp -d)
BENCH_DISK_CACHE=$(mktemp -d)
RESULTS="test/orfs/bench_results.md"

# BUILD files that contain orfs_flow/orfs_sweep arguments dicts
BUILD_FILES=(
    test/orfs/gcd/BUILD
    test/orfs/ram_8x7/BUILD
    test/orfs/mock-array/mock-array.bzl
)

monitor_load() {
    local max=0
    while true; do
        local load
        load=$(awk '{print $1}' /proc/loadavg)
        echo "$(date +%s) $load" >> "$1"
        if awk "BEGIN{exit !($load > $max)}"; then max=$load; fi
        echo "$max" > "${1%.log}_max.txt"
        sleep 1
    done
}

inject_or_args() {
    # Add OR_ARGS to every arguments dict in BUILD files
    for f in "${BUILD_FILES[@]}"; do
        sed -i 's/\("OPENROAD_HIERARCHICAL":.*\)/\1\n            "OR_ARGS": "-disable_throttle",/' "$f"
    done
}

restore_build_files() {
    git checkout -- "${BUILD_FILES[@]}"
}

run_bench() {
    local label="$1"
    echo "=== $label ==="

    bazelisk clean

    monitor_load "$LOGDIR/${label}_load.log" &
    local pid=$!

    local start
    start=$(date +%s)
    bazelisk test test/orfs/... \
        --build_tag_filters= \
        --test_output=errors \
        --disk_cache="$BENCH_DISK_CACHE" \
        2>&1 | tee "$LOGDIR/${label}_bazel.log"
    local end
    end=$(date +%s)

    kill $pid 2>/dev/null
    wait $pid 2>/dev/null || true

    echo "$((end - start))" > "$LOGDIR/${label}_seconds.txt"
    echo "=== $label: $((end - start))s ==="
}

# Run A: with throttle (default)
run_bench "throttled"

# Run B: without throttle (patch BUILD files)
inject_or_args
trap restore_build_files EXIT
run_bench "no_throttle"
restore_build_files
trap - EXIT

cat > "$RESULTS" <<EOF
# Throttle A/B Test Results

Date: $(date -Iseconds)
Host: $(hostname)
CPUs: $(nproc)

| Metric | Throttled | No Throttle |
|--------|-----------|-------------|
| Total time (s) | $(cat "$LOGDIR/throttled_seconds.txt") | $(cat "$LOGDIR/no_throttle_seconds.txt") |
| Max system load | $(cat "$LOGDIR/throttled_load_max.txt") | $(cat "$LOGDIR/no_throttle_load_max.txt") |
EOF

rm -rf "$LOGDIR" "$BENCH_DISK_CACHE"
echo ""
cat "$RESULTS"
