#!/usr/bin/env bash

set -e

# Make this work with a different pwd
PREFIX=$(realpath $(realpath --relative-to=$(pwd) $(dirname $0)))

mkdir -p $PREFIX/results

LOG_FILE=$PREFIX/results/$TEST_NAME-$TEST_EXT.log

ORD_ARGS=""
if [ "$TEST_TYPE" == "python" ]; then
    ORD_ARGS="-python"
fi

echo "Command:   $OPENROAD_EXE $ORD_ARGS -no_splash -no_init -exit  $PREFIX/$TEST_NAME.$TEST_EXT > $LOG_FILE"

$OPENROAD_EXE $ORD_ARGS -no_splash -no_init -exit <(cat <<EOF
cd "$PREFIX"
source "$TEST_NAME.$TEST_EXT"
EOF
) 2>&1 | tee $LOG_FILE

echo "Exitcode:  $?"

if [ "$TEST_CHECK_LOG" == "True" ]; then
    echo "Diff:      $PREFIX/results/$TEST_NAME-$TEST_EXT.diff"
    diff $LOG_FILE $PREFIX/$TEST_NAME.ok > $PREFIX/results/$TEST_NAME-$TEST_EXT.diff || (head -n 10 $LOG_FILE && exit 1)
fi

if [ "$TEST_CHECK_PASSFAIL" == "True" ]; then
    tail -n1 $LOG_FILE | grep -E '^(pass|OK)'
fi
