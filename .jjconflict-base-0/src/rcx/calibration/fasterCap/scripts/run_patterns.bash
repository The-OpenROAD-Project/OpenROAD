#!/bin/bash -f

if [ $# -lt 4 ] 
then
	echo "Usage <or_exec> <make_target> <script> <gold_dir>"
	exit
fi
or_exec=$2
dir=$1
tcl_script=$3
gold_dir=$4

rm -rf $dir
mkdir $dir
cd $dir
pwd
$or_exec < $tcl_script > OUT 
cat TYP/*/*/*/*/wires > all_wires
echo "diff OUT ../$gold_dir/OUT"
diff OUT ../$gold_dir/OUT
echo "diff -r . ../$gold_dir"
diff -r . ../$gold_dir.GOLD
cat OUT

