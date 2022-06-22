# repair_antennas ord flow gcd_sky130hs with fillers
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "repair_antennas2.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met5 0.15
set_routing_layers -signal met1-met5
global_route

check_antennas
repair_antennas
check_antennas
filler_placement sky130_fd_sc_hs__fill_*

set guide_file [make_result_file repair_antennas2.guide]
write_guides $guide_file
diff_file repair_antennas2.guideok $guide_file

set def_file [make_result_file repair_antennas2.def]
write_def $def_file
diff_file repair_antennas2.defok $def_file
