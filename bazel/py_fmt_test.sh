#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors
#
# Check that changed Python files are properly formatted.
set -euo pipefail
TOOL="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
GIT="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
# MODULE.bazel must be in the sh_test `data` deps so it appears as a
# runfiles symlink pointing at the real workspace.
[ -L MODULE.bazel ] || { echo "MODULE.bazel missing from runfiles" >&2; exit 1; }
WORKSPACE="$(cd "$(dirname "$(readlink MODULE.bazel)")" && pwd)"
cd "$WORKSPACE"

base_ref=""
for candidate in "${OPENROAD_LINT_BASE_REF:-}" origin/main main origin/master master; do
    if [ -n "$candidate" ] && "${GIT}" rev-parse --verify --quiet "$candidate^{commit}" >/dev/null; then
        if base_ref="$(${GIT} merge-base HEAD "$candidate")"; then
            break
        fi
    fi
done

changed_files=()
while IFS= read -r -d "" file; do
    # Tracked symlinks are plain text placeholders on some Windows worktrees.
    if [ -L "$file" ] || [ "$(${GIT} ls-files -s -- "$file" | awk '{print $1; exit}')" = "120000" ]; then
        continue
    fi
    changed_files+=("$file")
done < <(
    if [ -n "$base_ref" ]; then
        "${GIT}" diff --name-only --diff-filter=d -z "$base_ref" -- "*.py"
    fi
    "${GIT}" ls-files --others --exclude-standard -z -- "*.py"
)

if [ "${#changed_files[@]}" -eq 0 ]; then
    echo "No changed Python files to check."
    exit 0
fi

"$TOOL" --check "${changed_files[@]}"
