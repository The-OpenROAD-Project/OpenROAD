source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gated_clock2.def

create_clock -period 5 clk

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal1
set_wire_rc -clock -layer metal2

set_debug CTS "insertion delay" 1

set_cts_config -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3 \
  -wire_unit 20 \
  -distance_between_buffers 100 \
  -num_static_layers 1 \
  -skip_nets {"gclk1" "gclk3"}

report_cts_config

clock_tree_synthesis -sink_clustering_enable -repair_clock_nets
