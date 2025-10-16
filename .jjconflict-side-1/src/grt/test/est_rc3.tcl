# estimate parasitics based on gr results
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty Nangate45/Nangate45_typ.lib
read_def "gcd.def"
read_guides "est_rc3.guide"

set_routing_layers -signal metal2-metal10
estimate_parasitics -global_routing

report_net -digits 3 clk
