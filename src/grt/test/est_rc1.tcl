# estimate parasitics based on gr results
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty Nangate45/Nangate45_typ.lib
read_def "gcd.def"

set_routing_layers -signal metal2-metal10

global_route
estimate_parasitics -global_routing

report_net -connections -verbose -digits 3 clk
