# insert_buffer under global route parasitics, without any detailed
# placement command run before it: the resizer legalizes the new buffer
# with dpl, whose grid was never made.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def
create_clock [get_ports clk] -name core_clock -period 2

source Nangate45/Nangate45.rc
set_routing_layers -signal metal2-metal10
global_route
estimate_parasitics -global_routing

set buf [insert_buffer -net req_msg[0] -buffer_cell BUF_X1 \
  -buffer_name gr_buf -net_name gr_net]
puts "inserted [get_full_name $buf] at\
  [[sta::sta_to_db_inst $buf] getLocation]"

report_worst_slack -max -digits 3
