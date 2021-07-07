# Check if writing partition to verilog works
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_verilog "gcd.v"
link_design gcd

set id [partition_netlist -tool chaco -num_partitions 2 -seeds {-289663928}]

set v_file [make_result_file verilog_write.v]
write_partition_verilog -partitioning_id $id $v_file
diff_files verilog_write.vok $v_file
