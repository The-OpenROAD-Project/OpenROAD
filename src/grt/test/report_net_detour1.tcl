# Test report_net_global_routing_detour command
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"
set_global_routing_layer_adjustment metal2 0.9
set_routing_layers -signal metal2-metal10
global_route
report_net_global_routing_detour -nets [get_nets clk]
