#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, Precision Innovations Inc.
#
# Run all tests across workspaces: lint, root workspace tests, and
# ORFS integration tests (test/orfs/ sub-module).
#
# Usage:
#   bazelisk run //:test
#   bazelisk run //:test -- --keep_going --test_output=errors
set -euo pipefail

# Change to the workspace root. When run via `bazelisk run`, Bazel sets
# this environment variable to the workspace root directory.
if [[ -z "${BUILD_WORKSPACE_DIRECTORY:-}" ]]; then
    echo "ERROR: This script must be run with 'bazelisk run'" >&2
    exit 1
fi
cd "${BUILD_WORKSPACE_DIRECTORY}"

echo "=== Lint ==="
bazelisk run //:fix_lint

echo "=== Root workspace tests ==="
bazelisk test "$@" ...

echo "=== ORFS integration tests (test/orfs/) ==="
(cd test/orfs && bazelisk test "$@" ...)
