#!/bin/bash
set -u -eo pipefail
mkdir -p $RESULTS_DIR $LOG_DIR $REPORTS_DIR $OBJECTS_DIR
echo Running $2.tcl, stage $1
(trap 'mv $LOG_DIR/$1.tmp.log $LOG_DIR/$1.log' EXIT; \
 $OPENROAD_EXE $OPENROAD_ARGS -exit $SCRIPTS_DIR/noop.tcl 2>&1 >$LOG_DIR/$1.tmp.log; \
 eval "$TIME_CMD $OPENROAD_CMD -no_splash $SCRIPTS_DIR/$2.tcl -metrics $LOG_DIR/$1.json" 2>&1 | \
 tee -a $(realpath $LOG_DIR/$1.tmp.log))
