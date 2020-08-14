#!/usr/bin/env bash

GREEN=0
RED=2

if [ "$#" -ne 1 ]; then
	exit 2
fi

binary=$1

$binary -no_init < run.tcl > test.log 2>&1

if grep -q "Fanout distribution for the current clock = 2:4, 3:3, 4:3, 5:2, 6:6, 7:2, 8:4, 9:3, 10:42, 12:1, 13:1." test.log;
then
	exit $GREEN
else
	exit $RED
fi
