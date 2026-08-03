# Post-detailed-route repair_antennas in a CUGR session: the entry rebuild
# re-inits CUGR, dirty diode nets drop their stale wires and reroute, and a
# final detailed_route confirms the repair on the real wires.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met3 0.15
set_routing_layers -signal met1-met3
global_route -use_cugr

detailed_route -verbose 0

check_antennas
repair_antennas
check_antennas
check_placement

detailed_route -verbose 0
check_antennas
