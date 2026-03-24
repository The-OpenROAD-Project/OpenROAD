#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Check for duplicate logger message IDs across all source files.
set -e

# --- Resolve runfiles (Bazel-idiomatic) ---
# Bazel sets RUNFILES_DIR or we derive it from $0.
if [[ -z "${RUNFILES_DIR:-}" ]]; then
    if [[ -d "$0.runfiles" ]]; then
        RUNFILES_DIR="$0.runfiles"
    else
        RUNFILES_DIR="$(cd "$(dirname "$0")" && pwd)"
    fi
fi

FIND_MESSAGES="${RUNFILES_DIR}/_main/etc/find_messages.py"
if [ ! -f "${FIND_MESSAGES}" ]; then
    echo "ERROR: Cannot find find_messages.py at ${FIND_MESSAGES}" >&2
    exit 1
fi

python3 "${FIND_MESSAGES}" -d src
