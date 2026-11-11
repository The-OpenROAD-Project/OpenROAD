# Test that the detour metric builds the Steiner tree with the net's routing
# alpha: CUGR routes clk on its alpha 1.0 tree, so the ratio is 1.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set_routing_alpha 1.0 -net clk
global_route -use_cugr

report_net_detours -net clk
