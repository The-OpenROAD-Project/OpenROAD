#!/usr/bin/env bash
# Wrapper for sh_test targets that run the openroad binary against a Tcl
# repro using workspace-root-relative paths.
#
# Bazel sh_test cwd is the runfiles dir, but the test data (asap7 lib
# files, the .sv RTL) is laid out under the runfiles workspace root
# (TEST_SRCDIR/TEST_WORKSPACE), not the runfiles dir. cd-ing there before
# invoking openroad lets the Tcl use "test/asap7/..." style paths.
#
# args:
#   $1 - rootpath to the openroad binary
#   $2 - rootpath to the Tcl script
set -eu
OPENROAD_BIN="$1"
TCL="$2"
WORKSPACE_ROOT="${TEST_SRCDIR}/${TEST_WORKSPACE:-_main}"
exec "${WORKSPACE_ROOT}/${OPENROAD_BIN}" -no_splash -no_init -exit \
    "${WORKSPACE_ROOT}/${TCL}"
