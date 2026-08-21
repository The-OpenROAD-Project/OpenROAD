#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Helper script to run `git ls-files` excluding git submodules (gitlink mode 160000).

set -euo pipefail

GIT="$1"
shift

EXCLUDES=()
while read -r mode _ _ subpath; do
    if [ "$mode" = "160000" ] && [ -n "$subpath" ]; then
        EXCLUDES+=(":^$subpath")
    fi
done < <("${GIT}" -c submodule.recurse=false ls-files --stage)

"${GIT}" -c submodule.recurse=false ls-files "$@" ${EXCLUDES[@]+"${EXCLUDES[@]}"}
