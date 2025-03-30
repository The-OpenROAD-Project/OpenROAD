#!/usr/bin/env bash

set -e

mkdir -p results

LOG_FILE=$(realpath results/$TEST_NAME-$TEST_EXT.log)

ORD_ARGS=""
if [ "$TEST_TYPE" == "python" ]; then
    ORD_ARGS="-python"
fi

# this allows the script to be run from any directory, which
# is useful with bazel test as the OpenROAD binary does not
# refer to its dependencies with full, but with relative path
# to the runfiles folder.
SAVED_PWD=$(pwd)
if [ "$TEST_TYPE" == "python" ]; then
    script_content=$(cat <<EOF
import os
import runpy
import sys
sys.path.insert(0, "$SAVED_PWD")
os.chdir("$SAVED_PWD")
runpy.run_path("$TEST_NAME.$TEST_EXT", run_name="__main__")
EOF
    )
else
  script_content=$(cat <<EOF
cd "$SAVED_PWD"
source "$TEST_NAME.$TEST_EXT"
EOF
    )
fi

# Ideally, bazel would show continious output in the console when it is running
# a single command, but it doesn't.
#
# It is useful in bazel to ps -Af | grep tee to find the absolute path of
# the log file and tail -f it.
cd ${BAZEL_SAVED_PWD:-$SAVED_PWD}
$OPENROAD_EXE $ORD_ARGS -no_splash -no_init -exit <(echo "$script_content") 2>&1 | tee $LOG_FILE > /dev/null
cd $SAVED_PWD

echo "Exitcode:  $?"

if [ "$TEST_CHECK_LOG" == "True" ]; then
    echo "Diff:      results/$TEST_NAME-$TEST_EXT.diff"
    diff $LOG_FILE $TEST_NAME.ok > results/$TEST_NAME-$TEST_EXT.diff || (head -n 10 $LOG_FILE && exit 1)
fi

if [ "$TEST_CHECK_PASSFAIL" == "True" ]; then
    tail -n1 $LOG_FILE | grep -E '^(pass|OK)'
fi
