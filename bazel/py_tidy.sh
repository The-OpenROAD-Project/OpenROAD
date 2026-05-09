#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors
#
# Auto-format changed Python files in-place using Black.
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
cd "${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

base_ref=""
for candidate in "${OPENROAD_LINT_BASE_REF:-}" origin/main main origin/master master; do
    if [ -n "$candidate" ] && git rev-parse --verify --quiet "$candidate^{commit}" >/dev/null; then
        if base_ref="$(git merge-base HEAD "$candidate")"; then
            break
        fi
    fi
done

changed_files=()
while IFS= read -r -d "" file; do
    # Skip tracked symlinks. They are checked out as plain text placeholders on
    # some Windows worktrees, and Black cannot parse those placeholder files.
    if [ "$(git ls-files -s -- "$file" | awk '{print $1; exit}')" = "120000" ]; then
        continue
    fi
    changed_files+=("$file")
done < <(
    if [ -n "$base_ref" ]; then
        git diff --name-only --diff-filter=d -z "$base_ref" HEAD -- "*.py"
    fi
)

if [ "${#changed_files[@]}" -eq 0 ]; then
    echo "No changed Python files to format."
    exit 0
fi

"$TOOL" "${changed_files[@]}"
