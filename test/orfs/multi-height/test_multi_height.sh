#!/usr/bin/env bash
# Regression test for DPL-0400: topological sort cycle in the shift
# legalizer when multi-height cells coexist with single-height cells.
#
# improve_placement must complete without DPL-0400 or bad_optional_access.
set -euo pipefail

OPENROAD="$(readlink -f "$1")"
export MULTI_HEIGHT_LEF="$(readlink -f "$2")"
export MULTI_HEIGHT_DEF="$(readlink -f "$3")"
TCL_SCRIPT="$(readlink -f "$4")"

"$OPENROAD" -exit "$TCL_SCRIPT" 2>&1 | tee "$TEST_TMPDIR/output.log"

if grep -q "DPL-0400\|bad_optional_access\|Cells incorrectly ordered" "$TEST_TMPDIR/output.log"; then
    echo "FAIL: DPL-0400 or related error detected"
    exit 1
fi
