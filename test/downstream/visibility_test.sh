#!/bin/bash
# Test that OpenROAD's public API and dev_dependency isolation work
# correctly when consumed as a dependency from a downstream project.
set -e -u -o pipefail

cd "$(dirname "$(readlink -f "$0")")"

# Bazel test environments may not set HOME; bazelisk needs it.
export HOME="${HOME:-$(getent passwd "$(id -u)" | cut -d: -f6)}"

PASS=0
FAIL=0

expect_pass() {
    local desc="$1"; shift
    if "$@" >/dev/null 2>&1; then
        echo "  PASS: $desc"
        PASS=$((PASS + 1))
    else
        echo "  FAIL: $desc"
        FAIL=$((FAIL + 1))
    fi
}

expect_fail() {
    local desc="$1"; shift
    if "$@" >/dev/null 2>&1; then
        echo "  FAIL: $desc (should have failed)"
        FAIL=$((FAIL + 1))
    else
        echo "  PASS: $desc"
        PASS=$((PASS + 1))
    fi
}

echo "=== Public targets (should be visible) ==="
expect_pass "//:openroad is visible" \
    bazelisk build --nobuild :openroad_visible
expect_pass "//:openroad_py is visible" \
    bazelisk build --nobuild :openroad_py_test

echo ""
echo "=== Internal targets (should NOT be visible) ==="
expect_fail "//:openroad_lib is restricted" \
    bazelisk build --nobuild :openroad_lib_consumer
expect_fail "//:_openroadpy.so is restricted" \
    bazelisk build --nobuild :openroadpy_so_consumer

echo ""
echo "=== Dev dependencies (should NOT be available) ==="
expect_fail "@rules_shell is not a dep" \
    bazelisk query @rules_shell//:all
expect_fail "@rules_pkg is not a dep" \
    bazelisk query @rules_pkg//:all
expect_fail "@rules_verilator is not a dep" \
    bazelisk query @rules_verilator//:all

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
