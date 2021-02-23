#Check if the constructed clique graph maintains the same number of nodes/edges
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

report_partition_graph -graph_model "clique"

