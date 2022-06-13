# Check partitioning
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_verilog "gcd.v"
link_design gcd

set id [cluster_netlist -tool mlpart -coarsening_ratio 0.6]

set clus_file [make_result_file cluster.par]
write_clustering_to_db -clustering_id $id -dump_to_file $clus_file

diff_files cluster.parok $clus_file
