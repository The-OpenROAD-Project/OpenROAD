#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Check that all Bazel files are properly formatted (no lint warnings).
set -euo pipefail
TOOL="$(readlink -f "$1")"
WORKSPACE="$(dirname "$(readlink -f MODULE.bazel)")"
cd "$WORKSPACE"
# `git ls-files` skips submodule contents (src/sta, third-party/abc).
# Explicit -mode=check -lint=off separates format errors from lint warnings,
# and overrides the repo-root .buildifier.json default (mode: fix).
git ls-files '*.bazel' '*.bzl' '**/BUILD' 'BUILD' '**/WORKSPACE' 'WORKSPACE' -z \
    | xargs -0 "$TOOL" -mode=check -lint=off
