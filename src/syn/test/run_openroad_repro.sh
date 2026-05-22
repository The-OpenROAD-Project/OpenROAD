#!/usr/bin/env bash
# Wrapper for sh_test targets that run the openroad binary against a Tcl
# repro using workspace-root-relative paths.
#
# Test data (asap7 lib files, the .sv RTL) lives under the runfiles
# workspace root (TEST_SRCDIR/TEST_WORKSPACE). cd-ing there before
# invoking openroad lets the Tcl use "test/asap7/..." and
# "src/syn/test/..." style paths.
#
# args:
#   $1 - rootpath to the openroad binary
#   $2 - rootpath to the Tcl script
set -eu
OPENROAD_BIN="$1"
TCL="$2"
cd "${TEST_SRCDIR}/${TEST_WORKSPACE:-_main}"
exec "./${OPENROAD_BIN}" -no_splash -no_init -exit "./${TCL}"
