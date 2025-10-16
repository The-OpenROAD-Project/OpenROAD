#!/bin/bash -f

if [ $# -lt 6 ] 
then
	echo "Usage <openroad_exec> <makefile_target> <process_file> <process_name> <wire_cnt> <version>"
	exit
fi
dir=$1
or_exec=$2
process=$3
outname=$4
wire_cnt=$5
version=$6


tcl_script=tmp_gen_patterns.tcl

rm -rf $dir
mkdir $dir
cd $dir

echo "gen_solver_patterns -process_file $process -process_name $outname -wire_cnt $wire_cnt -version $version" > $tcl_script

$or_exec < $tcl_script > $outname.log

