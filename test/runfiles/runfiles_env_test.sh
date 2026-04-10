#!/usr/bin/env bash
# Test that OpenROAD works when invoked from a Bazel-built process
# that has RUNFILES_DIR / RUNFILES_MANIFEST_FILE set in the environment.
#
# This is the scenario that happens when a Bazel py_binary (e.g. a
# Python wrapper) runs OpenROAD as a subprocess.  The inherited
# RUNFILES_* variables point to the wrapper's runfiles tree, not
# OpenROAD's.  Without the unsetenv() fix in tcl_library_init.cc,
# Runfiles::Create() resolves paths in the wrong tree and OpenROAD
# fails to find its TCL library.
#
# We simulate this by pointing RUNFILES_DIR at an empty directory
# before invoking OpenROAD.

set -euo pipefail

OPENROAD="$1"

# Simulate cross-binary env leak: point RUNFILES_DIR at an empty
# directory that does NOT contain OpenROAD's tcl resources.
FAKE_RUNFILES=$(mktemp -d)
trap 'rm -rf "$FAKE_RUNFILES"' EXIT
export RUNFILES_DIR="$FAKE_RUNFILES"
export RUNFILES_MANIFEST_FILE=""
echo "Fake RUNFILES_DIR=$RUNFILES_DIR"

# Run OpenROAD with a real Tcl command (not -version, which exits
# before Tcl init).  Capture both stdout and stderr.
output=$("$OPENROAD" -no_splash -exit <<< 'puts "TCL_OK"' 2>&1) || {
    echo "FAIL: openroad exited with $?"
    echo "$output"
    exit 1
}

# Check for Tcl initialization failure
if echo "$output" | grep -q "application-specific initialization failed"; then
    echo "FAIL: Tcl library initialization failed despite exe-path fallback"
    echo "$output"
    exit 1
fi

echo "$output"
echo "PASS: OpenROAD initialized Tcl correctly despite misleading RUNFILES_DIR"
