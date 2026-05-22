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
# The "./" prefix on the binary is needed so exec resolves it relative
# to cwd rather than PATH-searching; the Tcl script is just a file
# argument and goes through openroad's own open(), which handles both
# relative-to-cwd and absolute paths without it.
exec "./${OPENROAD_BIN}" -no_splash -no_init -exit "${TCL}"
