#!/usr/bin/env bash

set -e

mkdir -p results

LOG_FILE=${LOG_FILE:-results/$TEST_NAME-$TEST_EXT.log}

ORD_ARGS=""
if [ "$TEST_TYPE" == "python" ]; then
    ORD_ARGS="-python"
fi

# this allows the script to be run from any directory, which
# is useful with bazel test as the OpenROAD binary does not
# refer to its dependencies with full, but with relative path
# to the runfiles folder.
SAVED_PWD=$(pwd)

echo "Directory: ${PWD}"
echo "Command:   $OPENROAD_EXE $ORD_ARGS -no_splash -no_init -exit  $TEST_NAME.$TEST_EXT > $LOG_FILE"

(
    # subshell to clean up after launching from a different directory
    cd ${BAZEL_SAVED_PWD:-$SAVED_PWD} &&
    $OPENROAD_EXE $ORD_ARGS -cd $SAVED_PWD -no_splash -no_init -exit $TEST_NAME.$TEST_EXT &> $LOG_FILE
)

echo "Exitcode:  $?"

if [ "$TEST_CHECK_LOG" == "True" ]; then
    echo "Diff:      ${PWD}/results/$TEST_NAME-$TEST_EXT.diff"
    diff $LOG_FILE $TEST_NAME.ok > results/$TEST_NAME-$TEST_EXT.diff
fi

if [ "$TEST_CHECK_PASSFAIL" == "True" ]; then
    tail -n1 $LOG_FILE | grep -E '^(pass|OK)'
fi
