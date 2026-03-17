#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Auto-format all TCL files in-place.
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"
git ls-files '*.tcl' -z | xargs -0 "$TOOL" --in-place
