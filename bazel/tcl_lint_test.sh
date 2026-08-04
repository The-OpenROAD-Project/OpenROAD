#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Lint all TCL files using tclint.

set -euo pipefail

TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GIT="$(realpath "$2")"

export RUNFILES_DIR="${RUNFILES_DIR:-${PWD%/*}}"

WORKSPACE="$(dirname "$(readlink -f tclint.toml)")"
cd "$WORKSPACE"

"bazel/git_ls_files.sh" "${GIT}" '*.tcl' '*.sdc' '*.upf' -z | xargs -0 "${TOOL}"
