source "helpers.tcl"
read_lef "overlapping_edges.lef"
read_def "overlapping_edges.def"

set guide_file [make_result_file overlapping_edges.guide]

set_global_routing_layer_adjustment * 0.3

set_routing_layers -signal met1-met5

global_route -verbose

write_guides $guide_file

diff_file overlapping_edges.guideok $guide_file
