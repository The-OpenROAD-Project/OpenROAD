#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Check that all C/C++ files are properly formatted.

set -euo pipefail

TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GIT="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
GIT_LS_FILES="$(realpath "bazel/git_ls_files.sh")"
CC_LS_FILES="$(realpath "bazel/cc_ls_files.sh")"

# With rules_python's script bootstrap (bootstrap_impl=script), a py_binary
# invoked via its realpath cannot locate its runfiles once we cd away.
export RUNFILES_DIR="${RUNFILES_DIR:-${PWD%/*}}"

# MODULE.bazel must be in the sh_test `data` deps so it appears as a
# runfiles symlink pointing at the real workspace. `readlink` (no -f,
# for macOS portability) resolves the absolute path Bazel wrote.
[ -L MODULE.bazel ] || { echo "MODULE.bazel missing from runfiles" >&2; exit 1; }
WORKSPACE="$(dirname "$(readlink MODULE.bazel)")"
cd "$WORKSPACE"

# --fallback-style=none leaves files without a .clang-format alone.
"${CC_LS_FILES}" "${GIT_LS_FILES}" "${GIT}" \
    | xargs -0 -r -n 50 -P "$(getconf _NPROCESSORS_ONLN)" \
        "${TOOL}" --fallback-style=none --dry-run --Werror
