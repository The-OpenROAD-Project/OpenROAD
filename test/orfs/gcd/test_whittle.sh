#!/usr/bin/env bash
#
# Whittle integration smoke-test for bazel.
# Adapted from flow/test/test_whittle.sh.
#
# Runs whittle.py against the gcd floorplan .odb, using
# global placement as the step command, and verifies that
# whittle can reduce the design while the error (reaching
# 100 placement iterations) is still reproducible.
#
set -eu -o pipefail

OPENROAD="$(readlink -f "$1")"
WHITTLE_PY="$(readlink -f "$2")"
BASE_ODB="$(readlink -f "$3")"

WORK="$TEST_TMPDIR"

# Copy the floorplan .odb to a writable location
cp "$BASE_ODB" "$WORK/test.odb"

# Create a placement TCL step script.
# global_placement -skip_io matches the ORFS do-3_1_place_gp_skip_io target.
cat > "$WORK/place.tcl" << EOF
read_db $WORK/test.odb
global_placement -density 0.35 -skip_io
EOF

export OPENROAD_EXE="$OPENROAD"
python3 "$WHITTLE_PY" \
    --persistence 2 \
    --use_stdout \
    --error_string "      100 | " \
    --timeout 120 \
    --base_db_path "$WORK/test.odb" \
    --step "$OPENROAD -exit $WORK/place.tcl"
