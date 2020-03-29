#!/usr/bin/env bash

GREEN=0
RED=2

if [ "$#" -ne 1 ]; then
	exit 2
fi

binary=$1

$binary -no_init run.tcl > run.log 2>&1

if cmp -s part_ids.txt part_ids.txt.golden;
then
	exit $GREEN
else
	exit $RED
fi
