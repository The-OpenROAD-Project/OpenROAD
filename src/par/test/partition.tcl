# Check partitioning
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_verilog "gcd.v"
link_design gcd

set id [partition_netlist -tool mlpart -num_partitions 2 -seeds {-289663928}]

set par_file [make_result_file partition.par]
write_partitioning_to_db -partitioning_id $id -dump_to_file $par_file

diff_files partition.parok $par_file
