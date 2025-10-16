#!/usr/bin/env bash
set -ex
# TEST_NAME is used in cmake and bazel, but in bazel it is a label
# not a test name.
export TEST_EXT=${TEST_FILE##*.}
export TEST_NAME=$(basename "$TEST_FILE" .$TEST_EXT)
export OPENROAD_EXE=$(realpath $OPENROAD_EXE)
export RESULTS_DIR="${TEST_UNDECLARED_OUTPUTS_DIR}/results"
export REGRESSION_TEST=$(realpath $REGRESSION_TEST)

cd $(dirname $TEST_FILE)
mkdir -p $RESULTS_DIR
$REGRESSION_TEST
