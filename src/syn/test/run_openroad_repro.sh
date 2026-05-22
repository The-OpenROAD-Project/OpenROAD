#!/usr/bin/env bash
# Wrapper for sh_test targets that run the openroad binary against a Tcl
# repro.
#
# Bazel's sh_test sets cwd to the runfiles workspace root before
# invoking this script, so test data (asap7 lib files, the .sv RTL,
# the openroad binary itself) is reachable via "./" relative paths
# without any explicit cd or workspace-name detection.
#
# args:
#   $1 - rootpath to the openroad binary
#   $2 - rootpath to the Tcl script
set -euo pipefail
OPENROAD_BIN="$1"
TCL="$2"
exec "./${OPENROAD_BIN}" -no_splash -no_init -exit "./${TCL}"
