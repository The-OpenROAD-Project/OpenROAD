#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Auto-fix then lint. Delegates to per-language tidy/lint scripts so
# that file-discovery logic is not duplicated (DRY).
set -euo pipefail

export BUILD_WORKSPACE_DIRECTORY="${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

# TCL: auto-format then lint
"$1" "$2"
"$3" "$4" || rc=$?

# Bazel: auto-format then lint
"$5" "$6"
"$7" "$8" || rc=$?

if command -v git >/dev/null 2>&1; then
    git -C "$BUILD_WORKSPACE_DIRECTORY" status
else
    echo "git not found; skipping workspace status report." >&2
fi

if [ "${rc:-0}" -ne 0 ]; then
    echo "Error: lint violations remain that require manual fixes." >&2
fi
exit "${rc:-0}"
