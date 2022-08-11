read_liberty read_guides1.lib1
read_liberty read_guides1.lib2
read_liberty read_guides1.lib3
read_liberty read_guides1.lib4
read_liberty read_guides1.lib5

read_lef read_guides1.lef1
read_lef read_guides1.lef2

read_def read_guides1.def

read_sdc read_guides1.sdc

source read_guides1_setRC.tcl

set_global_routing_layer_adjustment M2-M7 0.3
set_routing_layers -signal M2-M7

read_guides read_guides1.guide

set_propagated_clock [all_clocks]
estimate_parasitics -global_routing

report_checks -path_delay max -fields {slew cap input nets fanout} -format full_clock_expanded
