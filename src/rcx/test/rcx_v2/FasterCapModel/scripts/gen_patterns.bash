#!/bin/bash -f

if [ $# -lt 7 ] 
then
	echo "Usage <or_exec> <makefile_target> <process_file> <process_name> <wire_cnt> <version> <gold_dir>"
	exit
fi
or_exec=$2
dir=$1
process=$3
outname=$4
wire_cnt=$5
version=$6
gold_dir=$7

tcl_script=tmp_gen_patterns.tcl

rm -rf $dir
mkdir $dir
cd $dir
pwd

echo "gen_solver_patterns -process_file $process -process_name $outname -wire_cnt $wire_cnt -version $version" > $tcl_script

$or_exec < $tcl_script > OUT 

# cat TYP/*/*/*/*/wires > all_wires
# echo "diff OUT ../$gold_dir/OUT"
# diff OUT ../$gold_dir/OUT

echo "diff -r . ../$gold_dir"
diff -r . ../$gold_dir

