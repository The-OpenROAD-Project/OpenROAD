#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Lint all Bazel files using buildifier (check-only, with lint warnings).
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
# MODULE.bazel must be in the sh_test `data` deps so it appears as a
# runfiles symlink; readlink -f then resolves it to the real workspace.
# Without the data dep, readlink -f silently returns a bogus path.
[ -e MODULE.bazel ] || { echo "MODULE.bazel missing from runfiles" >&2; exit 1; }
WORKSPACE="$(cd "$(dirname "$(readlink -f MODULE.bazel)")" && pwd)"
cd "$WORKSPACE"
# `git ls-files` skips submodule contents (src/sta, third-party/abc), so
# we never try to reformat files owned by another repo.
# Explicit -mode=check overrides the repo-root .buildifier.json default.
git ls-files '*.bazel' '*.bzl' '**/BUILD' 'BUILD' '**/WORKSPACE' 'WORKSPACE' -z \
    | xargs -0 "$TOOL" -mode=check -lint=warn
