#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, Precision Innovations Inc.
#
# Run all tests across workspaces: lint, root workspace tests, and
# ORFS integration tests (test/orfs/ sub-module).
#
# Usage:
#   bazelisk run //:test
#   bazelisk run //:test -- --keep_going --test_output=errors
set -euo pipefail

# Resolve the repository root from the runfiles location.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# When run via `bazelisk run`, the script is in the runfiles tree.
# Walk up to find the workspace root by looking for MODULE.bazel.
ROOT_DIR="$SCRIPT_DIR"
while [ ! -f "$ROOT_DIR/MODULE.bazel" ] && [ "$ROOT_DIR" != "/" ]; do
    ROOT_DIR="$(dirname "$ROOT_DIR")"
done

if [ ! -f "$ROOT_DIR/MODULE.bazel" ]; then
    echo "ERROR: Could not find workspace root (MODULE.bazel)" >&2
    exit 1
fi

cd "$ROOT_DIR"

echo "=== Lint ==="
bazelisk run //:fix_lint

echo "=== Root workspace tests ==="
bazelisk test "$@" ...

echo "=== ORFS integration tests (test/orfs/) ==="
(cd test/orfs && bazelisk test "$@" ...)
