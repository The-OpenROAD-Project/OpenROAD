#!/usr/bin/env bash

GREEN=0
RED=2

if [ "$#" -ne 1 ]; then
	exit 2
fi

binary=$1

$binary -no_init run.tcl > run.log 2>&1

if cmp -s run.log golden.log;
then
	exit $GREEN
else
	exit $RED
fi
