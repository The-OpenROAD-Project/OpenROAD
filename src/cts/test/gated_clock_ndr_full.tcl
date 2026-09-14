# Check that -apply_ndr full also puts the NDR on the trunks of the sub trees
# driven by the clock gaters, leaving out only the leaf nets.
source "helpers.tcl"
source "cts-helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gated_clock2.def

create_clock -period 5 clk

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal1
set_wire_rc -clock -layer metal2

clock_tree_synthesis -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3 \
  -wire_unit 20 \
  -sink_clustering_enable \
  -distance_between_buffers 100 \
  -num_static_layers 1 \
  -apply_ndr full

report_ndr_nets
