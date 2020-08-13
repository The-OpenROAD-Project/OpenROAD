#!/usr/bin/env bash

GREEN=0
RED=2

if [ "$#" -ne 2 ]; then
	exit 2
fi

binary=$1

$binary -no_init < run.tcl > ${2}/no_sinks.log 2>&1

if grep -q " Net \"clk\" has 0 sinks" ${2}/no_sinks.log;
then
	exit $GREEN
else
	exit $RED
fi
