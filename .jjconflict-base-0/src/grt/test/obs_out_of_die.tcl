# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "obs_out_of_die.def"

set guide_file [make_result_file obs_out_of_die.guide]

set_routing_layers -signal met1-met5

global_route -verbose

write_guides $guide_file

diff_file obs_out_of_die.guideok $guide_file
