#!/bin/bash
# Test script for bazel/install.sh
set -e

echo "Running install.sh argument parser tests..."

# Mock the environment to bypass bazelisk info calls in the test sandbox
export BUILD_WORKSPACE_DIRECTORY="$(mktemp -d)"
export TARFILE="${BUILD_WORKSPACE_DIRECTORY}/openroad.tar"

# Create a valid, empty tar archive without triggering GNU tar's "cowardly" abort
touch "${BUILD_WORKSPACE_DIRECTORY}/dummy.txt"
tar -cf "$TARFILE" -C "${BUILD_WORKSPACE_DIRECTORY}" dummy.txt

echo "Mock TARFILE created at: $TARFILE"

# Test 1: Passing explicit destination path
TEST1_DEST="${BUILD_WORKSPACE_DIRECTORY}/dest1"
echo "--- Test 1: Explicit Destination ---"
./bazel/install.sh "$TEST1_DEST"
if [ ! -d "$TEST1_DEST" ]; then
    echo "FAIL: Test 1 did not create the explicit destination directory."
    exit 1
fi
echo "PASS: Test 1"

# Test 2: Passing the -f flag for ORFS location
TEST2_DEST="${BUILD_WORKSPACE_DIRECTORY}/../install/OpenROAD/bin"
echo "--- Test 2: ORFS -f Flag ---"
./bazel/install.sh -f
if [ ! -d "$TEST2_DEST" ]; then
    echo "FAIL: Test 2 did not correctly resolve the -f ORFS destination."
    exit 1
fi
echo "PASS: Test 2"

# Test 3: Passing no arguments (Should fail with error message)
echo "--- Test 3: No Arguments Expected Failure ---"
if ./bazel/install.sh; then
    echo "FAIL: Test 3 succeeded when it should have blocked empty arguments."
    exit 1
else
    echo "PASS: Test 3 correctly aborted."
fi

echo "All tests passed successfully!"
