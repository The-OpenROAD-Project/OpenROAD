#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Auto-format all TCL files in-place.

set -euo pipefail

TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GIT="$(realpath "$2")"
GIT_LS_FILES="$(realpath "bazel/git_ls_files.sh")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

"${GIT_LS_FILES}" "${GIT}" '*.tcl' '*.sdc' '*.upf' -z | xargs -0 "$TOOL" --in-place
