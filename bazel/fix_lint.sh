#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2026, The OpenROAD Authors
#
# Auto-fix then lint. Delegates to per-language tidy/lint scripts so
# that file-discovery logic is not duplicated (DRY).
set -euo pipefail

TCL_TIDY_SH="$1"
TCLFMT="$2"
TCL_LINT_SH="$3"
TCLINT="$4"
BZL_TIDY_SH="$5"
BZL_FMT_BUILDIFIER="$6"
BZL_LINT_SH="$7"
BZL_LINT_BUILDIFIER="$8"
GIT="$9"

export BUILD_WORKSPACE_DIRECTORY="${BUILD_WORKSPACE_DIRECTORY:-$PWD}"

# TCL: auto-format then lint
"${TCL_TIDY_SH}" "${TCLFMT}"
"${TCL_LINT_SH}" "${TCLINT}" "${GIT}" || rc=$?

# Bazel: auto-format then lint
"${BZL_TIDY_SH}" "${BZL_FMT_BUILDIFIER}"
"${BZL_LINT_SH}" "${BZL_LINT_BUILDIFIER}" "${GIT}" || rc=$?

"${GIT}" -C "$BUILD_WORKSPACE_DIRECTORY" status

if [ "${rc:-0}" -ne 0 ]; then
    echo "Error: lint violations remain that require manual fixes." >&2
fi
exit "${rc:-0}"
