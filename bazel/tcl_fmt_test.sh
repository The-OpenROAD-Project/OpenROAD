#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Check that all TCL files are properly formatted.
set -euo pipefail
TOOL="$(readlink -f "$1")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"
exec "$TOOL" --check .
