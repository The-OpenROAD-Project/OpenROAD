#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Auto-format all Python files in-place.

set -euo pipefail

TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GIT="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
GIT_LS_FILES="$(realpath "bazel/git_ls_files.sh")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

# Exclusions (third-party) come from [tool.black] in pyproject.toml.
"${GIT_LS_FILES}" "${GIT}" '*.py' -z | xargs -0 "$TOOL"
