#!/bin/bash -f

if [ $# -lt 6 ] 
then
	echo "Usage: parse_fasterCap <run_dir> <data_dir> <data_file_name> <python_script> <wire> <gold_dir>"
	exit
fi
dir=$1
data_dir=$2
data_file_name=$3
python_script=$4
wire=$5
gold_dir=$6

echo " ----------------------------------------------------------------------------------"
echo "parse_fasterCap.bash $data_dir $data_file_name $python_script $wire $gold_dir"
echo " ----------------------------------------------------------------------------------"

rm -rf $dir
mkdir $dir
cd $dir
pwd

echo "find $data_dir/$data_file_name -name "wires.log" -print > wire_log_file_list "
find $data_dir/$data_file_name -name "wires.log" -print > wire_log_file_list 
sort wire_log_file_list > sorted.input.list 

python3 $python_script -in_list_file  sorted.input.list -wire $wire -out_file $data_file_name.caps > OUT 

# cat OUT 
sort -n -r run_stats > run_stats.sorted 

# echo "diff OUT ../$gold_dir.GOLD/OUT" 
# diff OUT ../$gold_dir.GOLD/OUT 
echo "diff . ../$gold_dir" 
diff . ../$gold_dir 

