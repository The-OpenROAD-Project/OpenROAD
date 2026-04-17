#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Auto-format all Bazel files in-place using buildifier.
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"
# `git ls-files` skips submodule contents (src/sta, third-party/abc),
# so we never rewrite files owned by another repo.
git ls-files '*.bazel' '*.bzl' '**/BUILD' 'BUILD' '**/WORKSPACE' 'WORKSPACE' -z \
    | xargs -0 "$TOOL" -mode=fix -lint=fix
