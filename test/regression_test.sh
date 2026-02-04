#!/usr/bin/env bash

set -e

RESULTS_DIR="${RESULTS_DIR:-results}"
LOG_FILE="${RESULTS_DIR}/$TEST_NAME-$TEST_EXT.log"

mkdir -p ${RESULTS_DIR}

echo "Directory: ${PWD}"
echo "Results Directory: ${RESULTS_DIR}"

case "$TEST_TYPE" in
    standalone_python)
        CMD="python3 $TEST_NAME.$TEST_EXT"
        ;;
    python)
        CMD="$OPENROAD_EXE -python -no_splash -no_init -exit $TEST_NAME.$TEST_EXT"
        ;;
    *)
        CMD="$OPENROAD_EXE -no_splash -no_init -exit $TEST_NAME.$TEST_EXT"
        ;;
esac

echo "Command: $CMD"
$CMD |& tee $LOG_FILE

echo "Exitcode:  $?"

if [ "$TEST_CHECK_LOG" == "True" ]; then
    echo "Diff:      ${RESULTS_DIR}/$TEST_NAME-$TEST_EXT.diff"
    diff $LOG_FILE $TEST_NAME.ok > ${RESULTS_DIR}/$TEST_NAME-$TEST_EXT.diff
fi

if [ "$TEST_CHECK_PASSFAIL" == "True" ]; then
    tail -n1 $LOG_FILE | grep -E '^(pass|OK)'
fi
