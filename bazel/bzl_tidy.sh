#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Auto-format all Bazel files in-place using buildifier.
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LIST_SOURCES="$SCRIPT_DIR/list_sources.sh"
[ -x "$LIST_SOURCES" ] || LIST_SOURCES="$SCRIPT_DIR/bazel/list_sources.sh"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"
"$LIST_SOURCES" -z '*.bazel' '*.bzl' '**/BUILD' 'BUILD' '**/WORKSPACE' 'WORKSPACE' \
    | xargs -0 "$TOOL" -mode=fix -lint=fix
