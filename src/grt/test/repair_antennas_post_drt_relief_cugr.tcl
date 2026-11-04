# Post-DRT repair_antennas: nets the overflow relief moves drop their wires.
source "helpers.tcl"
source "repair_antennas_relief_helpers.tcl"
# Suppress DRT init logging: region/guide query sizes are a function of the
# exact route geometry, not the repair contract this test locks.
suppress_message DRT 33
suppress_message DRT 36
suppress_message DRT 167
suppress_message DRT 168
suppress_message DRT 178
suppress_message DRT 179
suppress_message DRT 349
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met1-met3 0.8
set_routing_layers -signal met1-met3
global_route -use_cugr

detailed_route -verbose 0

set block [ord::get_db_block]
record_routed_nets $block

set_debug_level GRT verify_demand 1
repair_antennas

report_moved_wires $block
