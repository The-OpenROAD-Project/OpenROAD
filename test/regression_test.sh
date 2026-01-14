#!/usr/bin/env bash

set -e

RESULTS_DIR="${RESULTS_DIR:-results}"
LOG_FILE="${RESULTS_DIR}/$TEST_NAME-$TEST_EXT.log"

mkdir -p ${RESULTS_DIR}

ORD_ARGS=""
if [ "$TEST_TYPE" == "python" ]; then
    ORD_ARGS="-python"
fi

echo "Directory: ${PWD}"
echo "Results Directory: ${RESULTS_DIR}"
echo "Command:   $OPENROAD_EXE $ORD_ARGS -no_splash -no_init -exit  $TEST_NAME.$TEST_EXT |& tee $LOG_FILE"

$OPENROAD_EXE $ORD_ARGS -no_splash -no_init -exit $TEST_NAME.$TEST_EXT |& tee $LOG_FILE

echo "Exitcode:  $?"

if [ "$TEST_CHECK_LOG" == "True" ]; then
    echo "Diff:      ${RESULTS_DIR}/$TEST_NAME-$TEST_EXT.diff"
    diff $LOG_FILE $TEST_NAME.ok > ${RESULTS_DIR}/$TEST_NAME-$TEST_EXT.diff
fi

if [ "$TEST_CHECK_PASSFAIL" == "True" ]; then
    tail -n1 $LOG_FILE | grep -E '^(pass|OK)'
fi
