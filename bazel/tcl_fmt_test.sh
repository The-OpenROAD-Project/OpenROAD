#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Check that all TCL files are properly formatted.

set -euo pipefail

TOOL="$(realpath "$1")"
GIT="$(realpath "$2")"

WORKSPACE="$(dirname "$(readlink -f tclint.toml)")"
cd "$WORKSPACE"

"bazel/git_ls_files.sh" "${GIT}" '*.tcl' '*.sdc' '*.upf' -z | xargs -0 "${TOOL}" --check
