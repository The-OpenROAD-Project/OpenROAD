#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Check for duplicate logger message IDs across all source files.
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# In Bazel runfiles, find_messages.py is alongside this script in etc/.
# Outside Bazel, it is in the same directory as this script.
if [ -n "${RUNFILES_DIR:-}" ] && [ -f "${RUNFILES_DIR}/_main/etc/find_messages.py" ]; then
    FIND_MESSAGES="${RUNFILES_DIR}/_main/etc/find_messages.py"
elif [ -f "${SCRIPT_DIR}/find_messages.py" ]; then
    FIND_MESSAGES="${SCRIPT_DIR}/find_messages.py"
else
    echo "ERROR: Cannot find find_messages.py" >&2
    exit 1
fi

python3 "${FIND_MESSAGES}" -d src
