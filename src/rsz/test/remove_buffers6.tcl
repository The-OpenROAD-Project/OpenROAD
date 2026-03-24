# Check RC update by incremental global routing after remove_buffer
source "helpers.tcl"
if { ![info exists repair_args] } {
  set repair_args {}
}

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers6.def
initialize_floorplan -die_area "0 0 40 1200" \
  -core_area "0 0 40 1200" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

source Nangate45/Nangate45.vars
source Nangate45/Nangate45.rc
source $tracks_file

create_clock -period 0.3 clk
set_propagated_clock clk

detailed_placement

source Nangate45/Nangate45.rc
# Intentionaly reduce RC so there is a large discrepancy between
# placement and global route parasitics.
set_wire_rc -resistance 0.0001 -capacitance 0.00001
set_routing_layers -signal $global_routing_layers
global_route
estimate_parasitics -global_routing

# 0. WNS - pre
report_worst_slack -max

#set_debug_level RSZ repair_setup 10
#set_debug_level RSZ opt_moves 10
#set_debug_level RSZ remove_buffer 10
#set_debug_level EST est_rc 10
#set_debug_level GRT incr 10

remove_buffers u5

# 1. WNS - after remove_buffer
report_worst_slack -max
estimate_parasitics -global_routing

# 2. WNS - after RC update
report_worst_slack -max

global_route
estimate_parasitics -global_routing

# 3. WNS - after redundant global routing
report_worst_slack -max

# WNS 1~3 should be the same
