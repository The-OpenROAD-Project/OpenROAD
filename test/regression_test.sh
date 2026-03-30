#!/usr/bin/env bash

set -e
set -o pipefail

RESULTS_DIR="${RESULTS_DIR:-results}"
LOG_FILE="${RESULTS_DIR}/$TEST_NAME-$TEST_EXT.log"

mkdir -p ${RESULTS_DIR}

echo "Directory: ${PWD}"
echo "Results Directory: ${RESULTS_DIR}"

case "$TEST_TYPE" in
standalone_python)
	CMD="python3 -u $TEST_NAME.$TEST_EXT"
	;;
python)
	if "$OPENROAD_EXE" -help 2>&1 | grep -q -- "-python"; then
		CMD="$OPENROAD_EXE -python -no_splash -no_init -exit $TEST_NAME.$TEST_EXT"
	else
		CMD="python3 -u $TEST_NAME.$TEST_EXT"
	fi
	;;
*)
	CMD="$OPENROAD_EXE -no_splash -no_init -exit $TEST_NAME.$TEST_EXT"
	;;
esac

echo "Command: $CMD"
EXPECTED_EXIT_CODE="${TEST_EXPECTED_EXIT_CODE:-0}"
set +e
$CMD 2>&1 | tee $LOG_FILE
CMD_EXIT=${PIPESTATUS[0]}
set -e

case "$TEST_TYPE" in
python | standalone_python)
	sed -E -i 's#(File ")[^"]*/([^"]+")#\1\2#g' "$LOG_FILE"
	sed -E -i '/^[[:space:]]+\^+$/d' "$LOG_FILE"
	awk '
		prev_internal_frame && $0 ~ /^[[:space:]]+/ { prev_internal_frame = 0; next }
		{
			print
			prev_internal_frame = ($0 ~ /File "(openroadpy|.*_py)\.py", line [0-9]+, in /)
		}
	' "$LOG_FILE" > "${LOG_FILE}.tmp"
	mv "${LOG_FILE}.tmp" "$LOG_FILE"
	;;
esac

echo "Exitcode:  $CMD_EXIT"

if [ "$CMD_EXIT" -ne "$EXPECTED_EXIT_CODE" ]; then
	echo "Expected exit code: $EXPECTED_EXIT_CODE"
	exit 1
fi

if [ "$TEST_CHECK_LOG" == "True" ]; then
    echo "Diff:      ${RESULTS_DIR}/$TEST_NAME-$TEST_EXT.diff"
    GOLDEN_FILE="${TEST_GOLDEN_FILE:-$TEST_NAME.ok}"
    diff "$GOLDEN_FILE" "$LOG_FILE" > "${RESULTS_DIR}/$TEST_NAME-$TEST_EXT.diff"
fi

if [ "$TEST_CHECK_PASSFAIL" == "True" ]; then
	tail -n1 $LOG_FILE | grep -E '^(pass|OK)'
fi
