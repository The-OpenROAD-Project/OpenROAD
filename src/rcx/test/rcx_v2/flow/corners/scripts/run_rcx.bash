#!/bin/bash -f

if [ $# -lt 3 ]
then
        echo "Usage <or_exec> <dir> <test_name> "
        exit
fi
or_exec=$1
dir=$2
test_name=$3

run_dir=$dir/$test_name

rm -r $run_dir
mkdir -p $run_dir
cd $run_dir

# echo "$test_name"
$or_exec < ../scripts/$test_name.tcl > OUT  
if [ ! -e "$test_name.spef" ]; then
	echo "Fail $test_name : NO SPEF"
	exit
fi
line_cnt=`diff $test_name.spef ../$test_name.GOLD/$test_name.spef | egrep -v DATE | egrep -v VERSION | egrep -v "\-\-\-" | wc -l `
if [ $line_cnt -lt 3 ]
then
	echo "Pass $test_name"
else 
	echo "Fail $test_name"

	egrep  D_NET $test_name.spef > y1.spef
	egrep  D_NET ../$test_name.GOLD/$test_name.spef > y2.spef
	diff -y y1.spef y2.spef > y.diff
	# diff -y $test_name.spef ../$test_name.GOLD/$test_name.spef > y.diff
	python3 ~/z/v2_rcx/OpenROAD-OpenRCX-v2-fotakis/src/rcx/test/rcx_v2/flow/scripts/diff_netcap_spef.py
	echo "$test_name  $run_dir"
fi

#  line_cnt=`diff $test_name.spef ../$test_name.GOLD/$test_name.spef | egrep -v DATE | wc -l | awk '$1 == 2 { print "pass" } '

