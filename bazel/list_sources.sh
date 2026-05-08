#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors
#
# List source files for lint/format targets without depending on git.
set -euo pipefail

print0=0
if [ "${1:-}" = "-z" ]; then
    print0=1
    shift
fi

if [ "$#" -eq 0 ]; then
    echo "usage: $0 [-z] <find-pattern>..." >&2
    exit 2
fi

patterns=()
for pattern in "$@"; do
    if [ "${#patterns[@]}" -gt 0 ]; then
        patterns+=("-o")
    fi

    if [[ "$pattern" == */* ]]; then
        patterns+=("-path" "./$pattern")
    else
        patterns+=("-name" "$pattern")
    fi
done

# Keep the VCS/upstream-owned directories explicit.  These are the paths that
# git ls-files used to skip implicitly.
while IFS= read -r -d '' path; do
    path="${path#./}"
    if [ "$print0" -eq 1 ]; then
        printf '%s\0' "$path"
    else
        printf '%s\n' "$path"
    fi
done < <(
    find . \
        \( \( -type d -o -type l \) \
            \( \
                -path "./.git" -o \
                -path "./src/sta" -o \
                -path "./third-party/abc" -o \
                -path "./bazel-*" -o \
                -path "./build" -o \
                -path "./build-*" \
            \) \
        \) -prune -o \
        \( -type f -o -type l \) \( "${patterns[@]}" \) -print0
)
