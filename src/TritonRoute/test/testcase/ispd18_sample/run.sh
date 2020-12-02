#!/bin/bash

set -e
set -o pipefail

if [ "$#" -ne 1 ]; then
	exit 2
fi

binary=$1

$binary -lef ./ispd18_sample.input.lef -def ./ispd18_sample.input.def -guide ./ispd18_sample.input.guide -output ../../result/ispd18_sample/ispd18_sample.output.def | tee ../../result/ispd18_sample/ispd18_sample.log

./compare.tcl ./golden/ispd18_sample.log ../../result/ispd18_sample/ispd18_sample.log
compare_return_code=$?
exit $compare_return_code
