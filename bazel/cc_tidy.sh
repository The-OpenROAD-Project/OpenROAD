#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Auto-format all C/C++ files in-place.

set -euo pipefail

TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GIT="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
GIT_LS_FILES="$(realpath "bazel/git_ls_files.sh")"
CC_LS_FILES="$(realpath "bazel/cc_ls_files.sh")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

# --fallback-style=none leaves files without a .clang-format alone, so a
# downstream workspace running //:fix_lint keeps its own C/C++ style.
"${CC_LS_FILES}" "${GIT_LS_FILES}" "${GIT}" \
    | xargs -0 -r -n 50 -P "$(getconf _NPROCESSORS_ONLN)" \
        "${TOOL}" --fallback-style=none -i
