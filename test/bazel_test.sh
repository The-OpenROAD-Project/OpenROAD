#!/usr/bin/env bash
set -e
# TEST_NAME is used in cmake and bazel, but in bazel it is a label
# not a test name.
export TEST_NAME=$TEST_NAME_BAZEL
export BAZEL_SAVED_PWD=$(pwd)
export OPENROAD_EXE=$(realpath $OPENROAD_EXE)
cd test/
./regression_test.sh
