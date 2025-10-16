source "helpers.tcl"

source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_liberty array_tile_ins_delay.lib
read_lef Nangate45/Nangate45.lef
read_lef array_tile_ins_delay.lef
read_def gated_clock5.def

create_clock -period 5 clk

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal1
set_wire_rc -clock -layer metal2

set_debug CTS "insertion delay" 1

clock_tree_synthesis -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3 \
  -wire_unit 20 \
  -sink_clustering_enable \
  -distance_between_buffers 100 \
  -num_static_layers 1 \
  -repair_clock_nets
