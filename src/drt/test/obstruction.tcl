# layer range for clock nets. def file from the openroad-flow (modified gcd_sky130hs)
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "obstruction.def"
read_guides "obstruction.guide"

set def_file [make_result_file obstruction.def]

set_routing_layers -signal met1-met5
detailed_route -verbose 0

write_def $def_file
diff_files obstruction.defok $def_file
