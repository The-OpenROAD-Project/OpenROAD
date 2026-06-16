#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Auto-format all Bazel files in-place using buildifier.
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"
# `-c submodule.recurse=false` keeps git ls-files from descending into
# submodules (src/sta, third-party/abc, third-party/slang-elab and the
# fmt sub-submodule nested in it) — we never want to rewrite files
# owned by another repo. The override is needed because CI sets
# submodule.recurse=true globally.
git -c submodule.recurse=false ls-files \
        '*.bazel' '*.bzl' '**/BUILD' 'BUILD' '**/WORKSPACE' 'WORKSPACE' -z \
    | xargs -0 "$TOOL" -mode=fix -lint=fix
