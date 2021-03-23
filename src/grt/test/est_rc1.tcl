# estimate parasitics based on gr results
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty Nangate45/Nangate45_typ.lib
read_def "gcd.def"

global_route -layers 2-10
estimate_parasitics -global_routing

report_net -connections -verbose -digits 3 clk
