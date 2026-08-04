#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Check YAML formatting with yamlfix.
set -euo pipefail
TOOL="$(realpath "$1")"
GIT="$(realpath "$2")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

OPTIONS=()
if [ -f "yamlfix.toml" ]; then
    OPTIONS+=("-c" "yamlfix.toml")
fi

FILES=()
while IFS= read -r -d '' file; do
    FILES+=("$file")
done < <("${GIT}" ls-files '*.yaml' '*.yml' -z)

if [ "${#FILES[@]}" -gt 0 ]; then
    "$TOOL" "${OPTIONS[@]}" "${FILES[@]}"
    if ! "${GIT}" diff --quiet -- '*.yaml' '*.yml'; then
        echo "YAML formatting errors found. Run 'bazelisk run //:fix_lint' to fix." >&2
        "${GIT}" diff -- '*.yaml' '*.yml'
        exit 1
    fi
fi
