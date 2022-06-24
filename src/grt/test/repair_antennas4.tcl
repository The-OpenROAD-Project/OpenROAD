# repair_antennas ANTENNADIFFSIDEAREARATIO constant so repair fails
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "repair_antennas4.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met5 0.15
set_routing_layers -signal met1-met5
global_route

repair_antennas
