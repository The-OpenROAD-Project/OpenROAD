#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Check YAML formatting with yamlfix.
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GIT="$(realpath "$2")"
# MODULE.bazel must be in the sh_test `data` deps so it appears as a
# runfiles symlink pointing at the real workspace. `readlink` (no -f,
# for macOS portability) resolves the absolute path Bazel wrote.
[ -L MODULE.bazel ] || { echo "MODULE.bazel missing from runfiles" >&2; exit 1; }
WORKSPACE="$(dirname "$(readlink MODULE.bazel)")"
cd "$WORKSPACE"

is_blacklisted() {
    local target="$1"
    if [ -f "yamlfix.ignore" ]; then
        grep -qxF "$target" "yamlfix.ignore" 2>/dev/null && return 0
    fi
    return 1
}

OPTIONS=()

FILES=()
while IFS= read -r -d '' file; do
    if ! is_blacklisted "$file"; then
        FILES+=("$file")
    fi
done < <("${GIT}" ls-files '*.yaml' '*.yml' -z)

if [ "${#FILES[@]}" -eq 0 ]; then
    exit 0
fi

# Create a temporary directory to avoid modifying the user's workspace during testing
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

# Copy files to the temporary directory preserving directory structure
for file in "${FILES[@]}"; do
    mkdir -p "$TMP_DIR/$(dirname "$file")"
    cp "$file" "$TMP_DIR/$file"
done

# Copy configuration if present
if [ -f "yamlfix.toml" ]; then
    cp "yamlfix.toml" "$TMP_DIR/"
    OPTIONS+=("-c" "$TMP_DIR/yamlfix.toml")
fi

# Run formatting on the temporary copies
(
    cd "$TMP_DIR"
    "$TOOL" "${OPTIONS[@]}" "${FILES[@]}"
)

# Check for differences
diff_found=0
for file in "${FILES[@]}"; do
    if ! diff -u "$file" "$TMP_DIR/$file"; then
        diff_found=1
    fi
done

if [ "$diff_found" -ne 0 ]; then
    echo "YAML formatting errors found. Run 'bazelisk run //:fix_lint' to fix." >&2
    exit 1
fi
