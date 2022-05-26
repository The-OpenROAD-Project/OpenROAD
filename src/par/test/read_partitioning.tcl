# Check report_netlist_partitions
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_verilog "gcd.v"
link_design gcd

set id [read_partitioning -read_file "read_partitioning_graph.txt" -instance_map_file "read_partitioning_instance_map.txt"]

set par_file [make_result_file read_partitioning.par]
write_partitioning_to_db -partitioning_id $id -dump_to_file $par_file

diff_files read_partitioning.parok $par_file
