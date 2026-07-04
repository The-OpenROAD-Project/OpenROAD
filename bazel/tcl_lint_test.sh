#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Lint all TCL files using tclint.

set -euo pipefail

TOOL="$(realpath "$1")"
GIT="$(realpath "$2")"

# With rules_python's script bootstrap (bootstrap_impl=script), a py_binary
# invoked via its realpath cannot locate its runfiles once we cd away.
# Export the runfiles root we start in (<target>.runfiles/_main) so nested
# tools resolve it from the environment instead.
export RUNFILES_DIR="${RUNFILES_DIR:-${PWD%/_main}}"

WORKSPACE="$(dirname "$(readlink -f tclint.toml)")"
cd "$WORKSPACE"

"${GIT}" ls-files '*.tcl' '*.sdc' '*.upf' -z | xargs -0 "${TOOL}"
