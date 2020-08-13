#!/usr/bin/env bash

GREEN=0
RED=2

if [ "$#" -ne 2 ]; then
	exit 2
fi

binary=$1

$binary -no_init < run.tcl > ${2}/create_lut.log 2>&1

if grep -q "The chacterization used 1 buffer(s) types. All of them are in the loaded DB." ${2}/create_lut.log;
then
	exit $GREEN
else
	exit $RED
fi
