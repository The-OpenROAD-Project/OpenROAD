#Check if the constructed hypergraph maintains the same number of nodes/hyperedges
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

report_partition_graph

