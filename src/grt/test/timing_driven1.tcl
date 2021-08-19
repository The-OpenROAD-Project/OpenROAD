source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef

read_def timing_driven1.def

current_design gcd
create_clock -name core_clock -period 0.4574 [get_ports {clk}]

set_propagated_clock [all_clocks]

set_wire_rc -signal -layer "metal3"
set_wire_rc -clock  -layer "metal6"
set_dont_use {CLKBUF_* AOI211_X1 OAI211_X1}

set guide_file [make_result_file timing_driven1.guide]

set_global_routing_layer_adjustment metal2 0.8
set_global_routing_layer_adjustment metal3 0.7
set_global_routing_layer_adjustment metal4-metal8 0.4

set_routing_layers -signal metal2-metal10 -timing_critical metal7-metal10
set_global_routing_timing_driven -critical_nets_percentage 0.1 -min_fanout 3
set_macro_extension 2

global_route

write_guides $guide_file

diff_file timing_driven1.guideok $guide_file
