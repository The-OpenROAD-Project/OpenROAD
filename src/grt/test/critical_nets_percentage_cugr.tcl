# gcd flow pipe cleaner
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

set guide_file [make_result_file critical_nets_percentage_cugr.guide]

set_routing_layers -signal met1-met5 -clock met3-met5
set_global_routing_layer_adjustment met1-met5 0.8

global_route -use_cugr -critical_nets_percentage 30 -verbose

write_guides $guide_file

diff_file critical_nets_percentage_cugr.guideok $guide_file
