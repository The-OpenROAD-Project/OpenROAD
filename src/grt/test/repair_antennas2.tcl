#repair_antennas for design w/ filler cells. def file generated from openroad-flow (gcd_sky130hs post filler insertion)
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"

read_def "repair_antennas2.def"

set_placement_padding -global -left 2 -right 2

set guide_file [make_result_file repair_antennas2.guide]
set def_file [make_result_file repair_antennas2.def]

set_global_routing_layer_adjustment met2-met5 0.15

set_routing_layers -signal met1-met5

set_clock_routing -pdrev_fanout 1

global_route

repair_antennas sky130_fd_sc_hs__diode_2/DIODE

set_placement_padding -global -left 2 -right 2

filler_placement sky130_fd_sc_hs__fill_*

write_guides $guide_file
write_def $def_file

diff_file repair_antennas2.guideok $guide_file
diff_file repair_antennas2.defok $def_file
