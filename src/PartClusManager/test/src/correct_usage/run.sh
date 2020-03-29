#!/usr/bin/env bash

GREEN=0
RED=2

if [ "$#" -ne 1 ]; then
	exit 2
fi

binary=$1

$binary -no_init run.tcl > run.log 2>/dev/null
cat run.log | tail -n 5 > run_edit.log 

if cmp -s run_edit.log golden.log;
then
	exit $GREEN
else
	exit $RED
fi
