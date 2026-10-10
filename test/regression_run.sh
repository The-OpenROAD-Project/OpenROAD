#!/usr/bin/env bash

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2026, The OpenROAD Authors

# Build-action driver for regression_run (test/golden.bzl). Runs one
# OpenROAD Tcl regression and always exits 0 so the outputs are cached
# even when openroad fails; the exit code is recorded for the test.
#
# Usage: regression_run.sh <coreutils> <timeout> <openroad> <test_file>
#                          <results_dir> <log> <exit_code> <manifest>
#                          [<result>...]
#
# <coreutils> is the hermetic multi-call binary from bazel_lib, so timeout
# and sha256sum need not be installed on the host (macOS has neither).
# Bazel has no timeout for build actions, so openroad is run under
# timeout; a timed-out run records exit code 124.
#
# A <result> ending in .sha256 is written as the digest of the file without
# that suffix, which is then deleted so only the digest is cached.

set -euo pipefail

abs() { echo "$(pwd)/$1"; }

coreutils=$(abs "$1")
run_timeout=$2
openroad=$(abs "$3")
test_file=$4
export RESULTS_DIR=$(abs "$5")
log=$(abs "$6")
exit_code=$(abs "$7")
export GOLDEN_MANIFEST=$(abs "$8")
shift 8
results=()
for r in "$@"; do
    results+=("$(abs "$r")")
done

mkdir -p "$RESULTS_DIR"
: > "$GOLDEN_MANIFEST"

cd "$(dirname "$test_file")"
set +e
"$coreutils" timeout "$run_timeout" \
    "$openroad" -no_splash -no_init -exit -threads 1 "$(basename "$test_file")" \
    > "$log" 2>&1
echo $? > "$exit_code"
set -e

# Declared side outputs must exist even if the run failed early; the test
# reports the exit code and the (empty) result mismatch.
for r in "${results[@]}"; do
    if [[ "$r" == *.sha256 ]]; then
        full=${r%.sha256}
        [ -e "$full" ] || : > "$full"
        "$coreutils" sha256sum < "$full" | "$coreutils" cut -d' ' -f1 > "$r"
        rm -f "$full"
    else
        [ -e "$r" ] || : > "$r"
    fi
done
exit 0
