# check that CUGR routes only the nets given to set_nets_to_route.
# -resistance_aware runs the timing update, which estimates parasitics
# for every net CUGR routed.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_def "gcd.def"

set guide_file [make_result_file set_nets_to_route1_cugr.guide]

set_routing_layers -signal metal2-metal8

set_nets_to_route {net* req_* resp_* clk reset}

global_route -verbose -use_cugr -resistance_aware

write_guides $guide_file

diff_files set_nets_to_route1_cugr.guideok $guide_file
