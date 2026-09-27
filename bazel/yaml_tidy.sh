#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Auto-format YAML files in-place using yamlfix.
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GIT="$(realpath "$2")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

is_blacklisted() {
    local target="$1"
    if [ -f "yamlfix.ignore" ]; then
        grep -qxF "$target" "yamlfix.ignore" 2>/dev/null && return 0
    fi
    return 1
}

OPTIONS=()
if [ -f "yamlfix.toml" ]; then
    OPTIONS+=("-c" "yamlfix.toml")
fi

FILES=()
while IFS= read -r -d '' file; do
    if ! is_blacklisted "$file"; then
        FILES+=("$file")
    fi
done < <("${GIT}" ls-files '*.yaml' '*.yml' -z)

if [ "${#FILES[@]}" -gt 0 ]; then
    "$TOOL" "${OPTIONS[@]}" "${FILES[@]}"
fi
