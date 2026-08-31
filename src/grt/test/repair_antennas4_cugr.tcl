# repair_antennas with CUGR routing under the harsh antenna rules of
# repair_antennas4.tlef: jumpers clear every violation that diode-only
# repair could not.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "repair_antennas4.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met5 0.15
set_routing_layers -signal met1-met5
global_route -use_cugr

check_antennas
repair_antennas
check_antennas
check_placement

set guide_file [make_result_file repair_antennas4_cugr.guide]
write_guides $guide_file
diff_file repair_antennas4_cugr.guideok $guide_file

set def_file [make_result_file repair_antennas4_cugr.def]
write_def $def_file
diff_file repair_antennas4_cugr.defok $def_file
