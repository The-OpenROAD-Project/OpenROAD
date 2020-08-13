#!/usr/bin/env bash

GREEN=0
RED=2

if [ "$#" -ne 2 ]; then
	exit 2
fi

binary=$1

$binary -no_init < run.tcl > ${2}/simple_test_clus.log 2>&1

if grep -q "Fanout distribution for the current clock = 7:2, 8:2, 10:30." ${2}/simple_test_clus.log; 
then
	exit $GREEN
else
	exit $RED
fi
