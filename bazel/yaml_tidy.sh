#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Auto-format YAML files in-place using yamlfix.
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GIT="$(realpath "$2")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

OPTIONS=()
if [ -f "yamlfix.toml" ]; then
    OPTIONS+=("-c" "yamlfix.toml")
fi

"${GIT}" ls-files '*.yaml' '*.yml' -z | xargs -0 "$TOOL" "${OPTIONS[@]}"
