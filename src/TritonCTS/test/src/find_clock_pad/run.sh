#!/usr/bin/env bash

#This unit test was created with the help of James Cherry. The script this test is based on can be found in dbSta/test.

GREEN=0
RED=2

if [ "$#" -ne 1 ]; then
	exit 2
fi

binary=$1

$binary -no_init < run.tcl > test.log 2>&1

if grep -q "TritonCTS found 1 clock nets." test.log;
then
	exit $GREEN
else
	exit $RED
fi
