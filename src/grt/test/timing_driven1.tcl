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
set_global_routing_layer_adjustment metal4-metal10 0.4

set_routing_layers -signal metal2-metal10 -timing_critical metal2-metal5
set_macro_extension 2

global_route

set_routing_alpha 0.7
repair_timing_critical_nets -critical_nets_percentage 0.1 -min_fanout 4

write_guides $guide_file

set_propagated_clock [all_clocks]
estimate_parasitics -global_routing

report_wns -digits 3
report_worst_slack -digits 3

# set_thread_count 8
# detailed_route -guide $guide_file \
#                -output_guide [make_result_file "timing_driven1_output_guide.mod"] \
#                -output_drc [make_result_file "timing_driven1_route_drc.rpt"] \
#                -output_maze [make_result_file "timing_driven1_maze.log"] \
#                -verbose 1
