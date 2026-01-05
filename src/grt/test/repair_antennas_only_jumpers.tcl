# repair_antennas. def file generated using the openroad-flow
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met4 0.8
set_routing_layers -signal met1-met5
global_route


check_antennas
repair_antennas -jumper_only
check_antennas
check_placement

set guide_file [make_result_file repair_antennas_only_jumpers.guide]
write_guides $guide_file
diff_file repair_antennas_only_jumpers.guideok $guide_file

set def_file [make_result_file repair_antennas_only_jumpers.def]
write_def $def_file
diff_file repair_antennas_only_jumpers.defok $def_file
