#!/usr/bin/env bash

GREEN=0
RED=2

if [ "$#" -ne 1 ]; then
	exit 2
fi

binary=$1

$binary -no_init < run.tcl > test.log 2>&1

if grep -q "Buffer RANDNAME_X1 is not in the loaded DB." test.log;
then
	exit $GREEN
else
	exit $RED
fi
