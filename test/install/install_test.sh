#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Smoke test: extract the packaging tarball and verify the installed
# binary can start with cleared runfiles env vars.
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

# Verify CLI startup and trivial Tcl evaluation works.
echo 'puts "install_test_ok"' > test_script.tcl
if OUTPUT=$(./openroad -no_init -no_splash -exit test_script.tcl 2>&1) && echo "$OUTPUT" | grep -q "install_test_ok"; then
    echo "PASS: installed binary CLI startup works"
else
    echo "FAIL: installed binary CLI startup failed or produced unexpected output"
    echo "Output was: $OUTPUT"
    exit 1
fi

# Verify Main.cc runfiles initialization: RUNFILES_DIR should be present even
# though it was cleared in this test process.
cat > runfiles_env_test.tcl <<'EOF'
if {![info exists ::env(RUNFILES_DIR)]} {
  puts "runfiles_env_missing"
  exit 1
}
if {![file isdirectory $::env(RUNFILES_DIR)]} {
  puts "runfiles_dir_not_found"
  exit 1
}
puts "runfiles_env_ok"
EOF

if OUTPUT=$(./openroad -no_init -no_splash -exit runfiles_env_test.tcl 2>&1) && echo "$OUTPUT" | grep -q "runfiles_env_ok"; then
    echo "PASS: RUNFILES_DIR initialized by openroad"
else
    echo "FAIL: RUNFILES_DIR was not initialized as expected"
    echo "Output was: $OUTPUT"
    exit 1
fi
