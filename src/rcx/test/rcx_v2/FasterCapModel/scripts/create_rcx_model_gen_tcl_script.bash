#!/bin/bash -f

if [ $# -lt 5 ] 
then
	echo "Usage: create_rcx_model_gen_tcl_script <run_dir> <corner_name> <data_file_name> <python_script> <wire> <gold_dir>"
	exit
fi
dir=$1
data_dir=$2
data_file_name=$3
python_script=$4
wire=$5
gold_dir=$6

echo "parse_fasterCap.bash $data_dir/$data_file_name $python_script $wire $gold_dir"

init_rcx_model -corner_names "TYP" -met_cnt 7

read_rcx_tables -corner TYP -file ../data/pattern.caps.sept24 -wire 3
write_rcx_model -file sept24.rcx.model


rm -rf $dir
mkdir $dir
cd $dir
pwd

find $data_dir -name "wires.log" -print > wire_log_file_list 
sort wire_log_file_list > sorted.input.list 
python3 $python_script -in_list_file  sorted.input.list -wire $wire -out_file $data_file_name.caps > OUT 
cat OUT 
sort -n -r run_stats > run_stats.sorted 
echo "diff OUT ../$gold_dir.GOLD/OUT" 
diff OUT ../$gold_dir.GOLD/OUT 
echo "diff . ../$gold_dir.GOLD" 
diff . ../$gold_dir.GOLD 

