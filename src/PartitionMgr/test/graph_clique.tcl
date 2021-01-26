source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

report_graph -graph_model "clique"

