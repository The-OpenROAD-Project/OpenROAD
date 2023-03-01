# repair_timing -setup r1/Q 5 loads
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.vars
source Nangate45/Nangate45.rc
source $tracks_file

####################################################
# Older way 
#set_wire_rc -layer metal3
#estimate_parasitics -placement
####################################################
# Newer way 
set_routing_layers -signal $global_routing_layers
global_route
estimate_parasitics -global_routing
####################################################
report_checks -fields input -digits 3
repair_timing -setup
report_checks -fields input -digits 3
