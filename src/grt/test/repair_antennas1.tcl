# repair_antennas. def file generated using the openroad-flow
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met5 0.15
set_routing_layers -signal met1-met5
global_route

check_antennas
repair_antennas
check_antennas
check_placement

set guide_file [make_result_file repair_antennas1.guide]
write_guides $guide_file
diff_file repair_antennas1.guideok $guide_file

set def_file [make_result_file repair_antennas1.def]
write_def $def_file
diff_file repair_antennas1.defok $def_file
