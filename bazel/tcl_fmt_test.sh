#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# Check that all TCL files are properly formatted.

set -euo pipefail

TOOL="$(realpath "$1")"
GIT="$(realpath "$2")"

WORKSPACE="$(dirname "$(readlink -f tclint.toml)")"
cd "$WORKSPACE"

"${GIT}" ls-files '*.tcl' '*.sdc' '*.upf' -z | xargs -0 "${TOOL}" --check
