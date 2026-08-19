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
# Absolute: the scan below runs from a different directory.
FIND_MESSAGES="$(realpath "${FIND_MESSAGES}")"

# Run from the source tree. A test runs in its runfiles directory, which holds
# nothing but its own declared data, so scanning "src" from there would walk a
# path that does not exist -- find_messages.py would report zero messages and
# the check would pass unconditionally. MODULE.bazel is declared as data purely
# so its runfiles entry is a symlink pointing back at the real workspace, the
# same handle the lint tests use.
[ -L MODULE.bazel ] || { echo "MODULE.bazel missing from runfiles" >&2; exit 1; }
cd "$(dirname "$(readlink MODULE.bazel)")"

# One walk over all of src, so an ID reused across two modules is caught along
# with one reused inside a single module. This is wider than the per-module
# //src/<module>:messages_txt targets, which each scan their own module only.
# find_messages.py exits non-zero naming both sites; its listing of every
# message goes to stdout and is not interesting here.
python3 "${FIND_MESSAGES}" -d src > /dev/null
