# test set_nets_to_route commands
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file set_nets_to_route1.guide]

set_routing_layers -signal metal2-metal8

set_nets_to_route {net* req_* resp_* clk reset}

global_route -verbose

write_guides $guide_file

diff_file set_nets_to_route1.guideok $guide_file
