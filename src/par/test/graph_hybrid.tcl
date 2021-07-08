#Check if the constructed hybrid graph maintains the same number of nodes/edges
#As this netlist is small, setting a small threshold is needed to achieve an actual hybrid model 
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

report_partition_graph -graph_model "hybrid" -clique_threshold 10

