# estimate parasitics based on gr results
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty Nangate45/Nangate45_typ.lib
read_def "gcd.def"

set_routing_layers -signal metal1-metal10
read_global_route_segments read_segments3.segs
estimate_parasitics -global_routing

report_net -digits 3 clk
