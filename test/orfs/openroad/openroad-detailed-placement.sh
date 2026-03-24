#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Standalone wrapper for detailed_placement.
# Drop-in replacement for: openroad -exit detail_place.tcl
#
# Usage:
#   openroad-detailed-placement --read_db in.odb --write_db out.odb [flags...]
#
# ORFS Makefile migration (one line change):
#   Before: $(OPENROAD_CMD) $(SCRIPTS_DIR)/detail_place.tcl
#   After:  openroad-detailed-placement --read_db $< --write_db $@

set -euo pipefail

# Find the native binary relative to this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Try runfiles location (bazelisk run)
if [[ -n "${RUNFILES_DIR:-}" ]]; then
    BINARY="${RUNFILES_DIR}/_main/test/orfs/openroad/detailed_placement"
elif [[ -f "${SCRIPT_DIR}/detailed_placement" ]]; then
    BINARY="${SCRIPT_DIR}/detailed_placement"
else
    # Try bazel-bin
    BINARY="$(dirname "${SCRIPT_DIR}")/../../bazel-bin/test/orfs/openroad/detailed_placement"
fi

if [[ ! -x "${BINARY}" ]]; then
    echo "Error: detailed_placement binary not found." >&2
    echo "Build it with: bazelisk build //test/orfs/openroad:detailed_placement" >&2
    exit 1
fi

exec "${BINARY}" "$@"
