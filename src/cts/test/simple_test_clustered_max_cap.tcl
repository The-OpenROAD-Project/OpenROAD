source "helpers.tcl"
source "cts-helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

make_array 300
sta::db_network_defined

create_clock -period 5 clk

set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

set_cts_config -wire_unit 20 \
  -distance_between_buffers 100 \
  -num_static_layers 1 \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3

clock_tree_synthesis -sink_clustering_enable
