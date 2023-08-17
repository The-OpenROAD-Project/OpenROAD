# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "obstruction.def"

set guide_file [make_result_file obstruction.guide]

set_global_routing_layer_adjustment met1 0.8
set_global_routing_layer_adjustment met2 0.7
set_global_routing_layer_adjustment * 0.5

set_routing_layers -signal met1-met5 -clock met3-met5

global_route -verbose

write_guides $guide_file
diff_files obstruction.guideok $guide_file
