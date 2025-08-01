# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "top_level_term2.def"
read_guides "top_level_term2.guide"

set def_file [make_result_file top_level_term2.def]

set_routing_layers -signal met1-met3
detailed_route -verbose 0

write_def $def_file
diff_files top_level_term2.defok $def_file
