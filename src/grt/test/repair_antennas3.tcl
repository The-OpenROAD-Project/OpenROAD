# repair_antennas. def file generated using the openroad-flow
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "repair_antennas2.def"

set_placement_padding -global -left 2 -right 2

set guide_file [make_result_file repair_antennas3.guide]
set def_file [make_result_file repair_antennas3.def]

set_routing_layers -signal met1-met5

global_route

repair_antennas sky130_fd_sc_hs__diode_2/DIODE -iterations 3

set_placement_padding -global -left 0 -right 0
check_placement

write_guides $guide_file
write_def $def_file

diff_file repair_antennas3.guideok $guide_file
diff_file repair_antennas3.defok $def_file
