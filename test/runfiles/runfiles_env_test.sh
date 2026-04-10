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
# Because this test is itself a Bazel sh_test, its RUNFILES_DIR points
# to this test's runfiles — NOT OpenROAD's.  Running OpenROAD here
# reproduces the cross-binary runfiles leak.

set -euo pipefail

OPENROAD="$1"

# Verify precondition: RUNFILES_DIR is set by the Bazel test runner,
# pointing to *this test's* runfiles tree (not OpenROAD's).
if [[ -z "${RUNFILES_DIR:-}" ]]; then
    echo "SKIP: RUNFILES_DIR not set — not running under Bazel"
    exit 0
fi
echo "RUNFILES_DIR=$RUNFILES_DIR"

# Run OpenROAD -version.  This exercises tcl_library_init.cc:
# if RUNFILES_DIR is not cleared, Runfiles::Create() looks for
# tcl_resources_dir inside this test's runfiles and fails.
output=$("$OPENROAD" -version 2>&1) || {
    echo "FAIL: openroad -version exited with $?"
    echo "$output"
    exit 1
}
echo "$output"
echo "PASS: OpenROAD started successfully despite inherited RUNFILES_DIR"
