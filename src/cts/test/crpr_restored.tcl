# Latency balancing reads clock arrivals with clock reconvergence
# pessimism removal off, and clock_tree_synthesis gives the setting back
# as it found it, so the timing after CTS is the timing the user set up.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gated_clock2.def

create_clock -period 5 clk

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal1
set_wire_rc -clock -layer metal2

check "CRPR is on before CTS" { sta::crpr_enabled } 1

clock_tree_synthesis -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3 \
  -wire_unit 20 \
  -sink_clustering_enable \
  -distance_between_buffers 100 \
  -num_static_layers 1 \
  -repair_clock_nets

check "latency balancing inserted delay buffers" \
  { expr { [llength [get_cells -quiet delaybuf_*]] > 0 } } 1
check "CRPR is on after CTS" { sta::crpr_enabled } 1
exit_summary
