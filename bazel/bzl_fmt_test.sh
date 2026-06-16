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
# `-c submodule.recurse=false` keeps git ls-files from descending into
# submodules (src/sta, third-party/abc, third-party/slang-elab and the
# fmt sub-submodule nested in it) — we never want to reformat files
# owned by another repo. The override is needed because CI sets
# submodule.recurse=true globally.
# Explicit -mode=check -lint=off separates format errors from lint warnings,
# and overrides the repo-root .buildifier.json default (mode: fix).
git -c submodule.recurse=false ls-files \
        '*.bazel' '*.bzl' '**/BUILD' 'BUILD' '**/WORKSPACE' 'WORKSPACE' -z \
    | xargs -0 "$TOOL" -mode=check -lint=off
