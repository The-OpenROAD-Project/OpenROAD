#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Lint all Starlark files using buildifier.
set -euo pipefail
TOOL="$(readlink -f "$1")"
WORKSPACE="$(dirname "$(readlink -f BUILD.bazel)")"
cd "$WORKSPACE"
git ls-files -- '*.bzl' 'BUILD' 'BUILD.bazel' 'MODULE.bazel' 'WORKSPACE' 'WORKSPACE.bazel' -z | xargs -0 "$TOOL" -mode=check -lint=warn
