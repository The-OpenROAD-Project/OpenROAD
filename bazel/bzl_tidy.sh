#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Fix all Starlark files using buildifier.
set -euo pipefail
TOOL="$(readlink -f "$1")"
if [ -z "$BUILD_WORKSPACE_DIRECTORY" ]; then
  echo "Error: This script must be run via 'bazelisk run'"
  exit 1
fi
cd "$BUILD_WORKSPACE_DIRECTORY"
git ls-files -- '*.bzl' 'BUILD' 'BUILD.bazel' 'MODULE.bazel' 'WORKSPACE' 'WORKSPACE.bazel' -z | xargs -0 "$TOOL" -mode=fix -lint=fix
echo "Buildifier tidy complete!"
