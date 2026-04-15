#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Lint all Bazel files using buildifier (check-only, with lint warnings).
set -euo pipefail
TOOL="$(readlink -f "$1")"
WORKSPACE="$(dirname "$(readlink -f MODULE.bazel)")"
cd "$WORKSPACE"
# `git ls-files` skips submodule contents (src/sta, third-party/abc), so
# we never try to reformat files owned by another repo.
# Explicit -mode=check overrides the repo-root .buildifier.json default.
git ls-files '*.bazel' '*.bzl' '**/BUILD' 'BUILD' '**/WORKSPACE' 'WORKSPACE' -z \
    | xargs -0 "$TOOL" -mode=check -lint=warn
