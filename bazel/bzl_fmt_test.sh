#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Check that all Bazel files are properly formatted (no lint warnings).
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
# MODULE.bazel must be in the sh_test `data` deps so it appears as a
# runfiles symlink pointing at the real workspace. `readlink` (no -f,
# for macOS portability) resolves the absolute path Bazel wrote.
[ -L MODULE.bazel ] || { echo "MODULE.bazel missing from runfiles" >&2; exit 1; }
WORKSPACE="$(dirname "$(readlink MODULE.bazel)")"
cd "$WORKSPACE"
# `git ls-files` skips submodule contents (src/sta, third-party/abc).
# Explicit -mode=check -lint=off separates format errors from lint warnings,
# and overrides the repo-root .buildifier.json default (mode: fix).
git ls-files '*.bazel' '*.bzl' '**/BUILD' 'BUILD' '**/WORKSPACE' 'WORKSPACE' -z \
    | xargs -0 "$TOOL" -mode=check -lint=off
