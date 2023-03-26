source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def upper_layer_net.def

set guide_file [make_result_file upper_layer_net.guide]

set_routing_layers -signal metal1-metal9

global_route -verbose

write_guides $guide_file

diff_file upper_layer_net.guideok $guide_file