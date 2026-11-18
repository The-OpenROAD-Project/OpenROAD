# CUGR maze passes roll back reroutes that add overflow and RRR stops on stall
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_liberty "sky130hs/sky130hs_tt.lib"
read_def "critical_nets_percentage.def"
read_sdc "critical_nets_percentage.sdc"

source "sky130hs/sky130hs.rc"
set_wire_rc -signal -layer "met2"
set_wire_rc -clock -layer "met5"

set_propagated_clock [all_clocks]
estimate_parasitics -placement

set_routing_layers -signal met1-met5 -clock met3-met5
set_global_routing_layer_adjustment met1-met5 0.8

# Per pass: nets rerouted, rollbacks, overflow change and total overflow.
set_debug_level GRT rrr 1
# The maze's cost view must match a full extraction after every refresh.
set_debug_level GRT verify_view 1

global_route -use_cugr -critical_nets_percentage 30 -verbose
