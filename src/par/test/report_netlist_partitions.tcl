# Check report_netlist_partitions
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_verilog "gcd.v"
link_design gcd

set id [partition_netlist -tool mlpart -num_partitions 4 -num_starts 1]

report_netlist_partitions -partitioning_id $id
