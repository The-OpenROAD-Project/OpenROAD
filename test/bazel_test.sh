#!/usr/bin/env bash

set -e

echo "Directory: ${PWD}"

DIR=$(dirname "$0")
cd $DIR

# TEST_NAME is used in cmake and bazel...
export TEST_NAME=$TEST_NAME_BAZEL
./regression_test.sh
