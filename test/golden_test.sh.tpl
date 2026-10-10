#!/usr/bin/env bash

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2026, The OpenROAD Authors

# Template for golden_test (test/golden.bzl): checks a cached regression_run
# against the checked-in goldens. Does not run openroad.

set -uo pipefail

exit_code_file="@EXIT_CODE@"
expected_exit_code="@EXPECTED_EXIT_CODE@"
run_timeout="@RUN_TIMEOUT@"
golden_log="@GOLDEN_LOG@"
log="@LOG@"
manifest="@MANIFEST@"
pairs=(@PAIRS@)
recorded=" @RECORDED@ "
debug_dir="@DEBUG_DIR@"
debug_target="@DEBUG_TARGET@"
update_target="@UPDATE_TARGET@"

out_dir="${TEST_UNDECLARED_OUTPUTS_DIR:-${TEST_TMPDIR:-/tmp}}"
status=0

# Compare one golden against its result; keep the full diff as an output
# and show the start of it in the test log.
check() {
    local golden=$1 result=$2
    if cmp -s "$golden" "$result"; then
        echo "PASS: $(basename "$golden")"
        return
    fi
    status=1
    if [[ "$golden" == *.sha256 ]]; then
        local full
        full=$(basename "${result%.sha256}")
        echo "FAIL: $(basename "$golden"): digest of ${full} changed"
        echo "  Only the digest is kept. To see the change, run"
        echo "    bazel build ${debug_target}"
        echo "  at the base commit and at yours (it re-runs openroad), copying"
        echo "  ${debug_dir}${full} aside in between, and diff the two."
        return
    fi
    local diff_file
    diff_file="${out_dir}/$(basename "$golden").diff"
    diff "$golden" "$result" > "$diff_file"
    echo "FAIL: $(basename "$golden") differs from $(basename "$result")"
    head -n 20 "$diff_file"
}

actual_exit_code=$(cat "$exit_code_file")
if [ "$actual_exit_code" = 124 ]; then
    echo "FAIL: openroad timed out after ${run_timeout}s (run_timeout)"
    status=1
elif [ "$actual_exit_code" != "$expected_exit_code" ]; then
    echo "FAIL: exit code ${actual_exit_code}, expected ${expected_exit_code}"
    status=1
fi

check "$golden_log" "$log"

for pair in "${pairs[@]}"; do
    check "${pair%%:*}" "${pair#*:}"
done

# Every diff_files call in the script must be backed by a declared golden,
# otherwise the comparison would silently not happen.
while IFS=$'\t' read -r file1 file2 ignore; do
    [ -n "$file1" ] || continue
    if [ -n "$ignore" ]; then
        echo "FAIL: diff_files ignore regex not supported yet: $file1 $file2"
        status=1
    fi
    if [[ "$recorded" != *" $file1:$file2 "* &&
          "$recorded" != *" $file2:$file1 "* ]]; then
        echo "FAIL: diff_files $file1 $file2 has no matching goldens entry"
        status=1
    fi
done < "$manifest"

if [ "$status" -ne 0 ]; then
    echo
    echo "If the new output is correct, accept it with:"
    echo "  bazel run ${update_target}"
fi
exit "$status"
