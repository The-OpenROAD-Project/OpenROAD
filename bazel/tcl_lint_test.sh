#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Lint all TCL files using tclint.

set -euo pipefail

TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GIT="$(realpath "$2")"

# With rules_python's script bootstrap (bootstrap_impl=script), a py_binary
# invoked via its realpath cannot locate its runfiles once we cd away.
# Export the runfiles root (the .runfiles directory) so nested tools
# resolve it from the environment instead. We start in the workspace
# subdirectory of it (<target>.runfiles/<workspace>); strip the last path
# component rather than a hardcoded workspace name so this also holds when
# the workspace is not "_main" (e.g. consumed as a dependency).
export RUNFILES_DIR="${RUNFILES_DIR:-${PWD%/*}}"

WORKSPACE="$(dirname "$(readlink -f tclint.toml)")"
cd "$WORKSPACE"

"${GIT}" ls-files '*.tcl' '*.sdc' '*.upf' -z | xargs -0 "${TOOL}"
