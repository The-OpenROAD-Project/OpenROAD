# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "top_level_term2.def"

set guide_file [make_result_file top_level_term2.guide]

set_global_routing_layer_adjustment met1 0.8
set_global_routing_layer_adjustment met2 0.7
set_global_routing_layer_adjustment * 0.5

set_routing_layers -signal met1-met4

global_route -verbose

write_guides $guide_file

diff_file top_level_term2.guideok $guide_file
