#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Check that all Bazel files are properly formatted.

set -euo pipefail

TOOL="$(realpath "$1")"
GIT="$(realpath "$2")"
GIT_LS_FILES="$(realpath "bazel/git_ls_files.sh")"

export RUNFILES_DIR="${RUNFILES_DIR:-${PWD%/*}}"

[ -L MODULE.bazel ] || { echo "MODULE.bazel missing from runfiles" >&2; exit 1; }
WORKSPACE="$(dirname "$(readlink MODULE.bazel)")"
cd "$WORKSPACE"

"${GIT_LS_FILES}" "${GIT}" \
        '*.bazel' '*.bzl' '**/BUILD' 'BUILD' '**/WORKSPACE' 'WORKSPACE' -z \
    | xargs -0 "${TOOL}" -mode=check -lint=off
