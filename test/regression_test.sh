#!/usr/bin/env bash

set -e

mkdir -p results

LOG_FILE=results/$TEST_NAME-$TEST_EXT.log

ORD_ARGS=""
if [ "$TEST_TYPE" == "python" ]; then
    ORD_ARGS="-python"
fi

echo "Directory: ${PWD}"
echo "Command:   $OPENROAD_EXE $ORD_ARGS -no_splash -no_init -exit  $TEST_NAME.$TEST_EXT > $LOG_FILE"

$OPENROAD_EXE $ORD_ARGS -no_splash -no_init -exit $TEST_NAME.$TEST_EXT > $LOG_FILE

echo "Exitcode:  $?"

if [ "$TEST_CHECK_LOG" == "True" ]; then
    echo "Diff:      ${PWD}/results/$TEST_NAME-$TEST_EXT.diff"
    diff $LOG_FILE $TEST_NAME.ok > results/$TEST_NAME-$TEST_EXT.diff
fi

if [ "$TEST_CHECK_PASSFAIL" == "True" ]; then
    tail -n1 $LOG_FILE | grep -G '^pass$'
fi
