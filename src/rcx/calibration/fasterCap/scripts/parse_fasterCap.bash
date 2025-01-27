#!/bin/bash -f

if [ $# -lt 5 ] 
then
	echo "Usage: parse_fasterCap <run_dir> <data_dir> <data_file_name> <python_script> <wire> <gold_dir>"
	exit
fi
dir=$1
data_dir=$2
data_file_name=$3
python_script=$4
wire=$5

echo " ----------------------------------------------------------------------------------"
echo "parse_fasterCap.bash $data_dir $data_file_name $python_script $wire $gold_dir"
echo " ----------------------------------------------------------------------------------"

rm -rf $dir
mkdir $dir
cd $dir
pwd

find_start_dir="$data_dir/$data_file_name"
echo "find $find_start_dir -name "wires.log" -print > wire_log_file_list "
find $find_start_dir  -name "wires.log" -print > wire_log_file_list 
sort wire_log_file_list > sorted.input.list 

python3 $python_script -in_list_file  sorted.input.list -wire $wire -out_file $data_file_name.caps > OUT 

# cat OUT 
sort -n -r run_stats > run_stats.sorted 

if [ $# -eq 6 ] 
then
	gold_dir=$6
	echo "diff . ../$gold_dir" 
	diff . ../$gold_dir 
fi
