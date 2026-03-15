#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Auto-format all TCL files in-place.
set -euo pipefail
TOOL="$(readlink -f "$1")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"
exec "$TOOL" --in-place .
