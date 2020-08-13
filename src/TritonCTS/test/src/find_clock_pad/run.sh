#!/usr/bin/env bash

#This unit test was created with the help of James Cherry. The script this test is based on can be found in dbSta/test.

GREEN=0
RED=2

if [ "$#" -ne 2 ]; then
	exit 2
fi

binary=$1

$binary -no_init < run.tcl > ${2}/find_clock_pad.log 2>&1

if grep -q "TritonCTS found 1 clock nets." ${2}/find_clock_pad.log;
then
	exit $GREEN
else
	exit $RED
fi
