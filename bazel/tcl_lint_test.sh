#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Lint all TCL files using tclint.
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
WORKSPACE="$(dirname "$(readlink -f tclint.toml)")"
cd "$WORKSPACE"
git ls-files '*.tcl' '*.sdc' '*.upf' -z | xargs -0 "$TOOL"
