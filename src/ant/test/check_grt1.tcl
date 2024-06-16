source "helpers.tcl"
# repair_antennas global route
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_global_routing_layer_adjustment met2-met5 0.15
set_routing_layers -signal met1-met5
global_route

check_antennas -verbose
