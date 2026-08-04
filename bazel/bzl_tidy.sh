#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Auto-format all Bazel files in-place.

set -euo pipefail

TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GIT="$(realpath "$2")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

"bazel/git_ls_files.sh" "${GIT}" \
        '*.bazel' '*.bzl' '**/BUILD' 'BUILD' '**/WORKSPACE' 'WORKSPACE' -z \
    | xargs -0 "$TOOL" -mode=fix -lint=fix
