#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Smoke test: extract the packaging tarball and verify the installed
# binary can start (Tcl initialisation succeeds).
set -euo pipefail

TARFILE="$1"

DEST_DIR=$(mktemp -d)
trap 'rm -rf "$DEST_DIR"' EXIT

cp "$TARFILE" "$DEST_DIR"
cd "$DEST_DIR"
tar -xf openroad.tar
rm -f openroad.tar

# Remove repo_mapping (same as install.sh)
if [ -e openroad.repo_mapping ]; then
    chmod u+w openroad.repo_mapping
    rm -rf openroad.repo_mapping
fi

# Clear Bazel runfiles env vars so the installed binary resolves its own
# runfiles tree (not the test runner's).
unset RUNFILES_DIR RUNFILES_MANIFEST_FILE TEST_SRCDIR

# Verify the binary starts and can evaluate a trivial Tcl expression.
echo 'puts "install_test_ok"' > test_script.tcl
if OUTPUT=$(./openroad -no_init -no_splash -exit test_script.tcl 2>&1) && echo "$OUTPUT" | grep -q "install_test_ok"; then
    echo "PASS: installed binary started successfully"
else
    echo "FAIL: installed binary failed to start or produced unexpected output"
    echo "Output was: $OUTPUT"
    exit 1
fi
