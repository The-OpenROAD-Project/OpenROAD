# Test CTS gated clock tree synthesis on a hierarchical design
source "helpers.tcl"
source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog gated_clock4.v
link_design -hier multi_sink
read_def -floorplan_initialize gated_clock4.def


source Nangate45/Nangate45.rc
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk

create_clock -name core -period 5 clk

set_debug CTS "insertion delay" 1

clock_tree_synthesis -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3 \
  -wire_unit 20 \
  -sink_clustering_enable \
  -distance_between_buffers 100 \
  -num_static_layers 1 \
  -repair_clock_nets

set vout_file [make_result_file gated_clock4.v]
write_verilog $vout_file
diff_files gated_clock4.vok $vout_file
