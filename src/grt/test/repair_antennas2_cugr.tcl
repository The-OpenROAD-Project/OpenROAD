# repair_antennas with CUGR routing on the repair_antennas2 design:
# diode-only repair on a second placement/density at met1-met3.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "repair_antennas2.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met3 0.15
set_routing_layers -signal met1-met3
global_route -use_cugr

check_antennas
repair_antennas
check_antennas
check_placement

set guide_file [make_result_file repair_antennas2_cugr.guide]
write_guides $guide_file
diff_file repair_antennas2_cugr.guideok $guide_file

set def_file [make_result_file repair_antennas2_cugr.def]
write_def $def_file
diff_file repair_antennas2_cugr.defok $def_file
