# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "top_level_term.def"
read_guides "top_level_term.guide"

set def_file [make_result_file top_level_term.def]

detailed_route -bottom_routing_layer met1 -top_routing_layer met3 -verbose 0

write_def $def_file
diff_files top_level_term.defok $def_file
