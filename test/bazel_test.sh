#!/usr/bin/env bash
set -ex
# TEST_NAME is used in cmake and bazel, but in bazel it is a label
# not a test name.
export TEST_NAME=$TEST_NAME_BAZEL
export BAZEL_SAVED_PWD=$(pwd)
export OPENROAD_EXE=$(realpath $OPENROAD_EXE)
export LOG_FILE=/dev/stdout

cd test/

if [ -f $TEST_NAME_BAZEL.tcl ]; then
    export TEST_EXT=tcl
    export TEST_TYPE=tcl
else
    export TEST_EXT=py
    export TEST_TYPE=python
fi

./regression_test.sh
