#!/usr/bin/env bash

set -e

echo "Directory: ${PWD}"

# TEST_NAME is used in cmake and bazel...
export TEST_NAME=$TEST_NAME_BAZEL
test/regression_test.sh
