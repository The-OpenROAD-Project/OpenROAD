#!/usr/bin/env bash

GREEN=0
RED=2

if [ "$#" -ne 1 ]; then
	exit 2
fi

binary=$1

$binary -no_init < run.tcl > test.log 2>&1

if grep -q "Sinks will be clustered in groups of 10 and a maximum diameter of 60 um" test.log;
then
	exit $GREEN
else
	exit $RED
fi
