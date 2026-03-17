#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Check for duplicate logger message IDs across all source files.
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Find find_messages.py: check alongside script first, then Bazel runfiles.
if [ -f "${SCRIPT_DIR}/find_messages.py" ]; then
    FIND_MESSAGES="${SCRIPT_DIR}/find_messages.py"
elif [ -n "${RUNFILES_DIR:-}" ] && [ -f "${RUNFILES_DIR}/_main/etc/find_messages.py" ]; then
    FIND_MESSAGES="${RUNFILES_DIR}/_main/etc/find_messages.py"
else
    echo "ERROR: Cannot find find_messages.py" >&2
    exit 1
fi

python3 "${FIND_MESSAGES}" -d src
