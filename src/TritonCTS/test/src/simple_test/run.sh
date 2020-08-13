#!/usr/bin/env bash

GREEN=0
RED=2

if [ "$#" -ne 2 ]; then
	exit 2
fi

binary=$1

$binary -no_init < run.tcl > ${2}/simple_test.log 2>&1

if grep -q "Fanout distribution for the current clock = 7:1, 9:1." ${2}/simple_test.log;
then
	exit $GREEN
else
	exit $RED
fi
