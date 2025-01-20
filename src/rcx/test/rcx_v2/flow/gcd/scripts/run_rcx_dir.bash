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
# line_cnt=`diff $test_name.spef ../$test_name.GOLD/$test_name.spef | egrep -v DATE | egrep -v VERSION | egrep -v "\-\-\-" | wc -l `
line_cnt=`diff -w -r . ../$test_name.GOLD | egrep -v DATE | egrep -v VERSION | egrep -v "\-\-\-" | wc -l `
if [ $line_cnt -lt 3 ]
then
	echo "Pass $test_name"
else
	echo "Fail $test_name"
fi

#  line_cnt=`diff $test_name.spef ../$test_name.GOLD/$test_name.spef | egrep -v DATE | wc -l | awk '$1 == 2 { print "pass" } '

