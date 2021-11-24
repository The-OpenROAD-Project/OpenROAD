# Check if changing evaluation methods results in different answers
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_verilog "gcd.v"
link_design gcd

set num_starts 10
set num_partitions 4

set ids {}
lappend ids [partition_netlist -tool mlpart  -num_partitions $num_partitions -num_starts $num_starts]
lappend ids [partition_netlist -tool gpmetis -num_partitions $num_partitions -num_starts $num_starts]
lappend ids [partition_netlist -tool chaco   -num_partitions $num_partitions -num_starts $num_starts]

set bestId [evaluate_partitioning -partition_ids $ids -evaluation_function area]
set bestId [evaluate_partitioning -partition_ids $ids -evaluation_function size]
