#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Auto-fix then lint. Delegates to tcl_tidy.sh and tcl_lint_test.sh
# so that file-discovery logic is not duplicated (DRY).
set -euo pipefail

export BUILD_WORKSPACE_DIRECTORY="${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

# TCL: auto-format then lint
"$1" "$2"
"$3" "$4" || rc=$?

git -C "$BUILD_WORKSPACE_DIRECTORY" status

if [ "${rc:-0}" -ne 0 ]; then
    echo "Error: lint violations remain that require manual fixes." >&2
fi
exit "${rc:-0}"
