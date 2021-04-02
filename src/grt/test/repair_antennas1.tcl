# repair_antennas. def file generated using the openroad-flow
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2

set guide_file [make_result_file repair_antennas1.guide]
set def_file [make_result_file repair_antennas1.def]

set_global_routing_layer_adjustment 3-6 0.15

set_global_routing_layer_pitch 2 0.37
set_global_routing_layer_pitch 3 0.48
set_global_routing_layer_pitch 4 0.74
set_global_routing_layer_pitch 5 0.96
set_global_routing_layer_pitch 6 3.33

set_layer_ranges -layers 2-6

global_route

repair_antennas sky130_fd_sc_hs__diode_2/DIODE

set_placement_padding -global -left 0 -right 0
check_placement

write_guides $guide_file
write_def $def_file

diff_file repair_antennas1.guideok $guide_file
diff_file repair_antennas1.defok $def_file
